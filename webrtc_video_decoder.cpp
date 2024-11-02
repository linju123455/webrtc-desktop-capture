#include "webrtc_video_decoder.h"
#include "api/video/i420_buffer.h"
#include <iostream>
#include <fstream>
namespace webrtc {

static bool IsIFrame(const uint8_t* data)
{
    uint8_t nalType = data[4] & 0x1F;
    if (nalType == 5 || nalType == 7 || nalType == 8) {
        return true;
    } 
    return false;
}

WebRTCH264VideoDecoder::WebRTCH264VideoDecoder(DecodedImageCallback* callback)
    : callback_(callback)
{
    
}

WebRTCH264VideoDecoder::~WebRTCH264VideoDecoder()
{

}

int32_t WebRTCH264VideoDecoder::InitDecode(const VideoCodec* config, int32_t number_of_cores)
{
    std::cout << "come in init decode" << std::endl;
    int ret = m_decoder.InitDecode();
    if (ret < 0) {
        return -1;
    }
    m_decThreadFlag = true;
    m_decThread = std::thread(std::bind(&WebRTCH264VideoDecoder::DecodeThread, this));
    return 0;
}

int32_t WebRTCH264VideoDecoder::Decode(const EncodedImage& input, bool missing_frames, int64_t render_time_ms)
{
    std::cout << "width is: " << input._encodedWidth 
                << " height is: " << input._encodedHeight 
                << " size is: " << input.size()
                << " Timestamp is: " << input.Timestamp() << std::endl;

    if (input._encodedWidth > 0 || input._encodedHeight > 0) {
        width_ = input._encodedWidth;
        height_ = input._encodedHeight;
    }
    if (input.size() < 5) {
        return -1;
    }
    printf("0x%02x 0x%02x 0x%02x 0x%02x 0x%02x\n", 
            input.data()[0], input.data()[1], input.data()[2], input.data()[3], input.data()[4]);
    bool flags = IsIFrame(input.data());

    // std::ofstream file("test.h264", std::ios::binary | std::ios::app);
    // if (!file.is_open()) {
    //     std::cerr << "Failed to open file for writing: " << std::endl;
    //     return -1;
    // }

    // file.write(reinterpret_cast<const char*>(input.data()), input.size());

    m_decoder.AsyncDecodePacket(input.data(), input.size(), input.Timestamp(), flags);
    return 0;
}

int32_t WebRTCH264VideoDecoder::RegisterDecodeCompleteCallback(DecodedImageCallback* callback)
{
    return 0;
}

int32_t WebRTCH264VideoDecoder::Release()
{
    std::cout << "come in decode release" << std::endl;
    m_decThreadFlag = false;
    if (m_decThread.joinable()) {
        m_decThread.join();
    }
    m_decoder.Release();
    return 0;
}

rtc::scoped_refptr<VideoFrameBuffer> WebRTCH264VideoDecoder::AVFrameToI420Buffer(const AVFrame* frame) { 
    // 复制 Y, U, V 数据到 I420Buffer
    int y_stride = frame->linesize[0];
    int u_stride = frame->linesize[1];
    int v_stride = frame->linesize[2];

    uint8_t* y_data = frame->data[0];
    uint8_t* u_data = frame->data[1];
    uint8_t* v_data = frame->data[2];

    return I420Buffer::Copy(frame->width, frame->height, y_data, y_stride, u_data, u_stride, v_data, v_stride);
}

void WebRTCH264VideoDecoder::ConvertAndUseFrame(const AVFrame* av_frame) {
    std::cout << "get avframe" << std::endl;
    auto buffer = AVFrameToI420Buffer(av_frame);

    // 创建 WebRTC 的 VideoFrame
    int64_t timestamp_rtp = rtc::TimeMicros();  // 需要为帧设置适当的时间戳
    VideoFrame video_frame = VideoFrame::Builder()
        .set_video_frame_buffer(buffer)
        .set_rotation(kVideoRotation_0)
        .set_timestamp_us(timestamp_rtp)
        .build();

    if (callback_)
        callback_->Decoded(video_frame);
}

void WebRTCH264VideoDecoder::DecodeThread()
{
    while (m_decThreadFlag) {
        AVFrame* frm = m_decoder.GetFrameFromCache();
        if (!frm) {
            continue;
        }

        ConvertAndUseFrame(frm);
    }
}

}