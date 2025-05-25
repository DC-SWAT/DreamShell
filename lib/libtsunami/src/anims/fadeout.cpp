/*
   Tsunami for KallistiOS ##version##

   fadeout.cpp

   Copyright (C) 2002 Megan Potter
   Copyright (C) 2024 Maniac Vera
   
*/

#include "anims/fadeout.h"
#include "math.h"

FadeOut::FadeOut(float factor) {
	m_first = false;
	m_factor = factor;
}

FadeOut::~FadeOut() { }

void FadeOut::nextFrame(Drawable *t) {
	float w = 0, h = 0;
	Vector scale = t->getScale();

	if (!m_first) {
		v_scale = t->getScale();
		m_first = true;

		scale.x = v_scale.x;
		scale.y = v_scale.y;
	}

	if (scale.x >= 0 && scale.y >= 0) {
		w = scale.x/v_scale.x*100;
		h = scale.y/v_scale.y*100;

		if (w - m_factor < 0) {
			w = 0;
		}

		if (h - m_factor < 0) {
			h = 0;
		}
	}

	if (w <= 0 && h <= 0) {
		scale.x = 0;
		scale.y = 0;
		t->setScale(scale);

		complete(t);
		m_first = false;
	} else {
		scale.x -= m_factor/v_scale.x*v_scale.x/(100/v_scale.x);
		scale.y -= m_factor/v_scale.y*v_scale.y/(100/v_scale.y);		
		t->setScale(scale);
	}
}


extern "C"
{
	FadeOut* TSU_FadeOutCreate(float factor)
	{	
		return new FadeOut(factor);
	}

	void TSU_FadeOutDestroy(FadeOut **fadeout_ptr)
	{
		if (*fadeout_ptr != NULL) {
			delete *fadeout_ptr;
			*fadeout_ptr = NULL;
		}
	}
}