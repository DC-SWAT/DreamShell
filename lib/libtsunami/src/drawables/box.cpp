/*
   Tsunami for KallistiOS ##version##

   box.cpp

   Copyright (C) 2024-2025 Maniac Vera
   
*/

#include <dc/fmath.h>
#include "drawables/box.h"

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
}

Box::~Box() {
}

void Box::setSize(float w, float h) {
	width = w;
	height = h;
}

void Box::drawBox(float x, float y, float width, float height, float lineWidth, uint32 color, float zIndex, int radius)
{
	const float PI_2 = F_PI / 2;
	float dx = 0, dy = 0;
	radius = radius <= 0 ? 0 : radius + 5;

	float quantity_slices = PI_2 / (float)(radius - 1);
	x -= lineWidth;
	y += lineWidth;
	width += lineWidth * 2;
	height += lineWidth * 2;

	// LEFT BOTTOM VERTICES
	plx_vert_inp(PLX_VERT, x, (y - radius), zIndex, color);

	// LEFT BOTTOM CORNER
	if (radius > 0)
	{
		for (float slices_count = 0, count = 0; count <= radius; count++, slices_count += quantity_slices)
		{
			dx = (x + radius) - fcos(slices_count) * radius;
			dy = (y - radius) + fsin(slices_count) * radius;
			plx_vert_inp(PLX_VERT, dx, dy, zIndex, color);

			dx = (x + (radius + lineWidth)) - fcos(slices_count) * radius;
			dy = (y - (radius + lineWidth)) + fsin(slices_count) * radius;
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
		for (float slices_count = 0, count = 0; count <= radius; count++, slices_count += quantity_slices)
		{
			dx = (x + width - radius) + fsin(slices_count) * radius;
			dy = (y - radius) + fcos(slices_count) * radius;
			plx_vert_inp(PLX_VERT, dx, dy, zIndex, color);

			dx = (x + width - (radius + lineWidth)) + fsin(slices_count) * radius;
			dy = (y - (radius + lineWidth)) + fcos(slices_count) * radius;
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
		for (float slices_count = 0, count = 0; count <= radius; count++, slices_count += quantity_slices)
		{
			dx = (x + width - radius) + fcos(slices_count) * radius;
			dy = (y - height + radius) - fsin(slices_count) * radius;
			plx_vert_inp(PLX_VERT, dx, dy, zIndex, color);

			dx = (x + width - (radius + lineWidth)) + fcos(slices_count) * radius;
			dy = (y - height + (radius + lineWidth)) - fsin(slices_count) * radius;
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
		for (float slices_count = 0, count = 0; count <= radius; count++, slices_count += quantity_slices)
		{
			dx = (x + radius) - fsin(slices_count) * radius;
			dy = (y - height + radius) - fcos(slices_count) * radius;
			plx_vert_inp(PLX_VERT, dx, dy, zIndex, color);

			dx = (x + (radius + lineWidth)) - fsin(slices_count) * radius;
			dy = (y - height + (radius + lineWidth)) - fcos(slices_count) * radius;
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

	pvr_poly_cxt_col(&cxt, list);
	cxt.gen.culling = PVR_CULLING_NONE;
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