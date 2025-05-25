/*
   Tsunami for KallistiOS ##version##

   fadein.h

   Copyright (C) 2002 Megan Potter
   Copyright (C) 2024 Maniac Vera

*/

#ifndef __TSUNAMI_ANIMS_FADEIN_H
#define __TSUNAMI_ANIMS_FADEIN_H

#include "../animation.h"
#include "../drawable.h"

#ifdef __cplusplus

class FadeIn : public Animation {
public:
	FadeIn(float factor);
	virtual ~FadeIn();

	virtual void nextFrame(Drawable *t);

private:
	float	m_factor;
   bool m_first;
   Vector v_scale;
};

#else

typedef struct fadeIn FadeIn;

#endif

#ifdef __cplusplus
extern "C"
{
#endif

FadeIn* TSU_FadeInCreate(float factor);
void TSU_FadeInDestroy(FadeIn **fadein_ptr);

#ifdef __cplusplus
};
#endif

#endif	/* __TSUNAMI_ANIMS_FADEIN_H */
