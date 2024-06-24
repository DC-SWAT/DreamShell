/*
   Tsunami for KallistiOS ##version##

   trigger.h

   Copyright (C) 2002 Megan Potter
   Copyright (C) 2024 Maniac Vera

*/

#ifndef __TSUNAMI_TRIGGER_H
#define __TSUNAMI_TRIGGER_H

#ifdef __cplusplus

class Drawable;
class Animation;
class Trigger;

class Trigger {
public:
	// Constructor / Destructor
	Trigger();
	virtual ~Trigger();

	// Called when we have reached the trigger point in the
	// given animation/drawable
	virtual void trigger(Drawable *t, Animation *a);
};

#else

typedef struct trigger Trigger;

#endif


#ifdef __cplusplus
extern "C"
{
#endif

void TSU_TriggerDestroy(Trigger *trigger_ptr);

#ifdef __cplusplus
};
#endif

#endif	/* __TSUNAMI_TRIGGER_H */
