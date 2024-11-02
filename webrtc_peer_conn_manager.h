#ifndef PEERCONNECTION_CLIENT_H_
#define PEERCONNECTION_CLIENT_H_
#include <iostream>
#include "api/media_stream_interface.h"
#include "media/engine/webrtc_media_engine.h"
#include "api/peer_connection_interface.h"
#include "api/create_peerconnection_factory.h"
#include "api/audio_codecs/builtin_audio_decoder_factory.h"
#include "api/audio_codecs/builtin_audio_encoder_factory.h"
#include "api/video_codecs/builtin_video_decoder_factory.h"
#include "api/video_codecs/builtin_video_encoder_factory.h"
#include "api/video_codecs/video_decoder.h"
#include "api/task_queue/default_task_queue_factory.h"
#include "api/rtc_event_log/rtc_event_log_factory.h"
#include "api/call/audio_sink.h"
#include "websocketpp/config/asio.hpp"
#include "websocketpp/server.hpp"
#include <fstream>
#include "webrtc_video_decoder_factory.h"

static int64_t count = 0;

static void SaveI420Image(const rtc::scoped_refptr<webrtc::I420BufferInterface>& buffer, const std::string& file_path) {
    std::cout << "Received a frame with width: " << buffer->width()
                << ", height: " << buffer->height() << std::endl;
    const uint8_t* y_plane = buffer->DataY();
    const uint8_t* u_plane = buffer->DataU();
    const uint8_t* v_plane = buffer->DataV();

    int y_stride = buffer->StrideY();
    int u_stride = buffer->StrideU();
    int v_stride = buffer->StrideV();

    int width = buffer->width();
    int height = buffer->height();

    // std::ofstream file(file_path, std::ios::binary);
    std::ofstream file(file_path, std::ios::binary | std::ios::app);
    if (!file.is_open()) {
        std::cerr << "Failed to open file for writing: " << file_path << std::endl;
        return;
    }

    // 写入 Y 分量
    for (int y = 0; y < height; ++y) {
        file.write(reinterpret_cast<const char*>(y_plane + y * y_stride), width);
    }

    // 写入 U 分量
    for (int y = 0; y < height / 2; ++y) {
        file.write(reinterpret_cast<const char*>(u_plane + y * u_stride), width / 2);
    }

    // 写入 V 分量
    for (int y = 0; y < height / 2; ++y) {
        file.write(reinterpret_cast<const char*>(v_plane + y * v_stride), width / 2);
    }

    file.close();
}

class DummySetSessionDescriptionObserver : public webrtc::SetSessionDescriptionObserver {
public:
    static DummySetSessionDescriptionObserver* Create() {
        std::cout << "#############come in DummySetSessionDescriptionObserver Create" << std::endl;
        return new rtc::RefCountedObject<DummySetSessionDescriptionObserver>();
    }
    virtual void OnSuccess() { 
        std::cout << "#############come in DummySetSessionDescriptionObserver OnSuccess" << std::endl;
        RTC_LOG(INFO) << "############################" << __FUNCTION__ << " : " << "SetRemoteDesp success"; 
    }
    virtual void OnFailure(webrtc::RTCError error) {
        std::cout << "#############come in DummySetSessionDescriptionObserver OnFailure" << std::endl;
        RTC_LOG(INFO) << "############################SetRemoteDesp error" <<__FUNCTION__ << " " << ToString(error.type()) << ": "
                    << error.message();
    }
};

class VideoDecodedImageCallback : public webrtc::DecodedImageCallback {
public:
    VideoDecodedImageCallback() {
        std::cout << "$$$$$$$$$$$$$$$VideoDecodedImageCallback is : " << this << std::endl;
    }
    virtual ~VideoDecodedImageCallback() {
        std::cout << "*******************VideoDecodedImageCallback is : " << this << std::endl;
    }

