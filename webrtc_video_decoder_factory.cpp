#include "webrtc_video_decoder_factory.h"
#include "api/video_codecs/sdp_video_format.h"
#include <iostream>

namespace webrtc {

WebRTCVideoDecoderFactory::WebRTCVideoDecoderFactory(DecodedImageCallback* callback)
    : callback_(callback)
{
;
}

WebRTCVideoDecoderFactory::~WebRTCVideoDecoderFactory()
{

}

std::vector<SdpVideoFormat> WebRTCVideoDecoderFactory::GetSupportedFormats() const
{
    return std::vector<SdpVideoFormat>(1, SdpVideoFormat(kCodecFactoryCodecName));
}

std::unique_ptr<VideoDecoder> WebRTCVideoDecoderFactory::CreateVideoDecoder(const SdpVideoFormat& format)
{ 
    return absl::make_unique<WebRTCH264VideoDecoder>(callback_);
}

}