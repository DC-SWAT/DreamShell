/*
   Tsunami for KallistiOS ##version##

   wave.cpp

   Copyright (C) 2026 SWAT
   
*/

#include "drawables/wave.h"
#include <cmath>
#include <kos/timer.h>
#include <dc/pvr.h>
#include "../plx/prim.h"
#include "../plx/context.h"
#include <sh4zam/shz_trig.h>

Wave::Wave(pvr_list_type_t list) {
    setObjectType(ObjectTypeEnum::WAVE_TYPE); 
    m_list = list;
    m_segs = 32;
    m_w = -1.0f;
    m_h = -1.0f;
    m_freq = 0.1f;
    m_amp = 10.0f;
    m_speed = 0.01f;
    m_phase = 0.0f;
    m_anim_enabled = false;
    precomputeRatios();

    pvr_poly_cxt_t cxt;
    pvr_poly_cxt_col(&cxt, list);
    cxt.gen.culling = PVR_CULLING_NONE;
    pvr_poly_compile(&m_hdr, &cxt);
}

Wave::~Wave() {
}

void Wave::precomputeRatios() {
    int segs = m_segs;
    m_precomputed.resize(segs + 1);
    for (int i = 0; i <= segs; i++) {
        float ratio = (float)i / segs;
        m_precomputed[i].ratio = ratio;
        m_precomputed[i].angle_factor = ratio * m_freq * 2.0f * F_PI;
        m_precomputed[i].alpha_angle = ratio * 15.0f;
    }
}

void Wave::setSize(float w, float h) {
    m_w = w;
    m_h = h;
}

void Wave::setParams(float freq, float amp, float speed, int segs) {
    m_freq = freq;
    m_amp = amp;
    m_speed = speed;
    if (segs > 2) {
        m_segs = segs;
    }
    precomputeRatios();
}

void Wave::setColor(float r, float g, float b, float a) {
    setTint(Color(a, r, g, b));
    m_anim_enabled = false;
}

void Wave::setColorAnimation(float r1, float g1, float b1, float a1,
                             float r2, float g2, float b2, float a2,
                             uint32_t duration, uint32_t hold_duration) {
    m_anim_c1 = Color(a1, r1, g1, b1);
    m_anim_c2 = Color(a2, r2, g2, b2);
    m_anim_duration = duration;
    m_anim_hold_duration = hold_duration;

    m_anim_start = m_anim_c1;
    m_anim_target = m_anim_c2;
    m_anim_start_time = 0;
    m_anim_time = 0;
    m_last_update_time = 0;
    m_anim_is_holding = true;
    m_anim_direction = false;
    m_anim_enabled = true;

    setTint(m_anim_c1);
}

void Wave::nextFrame() {
    Drawable::nextFrame();
    m_phase += m_speed;

    if (m_anim_enabled) {
        uint64_t now = timer_ms_gettime64();
        if (m_last_update_time == 0) {
            m_last_update_time = now;
        }

        uint64_t dt = now - m_last_update_time;
        if (dt > 100) {
            dt = 16;
        }
        m_last_update_time = now;
        m_anim_time += dt;

        if (m_anim_is_holding) {
            if (m_anim_time >= m_anim_start_time + m_anim_hold_duration) {
                m_anim_is_holding = false;
                m_anim_start_time = m_anim_time;

                if (m_anim_direction) {
                    m_anim_start = m_anim_c2;
                    m_anim_target = m_anim_c1;
                }
                else {
                    m_anim_start = m_anim_c1;
                    m_anim_target = m_anim_c2;
                }
            }
        }
        else {
            if (m_anim_time >= m_anim_start_time + m_anim_duration) {
                m_anim_is_holding = true;
                m_anim_start_time = m_anim_time;
                m_anim_direction = !m_anim_direction;
                setTint(m_anim_target);
            }
            else {
                float t = (float)(m_anim_time - m_anim_start_time) / (float)m_anim_duration;
                if (t > 1.0f) t = 1.0f;

                float r = shz_lerpf(m_anim_start.r, m_anim_target.r, t);
                float g = shz_lerpf(m_anim_start.g, m_anim_target.g, t);
                float b = shz_lerpf(m_anim_start.b, m_anim_target.b, t);
                float a = shz_lerpf(m_anim_start.a, m_anim_target.a, t);

                setTint(Color(a, r, g, b));
            }
        }
    }
}

