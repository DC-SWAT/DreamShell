/*
   Tsunami for KallistiOS ##version##

   logxymover.cpp

   Copyright (C) 2002 Megan Potter
   Copyright (C) 2024 Maniac Vera
   
*/

#include "anims/fadein.h"
#include "math.h"

FadeIn::FadeIn(float factor) {
	m_first = false;
	m_factor = factor;
}

FadeIn::~FadeIn() { }

void FadeIn::nextFrame(Drawable *t) {
	float w = 0, h = 0;
	Vector scale = t->getScale();

	if (!m_first) {
		v_scale = t->getScale();
		m_first = true;

		scale.x = 0;
		scale.y = 0;
	}

	if (scale.x > 0 && scale.y > 0) {
		w = scale.x/v_scale.x*100;
		h = scale.y/v_scale.y*100;

		if (w + m_factor > 100) {
			w = 100;
		}

		if (h + m_factor > 100) {
			h = 100;
		}
	}

	if (w >= 100 && h >= 100) {
		scale.x = v_scale.x;
		scale.y = v_scale.y;
		t->setScale(scale);

		complete(t);
		m_first = false;
	} else {
		scale.x += m_factor/v_scale.x*v_scale.x/(100/v_scale.x);
		scale.y += m_factor/v_scale.y*v_scale.y/(100/v_scale.y);		
		t->setScale(scale);
	}
}


extern "C"
{
	FadeIn* TSU_FadeInCreate(float factor)
	{	
		return new FadeIn(factor);
	}
}