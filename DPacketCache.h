#pragma once
#include "inc-ffmpeg.h"
#include <stdint.h>
#include <queue>
#include <mutex>

using namespace std;

class DPacketCache
{
public:
    DPacketCache();
    virtual ~DPacketCache();

public:
    void SetMaxCacheSize(uint32_t size);
    int Size();
    int AddPacket(const AVPacket* pkt);
    AVPacket* GetPacket();  //must call av_packet_free to free.
    int ClearAll();
    void ClearUntilKey();
    uint32_t PackNum();

protected:

private:
    mutex m_mtxCache;
    queue<AVPacket*> m_cache;
    uint32_t m_maxCacheSize;
};