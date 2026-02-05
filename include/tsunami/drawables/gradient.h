/*
   Tsunami for KallistiOS ##version##

   gradient.h

   Copyright (C) 2026 SWAT

*/

#ifndef __TSUNAMI_DRW_GRADIENT_H
#define __TSUNAMI_DRW_GRADIENT_H

#include "../../plx/list.h"
#include "../../plx/sprite.h"
#include "../drawable.h"
#include "../texture.h"
#include "../color.h"

#ifdef __cplusplus

class Gradient : public Drawable {
public:
	Gradient(int list, float width, float height, float zIndex);
	virtual ~Gradient();
	virtual void draw(int list);

    void setColors(uint32_t tl, uint32_t tr, uint32_t br, uint32_t bl);

private:
	int m_list;
	float zIndex;
    uint32_t m_colors[4];
	pvr_poly_hdr_t hdr;
	pvr_poly_cxt_t cxt;
};

#else

typedef struct gradient Gradient;

#endif

#ifdef __cplusplus
extern "C"
{
#endif

Gradient* TSU_GradientCreate(int list, float width, float height, float zIndex);
void TSU_GradientDestroy(Gradient **gradient_ptr);
void TSU_GradientSetSize(Gradient *gradient_ptr, float width, float height);
void TSU_GradientSetColors(Gradient *gradient_ptr, uint32_t tl, uint32_t tr, uint32_t br, uint32_t bl);

#ifdef __cplusplus
};
#endif

#endif	/* __TSUNAMI_DRW_GRADIENT_H */
