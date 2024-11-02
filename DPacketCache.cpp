#include "DPacketCache.h"

DPacketCache::DPacketCache()
    : m_maxCacheSize(15)
{
}

DPacketCache::~DPacketCache()
{
}

void DPacketCache::SetMaxCacheSize(uint32_t size)
{
    m_maxCacheSize = size;
}

int DPacketCache::AddPacket(const AVPacket *pkt)
{
    if (!pkt) {
        return -1;
    }

    lock_guard<mutex> lk(m_mtxCache);

    if (m_cache.size() > m_maxCacheSize)
    {
        return -1;
    }

    AVPacket *pktClone = av_packet_clone(pkt);
    m_cache.push(pktClone);

    return 0;
}

int DPacketCache::Size()
{
    lock_guard<mutex> lk(m_mtxCache);
    return m_cache.size();
}

AVPacket *DPacketCache::GetPacket()
{
    lock_guard<mutex> lk(m_mtxCache);

    if (m_cache.empty())
    {
        return NULL;
    }

    AVPacket *pkt = m_cache.front();
    m_cache.pop();

    return pkt;
}

int DPacketCache::ClearAll()
{
    lock_guard<mutex> lk(m_mtxCache);
    AVPacket *pkt = NULL;

    while (!m_cache.empty())
    {
        pkt = m_cache.front();
        if (pkt)
        {
            av_packet_free(&pkt);
        }
        m_cache.pop();
    }

    return 0;
}

void DPacketCache::ClearUntilKey()
{
    lock_guard<mutex> lk(m_mtxCache);
    uint32_t idx = 0;
    AVPacket *pkt = NULL;

    while (!m_cache.empty())
    {
        pkt = m_cache.front();
        if (pkt->flags == AV_PKT_FLAG_KEY && idx != 0)
        {
            break;
        }

        ++idx;
        av_packet_free(&pkt);
        m_cache.pop();
    }
}

uint32_t DPacketCache::PackNum()
{
    lock_guard<mutex> lk(m_mtxCache);

    return m_cache.size();
}