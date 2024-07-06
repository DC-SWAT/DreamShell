/*
   Tsunami for KallistiOS ##version##

   oneshot.h

   Copyright (C) 2003 Megan Potter
   Copyright (C) 2024 Maniac Vera

*/

#ifndef __TSUNAMI_TRIG_ONESHOT_H
#define __TSUNAMI_TRIG_ONESHOT_H

/* This defines a one-shot trigger which makes a callback to a compatible
   object. You can pass an arbitrary value along as well to be passed back
   to the callback. Just implement the OneShot::Target interface. */

#include "../trigger.h"

#ifdef __cplusplus

class OneShot : public Trigger {
public:
	class Target {
	public:
		virtual void shoot(int code) = 0;
	};

	// Constructor / Destructor
	OneShot(Target *t, int code) {
		m_target = t;
		m_code = code;
	}
	virtual ~OneShot() { }

	virtual void trigger(Drawable *t, Animation *a) {
		m_target->shoot(m_code);
		Trigger::trigger(t, a);
	}

private:
	Target	*m_target;
	int		m_code;
};

#endif

#endif	/* __TSUNAMI_TRIG_BIRTH_H */
