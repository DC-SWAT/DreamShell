/*
   Tsunami for KallistiOS ##version##

   birth.h

   Copyright (C) 2003 Megan Potter

*/

#ifndef __TSUNAMI_TRIG_BIRTH_H
#define __TSUNAMI_TRIG_BIRTH_H

#include "../trigger.h"

#include <memory>

class Birth : public Trigger {
public:
	// Constructor / Destructor
	Birth(std::shared_ptr<Drawable> newDrawable,
	      std::shared_ptr<Drawable> target = nullptr);
	virtual ~Birth();

	virtual void trigger(Drawable *t, Animation *a);

private:
	std::shared_ptr<Drawable>	m_newDrawable;
	std::shared_ptr<Drawable>	m_target;
};

#endif	/* __TSUNAMI_TRIG_BIRTH_H */
