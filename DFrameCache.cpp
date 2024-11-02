#include "DFrameCache.h"

DFrameCache::DFrameCache()
    : m_maxCacheSize(15)
{
}

DFrameCache::~DFrameCache()
{
}

void DFrameCache::SetMaxCacheSize(uint32_t size)
{
}
int DFrameCache::AddFrame(const AVFrame *frm)
{
    if (!frm)
    {
        return -1;
    }

    lock_guard<mutex> lk(m_mtxCache);

    if (m_cache.size() > m_maxCacheSize)
    {
        return -1;
    }

    AVFrame *frmClone = av_frame_clone(frm);
    m_cache.push(frmClone);

    return 0;
}

int DFrameCache::Size()
{
    lock_guard<mutex> lk(m_mtxCache);
    return m_cache.size();
}

AVFrame *DFrameCache::GetFrame()
{
    lock_guard<mutex> lk(m_mtxCache);

    if (m_cache.empty())
    {
        return NULL;
    }

    AVFrame *frm = m_cache.front();
    m_cache.pop();
    // std::cout << "frm cache size is : " << m_cache.size() << std::endl;
    return frm;
}

int DFrameCache::ClearAll()
{
    lock_guard<mutex> lk(m_mtxCache);
    AVFrame *frm = NULL;

    while (!m_cache.empty())
    {
        frm = m_cache.front();
        if (frm)
        {
            av_frame_free(&frm);
        }
        m_cache.pop();
    }

    return 0;
}

void DFrameCache::RemoveFront()
{
    lock_guard<mutex> lk(m_mtxCache);
    AVFrame *frm = NULL;

    if (m_cache.empty())
    {
        return;
    }

    frm = m_cache.front();
    av_frame_free(&frm);
    m_cache.pop();
}

uint32_t DFrameCache::FrameNum()
{
    lock_guard<mutex> lk(m_mtxCache);
    return m_cache.size();
}