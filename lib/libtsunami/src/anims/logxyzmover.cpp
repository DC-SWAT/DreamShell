/*
   Tsunami for KallistiOS ##version##

   logxyzmover.cpp

   Copyright (C) 2026 SWAT
   
*/

#include "anims/logxyzmover.h"
#include "math.h"

LogXYZMover::LogXYZMover(float dstx, float dsty, float dstz) {
    m_dstx = dstx;
    m_dsty = dsty;
    m_dstz = dstz;
    m_factor = 8.0f;
}

LogXYZMover::~LogXYZMover() { }

void LogXYZMover::nextFrame(Drawable *t) {
    Vector pos = t->getTranslate();
    if (fabs(pos.x - m_dstx) < 0.1f && 
        fabs(pos.y - m_dsty) < 0.1f && 
        fabs(pos.z - m_dstz) < 0.1f) {
        t->setTranslate(Vector(m_dstx, m_dsty, m_dstz));
        complete(t);
    }
    else {
        // Move 1/factor of the distance each frame
        float dx = m_dstx - pos.x;
        float dy = m_dsty - pos.y;
        float dz = m_dstz - pos.z;
        t->setTranslate(Vector(
            pos.x + dx/m_factor, 
            pos.y + dy/m_factor, 
            pos.z + dz/m_factor));
    }
}

void LogXYZMover::setFactor(float factor) {
    if (factor > 0.0f) {
        m_factor =  factor;
    }
}

extern "C"
{
    LogXYZMover* TSU_LogXYZMoverCreate(float dstx, float dsty, float dstz)
    {	
        return new LogXYZMover(dstx, dsty, dstz);
    }

    void TSU_LogXYZMoverDestroy(LogXYZMover **logxyzmover_ptr)
    {
        if (*logxyzmover_ptr != NULL) {
            delete *logxyzmover_ptr;
            *logxyzmover_ptr = NULL;
        }
    }

    void TSU_LogXYZMoverSetFactor(LogXYZMover *logxyzmover_ptr, float factor)
    {
        if (logxyzmover_ptr != NULL) {
            logxyzmover_ptr->setFactor(factor);
        }
    }
}
