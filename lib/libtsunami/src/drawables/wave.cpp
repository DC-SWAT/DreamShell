/*
   Tsunami for KallistiOS ##version##

   wave.cpp

   Copyright (C) 2026 SWAT
   
*/

#include "drawables/wave.h"
#include <cmath>
#include <dc/pvr.h>

Wave::Wave(int list) {
    setObjectType(ObjectTypeEnum::WAVE_TYPE); 
    m_list = list;
    m_w = -1.0f;
    m_h = -1.0f;
    m_freq = 0.1f;
    m_amp = 10.0f;
    m_speed = 0.01f;
    m_phase = 0.0f;
}

Wave::~Wave() {
}

void Wave::setSize(float w, float h) {
    m_w = w;
    m_h = h;
}

void Wave::setParams(float freq, float amp, float speed) {
    m_freq = freq;
    m_amp = amp;
    m_speed = speed;
}

void Wave::setColor(float r, float g, float b, float a) {
    setTint(Color(a, r, g, b));
}

void Wave::nextFrame() {
    Drawable::nextFrame();
    m_phase += m_speed;
}

void Wave::draw(int list) {
    if (list != m_list)
        return;

    // Send header
    pvr_poly_cxt_t cxt;
    pvr_poly_hdr_t hdr;
    pvr_poly_cxt_col(&cxt, list);
    cxt.gen.culling = PVR_CULLING_NONE;
    pvr_poly_compile(&hdr, &cxt);
    pvr_prim(&hdr, sizeof(hdr));

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
    int segs = 64;
    float seg_w = w / segs;
    float start_x = tv.x - w/2;

    pvr_vertex_t vert;
    vert.flags = PVR_CMD_VERTEX;
    vert.oargb = 0;
    vert.z = tv.z;

    for (int i = 0; i <= segs; i++) {
        float ratio = (float)i / segs;
        float x = start_x + i * seg_w;
        
        float angle = ratio * m_freq * 2.0f * 3.14159f + m_phase;
        float s1 = sinf(angle);
        float s2 = sinf(angle * 2.3f + 1.0f);
        float s3 = sinf(angle * 0.7f + m_phase * 0.3f);
        float s4 = sinf(ratio * 3.0f + m_phase * 1.5f);

        float ripple = s4 * (0.5f + 0.5f * s3) * 0.01f;

        float y_off_top = (s1 + 0.02f * s2 + ripple) * m_amp;

        float angle_bot = angle + 0.7f;
        float s1b = sinf(angle_bot);
        float s2b = sinf(angle_bot * 2.1f + 1.5f);

        float y_off_bot = (s1b + 0.04f * s2b + ripple) * m_amp;
        float alpha_var = 0.8f + 0.2f * sinf(ratio * 15.0f - m_phase * 3.0f);

        uint8_t a = (uint8_t)(c.a * 255.0f * alpha_var);
        uint32_t top_col = (a << 24) | (r << 16) | (g << 8) | b;
        uint32_t bot_col = (a << 24) | ((r / 2) << 16) | ((g / 2) << 8) | (b / 2);

        // Top vertex
        vert.x = x;
        vert.y = tv.y - h/2 + y_off_top;
        vert.u = 0.0f;
        vert.v = 0.0f;
        vert.argb = top_col;
        pvr_prim(&vert, sizeof(vert));

        // Bottom vertex
        if (i == segs) vert.flags = PVR_CMD_VERTEX_EOL;
        vert.x = x;
        vert.y = tv.y + h/2 + y_off_bot;
        vert.u = 0.0f;
        vert.v = 0.0f;
        vert.argb = bot_col;
        pvr_prim(&vert, sizeof(vert));
    }

    Drawable::draw(list);
}


extern "C"
{
    Wave* TSU_WaveCreate(int list)
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

    void TSU_WaveSetParams(Wave *wave_ptr, float freq, float amp, float speed)
    {
        if (wave_ptr != NULL) {
            wave_ptr->setParams(freq, amp, speed);
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
}
