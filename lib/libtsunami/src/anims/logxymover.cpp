/*
   Tsunami for KallistiOS ##version##

   logxymover.cpp

   Copyright (C) 2002 Megan Potter
   Copyright (C) 2024 Maniac Vera
   Copyright (C) 2026 SWAT
   
*/

#include "anims/logxymover.h"
#include <sh4zam/shz_scalar.h>

LogXYMover::LogXYMover(float dstx, float dsty) {
	m_dstx = dstx;
	m_dsty = dsty;
	m_factor = 8.0f;
}

LogXYMover::~LogXYMover() { }

void LogXYMover::nextFrame(Drawable *t) {
	Vector pos = t->getTranslate();
	if (shz_fabsf(pos.x - m_dstx) < 0.1f &&
		shz_fabsf(pos.y - m_dsty) < 0.1f) {
		t->setTranslate(Vector(m_dstx, m_dsty, pos.z));
		complete(t);
	} else {
		// Move 1/8 of the distance each frame
		float dx = m_dstx - pos.x;
		float dy = m_dsty - pos.y;
		float inv_factor = shz_invf(m_factor);
		t->setTranslate(Vector(
			shz_fmaf(dx, inv_factor, pos.x),
			shz_fmaf(dy, inv_factor, pos.y),
			pos.z));
	}
}

void LogXYMover::setFactor(float factor) {
	if (factor > 0.0f) {
		m_factor =  factor;
	}
}

extern "C"
{
	LogXYMover* TSU_LogXYMoverCreate(float dstx, float dsty)
	{	
		return new LogXYMover(dstx, dsty);
	}

	void TSU_LogXYMoverDestroy(LogXYMover **logxymover_ptr)
	{
		if (*logxymover_ptr != NULL) {
			delete *logxymover_ptr;
			*logxymover_ptr = NULL;
		}
	}

	void TSU_LogXYMoverSetFactor(LogXYMover *logxymover_ptr, float factor)
	{
		if (logxymover_ptr != NULL) {
			logxymover_ptr->setFactor(factor);
		}
	}
}