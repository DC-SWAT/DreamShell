/*
   Tsunami for KallistiOS ##version##

   box.cpp

   Copyright (C) 2024-2025 Maniac Vera
   Copyright (C) 2026 SWAT
   
*/

#include <dc/fmath.h>
#include "drawables/box.h"
#include "../plx/prim.h"
#include "../plx/context.h"

Box::Box(pvr_list_type_t list, float x, float y, float width, float height, float borderWidth, const Color &color, float zIndex, float radius = 0) {
	m_list = list;
	setObjectType(ObjectTypeEnum::BOX_TYPE);

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

	Drawable::setTranslate(Vector(x, y, zIndex, 1.0f));
	Drawable::setTint(color);
	precomputePoints();

	pvr_poly_cxt_t cxt;
	pvr_poly_cxt_col(&cxt, list);
	cxt.gen.culling = PVR_CULLING_NONE;
	pvr_poly_compile(&m_hdr, &cxt);
}

Box::~Box() {
}

void Box::precomputePoints() {
	int rad = radius <= 0 ? 0 : (int)radius + 5;
	if (rad > 0) {
		m_precomputed.resize(rad + 1);
		const float PI_2 = F_PI / 2;
		float quantity_slices = PI_2 / (float)(rad - 1);
		for (int i = 0; i <= rad; i++) {
			float slices_count = i * quantity_slices;
			m_precomputed[i].cos_val = fcos(slices_count);
			m_precomputed[i].sin_val = fsin(slices_count);
		}
	}
}

void Box::setSize(float w, float h) {
	width = w;
	height = h;
}

void Box::drawBox(float x, float y, float width, float height, float lineWidth, uint32 color, float zIndex, int radius)
{
	float dx = 0, dy = 0;
	radius = radius <= 0 ? 0 : radius + 5;

	x -= lineWidth;
	y += lineWidth;
	width += lineWidth * 2;
	height += lineWidth * 2;

	// LEFT BOTTOM VERTICES
	plx_vert_inp(PLX_VERT, x, (y - radius), zIndex, color);

	// LEFT BOTTOM CORNER
	if (radius > 0)
	{
		for (int count = 0; count <= radius; count++)
		{
			dx = (x + radius) - m_precomputed[count].cos_val * radius;
			dy = (y - radius) - m_precomputed[count].sin_val * radius;
			plx_vert_inp(PLX_VERT, dx, dy, zIndex, color);

			dx = (x + (radius + lineWidth)) - m_precomputed[count].cos_val * radius;
			dy = (y - (radius + lineWidth)) - m_precomputed[count].sin_val * radius;
			plx_vert_inp(PLX_VERT, dx, dy, zIndex, color);
		}
	}

	// FINISH LEFT BOTTOM CORNER
	plx_vert_inp(PLX_VERT, x + radius, (y - lineWidth), zIndex, color);
	plx_vert_inp(PLX_VERT, x + radius, y, zIndex, color);

	// RIGHT BOTTOM VERTICES
	plx_vert_inp(PLX_VERT, x + width - radius, (y - lineWidth), zIndex, color);
	plx_vert_inp(PLX_VERT, x + width - radius, y, zIndex, color);

	// RIGHT BOTTOM CORNER
	if (radius > 0)
	{
		for (int count = 0; count <= radius; count++)
		{
			dx = (x + width - radius) + m_precomputed[count].sin_val * radius;
			dy = (y - radius) - m_precomputed[count].cos_val * radius;
			plx_vert_inp(PLX_VERT, dx, dy, zIndex, color);

			dx = (x + width - (radius + lineWidth)) + m_precomputed[count].sin_val * radius;
			dy = (y - (radius + lineWidth)) - m_precomputed[count].cos_val * radius;
			plx_vert_inp(PLX_VERT, dx, dy, zIndex, color);
		}
	}

	// FINISH RIGHT BOTTOM CORNER
	plx_vert_inp(PLX_VERT, x + width, (y - radius), zIndex, color);
	plx_vert_inp(PLX_VERT, x + width - lineWidth, (y - radius), zIndex, color);

	// TOP BOTTOM VERTICES
	plx_vert_inp(PLX_VERT, x + width, (y - height + radius), zIndex, color);
	plx_vert_inp(PLX_VERT, x + width - lineWidth, (y - height + radius), zIndex, color);

	// RIGHT TOP CORNER
	if (radius > 0)
	{
		for (int count = 0; count <= radius; count++)
		{
			dx = (x + width - radius) + m_precomputed[count].cos_val * radius;
			dy = (y - height + radius) - m_precomputed[count].sin_val * radius;
			plx_vert_inp(PLX_VERT, dx, dy, zIndex, color);

			dx = (x + width - (radius + lineWidth)) + m_precomputed[count].cos_val * radius;
			dy = (y - height + (radius + lineWidth)) - m_precomputed[count].sin_val * radius;
			plx_vert_inp(PLX_VERT, dx, dy, zIndex, color);
		}
	}

	// FINISH RIGHT TOP CORNER
	plx_vert_inp(PLX_VERT, x + width - radius, y - height, zIndex, color);
	plx_vert_inp(PLX_VERT, x + width - radius, (y - height + lineWidth), zIndex, color);

	// LEFT TOP VERTICES
	plx_vert_inp(PLX_VERT, x + radius, y - height, zIndex, color);
	plx_vert_inp(PLX_VERT, x + radius, (y - height + lineWidth), zIndex, color);

	// LEFT TOP CORNER
	if (radius > 0)
	{
		for (int count = 0; count <= radius; count++)
		{
			dx = (x + radius) - m_precomputed[count].sin_val * radius;
			dy = (y - height + radius) - m_precomputed[count].cos_val * radius;
			plx_vert_inp(PLX_VERT, dx, dy, zIndex, color);

			dx = (x + (radius + lineWidth)) - m_precomputed[count].sin_val * radius;
			dy = (y - height + (radius + lineWidth)) - m_precomputed[count].cos_val * radius;
			plx_vert_inp(PLX_VERT, dx, dy, zIndex, color);
		}
	}

	// FINISH LEFT TOP CORNER
	plx_vert_inp(PLX_VERT, x + lineWidth, (y - height + radius), zIndex, color);
	plx_vert_inp(PLX_VERT, x, (y - height + radius), zIndex, color);

	// LEFT LINE
	plx_vert_inp(PLX_VERT, x + lineWidth, (y - radius), zIndex, color);
	plx_vert_inp(PLX_VERT_EOS, x, (y - radius), zIndex, color);
}

void Box::draw(pvr_list_type_t list) {
	if (list != m_list)
		return;

	pvr_prim(&m_hdr, sizeof(m_hdr));

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
	
	drawBox(tv.x, tv.y, w, h, borderWidth, vert.argb, tv.z, radius);

	Drawable::draw(list);
}


extern "C"
{
	Box* TSU_BoxCreate(pvr_list_type_t list, float x, float y, float width, float height, float border_width, const Color *color, float zIndex, float radius)
	{
		if (list >= 0) {
			if (zIndex < 0) {
				zIndex = 1;
			}

			if (radius < 0) {
				radius = 0;
			}

			return new Box(list, x, y, width, height, border_width, *color, zIndex, radius);
		}
		else {
			return NULL;
		}
	}

	void TSU_BoxDestroy(Box **box_ptr)
	{
		if (*box_ptr != NULL) {
			delete *box_ptr;
			*box_ptr = NULL;
		}
	}

	void TSU_BoxSetSize(Box *box_ptr, float w, float h)
	{
		if (box_ptr != NULL) {
			box_ptr->setSize(w, h);
		}
	}
}