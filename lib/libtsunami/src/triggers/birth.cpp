/*
   Tsunami for KallistiOS ##version##

   birth.cpp

   Copyright (C) 2003 Megan Potter
   Copyright (C) 2024 Maniac Vera
   
*/

#include "triggers/birth.h"
#include "drawable.h"

Birth::Birth(Drawable *newDrawable,
	     Drawable *target) {
	m_newDrawable = newDrawable;
	m_target = target;
}

Birth::~Birth() {
}

void Birth::trigger(Drawable *t, Animation *a) {
	// Insert the next object into the parent
	if (m_target)
		m_target->subAdd(m_newDrawable);
	else
		t->subAdd(m_newDrawable);

	// Remove ourselves
	Trigger::trigger(t, a);
}

extern "C"
{
	Birth *TSU_BirthCreate(Drawable *new_drawable_ptr, Drawable *target_ptr)
	{
		if (new_drawable_ptr != NULL && target_ptr != NULL) {
			return new Birth(new_drawable_ptr, target_ptr);
		}
		else {
			return NULL;
		}
	}

	void TSU_BirthDestroy(Birth **birth_ptr)
	{
		if (*birth_ptr != NULL) {
			delete *birth_ptr;
			*birth_ptr = NULL;
		}
	}
}