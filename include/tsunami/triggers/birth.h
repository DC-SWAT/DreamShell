/*
   Tsunami for KallistiOS ##version##

   birth.h

   Copyright (C) 2003 Megan Potter
   Copyright (C) 2024 Maniac Vera

*/

#ifndef __TSUNAMI_TRIG_BIRTH_H
#define __TSUNAMI_TRIG_BIRTH_H

#include "../trigger.h"

#ifdef __cplusplus

#include <memory>

class Birth : public Trigger {
public:
	// Constructor / Destructor
	Birth(Drawable *newDrawable,
	      Drawable *target = nullptr);
	virtual ~Birth();

	virtual void trigger(Drawable *t, Animation *a);

private:
	Drawable	*m_newDrawable;
	Drawable	*m_target;
};

#else 

typedef struct birth Birth;

#endif


#ifdef __cplusplus
extern "C"
{
#endif

Birth* TSU_BirthCreate(Drawable *new_drawable_ptr, Drawable *target_ptr);

#ifdef __cplusplus
};
#endif

#endif	/* __TSUNAMI_TRIG_BIRTH_H */
