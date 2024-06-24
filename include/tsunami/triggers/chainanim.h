/*
   Tsunami for KallistiOS ##version##

   chainanim.h

   Copyright (C) 2002 Megan Potter
   Copyright (C) 2024 Maniac Vera

*/

#ifndef __TSUNAMI_TRIG_CHAINANIM_H
#define __TSUNAMI_TRIG_CHAINANIM_H

#include "../trigger.h"
#include "../animation.h"
#include "../drawable.h"

#ifdef __cplusplus

class ChainAnimation : public Trigger {
public:
	// Constructor / Destructor
	ChainAnimation(Animation *na, Drawable *target = nullptr) {
		m_newanim = na;
		m_target = target;
	}
	virtual ~ChainAnimation() {
	}

	// Called when we have reached the trigger point in the
	// given animation/drawable
	virtual void trigger(Drawable *t, Animation *a) {
		if (m_target)
			m_target->animAdd(m_newanim);
		else
			t->animAdd(m_newanim);
		Trigger::trigger(t, a);
	}

private:
	Animation	*m_newanim;
	Drawable	*m_target;
};

#endif

#endif	/* __TSUNAMI_TRIG_CHAINANIM_H */
