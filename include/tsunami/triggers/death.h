/*
   Tsunami for KallistiOS ##version##

   death.h

   Copyright (C) 2002 Megan Potter
   Copyright (C) 2024 Maniac Vera

*/

#ifndef __TSUNAMI_TRIG_DEATH_H
#define __TSUNAMI_TRIG_DEATH_H

#include "../trigger.h"

#ifdef __cplusplus

#include <memory>

class Death : public Trigger {
public:
	// Constructor / Destructor
	Death(Drawable *target = nullptr);
	virtual ~Death();

	virtual void trigger(Drawable *t, Animation *a);
private:
	Drawable* m_target;
};

#else 

typedef struct death Death;

#endif


#ifdef __cplusplus
extern "C"
{
#endif

Death* TSU_DeathCreate(Drawable *target_ptr);
void TSU_DeathDestroy(Death **death_ptr);

#ifdef __cplusplus
};
#endif

#endif	/* __TSUNAMI_TRIG_DEATH_H */
