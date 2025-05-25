/*
   Tsunami for KallistiOS ##version##

   fadeout.h

   Copyright (C) 2002 Megan Potter
   Copyright (C) 2024 Maniac Vera

*/

#ifndef __TSUNAMI_ANIMS_FADEOUT_H
#define __TSUNAMI_ANIMS_FADEOUT_H

#include "../animation.h"
#include "../drawable.h"

#ifdef __cplusplus

class FadeOut : public Animation {
public:
	FadeOut(float factor);
	virtual ~FadeOut();

	virtual void nextFrame(Drawable *t);

private:
	float	m_factor;   
   bool m_first;
   Vector v_scale;
};

#else

typedef struct fadeOut FadeOut;

#endif

#ifdef __cplusplus
extern "C"
{
#endif

FadeOut* TSU_FadeOutCreate(float factor);
void TSU_FadeOutDestroy(FadeOut **fadeout_ptr);

#ifdef __cplusplus
};
#endif

#endif	/* __TSUNAMI_ANIMS_FADEOUT_H */
