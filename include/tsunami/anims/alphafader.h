/*
   Tsunami for KallistiOS ##version##

   alphafader.h

   Copyright (C) 2002 Megan Potter
   Copyright (C) 2024 Maniac Vera

*/

#ifndef __TSUNAMI_ANIMS_ALPHAFADER_H
#define __TSUNAMI_ANIMS_ALPHAFADER_H

#include "../animation.h"

#ifdef __cplusplus

/// Fades the alpha value of an object
class AlphaFader : public Animation {
public:
	AlphaFader(float fade_to, float delta);
	virtual ~AlphaFader();

	virtual void nextFrame(Drawable *t);

private:
	float	m_fade_to, m_delta;
};

#else

typedef struct alphaFader AlphaFader;

#endif

#ifdef __cplusplus
extern "C"
{
#endif

AlphaFader* TSU_AlphaFaderCreate(float fade_to, float delta);

#ifdef __cplusplus
};
#endif

#endif	/* __TSUNAMI_ANIMS_ALPHAFADER_H */
