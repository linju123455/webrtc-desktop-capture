#include "ffmpeg_decode.h"

FFmpegDecode::FFmpegDecode()
{

}

FFmpegDecode::~FFmpegDecode()
{

}

int FFmpegDecode::InitDecode()
{
    avcodec_register_all();
    av_log_set_level(AV_LOG_ERROR);

    AVCodec* codec = avcodec_find_decoder(AV_CODEC_ID_H264);
    if (!codec) {
        fprintf(stderr, "Codec not found\n");
        return -1;
    }

    m_codecCtx = avcodec_alloc_context3(codec);
    if (!m_codecCtx) {
        fprintf(stderr, "Could not allocate video codec context\n");
        return -1;
    }

    if (avcodec_open2(m_codecCtx, codec, NULL) < 0) {
        fprintf(stderr, "Could not open codec\n");
        return -1;
    }

    m_working = true;
    m_thWork = std::thread(std::bind(&FFmpegDecode::DecodeThreadFunc, this));
    
    return 0;
}

void FFmpegDecode::Release()
{
    m_working = false;
    if (m_thWork.joinable())
    {
        m_thWork.join();
    }

    if (m_codecCtx)
    {
        avcodec_free_context(&m_codecCtx);
    }
}

void FFmpegDecode::AsyncDecodePacket(const uint8_t* data, size_t size, uint64_t time_stamp, bool flags)
{
    AVPacket *pkt = av_packet_alloc();
    av_new_packet(pkt, size);

    pkt->pts = time_stamp;
    pkt->flags = flags;
    pkt->size = size;
    memcpy(pkt->data, data, size);

    if (m_pktCache.AddPacket(pkt) < 0) {
        m_pktCache.ClearUntilKey();
        m_pktCache.AddPacket(pkt);
    }
}

AVFrame* FFmpegDecode::GetFrameFromCache()
{
    return m_frameCache.GetFrame();
}

void FFmpegDecode::DecodeThreadFunc()
{
    while (m_working) {
        AVFrame* frmV = av_frame_alloc();
        AVPacket* pktCached = m_pktCache.GetPacket();
        if (!pktCached) {
            av_frame_free(&frmV);
            this_thread::sleep_for(chrono::milliseconds(10));
            continue;
        }

        int got = 0;
        int ret = avcodec_decode_video2(m_codecCtx, frmV, &got, pktCached);
        av_packet_free(&pktCached);
        if (ret < 0) {
            char buf[64] = {0};
            av_strerror(ret, buf, sizeof(buf));
            printf("%s\n", buf);
        }
        if (got <= 0)
        {
            printf("decode error...\n");
            av_frame_free(&frmV);
            continue;
        }

        if (m_frameCache.AddFrame(frmV) < 0)
        {
            // printf("AddFrame error...\n");
            m_frameCache.RemoveFront();
            m_frameCache.AddFrame(frmV);
        }

        av_frame_free(&frmV);
    }
}

