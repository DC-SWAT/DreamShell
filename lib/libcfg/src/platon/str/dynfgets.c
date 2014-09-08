#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include "dynfgets.h"

	char *
PLATON_FUNC(dynamic_fgets)(fp)
	FILE *fp;
{
	char temp[DYNAMIC_FGETS_BUFSIZE];
	register char *ptr;
	register int i;

	if ((ptr = (char *) malloc(1)) == NULL)
		return NULL;

	for (*ptr = '\0', i = 0; ; i++) {
		if (fgets(temp, DYNAMIC_FGETS_BUFSIZE, fp) == NULL) {
			if (ferror(fp) != 0 || i == 0) {
				free(ptr);
				return NULL;
			}

			return ptr;
		}

		ptr = (char *) realloc(ptr, (DYNAMIC_FGETS_BUFSIZE - 1) * (i + 1) + 1);
		if (ptr == NULL)
			return NULL;

		strcat(ptr, temp);

		if (strchr(temp, '\n') != NULL) {
			*strchr(ptr, '\n') = '\0';
			return ptr;
		}
	}
}

