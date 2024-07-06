/*
   Tsunami for KallistiOS ##version##

   expxymover.h

   Copyright (C) 2002 Megan Potter

*/

#ifndef __TSUNAMI_ANIMS_EXPXYMOVER_H
#define __TSUNAMI_ANIMS_EXPXYMOVER_H

#include "../animation.h"

/// Exponential object mover in the X/Y plane
class ExpXYMover : public Animation {
public:
	ExpXYMover(float dx, float dy, float maxx, float maxy);
	virtual ~ExpXYMover();

	virtual void nextFrame(Drawable *t);

private:
	float	m_dx, m_dy, m_maxx, m_maxy;
};

#endif	/* __TSUNAMI_ANIMS_EXPXYMOVER_H */
