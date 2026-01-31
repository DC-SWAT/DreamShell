/*
   Tsunami for KallistiOS ##version##

   circle.h

   Copyright (C) 2026 SWAT

*/

#ifndef __TSUNAMI_DRAWABLES_CIRCLE_H
#define __TSUNAMI_DRAWABLES_CIRCLE_H

#include "../drawable.h"
#include "../texture.h"

#ifdef __cplusplus

class Circle : public Drawable {
public:
    Circle(int list, float radius, int points, const Color &centerColor, const Color &edgeColor);
    virtual ~Circle();

    virtual void draw(int list);

    void setRadius(float r);
    float getRadius() const;

    void setColors(const Color &center, const Color &edge);

private:
    int m_list;
    float m_radius;
    int m_points;
    Color m_centerColor;
    Color m_edgeColor;

    pvr_poly_cxt_t cxt;
    pvr_poly_hdr_t hdr;
};

#else
typedef struct Circle Circle;
#endif

#ifdef __cplusplus
extern "C"
{
#endif

    Circle* TSU_CircleCreate(int list, float radius, int points, const Color *centerColor, const Color *edgeColor);
    void TSU_CircleDestroy(Circle **circle_ptr);
    void TSU_CircleSetRadius(Circle *circle_ptr, float radius);
    void TSU_CircleSetColors(Circle *circle_ptr, const Color *centerColor, const Color *edgeColor);

#ifdef __cplusplus
};
#endif

#endif	/* __TSUNAMI_DRAWABLES_CIRCLE_H */