void Wave::draw(pvr_list_type_t list) {
    if (list != m_list)
        return;

    pvr_prim(&m_hdr, sizeof(m_hdr));

    float w, h;
    if (m_w != -1 && m_h != -1) {
        w = m_w;
        h = m_h;
    }
    else {
        w = 640;
        h = 128;
    }

    const Vector & sv = getScale();
    w *= sv.x;
    h *= sv.y;

    const Vector & tv = getPosition();
    Color c = getColor();

    uint8_t r = (uint8_t)(c.r * 255.0f);
    uint8_t g = (uint8_t)(c.g * 255.0f);
    uint8_t b = (uint8_t)(c.b * 255.0f);

    // Render as a grid of strips
    int segs = m_segs;
    float seg_w = w / segs;
    float start_x = tv.x - w/2;

    float phase_0_3 = m_phase * 0.3f;
    float phase_1_5 = m_phase * 1.5f;
    float phase_3_0 = m_phase * 3.0f;

    float tv_y = tv.y;
    float tv_z = tv.z;
    float h_half = h / 2.0f;

    for (int i = 0; i <= segs; i++) {
        float ratio = m_precomputed[i].ratio;
        float x = start_x + i * seg_w;

        float angle = m_precomputed[i].angle_factor + m_phase;

        shz_sincos_t sc1 = shz_sincosf(angle);
        float s1 = sc1.sin;
        float s2 = shz_sinf(angle * 2.3f + 1.0f);
        
        shz_sincos_t sc3 = shz_sincosf(angle * 0.7f + phase_0_3);
        float s3 = sc3.sin;
        float s4 = shz_sinf(ratio * 3.0f + phase_1_5);

        float ripple = s4 * (0.5f + 0.5f * s3) * 0.01f;

        float y_off_top = (s1 + 0.02f * s2 + ripple) * m_amp;

        float angle_bot = angle + 0.7f;
        shz_sincos_t sc1b = shz_sincosf(angle_bot);
        float s1b = sc1b.sin;
        float s2b = shz_sinf(angle_bot * 2.1f + 1.5f);

        float y_off_bot = (s1b + 0.04f * s2b + ripple) * m_amp;
        float alpha_var = 0.8f + 0.2f * shz_sinf(m_precomputed[i].alpha_angle - phase_3_0);

        uint8_t a = (uint8_t)(c.a * 255.0f * alpha_var);

        uint32_t top_col = (a << 24) | (r << 16) | (g << 8) | b;
        uint32_t bot_col = (a << 24) | ((r >> 1) << 16) | ((g >> 1) << 8) | (b >> 1);

        // Top vertex
        plx_vert_inp(PLX_VERT, x, tv_y - h_half + y_off_top, tv_z, top_col);

        // Bottom vertex
        if (i == segs) {
            plx_vert_inp(PLX_VERT_EOS, x, tv_y + h_half + y_off_bot, tv_z, bot_col);
        }
        else {
            plx_vert_inp(PLX_VERT, x, tv_y + h_half + y_off_bot, tv_z, bot_col);
        }
    }

    Drawable::draw(list);
}


extern "C"
{
    Wave* TSU_WaveCreate(pvr_list_type_t list)
    {
        return new Wave(list);
    }

    void TSU_WaveDestroy(Wave **wave_ptr)
    {
        if (*wave_ptr != NULL) {
            delete *wave_ptr;
            *wave_ptr = NULL;
        }
    }

    void TSU_WaveSetParams(Wave *wave_ptr, float freq, float amp, float speed, int segs)
    {
        if (wave_ptr != NULL) {
            wave_ptr->setParams(freq, amp, speed, segs);
        }
    }

    void TSU_WaveSetSize(Wave *wave_ptr, float w, float h)
    {
        if (wave_ptr != NULL) {
            wave_ptr->setSize(w, h);
        }
    }

    void TSU_WaveSetColor(Wave *wave_ptr, float r, float g, float b, float a)
    {
        if (wave_ptr != NULL) {
            wave_ptr->setColor(r, g, b, a);
        }
    }

    void TSU_WaveSetColorAnimation(Wave *wave_ptr, float r1, float g1, float b1, float a1,
                                   float r2, float g2, float b2, float a2,
                                   uint32_t duration, uint32_t hold_duration)
    {
        if (wave_ptr != NULL) {
            wave_ptr->setColorAnimation(r1, g1, b1, a1, r2, g2, b2, a2, duration, hold_duration);
        }
    }
}
