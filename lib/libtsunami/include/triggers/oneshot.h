/*
   Tsunami for KallistiOS ##version##

   oneshot.h

   Copyright (C) 2003 Megan Potter

*/

#ifndef __TSUNAMI_TRIG_ONESHOT_H
#define __TSUNAMI_TRIG_ONESHOT_H

/* This defines a one-shot trigger which makes a callback to a compatible
   object. You can pass an arbitrary value along as well to be passed back
   to the callback. Just implement the OneShot::Target interface. */

#include "../trigger.h"

class OneShot : public Trigger {
public:
	class Target {
	public:
		virtual void shoot(int code) = 0;
	};

	// Constructor / Destructor
	OneShot(std::shared_ptr<Target> t, int code) {
		m_target = t;
		m_code = code;
	}
	virtual ~OneShot() { }

	virtual void trigger(Drawable *t, Animation *a) {
		m_target->shoot(m_code);
		Trigger::trigger(t, a);
	}

private:
	std::shared_ptr<Target> m_target;
	int		m_code;
};

#endif	/* __TSUNAMI_TRIG_BIRTH_H */
