/*
   Tsunami for KallistiOS ##version##

   animation.cpp

   Copyright (C) 2002 Megan Potter
   Copyright (C) 2024 Maniac Vera
   
*/

#include "animation.h"
#include "drawable.h"
#include <algorithm>

Animation::Animation() {
}

Animation::~Animation() {
}

void Animation::triggerAdd(Trigger *t) {
	m_triggers.push_front(t);
}

void Animation::triggerRemove(Trigger *tr) {
	auto is_ptr = [=](Trigger *sp) { return sp == tr; };
	auto it = std::find_if(m_triggers.begin(), m_triggers.end(), is_ptr);

	if (it != m_triggers.end())
		m_triggers.erase(it);
}

void Animation::triggerRemoveAll() {
	m_triggers.clear();
}

void Animation::nextFrame(Drawable *t) {
}

void Animation::trigger(Drawable *d) {
	/* Duplicate the array of triggers. This makes the "for" loop much
	 * easier as we don't have to handle it->trigger() calling
	 * triggerRemove(). */
	auto triggers = m_triggers;

	for (auto it: m_triggers) {
		it->trigger(d, this);
	}
}

void Animation::complete(Drawable *d) {
	// Call any completion triggers
	trigger(d);

	// Remove us from the parent Drawable
	d->animRemove(this);
}


extern "C"
{
	void TSU_AnimationComplete(Animation *animation_ptr, Drawable *drawable_ptr) {
		if (animation_ptr != NULL) {
			animation_ptr->complete(drawable_ptr);
		}
	}

	void TSU_AnimationDestroy(Animation **animation_ptr) {
		if (*animation_ptr != NULL) {
			delete *animation_ptr;
			*animation_ptr = NULL;
		}
	}

	void TSU_TriggerAdd(Animation *animation_ptr, Trigger *trigger_ptr)
	{
		if (animation_ptr != NULL && trigger_ptr != NULL) {
			animation_ptr->triggerAdd(trigger_ptr);
		}
	}

	void TSU_TriggerRemove(Animation *animation_ptr, Trigger *trigger_ptr)
	{
		if (animation_ptr != NULL && trigger_ptr != NULL) {
			animation_ptr->triggerRemove(trigger_ptr);
		}
	}

	void TSU_TriggerRemoveAll(Animation *animation_ptr)
	{
		if (animation_ptr != NULL) {
			animation_ptr->triggerRemoveAll();
		}
	}
}