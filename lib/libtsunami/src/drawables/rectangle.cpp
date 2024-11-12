/*
   Tsunami for KallistiOS ##version##

   rectangle.cpp

   Copyright (C) 2024 Maniac Vera
   
*/

#include "drawables/rectangle.h"

Rectangle::Rectangle(int list, float x, float y, float width, float height, const Color &color, float zIndex, float borderWidth, const Color &borderColor, float radius = 0) {
	setObjectType(ObjectTypeEnum::RECTANGLE_TYPE);
	m_list = list;

	if (zIndex < 0) {
		zIndex = 0;
	}

	if (borderWidth < 0) {
		borderWidth = 0;
	}

	if (radius < 0) {
		radius = 0;
	}

	this->width = width;
	this->height = height;
	this->zIndex = zIndex;
	this->radius = radius;
	this->borderWidth = borderWidth;
	setReadOnly(true);

	if (borderWidth > 0) {
		this->borderColor = plx_pack_color(borderColor.a, borderColor.r, borderColor.g, borderColor.b);
	}
	else {
		this->borderColor = 0;
	}

	Drawable::setTranslate(Vector(x, y, zIndex, 1.0f));
	Drawable::setTint(color);
}

Rectangle::~Rectangle() {
}

void Rectangle::setSize(float w, float h) {
	width = w;
	height = h;
}

void Rectangle::getSize(float *w, float *h) {
	*w = width;
	*h = height;
}


void Rectangle::drawRectangle(float x, float y, float width, float height, uint32 color, float zIndex) {
	pvr_vertex_t vert;

	vert.flags = PVR_CMD_VERTEX;
	vert.x = x;
	vert.y = y;
	vert.z = zIndex;
	vert.u = vert.v = 0.0f;
	vert.argb = color;
	vert.oargb = 0;
	pvr_prim(&vert, sizeof(vert));

	vert.x = x;
	vert.y = y - height;
	pvr_prim(&vert, sizeof(vert));

	vert.x = x + width;
	vert.y = y;
	pvr_prim(&vert, sizeof(vert));

	vert.flags = PVR_CMD_VERTEX_EOL;
	vert.x = x + width;
	vert.y = y - height;
	pvr_prim(&vert, sizeof(vert));
}

void Rectangle::drawBox(float x, float y, float width, float height, float lineWidth, uint32 color, float zIndex)
{
	// HORIZONTAL LINES	
	drawRectangle(x, y + lineWidth, width, lineWidth, color, zIndex); // DOWN
	drawRectangle(x, y - height, width, lineWidth, color, zIndex); // UP

	// VERTICAL LINES
	drawRectangle(x - lineWidth, y, lineWidth, height, color, zIndex); // LEFT
	drawRectangle(x + width, y, lineWidth, height, color, zIndex); // RIGHT

	drawRectangle(x + width, y + lineWidth, lineWidth, lineWidth, color, zIndex); // RIGHT CORNER DOWN
	drawRectangle(x - lineWidth, y - height, lineWidth, lineWidth, color, zIndex); // LEFT CORNER UP
	drawRectangle(x - lineWidth, y + lineWidth, lineWidth, lineWidth, color, zIndex); // LEFT CORNER DOWN
	drawRectangle(x + width, y - height, lineWidth, lineWidth, color, zIndex); // RIGHT CORNER UP
}

void Rectangle::draw(int list) {
	if (list != m_list)
		return;

	pvr_poly_cxt_col(&cxt, list);
	pvr_poly_compile(&hdr, &cxt);
	pvr_prim(&hdr, sizeof(hdr));

	float w, h;
	if (width != -1 && height != -1) {
		w = width;
		h = height;
	} else {
		w = 1;
		h = 1;
	}

	const Vector & sv = getScale();
	w *= sv.x;
	h *= sv.y;

	const Vector & tv = getPosition();

	plx_vertex_t vert;
	if (list == PLX_LIST_TR_POLY) {
		vert.argb = getTint();
	} else {
		Color t = getTint(); 
		t.a = 1.0f;
		vert.argb = t;
	}
	vert.oargb = 0;
	
	if (borderWidth > 0) {
		drawBox(tv.x, tv.y, w, h, borderWidth, borderColor, tv.z);
	}
	drawRectangle(tv.x, tv.y, w, h, vert.argb, tv.z);
	Drawable::draw(list);
}


extern "C"
{
	Rectangle* TSU_RectangleCreate(int list, float x, float y, float width, float height, const Color *color, float zIndex, float radius)
	{
		if (list >= 0) {
			if (zIndex < 0) {
				zIndex = 1;
			}

			if (radius < 0) {
				radius = 0;
			}

			Color border_color = {0, 0.0f, 0.0f, 0.0f};
			return new Rectangle (list, x, y, width, height, *color, zIndex, 0, border_color, radius);
		}
		else {
			return NULL;
		}
	}

	Rectangle* TSU_RectangleCreateWithBorder(int list, float x, float y, float width, float height, const Color *color, float zIndex, float borderWidth, const Color *borderColor, float radius)
	{
		if (list >= 0) {
			if (borderWidth > 0) {
				if (zIndex < 0) {
					zIndex = 1;
				}

				if (radius < 0) {
					radius = 0;
				}
				
				return new Rectangle (list, x, y, width, height, *color, zIndex, borderWidth, *borderColor, radius);
			}
			else {
				return NULL;
			}
		}
		else {
			return NULL;
		}
	}

	void TSU_RectangleDestroy(Rectangle **rectangle_ptr)
	{
		if (*rectangle_ptr != NULL) {
			delete *rectangle_ptr;
			*rectangle_ptr = NULL;
		}
	}

	void TSU_RectangleSetSize(Rectangle *rectangle_ptr, float w, float h)
	{
		if (rectangle_ptr != NULL) {
			rectangle_ptr->setSize(w, h);
		}
	}
}