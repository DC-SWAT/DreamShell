/*
   Tsunami for KallistiOS ##version##

   logxymover.h

   Copyright (C) 2002 Megan Potter
   Copyright (C) 2024 Maniac Vera

*/

#ifndef __TSUNAMI_ANIMS_LOGXYMOVER_H
#define __TSUNAMI_ANIMS_LOGXYMOVER_H

#include "../animation.h"
#include "../drawable.h"

#ifdef __cplusplus

/// Logarithmic object mover in the X/Y plane
class LogXYMover : public Animation {
public:
	LogXYMover(float dstx, float dsty);
	virtual ~LogXYMover();

	virtual void nextFrame(Drawable *t);

private:
	float	m_dstx, m_dsty;
};

#else

typedef struct logXYMover LogXYMover;

#endif

#ifdef __cplusplus
extern "C"
{
#endif

LogXYMover* TSU_LogXYMoverCreate(float dstx, float dsty);
void TSU_LogXYMoverDestroy(LogXYMover **logxymover_ptr);

#ifdef __cplusplus
};
#endif

#endif	/* __TSUNAMI_ANIMS_LOGXYMOVER_H */
