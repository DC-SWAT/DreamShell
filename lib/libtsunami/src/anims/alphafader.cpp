/*
   Tsunami for KallistiOS ##version##

   alphafader.cpp

   Copyright (C) 2002 Megan Potter
   Copyright (C) 2024 Maniac Vera
*/

#include "drawable.h"
#include "anims/alphafader.h"

AlphaFader::AlphaFader(float fade_to, float delta) {
	m_fade_to = fade_to;
	m_delta = delta;
}

AlphaFader::~AlphaFader() { }

void AlphaFader::nextFrame(Drawable *t) {
	Color c = t->getTint();
	c.a += m_delta;
	t->setTint(c);
	if (m_delta < 0.0f) {
		if (c.a <= m_fade_to) {
			c.a = m_fade_to;
			t->setTint(c);
			complete(t);
		}
	} else {
		if (c.a >= m_fade_to) {
			c.a = m_fade_to;
			t->setTint(c);
			complete(t);
		}
	}
}


extern "C"
{
	AlphaFader* TSU_AlphaFaderCreate(float fade_to, float delta)
	{
		return new AlphaFader(fade_to, delta);
	}

	void TSU_AlphaFaderDestroy(AlphaFader **alphafader_ptr)
	{
		if (*alphafader_ptr != NULL) {
			delete *alphafader_ptr;
			*alphafader_ptr = NULL;
		}
	}
}