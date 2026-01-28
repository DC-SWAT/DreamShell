/*
   Tsunami for KallistiOS ##version##

   wave.h

   Copyright (C) 2026 SWAT

*/

#ifndef __TSUNAMI_DRW_WAVE_H
#define __TSUNAMI_DRW_WAVE_H

#include <plx/list.h>
#include <plx/sprite.h>
#include "../drawable.h"
#include "../color.h"

#ifdef __cplusplus

class Wave : public Drawable {
public:
    Wave(int list);
    virtual ~Wave();

    void setSize(float w, float h);
    void setParams(float freq, float amp, float speed);
    void setColor(float r, float g, float b, float a);

    virtual void draw(int list);
    virtual void nextFrame();

private:
    int   m_list;
    float m_w, m_h;
    float m_freq, m_amp, m_speed;
    float m_phase;
};

#else

typedef struct wave Wave;

#endif

#ifdef __cplusplus
extern "C"
{
#endif

    Wave* TSU_WaveCreate(int list);
    void TSU_WaveDestroy(Wave **wave_ptr);
    void TSU_WaveSetParams(Wave *wave_ptr, float freq, float amp, float speed);
    void TSU_WaveSetSize(Wave *wave_ptr, float w, float h);
    void TSU_WaveSetColor(Wave *wave_ptr, float r, float g, float b, float a);

#ifdef __cplusplus
};
#endif

#endif	/* __TSUNAMI_DRW_WAVE_H */
