/*
   Tsunami for KallistiOS ##version##

   trigger.h

   Copyright (C) 2002 Megan Potter

*/

#ifndef __TSUNAMI_TRIGGER_H
#define __TSUNAMI_TRIGGER_H

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

#endif	/* __TSUNAMI_TRIGGER_H */
