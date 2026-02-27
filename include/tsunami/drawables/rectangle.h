/*
   Tsunami for KallistiOS ##version##

   rectangle.h

   Copyright (C) 2002 Megan Potter
   Copyright (C) 2024-2025 Maniac Vera

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
	Rectangle(pvr_list_type_t list, float x, float y, float width, float height, const Color &color, float zIndex, float borderWidth, const Color &borderColor, float radius);
	virtual ~Rectangle();
	virtual void draw(pvr_list_type_t list);
	virtual void setBorderColor(Color color);

private:
	pvr_list_type_t m_list;
	float radius, zIndex, borderWidth;
	uint32 borderColor;
	pvr_poly_hdr_t hdr;
	pvr_poly_cxt_t cxt;

	void drawBox(float x, float y, float width, float height, float lineWidth, uint32 color, float zIndex, int radius);
	void drawRectangle(float x, float y, float width, float height, uint32 color, float zIndex, int radius);
};

#else

typedef struct rectangle Rectangle;

#endif

#ifdef __cplusplus
extern "C"
{
#endif

Rectangle* TSU_RectangleCreate(pvr_list_type_t list, float x, float y, float width, float height, const Color *color, float zIndex, float radius);
Rectangle* TSU_RectangleCreateWithBorder(pvr_list_type_t list, float x, float y, float width, float height, const Color *color, float zIndex, float borderWidth, const Color *borderColor, float radius);
void TSU_RectangleDestroy(Rectangle **rectangle_ptr);
void TSU_RectangleSetSize(Rectangle *rectangle_ptr, float width, float height);
void TSU_DrawableSetBorderColor(Rectangle *rectangle_ptr, const Color *color);

#ifdef __cplusplus
};
#endif

#endif	/* __TSUNAMI_DRW_RECTANGLE_H */
