/*
   Tsunami for KallistiOS ##version##

   gradient.cpp

   Copyright (C) 2026 SWAT

*/

#include <dc/fmath.h>
#include "../plx/prim.h"
#include "../plx/context.h"
#include "drawables/gradient.h"

Gradient::Gradient(pvr_list_type_t list, float width, float height, float zIndex)
{
	setObjectType(ObjectTypeEnum::GRADIENT_TYPE);
	m_list = list;

	if (zIndex < 0) {
		zIndex = 0;
	}

	this->m_width = width;
	this->m_height = height;
	this->zIndex = zIndex;

    for(int i = 0; i < 4; i++) {
        m_colors[i] = 0xFFFFFFFF;
        m_cached_colors[i] = 0xFFFFFFFF;
    }
    m_cached_alpha = -1.0f;

	Drawable::setTranslate(Vector(0, 0, zIndex, 1.0f));

	pvr_poly_cxt_t cxt;
	pvr_poly_cxt_col(&cxt, list);
	cxt.gen.culling = PVR_CULLING_NONE;
	pvr_poly_compile(&m_hdr, &cxt);
}

Gradient::~Gradient()
{
}

void Gradient::setColors(uint32_t tl, uint32_t tr, uint32_t br, uint32_t bl)
{
    m_colors[0] = tl;
    m_colors[1] = tr;
    m_colors[2] = br;
    m_colors[3] = bl;
    m_cached_alpha = -1.0f;
}

void Gradient::draw(pvr_list_type_t list)
{
	if (list != m_list)
		return;

	pvr_prim(&m_hdr, sizeof(m_hdr));

	float w, h;
	if (m_width > 0 && m_height > 0) {
		w = m_width;
		h = m_height;
	}
	else {
		w = 640;
		h = 480;
	}

	const Vector &sv = getScale();
	w *= sv.x;
	h *= sv.y;

	const Vector &tv = getPosition();
    float x = tv.x;
    float y = tv.y;
    float z = tv.z;

    // Apply alpha from Drawable
    float alpha = getAlpha();

    if (alpha != m_cached_alpha) {
        m_cached_alpha = alpha;
        for(int i = 0; i < 4; i++) {
            if (alpha < 0.99f) {
                uint32_t col = m_colors[i];
                uint32_t a = (col >> 24) & 0xFF;
                a = (uint32_t)(a * alpha);
                m_cached_colors[i] = (a << 24) | (col & 0xFFFFFF);
            }
            else {
                m_cached_colors[i] = m_colors[i];
            }
        }
    }

    plx_vert_inp(PLX_VERT, x, y, z, m_cached_colors[3]);
    plx_vert_inp(PLX_VERT, x, y - h, z, m_cached_colors[0]);
    plx_vert_inp(PLX_VERT, x + w, y, z, m_cached_colors[2]);
    plx_vert_inp(PLX_VERT_EOS, x + w, y - h, z, m_cached_colors[1]);

	Drawable::draw(list);
}

extern "C"
{
	Gradient *TSU_GradientCreate(pvr_list_type_t list, float width, float height, float zIndex)
	{
		if (list >= 0) {
			if (zIndex < 0) {
				zIndex = 1;
			}
			return new Gradient(list, width, height, zIndex);
		}
		else {
			return NULL;
		}
	}

	void TSU_GradientDestroy(Gradient **gradient_ptr)
	{
		if (*gradient_ptr != NULL) {
			delete *gradient_ptr;
			*gradient_ptr = NULL;
		}
	}

	void TSU_GradientSetSize(Gradient *gradient_ptr, float w, float h)
	{
		if (gradient_ptr != NULL) {
			gradient_ptr->setSize(w, h);
		}
	}

    void TSU_GradientSetColors(Gradient *gradient_ptr, uint32_t tl, uint32_t tr, uint32_t br, uint32_t bl)
    {
        if (gradient_ptr != NULL) {
            gradient_ptr->setColors(tl, tr, br, bl);
        }
    }
}
