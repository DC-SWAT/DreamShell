/*
   Tsunami for KallistiOS ##version##

   banner.h

   Copyright (C) 2002 Megan Potter
   Copyright (C) 2024 Maniac Vera

*/

#ifndef __TSUNAMI_DRW_RECTANGLE_H
#define __TSUNAMI_DRW_RECTANGLE_H

#include "../../plx/list.h"
#include "../../plx/sprite.h"
#include "../drawable.h"
#include "../texture.h"
#include "../color.h"

#ifdef __cplusplus

#include <memory>


class Rectangle : public Drawable {
public:
	Rectangle(int list, float x, float y, float width, float height, const Color &color, float zIndex, float borderWidth, const Color &borderColor, float radius);
	virtual ~Rectangle();

	void setSize(float w, float h);

	virtual void draw(int list);

private:
	int m_list;
	float width, height, radius, zIndex, borderWidth;
	uint32 borderColor;
	pvr_poly_hdr_t hdr;
	pvr_poly_cxt_t cxt;

	void drawBox(float x, float y, float width, float height, float lineWidth, uint32 color, float zIndex);
	void drawRectangle(float x, float y, float width, float height, uint32 color, float zIndex);	
};

#else

typedef struct rectangle Rectangle;

#endif

#ifdef __cplusplus
extern "C"
{
#endif

Rectangle* TSU_RectangleCreate(int list, float x, float y, float width, float height, const Color *color, float zIndex, float radius);
Rectangle* TSU_RectangleCreateWithBorder(int list, float x, float y, float width, float height, const Color *color, float zIndex, float borderWidth, const Color *borderColor, float radius);
void TSU_RectangleDestroy(Rectangle **rectangle_ptr);
void TSU_RectangleSetSize(Rectangle *rectangle_ptr, float width, float height);

#ifdef __cplusplus
};
#endif

#endif	/* __TSUNAMI_DRW_RECTANGLE_H */
