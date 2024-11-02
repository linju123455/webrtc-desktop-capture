#ifndef FFMPEG_DECODE_H_
#define FFMPEG_DECODE_H_
#include "inc-ffmpeg.h"
#include "DPacketCache.h"
#include "DFrameCache.h"
#include <thread>
#include <functional>

class FFmpegDecode
{
public:
    FFmpegDecode();
    virtual ~FFmpegDecode();

    int InitDecode();

    void AsyncDecodePacket(const uint8_t* data, size_t size, uint64_t time_stamp, bool flags);

    AVFrame *GetFrameFromCache();

    void Release();

protected:
    void DecodeThreadFunc();

private:
    AVCodecContext* m_codecCtx;

    DFrameCache     m_frameCache;
    DPacketCache    m_pktCache;

    bool m_working;
    std::thread m_thWork;
};


#endif // FFMPEG_DECODE_H_
