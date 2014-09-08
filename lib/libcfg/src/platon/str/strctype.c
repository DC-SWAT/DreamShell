#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <string.h>
#include <ctype.h>

#include "strctype.h"

	int
PLATON_FUNC(strctype_fnc)(s, fnc)
	const char *s;
	int (*     fnc)(int);
{
	register int i;

	for (i = 0; s[i] != '\0'; i++)
		if (! fnc(s[i]))
			return 0;

	return 1;
}
/*
	char *
PLATON_FUNC(strtolower)(s)
	char *s;
{
	register int i;

	for (i = 0; s[i] != '\0'; i++)
		s[i] = tolower(s[i]);

	return s;
}*/

	char *
PLATON_FUNC(strtoupper)(s)
	char *s;
{
	register int i;

	for (i = 0; s[i] != '\0'; i++)
		s[i] = toupper(s[i]);

	return s;
}

#if defined(SELF) || defined(SELFTEST) || defined(SELF_STRCTYPE)

#include <stdio.h>
#include <stdlib.h>

#define TESTSTR1 "___AAA_BBBB__C_DaDaD____"
#define TESTSTR2 "aaa~!@#$%^&*()_+|{];':\",./<>?"
#define TESTSTR3 "abcdefghijklmnoprstu"
#define TESTSTR4 "ABCDEFGHIJKLMNOPRSTU"

	int
main(void)
{

	/* Testing strisXXX() functions. */

	printf("strislower(\"%s\") = %d\n", TESTSTR1, strislower(TESTSTR1));
	printf("strisupper(\"%s\") = %d\n", TESTSTR1, strisupper(TESTSTR1));
	printf("strislower(\"%s\") = %d\n", TESTSTR2, strislower(TESTSTR2));
	printf("strisupper(\"%s\") = %d\n", TESTSTR2, strisupper(TESTSTR2));
	printf("strislower(\"%s\") = %d\n", TESTSTR3, strislower(TESTSTR3));
	printf("strisupper(\"%s\") = %d\n", TESTSTR3, strisupper(TESTSTR3));
	printf("strislower(\"%s\") = %d\n", TESTSTR4, strislower(TESTSTR4));
	printf("strisupper(\"%s\") = %d\n", TESTSTR4, strisupper(TESTSTR4));

	/* Testing strtoXXX() functions. */

	{
		char *s1, *s2;

		s1 = strdup(TESTSTR1);
		s2 = strdup(TESTSTR2);

		if (s1 == NULL || s2 == NULL)
			return 1;

		printf("strtolower(\"%s\") =", s1);
		printf(" \"%s\"\n", strtolower(s1));

		printf("strtoupper(\"%s\") =", s2);
		printf(" \"%s\"\n", strtoupper(s2));

		free(s1);
		free(s2);

	}

	return 0;
}

#endif /* #if defined(SELF) || defined(SELFTEST) || defined(SELF_STRCTYPE) */

