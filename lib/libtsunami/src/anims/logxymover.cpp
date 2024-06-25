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
			pos.x + dx/8.0f, pos.y + dy/8.0f, pos.z));
	}
}


extern "C"
{
	LogXYMover* TSU_LogXYMoverCreate(float dstx, float dsty)
	{	
		return new LogXYMover(dstx, dsty);
	}
}