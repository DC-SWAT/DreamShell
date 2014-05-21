#ifndef _SPINLOCK_H
#define _SPINLOCK_H

#include "interrupt.h"
#include "task.h"

typedef volatile int spinlock_t;

#define SPINLOCK_INITIALIZER 0

#define spinlock_init(A) *(A) = SPINLOCK_INITIALIZER

static inline int spinlock_trylock(spinlock_t *lock)
{
	int res = 0, context = int_disable();

	if (!*lock)
		res = *lock = 1;

	int_restore(context);
	return res;
}

#define spinlock_lock(A) while (!spinlock_trylock(A)) task_reschedule()
#define spinlock_unlock(A) *(A) = 0
#define spinlock_is_locked(A) ( *(A) != 0 )

#endif
