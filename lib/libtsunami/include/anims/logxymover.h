/*
   Tsunami for KallistiOS ##version##

   logxymover.h

   Copyright (C) 2002 Megan Potter

*/

#ifndef __TSUNAMI_ANIMS_LOGXYMOVER_H
#define __TSUNAMI_ANIMS_LOGXYMOVER_H

#include "../animation.h"
#include "../drawable.h"

/// Logarithmic object mover in the X/Y plane
class LogXYMover : public Animation {
public:
	LogXYMover(float dstx, float dsty);
	virtual ~LogXYMover();

	virtual void nextFrame(Drawable *t);

private:
	float	m_dstx, m_dsty;
};

#endif	/* __TSUNAMI_ANIMS_LOGXYMOVER_H */
