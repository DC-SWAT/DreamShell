/*
   Tsunami for KallistiOS ##version##

   circle.cpp

   Copyright (C) 2026 SWAT

*/

#include "drawables/circle.h"
#include <dc/fmath.h>
#include "../plx/prim.h"
#include "../plx/context.h"

Circle::Circle(pvr_list_type_t list, float radius, int points, const Color &centerColor, const Color &edgeColor) {
    setObjectType(ObjectTypeEnum::CIRCLE_TYPE);
    m_list = list;
    m_radius = radius;
    m_points = points;
    m_centerColor = centerColor;
    m_edgeColor = edgeColor;
    
    if (m_points < 3) m_points = 3;
    precomputePoints();

    pvr_poly_cxt_t cxt;
    pvr_poly_cxt_col(&cxt, list);
    cxt.gen.culling = PVR_CULLING_NONE;
    pvr_poly_compile(&m_hdr, &cxt);
}

Circle::~Circle() {
}

void Circle::precomputePoints() {
    m_precomputed.resize(m_points + 1);
    float angle_step = (2.0f * F_PI) / m_points;
    for (int i = 0; i <= m_points; i++) {
        float angle = i * angle_step;
        m_precomputed[i].cos_val = fcos(angle);
        m_precomputed[i].sin_val = fsin(angle);
    }
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

void Circle::draw(pvr_list_type_t list) {
    if (list != m_list)
        return;

    pvr_prim(&m_hdr, sizeof(m_hdr));

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

    float tv_x = tv.x;
    float tv_y = tv.y;
    float tv_z = tv.z;

    for (int i = 0; i <= m_points; i++) {
        float ca = m_precomputed[i].cos_val;
        float sa = m_precomputed[i].sin_val;

        // Rim vertex
        plx_vert_inp(PLX_VERT, tv_x + ca * rx, tv_y + sa * ry, tv_z, c_edge);

        // Center vertex
        if (i == m_points) {
            plx_vert_inp(PLX_VERT_EOS, tv_x, tv_y, tv_z, c_center);
        }
        else {
            plx_vert_inp(PLX_VERT, tv_x, tv_y, tv_z, c_center);
        }
    }

    Drawable::draw(list);
}

extern "C" {
    Circle* TSU_CircleCreate(pvr_list_type_t list, float radius, int points, const Color *centerColor, const Color *edgeColor) {
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
