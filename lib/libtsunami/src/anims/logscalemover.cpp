/*
   Tsunami for KallistiOS ##version##

   logscalemover.cpp

   Copyright (C) 2026 SWAT
   
*/

#include "anims/logscalemover.h"
#include "math.h"

LogScaleMover::LogScaleMover(float dstx, float dsty) {
    m_dstx = dstx;
    m_dsty = dsty;
    m_factor = 8.0f;
}

LogScaleMover::~LogScaleMover() { }

void LogScaleMover::nextFrame(Drawable *t) {
    Vector scale = t->getScale();

    if (fabs(scale.x - m_dstx) < 0.001f && fabs(scale.y - m_dsty) < 0.001f) {
        scale.x = m_dstx;
        scale.y = m_dsty;
        t->setScale(scale);
        complete(t);
    }
    else {
        // Move 1/factor of the distance each frame
        float dx = m_dstx - scale.x;
        float dy = m_dsty - scale.y;
        scale.x += dx/m_factor;
        scale.y += dy/m_factor;
        t->setScale(scale);
    }
}

void LogScaleMover::setFactor(float factor) {
    if (factor > 0.0f) {
        m_factor =  factor;
    }
}

void LogScaleMover::setTarget(float dstx, float dsty) {
    m_dstx = dstx;
    m_dsty = dsty;
}

extern "C"
{
    LogScaleMover* TSU_LogScaleMoverCreate(float dstx, float dsty)
    {	
        return new LogScaleMover(dstx, dsty);
    }

    void TSU_LogScaleMoverDestroy(LogScaleMover **logscalemover_ptr)
    {
        if (*logscalemover_ptr != NULL) {
            delete *logscalemover_ptr;
            *logscalemover_ptr = NULL;
        }
    }

    void TSU_LogScaleMoverSetFactor(LogScaleMover *logscalemover_ptr, float factor)
    {
        if (logscalemover_ptr != NULL) {
            logscalemover_ptr->setFactor(factor);
        }
    }

    void TSU_LogScaleMoverSetTarget(LogScaleMover *logscalemover_ptr, float dstx, float dsty)
    {
        if (logscalemover_ptr != NULL) {
            logscalemover_ptr->setTarget(dstx, dsty);
        }
    }
}
