#ifndef WEBRTC_VIDEO_DECODER_H_
#define WEBRTC_VIDEO_DECODER_H_
#include "api/video_codecs/video_decoder.h"
#include "api/video/i420_buffer.h"
#include "rtc_base/time_utils.h"
#include "ffmpeg_decode.h"
namespace webrtc {
class WebRTCH264VideoDecoder : public VideoDecoder
{
public:
    WebRTCH264VideoDecoder(DecodedImageCallback* callback);
    virtual ~WebRTCH264VideoDecoder();

    virtual int32_t InitDecode(const VideoCodec* config, int32_t number_of_cores) override;

    virtual int32_t Decode(const EncodedImage& input, bool missing_frames, int64_t render_time_ms) override;

    virtual int32_t RegisterDecodeCompleteCallback(DecodedImageCallback* callback) override;

    virtual int32_t Release() override;

protected:
    rtc::scoped_refptr<VideoFrameBuffer> AVFrameToI420Buffer(const AVFrame* frame);

    void ConvertAndUseFrame(const AVFrame* av_frame);

    void DecodeThread();

private:
    DecodedImageCallback* callback_;
    int width_;
    int height_;

    FFmpegDecode m_decoder;
    std::thread m_decThread;
    bool m_decThreadFlag;
};

}

#endif // WEBRTC_VIDEO_DECODER_H_