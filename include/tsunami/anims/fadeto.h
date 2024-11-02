/*
   Tsunami for KallistiOS ##version##

   logxymover.h

   Copyright (C) 2002 Megan Potter
   Copyright (C) 2024 Maniac Vera

*/

#ifndef __TSUNAMI_ANIMS_FADETO_H
#define __TSUNAMI_ANIMS_FADETO_H

#include "../animation.h"
#include "../drawable.h"

#ifdef __cplusplus

class FadeTo : public Animation {
public:
	FadeTo(float init_scale, float end_scale, float factor);
	virtual ~FadeTo();

	virtual void nextFrame(Drawable *t);

private:
   float m_init_scale;
   float m_end_scale;
	float	m_factor;
   bool m_first;
   Vector v_scale;
};

#else

typedef struct fadeTo FadeTo;

#endif

#ifdef __cplusplus
extern "C"
{
#endif

FadeTo* TSU_FadeToCreate(float init_scale, float end_scale, float factor);
void TSU_FadeToDestroy(FadeTo **fadeto_ptr);

#ifdef __cplusplus
};
#endif

#endif	/* __TSUNAMI_ANIMS_FADETO_H */
