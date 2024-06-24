/*
   Tsunami for KallistiOS ##version##

   animation.h

   Copyright (C) 2002 Megan Potter
   Copyright (C) 2024 Maniac Vera

*/

#ifndef __TSUNAMI_ANIMATION_H
#define __TSUNAMI_ANIMATION_H

#ifdef __cplusplus
class Drawable;
#endif

#include "trigger.h"

#ifdef __cplusplus

#include <memory>
#include <deque>

class Animation {
public:
	/// Constructor / Destructor
	Animation();
	virtual ~Animation();

	/// Add a trigger to our list of triggers
	void triggerAdd(Trigger *t);

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
	std::deque<Trigger*> m_triggers; // Animation triggers
};

#else

typedef struct animation Animation;

#endif

#ifdef __cplusplus
extern "C"
{
#endif

void TSU_AnimationDestroy(Animation *animation_ptr);
void TSU_TriggerAdd(Animation *animation_ptr, Trigger *trigger_ptr);
void TSU_TriggerRemove(Animation *animation_ptr, Trigger *trigger_ptr);
void TSU_TriggerRemoveAll(Animation *animation_ptr);

#ifdef __cplusplus
};
#endif

#endif	/* __TSUNAMI_ANIMATION_H */
