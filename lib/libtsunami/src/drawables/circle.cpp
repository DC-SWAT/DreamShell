/*
   Tsunami for KallistiOS ##version##

   circle.cpp

   Copyright (C) 2026 SWAT

*/

#include "drawables/circle.h"
#include <dc/fmath.h>

Circle::Circle(int list, float radius, int points, const Color &centerColor, const Color &edgeColor) {
    setObjectType(ObjectTypeEnum::CIRCLE_TYPE);
    m_list = list;
    m_radius = radius;
    m_points = points;
    m_centerColor = centerColor;
    m_edgeColor = edgeColor;
    
    if (m_points < 3) m_points = 3;
}

Circle::~Circle() {
}

void Circle::setRadius(float r) {
    m_radius = r;
}

float Circle::getRadius() const {
    return m_radius;
}

void Circle::setColors(const Color &center, const Color &edge) {
    m_centerColor = center;
    m_edgeColor = edge;
}

void Circle::draw(int list) {
    if (list != m_list)
        return;

    pvr_poly_cxt_col(&cxt, list);
    cxt.gen.culling = PVR_CULLING_NONE;
    pvr_poly_compile(&hdr, &cxt);
    pvr_prim(&hdr, sizeof(hdr));

    const Vector &tv = getPosition();
    const Vector &sv = getScale();
    
    float r = m_radius;
    float rx = r * sv.x;
    float ry = r * sv.y;

    Color tint = getTint();
    Color finalCenter = m_centerColor * tint;
    Color finalEdge = m_edgeColor * tint;

    uint32_t c_center = (uint32_t)finalCenter;
    uint32_t c_edge = (uint32_t)finalEdge;

    pvr_vertex_t vert;
    vert.flags = PVR_CMD_VERTEX;
    vert.oargb = 0;
    vert.u = 0.0f;
    vert.v = 0.0f;

    float angle_step = (2.0f * F_PI) / m_points;

    for (int i = 0; i <= m_points; i++) {
        float angle = i * angle_step;
        float ca = fcos(angle);
        float sa = fsin(angle);

        // Rim vertex
        vert.x = tv.x + ca * rx;
        vert.y = tv.y + sa * ry;
        vert.z = tv.z;
        vert.argb = c_edge;
        vert.flags = PVR_CMD_VERTEX;
        pvr_prim(&vert, sizeof(vert));

        // Center vertex
        vert.x = tv.x;
        vert.y = tv.y;
        vert.z = tv.z;
        vert.argb = c_center;

        if (i == m_points) {
            vert.flags = PVR_CMD_VERTEX_EOL;
        }
        else {
            vert.flags = PVR_CMD_VERTEX;
        }
        pvr_prim(&vert, sizeof(vert));
    }

    Drawable::draw(list);
}

extern "C" {
    Circle* TSU_CircleCreate(int list, float radius, int points, const Color *centerColor, const Color *edgeColor) {
        return new Circle(list, radius, points, *centerColor, *edgeColor);
    }
    
    void TSU_CircleDestroy(Circle **circle_ptr) {
        if (*circle_ptr) {
            delete *circle_ptr;
            *circle_ptr = NULL;
        }
    }
    
    void TSU_CircleSetRadius(Circle *circle_ptr, float radius) {
        if (circle_ptr) circle_ptr->setRadius(radius);
    }
    
    void TSU_CircleSetColors(Circle *circle_ptr, const Color *centerColor, const Color *edgeColor) {
        if (circle_ptr) circle_ptr->setColors(*centerColor, *edgeColor);
    }
}
