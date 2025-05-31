/*
   Tsunami for KallistiOS ##version##

   rectangle.cpp

   Copyright (C) 2024-2025 Maniac Vera

*/

#include <dc/fmath.h>
#include "drawables/rectangle.h"

Rectangle::Rectangle(int list, float x, float y, float width, float height, const Color &color, float zIndex, float borderWidth, const Color &borderColor, float radius = 0)
{
	setObjectType(ObjectTypeEnum::RECTANGLE_TYPE);
	m_list = list;

	if (zIndex < 0)
	{
		zIndex = 0;
	}

	if (borderWidth < 0)
	{
		borderWidth = 0;
	}

	if (radius < 0)
	{
		radius = 0;
	}

	this->width = width;
	this->height = height;
	this->zIndex = zIndex;
	this->radius = radius;
	this->borderWidth = borderWidth;
	setReadOnly(true);

	if (borderWidth > 0)
	{
		this->borderColor = plx_pack_color(borderColor.a, borderColor.r, borderColor.g, borderColor.b);
	}
	else
	{
		this->borderColor = 0;
	}

	Drawable::setTranslate(Vector(x, y, zIndex, 1.0f));
	Drawable::setTint(color);
}

Rectangle::~Rectangle()
{
}

void Rectangle::setSize(float w, float h)
{
	width = w;
	height = h;
}

void Rectangle::getSize(float *w, float *h)
{
	*w = width;
	*h = height;
}

void Rectangle::setBorderColor(Color color)
{
	if (borderWidth > 0)
	{
		this->borderColor = plx_pack_color(color.a, color.r, color.g, color.b);
	}
}

void Rectangle::drawRectangle(float x, float y, float width, float height, uint32 color, float zIndex, int radius)
{	
	radius = radius <= 0 ? 0 : radius + 5;

	// LEFT BOTTOM CORNER
	if (radius > 0)
	{
		const float PI_2 = F_PI / 2;
		float dx = 0, dy = 0;
		float quantity_slices = PI_2 / (float)(radius);
		float center_x = x + width / 2;
		float center_y = y - height / 2;
		
		// CENTER RECTANGLE
		plx_vert_inp(PLX_VERT, center_x, center_y, zIndex, color);
		
		// LEFT CORNER BOTTOM VERTICES
		for (float slices_count = 0, count = 0; count <= radius; count++, slices_count += quantity_slices)
		{
			dx = (x + radius) - fcos(slices_count) * radius;
			dy = (y - radius) + fsin(slices_count) * radius;
			plx_vert_inp(PLX_VERT, dx, dy, zIndex, color);

			plx_vert_inp(PLX_VERT, center_x, center_y, zIndex, color);
		}

		// RIGHT CORNER BOTTOM VERTICES
		for (float slices_count = 0, count = 0; count <= radius; count++, slices_count += quantity_slices)
		{
			dx = (x + width - radius) + fsin(slices_count) * radius;
			dy = (y - radius) + fcos(slices_count) * radius;
			plx_vert_inp(PLX_VERT, dx, dy, zIndex, color);

			plx_vert_inp(PLX_VERT, center_x, center_y, zIndex, color);
		}
		
		// RIGHT CORNER TOP VERTICES
		for (float slices_count = 0, count = 0; count <= radius; count++, slices_count += quantity_slices)
		{
			dx = (x + width - radius) + fcos(slices_count) * radius;
			dy = (y - height + radius) - fsin(slices_count) * radius;
			plx_vert_inp(PLX_VERT, dx, dy, zIndex, color);

			plx_vert_inp(PLX_VERT, center_x, center_y, zIndex, color);
		}

		// LEFT CORNER TOP VERTICES
		for (float slices_count = 0, count = 0; count <= radius; count++, slices_count += quantity_slices)
		{
			dx = (x + radius) - fsin(slices_count) * radius;
			dy = (y - height + radius) - fcos(slices_count) * radius;
			plx_vert_inp(PLX_VERT, dx, dy, zIndex, color);

			plx_vert_inp(PLX_VERT, center_x, center_y, zIndex, color);
		}
		
		// CLOSE RECTANGLE
		plx_vert_inp(PLX_VERT_EOS, (x + radius) - fcos(0) * radius, (y - radius) + fsin(0) * radius, zIndex, color);
	}
	else
	{
		// LEFT BOTTOM VERTEX
		plx_vert_inp(PLX_VERT, x, y, zIndex, color);

		// LEFT TOP VERTEX
		plx_vert_inp(PLX_VERT, x, y - height, zIndex, color);

		// RIGHT BOTTOM VERTEX
		plx_vert_inp(PLX_VERT, x + width, y, zIndex, color);

		// RIGHT TOP VERTEX (end of strip)
		plx_vert_inp(PLX_VERT_EOS, x + width, y - height, zIndex, color);
	}

}

