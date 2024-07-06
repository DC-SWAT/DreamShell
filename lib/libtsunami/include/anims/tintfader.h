/*
   Tsunami for KallistiOS ##version##

   tintfader.h

   Copyright (C) 2003 Megan Potter

*/

#ifndef __TSUNAMI_ANIMS_TINTFADER_H
#define __TSUNAMI_ANIMS_TINTFADER_H

#include "../animation.h"
#include "../color.h"

/// Fades the tint values of an object
class TintFader : public Animation {
public:
	TintFader(const Color & fade_to, const Color & delta);
	virtual ~TintFader();

	virtual void nextFrame(Drawable *t);

private:
	bool	clamp(Color & col);

	Color	m_fade_to, m_delta;
};

#endif	/* __TSUNAMI_ANIMS_TINTFADER_H */
