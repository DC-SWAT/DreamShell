/*
   Tsunami for KallistiOS ##version##

   logxyzmover.h

   Copyright (C) 2026 SWAT

*/

#ifndef __TSUNAMI_ANIMS_LOGXYZMOVER_H
#define __TSUNAMI_ANIMS_LOGXYZMOVER_H

#include "../animation.h"
#include "../drawable.h"

#ifdef __cplusplus

class LogXYZMover : public Animation {
public:
    LogXYZMover(float dstx, float dsty, float dstz);
    virtual ~LogXYZMover();

    virtual void nextFrame(Drawable *t);
    void setFactor(float factor);
    void setTarget(float dstx, float dsty, float dstz);

private:
    float m_dstx, m_dsty, m_dstz, m_factor;
};

#else

typedef struct logXYZMover LogXYZMover;

#endif

#ifdef __cplusplus
extern "C"
{
#endif

LogXYZMover* TSU_LogXYZMoverCreate(float dstx, float dsty, float dstz);
void TSU_LogXYZMoverDestroy(LogXYZMover **logxyzmover_ptr);
void TSU_LogXYZMoverSetFactor(LogXYZMover *logxyzmover_ptr, float factor);
void TSU_LogXYZMoverSetTarget(LogXYZMover *logxyzmover_ptr, float dstx, float dsty, float dstz);

#ifdef __cplusplus
};
#endif

#endif	/* __TSUNAMI_ANIMS_LOGXYZMOVER_H */
