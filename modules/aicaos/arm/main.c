
#include <stdio.h>
#include <stdint.h>

#include "../aica_common.h"
#include "task.h"

AICA_ADD_REMOTE(sh4_puts, 0);

static AICA_SHARED(arm_test) {
	static unsigned int nb = 0;
	printf("test %i\n", nb++);
	return 0;
}

AICA_SHARED_LIST = {
	AICA_SHARED_LIST_ELEMENT(arm_test, 0, 0),
	AICA_SHARED_LIST_END,
};

int main(int argc, char **argv)
{
	unsigned int nb = 0;

	sh4_puts(NULL, "world!");

	while(1) {
		printf("Hello %i\n", nb++);
		task_reschedule();
	}

	return 0;
}

