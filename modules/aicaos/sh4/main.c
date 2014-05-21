
#include <kos.h>

#include "../aica_common.h"
#include "aica_sh4.h"

/* Wrapper to the real function. */
static AICA_SHARED(sh4_puts)
{
	return puts((char *)in);
}

AICA_SHARED_LIST = {
	AICA_SHARED_LIST_ELEMENT(sh4_puts, 0x100, 0),
	AICA_SHARED_LIST_END,
};

AICA_ADD_REMOTE(arm_puts, PRIORITY_DEFAULT);

