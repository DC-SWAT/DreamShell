/*
   Tsunami for KallistiOS ##version##

   animation.h

   Copyright (C) 2002 Megan Potter

*/

#ifndef __TSUNAMI_ANIMATION_H
#define __TSUNAMI_ANIMATION_H

class Drawable;

#include "trigger.h"

#include <memory>
#include <deque>

class Animation {
public:
	/// Constructor / Destructor
	Animation();
	virtual ~Animation();

	/// Add a trigger to our list of triggers
	void triggerAdd(std::shared_ptr<Trigger> t);

	/// Remove a trigger from our list of triggers
	void triggerRemove(Trigger *t);

	/// Remove all triggers from our list of triggers
	void triggerRemoveAll();

	// Move to the next frame of animation
	virtual void nextFrame(Drawable *t);

protected:
	/// Trigger any triggers
	virtual void trigger(Drawable *d);

	/// Call when the animation has completed
	virtual void complete(Drawable *t);

private:
	std::deque<std::shared_ptr<Trigger>> m_triggers; // Animation triggers
};

#endif	/* __TSUNAMI_ANIMATION_H */