    virtual int32_t Decoded(webrtc::VideoFrame& decodedImage) {
        std::cout << "decodedImage size is : " << decodedImage.size() << std::endl;
        rtc::scoped_refptr<webrtc::I420BufferInterface> buffer(decodedImage.video_frame_buffer()->ToI420());
        SaveI420Image(buffer, "output.yuv");
        return 0;
    }
};

// video
class VideoSink : public rtc::VideoSinkInterface<webrtc::VideoFrame> {
public:
    VideoSink(webrtc::VideoTrackInterface* track_to_render) : rendered_track_(track_to_render) {
        rendered_track_->AddOrUpdateSink(this, rtc::VideoSinkWants());
    }

    virtual ~VideoSink() {
        rendered_track_->RemoveSink(this);
    }

    virtual void OnFrame(const webrtc::VideoFrame& frame) override {
        rtc::scoped_refptr<webrtc::I420BufferInterface> buffer(frame.video_frame_buffer()->ToI420());
        // std::string filename = std::to_string(count++) + ".yuv";
        // SaveI420Image(buffer, filename); // 测试保存下来yuv420数据查看
        SaveI420Image(buffer, "output.yuv");
    }

private:
    rtc::scoped_refptr<webrtc::VideoTrackInterface> rendered_track_;
};

// audio
class AudioSink : public webrtc::AudioSinkInterface {
public:
    AudioSink() {}
    virtual ~AudioSink() {}

    virtual void OnData(const Data& audio) override {
        
    }
};

class WebRTCPeerConnectionManager : public webrtc::PeerConnectionObserver,
                                    public webrtc::CreateSessionDescriptionObserver
{
public:
    WebRTCPeerConnectionManager();
    virtual ~WebRTCPeerConnectionManager();

    void SetWsHandle(void* ws_server) { ws_server_ = ws_server; }

    void SetSptrHdl(std::shared_ptr<void> shdl) { sptr_hdl_ = shdl; }

    int MsgProcess(const std::string& msg);

    void Close();

protected:
    virtual void OnSignalingChange(webrtc::PeerConnectionInterface::SignalingState new_state) override {}
    virtual void OnAddTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver,
                            const std::vector<rtc::scoped_refptr<webrtc::MediaStreamInterface>>&streams) override;
    virtual void OnRemoveTrack(rtc::scoped_refptr<webrtc::RtpReceiverInterface> receiver) override;
    virtual void OnDataChannel(rtc::scoped_refptr<webrtc::DataChannelInterface> channel) override {}
    virtual void OnRenegotiationNeeded() override {}
    virtual void OnIceConnectionChange(webrtc::PeerConnectionInterface::IceConnectionState new_state) override {}
    virtual void OnIceGatheringChange(webrtc::PeerConnectionInterface::IceGatheringState new_state) override {}
    virtual void OnIceCandidate(const webrtc::IceCandidateInterface* candidate) override;
    virtual void OnIceConnectionReceivingChange(bool receiving) override {}

    virtual void OnSuccess(webrtc::SessionDescriptionInterface* desc) override;
    virtual void OnFailure(webrtc::RTCError error) override;

protected:
    void SendMessage(const std::string& msg);

    bool Init();

private:
    rtc::scoped_refptr<webrtc::PeerConnectionInterface> peer_connection_;
    rtc::scoped_refptr<webrtc::PeerConnectionFactoryInterface> peer_connection_factory_;
    void* ws_server_;
    std::shared_ptr<void> sptr_hdl_;
    std::unique_ptr<VideoSink> video_sink_;
    std::unique_ptr<rtc::Thread> signaling_thread_;
    std::unique_ptr<rtc::Thread> network_thread_;
    std::unique_ptr<rtc::Thread> worker_thread_;
    VideoDecodedImageCallback decode_callback_;
    std::unique_ptr<webrtc::WebRTCVideoDecoderFactory> video_decoder_factory_;
};

#endif // PEERCONNECTION_CLIENT_H_