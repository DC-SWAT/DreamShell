/*
   Tsunami for KallistiOS ##version##

   triangle.cpp

   Copyright (C) 2024 Maniac Vera
   Copyright (C) 2026 SWAT
   
*/

#include "drawables/triangle.h"
#include "../plx/prim.h"
#include "../plx/context.h"

Triangle::Triangle(pvr_list_type_t list, float x1, float y1, float x2, float y2, float x3, float y3, const Color &color, float zIndex, float borderWidth, const Color &borderColor, float radius = 0) {
	setObjectType(ObjectTypeEnum::TRIANGLE_TYPE);
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

	this->x1 = x1;
	this->y1 = y1;
	this->x2 = x2;
	this->y2 = y2;
	this->x3 = x3;
	this->y3 = y3;

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

	Drawable::setTranslate(Vector(x1, y1, zIndex, 1.0f));
	Drawable::setTint(color);

	pvr_poly_cxt_t cxt;
	pvr_poly_cxt_col(&cxt, list);
	cxt.gen.culling = PVR_CULLING_NONE;
	pvr_poly_compile(&m_hdr, &cxt);
}

Triangle::~Triangle() {
}

void Triangle::drawTriangle(float x1, float y1, float x2, float y2, float x3, float y3, uint32 color, float zIndex) {
	plx_vert_inp(PLX_VERT, x1, y1, zIndex, color);
	plx_vert_inp(PLX_VERT, x2, y2, zIndex, color);
	plx_vert_inp(PLX_VERT, x3, y3, zIndex, color);
	plx_vert_inp(PLX_VERT_EOS, x1, y1, zIndex, color);
}

void Triangle::drawBox(float x1, float y1, float x2, float y2, float x3, float y3, float lineWidth, uint32 color, float zIndex) {
}

void Triangle::draw(pvr_list_type_t list) {
	if (list != m_list)
		return;

	pvr_prim(&m_hdr, sizeof(m_hdr));

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
	
	drawTriangle(tv.x + x1, tv.y + y1, tv.x + x2, tv.y + y2, tv.x + x3, tv.y + y3, vert.argb, tv.z);
	Drawable::draw(list);
}


extern "C"
{
	Triangle* TSU_TriangleCreate(pvr_list_type_t list, float x1, float y1, float x2, float y2, float x3, float y3, const Color *color, float zIndex, float radius)
	{
		if (list >= 0) {
			if (zIndex < 0) {
				zIndex = 1;
			}

			if (radius < 0) {
				radius = 0;
			}

			Color border_color = {0, 0.0f, 0.0f, 0.0f};
			return new Triangle (list, x1, y1, x2, y2, x3, y3, *color, zIndex, 0, border_color, radius);
		}
		else {
			return NULL;
		}
	}

	Triangle* TSU_TriangleCreateWithBorder(pvr_list_type_t list, float x1, float y1, float x2, float y2, float x3, float y3, const Color *color, float zIndex, float borderWidth, const Color *borderColor, float radius)
	{
		if (list >= 0) {
			if (borderWidth > 0) {
				if (zIndex < 0) {
					zIndex = 1;
				}

				if (radius < 0) {
					radius = 0;
				}
				
				return new Triangle (list, x1, y1, x2, y2, x3, y3, *color, zIndex, borderWidth, *borderColor, radius);
			}
			else {
				return NULL;
			}
		}
		else {
			return NULL;
		}
	}

	void TSU_TriangleDestroy(Triangle **triangle_ptr)
	{
		if (*triangle_ptr != NULL) {
			delete *triangle_ptr;
			*triangle_ptr = NULL;
		}
	}
}