/*
   Tsunami for KallistiOS ##version##

   banner.h

   Copyright (C) 2002 Megan Potter
   Copyright (C) 2024 Maniac Vera

*/

#ifndef __TSUNAMI_DRW_TRIANGLE_H
#define __TSUNAMI_DRW_TRIANGLE_H

#include "../../plx/list.h"
#include "../../plx/sprite.h"
#include "../drawable.h"
#include "../texture.h"
#include "../color.h"

#ifdef __cplusplus

#include <memory>


class Triangle : public Drawable {
public:
	Triangle(int list, float x1, float y1, float x2, float y2, float x3, float y3, const Color &color, float zIndex, float borderWidth, const Color &borderColor, float radius);
	virtual ~Triangle();
	virtual void draw(int list);

private:
	int m_list;
	float x1, y1, x2, y2, x3, y3, radius, zIndex, borderWidth;
	uint32 borderColor;
	pvr_poly_hdr_t hdr;
	pvr_poly_cxt_t cxt;

	void drawBox(float x1, float y1, float x2, float y2, float x3, float y3, float lineWidth, uint32 color, float zIndex);
	void drawTriangle(float x1, float y1, float x2, float y2, float x3, float y3, uint32 color, float zIndex);	
};

#else

typedef struct triangle Triangle;

#endif

#ifdef __cplusplus
extern "C"
{
#endif

Triangle* TSU_TriangleCreate(int list, float x1, float y1, float x2, float y2, float x3, float y3, const Color *color, float zIndex, float radius);
Triangle* TSU_TriangleCreateWithBorder(int list, float x1, float y1, float x2, float y2, float x3, float y3, const Color *color, float zIndex, float borderWidth, const Color *borderColor, float radius);
void TSU_TriangleDestroy(Triangle **triangle_ptr);

#ifdef __cplusplus
};
#endif

#endif	/* __TSUNAMI_DRW_TRIANGLE_H */
