/*
   Tsunami for KallistiOS ##version##

   tintfader.h

   Copyright (C) 2003 Megan Potter
   Copyright (C) 2024 Maniac Vera

*/

#ifndef __TSUNAMI_ANIMS_TINTFADER_H
#define __TSUNAMI_ANIMS_TINTFADER_H

#include "../animation.h"
#include "../color.h"

#ifdef __cplusplus

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

#else

typedef struct tintFader TintFader;

#endif

#ifdef __cplusplus
extern "C"
{
#endif

TintFader* TSU_TintFaderCreate(const Color *fade_to, const Color *delta);

#ifdef __cplusplus
};
#endif

#endif	/* __TSUNAMI_ANIMS_TINTFADER_H */
