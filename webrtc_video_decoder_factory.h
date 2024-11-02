#ifndef WEBRTC_VIDEO_CODEC_FACTORY_H_
#define WEBRTC_VIDEO_CODEC_FACTORY_H_
#include <memory>
#include <vector>

#include "api/video_codecs/video_decoder_factory.h"
#include "webrtc_video_decoder.h"
namespace webrtc {

static const char kCodecFactoryCodecName[] = "ITC H264";

class WebRTCVideoDecoderFactory : public VideoDecoderFactory
{
public:
    WebRTCVideoDecoderFactory(DecodedImageCallback* callback);
    virtual ~WebRTCVideoDecoderFactory();

    virtual std::vector<SdpVideoFormat> GetSupportedFormats() const override;

    virtual std::unique_ptr<VideoDecoder> CreateVideoDecoder(const SdpVideoFormat& format) override;

private:
    DecodedImageCallback* callback_;
};


}

#endif // WEBRTC_VIDEO_CODEC_FACTORY_H_