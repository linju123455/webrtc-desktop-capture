#pragma once
#include "inc-ffmpeg.h"
#include <stdint.h>
#include <iostream>
#include <queue>
#include <mutex>

using namespace std;

class DFrameCache
{
public:
    DFrameCache();
    virtual ~DFrameCache();

public:
    void SetMaxCacheSize(uint32_t size);
    int Size();
    int AddFrame(const AVFrame *frm);
    AVFrame *GetFrame(); //must call av_frame_free to free.
    int ClearAll();
    void RemoveFront();
    uint32_t FrameNum();

private:
    mutex m_mtxCache;
    queue<AVFrame *> m_cache;
    uint32_t m_maxCacheSize;
};