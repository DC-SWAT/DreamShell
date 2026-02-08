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
    void setColorAnimation(float r1, float g1, float b1, float a1,
                           float r2, float g2, float b2, float a2,
                           uint32_t duration, uint32_t hold_duration);

    virtual void draw(int list);
    virtual void nextFrame();

private:
    int   m_list;
    float m_w, m_h;
    float m_freq, m_amp, m_speed;
    float m_phase;

    bool m_anim_enabled;
    Color m_anim_c1, m_anim_c2;
    Color m_anim_start, m_anim_target;
    uint32_t m_anim_duration, m_anim_hold_duration;
    uint64_t m_anim_start_time;
    bool m_anim_is_holding;
    bool m_anim_direction;
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
    void TSU_WaveSetColorAnimation(Wave *wave_ptr, float r1, float g1, float b1, float a1,
                                   float r2, float g2, float b2, float a2,
                                   uint32_t duration, uint32_t hold_duration);

#ifdef __cplusplus
};
#endif

#endif	/* __TSUNAMI_DRW_WAVE_H */