void Rectangle::drawBox(float x, float y, float width, float height, float lineWidth, uint32 color, float zIndex, int radius)
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

void Rectangle::draw(int list)
{
	if (list != m_list)
		return;

	pvr_poly_cxt_col(&cxt, list);
	cxt.gen.culling = PVR_CULLING_NONE;
	pvr_poly_compile(&hdr, &cxt);
	pvr_prim(&hdr, sizeof(hdr));

	float w, h;
	if (width != -1 && height != -1)
	{
		w = width;
		h = height;
	}
	else
	{
		w = 1;
		h = 1;
	}

	const Vector &sv = getScale();
	w *= sv.x;
	h *= sv.y;

	const Vector &tv = getPosition();

	plx_vertex_t vert;
	if (list == PLX_LIST_TR_POLY)
	{
		vert.argb = getTint();
	}
	else
	{
		Color t = getTint();
		t.a = 1.0f;
		vert.argb = t;
	}
	vert.oargb = 0;

	if (borderWidth > 0)
	{
		drawBox(tv.x, tv.y, w, h, borderWidth, borderColor, tv.z, radius);
	}
	drawRectangle(tv.x, tv.y, w, h, vert.argb, tv.z, radius);
	Drawable::draw(list);
}

extern "C"
{
	Rectangle *TSU_RectangleCreate(int list, float x, float y, float width, float height, const Color *color, float zIndex, float radius)
	{
		if (list >= 0)
		{
			if (zIndex < 0)
			{
				zIndex = 1;
			}

			if (radius < 0)
			{
				radius = 0;
			}

			Color border_color = {0, 0.0f, 0.0f, 0.0f};
			return new Rectangle(list, x, y, width, height, *color, zIndex, 0, border_color, radius);
		}
		else
		{
			return NULL;
		}
	}

	Rectangle *TSU_RectangleCreateWithBorder(int list, float x, float y, float width, float height, const Color *color, float zIndex, float borderWidth, const Color *borderColor, float radius)
	{
		if (list >= 0)
		{
			if (borderWidth > 0)
			{
				if (zIndex < 0)
				{
					zIndex = 1;
				}

				if (radius < 0)
				{
					radius = 0;
				}

				return new Rectangle(list, x, y, width, height, *color, zIndex, borderWidth, *borderColor, radius);
			}
			else
			{
				return NULL;
			}
		}
		else
		{
			return NULL;
		}
	}

	void TSU_RectangleDestroy(Rectangle **rectangle_ptr)
	{
		if (*rectangle_ptr != NULL)
		{
			delete *rectangle_ptr;
			*rectangle_ptr = NULL;
		}
	}

	void TSU_RectangleSetSize(Rectangle *rectangle_ptr, float w, float h)
	{
		if (rectangle_ptr != NULL)
		{
			rectangle_ptr->setSize(w, h);
		}
	}

	void TSU_DrawableSetBorderColor(Rectangle *rectangle_ptr, const Color *color)
	{
		if (rectangle_ptr != NULL)
		{
			rectangle_ptr->setBorderColor(*color);
		}
	}
}