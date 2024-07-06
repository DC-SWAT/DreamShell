/*
   Tsunami for KallistiOS ##version##

   expxymover.cpp

   Copyright (C) 2002 Megan Potter
   Copyright (C) 2024 Maniac Vera
*/

#include "drawable.h"
#include "anims/expxymover.h"

ExpXYMover::ExpXYMover(float dx, float dy, float maxx, float maxy) {
	m_dx = dx;
	m_dy = dy;
	m_maxx = maxx;
	m_maxy = maxy;
}

ExpXYMover::~ExpXYMover() { }

void ExpXYMover::nextFrame(Drawable *t) {
	Vector p = t->getTranslate();

	bool xfin = m_dx < 0 ? (p.x <= m_maxx) : (p.x >= m_maxx);
	bool yfin = m_dy < 0 ? (p.y <= m_maxy) : (p.y >= m_maxy);
	if (xfin && yfin) {
		t->setTranslate(Vector(m_maxx, m_maxy, p.z));
		complete(t);
		return;
	}

	// Move 1.15x of the distance each frame
	p += Vector(m_dx, m_dy, 0);
	t->setTranslate(p);
	m_dx *= 1.15f;
	m_dy *= 1.15f;
}


extern "C"
{
	ExpXYMover* TSU_ExpXYMoverCreate(float dx, float dy, float maxx, float maxy)
	{
		return new ExpXYMover(dx, dy, maxx, maxy);
	}
}