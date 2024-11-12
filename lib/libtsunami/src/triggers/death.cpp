/*
   Tsunami for KallistiOS ##version##

   death.cpp

   Copyright (C) 2002 Megan Potter
   Copyright (C) 2024 Maniac Vera
   
*/

#include "triggers/death.h"
#include "drawable.h"

Death::Death(Drawable* target) {
	m_target = target;
}

Death::~Death() {
}

void Death::trigger(Drawable *t, Animation *a) {
	// Mark our parent drawable as "finished"
	if (m_target)
		m_target->setFinished();
	else
		t->setFinished();

	// Go the way of the dodo ourselves
	Trigger::trigger(t, a);
}


extern "C"
{
	Death* TSU_DeathCreate(Drawable *target_ptr)
	{
		if (target_ptr != NULL) {
			return new Death(target_ptr);
		}
		else {
			return new Death(nullptr);
		}
	}

	void TSU_DeathDestroy(Death **death_ptr)
	{
		if (*death_ptr != NULL) {
			delete *death_ptr;
			*death_ptr = NULL;
		}
	}
}