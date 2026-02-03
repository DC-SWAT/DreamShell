/*
   Tsunami for KallistiOS ##version##

   logscalemover.h

   Copyright (C) 2026 SWAT

*/

#ifndef __TSUNAMI_ANIMS_LOGSCALEMOVER_H
#define __TSUNAMI_ANIMS_LOGSCALEMOVER_H

#include "../animation.h"
#include "../drawable.h"

#ifdef __cplusplus

class LogScaleMover : public Animation {
public:
    LogScaleMover(float dstx, float dsty);
    virtual ~LogScaleMover();

    virtual void nextFrame(Drawable *t);
    void setFactor(float factor);
    void setTarget(float dstx, float dsty);

private:
    float m_dstx, m_dsty, m_factor;
};

#else

typedef struct logScaleMover LogScaleMover;

#endif

#ifdef __cplusplus
extern "C"
{
#endif

LogScaleMover* TSU_LogScaleMoverCreate(float dstx, float dsty);
void TSU_LogScaleMoverDestroy(LogScaleMover **logscalemover_ptr);
void TSU_LogScaleMoverSetFactor(LogScaleMover *logscalemover_ptr, float factor);
void TSU_LogScaleMoverSetTarget(LogScaleMover *logscalemover_ptr, float dstx, float dsty);

#ifdef __cplusplus
};
#endif

#endif	/* __TSUNAMI_ANIMS_LOGSCALEMOVER_H */
