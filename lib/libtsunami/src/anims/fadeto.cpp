/*
   Tsunami for KallistiOS ##version##

   logxymover.cpp

   Copyright (C) 2002 Megan Potter
   Copyright (C) 2024 Maniac Vera
   
*/

#include "anims/fadeto.h"
#include "math.h"

FadeTo::FadeTo(float init_scale, float end_scale, float factor) {
	m_init_scale = init_scale,
	m_end_scale = end_scale,
	m_first = false;
	m_factor = factor;
}

FadeTo::~FadeTo() { }

void FadeTo::nextFrame(Drawable *t) {
	float w = 0, h = 0;
	Vector scale = t->getScale();

	if (!m_first) {
		v_scale = t->getScale();
		Vector v_init = Vector(m_init_scale, m_init_scale, v_scale.z, v_scale.w);
		Vector v_end  = Vector(m_end_scale, m_end_scale, v_scale.z, v_scale.w);
		t->setScale(v_init);
		v_scale = v_end;

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
	FadeTo* TSU_FadeToCreate(float init_scale, float end_scale, float factor)
	{	
		return new FadeTo(init_scale, end_scale, factor);
	}

	void TSU_FadeToDestroy(FadeTo **fadeto_ptr)
	{
		if (*fadeto_ptr != NULL) {
			delete *fadeto_ptr;
			*fadeto_ptr = NULL;
		}
	}
}