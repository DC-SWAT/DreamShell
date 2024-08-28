/*
   Tsunami for KallistiOS ##version##

   expxymover.h

   Copyright (C) 2002 Megan Potter
   Copyright (C) 2024 Maniac Vera

*/

#ifndef __TSUNAMI_ANIMS_EXPXYMOVER_H
#define __TSUNAMI_ANIMS_EXPXYMOVER_H

#include "../animation.h"

#ifdef __cplusplus

/// Exponential object mover in the X/Y plane
class ExpXYMover : public Animation {
public:
	ExpXYMover(float dx, float dy, float maxx, float maxy);
	virtual ~ExpXYMover();

	virtual void nextFrame(Drawable *t);

private:
	float	m_dx, m_dy, m_maxx, m_maxy;
};

#else

typedef struct expXYMover ExpXYMover;

#endif

#ifdef __cplusplus
extern "C"
{
#endif

ExpXYMover* TSU_ExpXYMoverCreate(float dx, float dy, float maxx, float maxy);
void TSU_ExpXYMoverDestroy(ExpXYMover **expxymover_ptr);

#ifdef __cplusplus
};
#endif

#endif	/* __TSUNAMI_ANIMS_EXPXYMOVER_H */
