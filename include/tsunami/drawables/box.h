/*
   Tsunami for KallistiOS ##version##

   box.h

   Copyright (C) 2002 Megan Potter
   Copyright (C) 2024-2025 Maniac Vera

*/

#ifndef __TSUNAMI_DRW_BOX_H
#define __TSUNAMI_DRW_BOX_H

#include "../../plx/list.h"
#include "../../plx/sprite.h"
#include "../drawable.h"
#include "../texture.h"
#include "../color.h"

#ifdef __cplusplus

#include <memory>

class Box : public Drawable {
public:
	Box(pvr_list_type_t list, float x, float y, float width, float height, float borderWidth, const Color &color, float zIndex, float radius);
	virtual ~Box();

	void setSize(float w, float h);

	virtual void draw(pvr_list_type_t list);

private:
	pvr_list_type_t m_list;
	float width, height, radius, zIndex, borderWidth;
	pvr_poly_hdr_t hdr;
	pvr_poly_cxt_t cxt;

	void drawBox(float x, float y, float width, float height, float lineWidth, uint32 color, float zIndex, int radius);
};

#else

typedef struct box Box;

#endif

#ifdef __cplusplus
extern "C"
{
#endif

Box* TSU_BoxCreate(pvr_list_type_t list, float x, float y, float width, float height, float border_width, const Color *color, float zIndex, float radius);
void TSU_BoxDestroy(Box **box_ptr);
void TSU_BoxSetSize(Box *box_ptr, float width, float height);

#ifdef __cplusplus
};
#endif

#endif	/* __TSUNAMI_DRW_BOX_H */
