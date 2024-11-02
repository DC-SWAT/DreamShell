/*
   Tsunami for KallistiOS ##version##

   logxymover.cpp

   Copyright (C) 2002 Megan Potter
   Copyright (C) 2024 Maniac Vera
   
*/

#include "anims/logxymover.h"
#include "math.h"

LogXYMover::LogXYMover(float dstx, float dsty) {
	m_dstx = dstx;
	m_dsty = dsty;
	m_factor = 8.0f;
}

LogXYMover::~LogXYMover() { }

void LogXYMover::nextFrame(Drawable *t) {
	Vector pos = t->getTranslate();
	if (fabs(pos.x - m_dstx) < 1.0f && fabs(pos.y - m_dsty) < 1.0f) {
		t->setTranslate(Vector(m_dstx, m_dsty, pos.z));
		complete(t);
	} else {
		// Move 1/8 of the distance each frame
		float dx = m_dstx - pos.x;
		float dy = m_dsty - pos.y;
		t->setTranslate(Vector(
			pos.x + dx/m_factor, pos.y + dy/m_factor, pos.z));
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