/*
   Tsunami for KallistiOS ##version##

   trigger.cpp

   Copyright (C) 2002 Megan Potter
*/

#include "trigger.h"
#include "animation.h"

Trigger::Trigger() {
}

Trigger::~Trigger() {
}

void Trigger::trigger(Drawable *t, Animation *a) {
	// Autoclean ourselves once we've triggered
	a->triggerRemove(this);
}

extern "C"
{
	void TSU_TriggerDestroy(Trigger **trigger_ptr)
	{
		if (*trigger_ptr != NULL) {
			delete *trigger_ptr;
			*trigger_ptr = NULL;
		}
	}
}