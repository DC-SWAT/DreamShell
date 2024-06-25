/*
   Tsunami for KallistiOS ##version##

   alphafader.h

   Copyright (C) 2002 Megan Potter

*/

#ifndef __TSUNAMI_ANIMS_ALPHAFADER_H
#define __TSUNAMI_ANIMS_ALPHAFADER_H

#include "../animation.h"

/// Fades the alpha value of an object
class AlphaFader : public Animation {
public:
	AlphaFader(float fade_to, float delta);
	virtual ~AlphaFader();

	virtual void nextFrame(Drawable *t);

private:
	float	m_fade_to, m_delta;
};

#endif	/* __TSUNAMI_ANIMS_ALPHAFADER_H */
