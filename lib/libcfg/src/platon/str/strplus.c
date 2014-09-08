#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <stdlib.h>

#include "strplus.h"
#include "strctype.h"

	char *
PLATON_FUNC(strestr)(s1, s2)
	const char *s1;
	const char *s2;
{
	return strstr(s1,s2) == NULL ? NULL : strendstr(s1,s2);
}

#if ! defined(SELF) && ! defined(SELFTEST) && ! defined(SELF_STRPLUS)

/*
 * Functions strtolower() and strtoupper(), used by function below,
 * are defined in strctype.c so when we are building self-testing
 * binary, skip this section.
 */

	char *
PLATON_FUNC(stristr)(s1, s2)
	const char *s1;
	const char *s2;
{
	char *a_s1, *a_s2;
	register char *ret = NULL;

	a_s1 = strdup(s1);
	a_s2 = strdup(s2);

	if (a_s1 != NULL && a_s2 != NULL) {
		ret = strstr(PLATON_FUNC(strtolower)(a_s1),
				PLATON_FUNC(strtolower)(a_s2));
		if (ret != NULL)
			ret = (char *) s1 + (ret - a_s1);
	}

	if (a_s2 != NULL)
		free(a_s2);

	if (a_s1 != NULL)
		free(a_s1);

	return ret;
}

#endif

	char *
PLATON_FUNC(str_white_str)(str, substr, size)
	char *str;
	char *substr;
	int  *size;
{
#if 0
	/*
	 * This is fastfix code substitution for str_white_str() function,
	 * because new wersion from 'Crasher' was not fully tested yet.
	 */

	*size = strlen(substr);
	return strstr(str, substr);

#else
	register int slen, plen, ssize;
	register char *pptr, *sptr, *start;

	slen = strlen(str);
	plen = strlen(substr);

	for (start = str, pptr = substr; slen >= plen; start++, slen--) {

		/* Find start of pattern in string. */
		while (*start != *substr) {

			if ((isspace(*start)) && (isspace(*substr)))
				break;

			start++;
			slen--;
			/* If pattern longer than string. */
			if (slen < plen)
				return NULL;
		}

		ssize = 0;
		sptr = start;
		pptr = substr;

		while (1) {

#ifdef DEBUG_STRPLUS /* if str_white_str() works properly, delete this */
			printf("comparing %d [%s] with %d [%s]\n",
					*sptr, sptr, *pptr, pptr);
#endif

			if ((isspace(*sptr)) && (isspace(*pptr))) {
				while (isspace(*sptr)) {
					++sptr;

					if (isspace(*pptr)) {
						++ssize;
						++pptr;
					}
					else
						++ssize;
				}
			}
			else if (*sptr == *pptr) {
				while (*sptr == *pptr && *sptr != '\0' && ! isspace(*sptr)) {
					sptr++;
					pptr++;
					ssize++;
				}
			}
			else {
				break;
			}

			/* If end of pattern then pattern was found. */
			if (*pptr == '\0') {
				if (size != NULL)
					*size = ssize;
				return start;
			}
		}
	}

	return NULL;
#endif
}


	int
PLATON_FUNC(strcnt)(str, c)
	const char *str;
	const int c;
{
	register int i = 0;

	if (str != NULL)
		while (*str != '\0')
			if (*str++ == (char) c)
				i++;

	return i;
}

	int
PLATON_FUNC(strcnt_str)(str, substr)
	const char *str;
	const char *substr;
{
	register char *s;
	register int count;

	for (count = 0; ; count++) {
		if ((s = strstr(str, substr)) == NULL)
			break;
		else
			str += (s - str) + 1;
	}

	return count;
}

	int
PLATON_FUNC(strcnt_sepstr)(str, substr)
	const char *str;
	const char *substr;
{
	register char *s;
	register int count, substr_size;

	substr_size = strlen(substr);

	for (count = 0; ; count++) {
		if ((s = strstr(str, substr)) == NULL)
			break;
		else
			str += (s - str) + substr_size;
	}

	return count;
}

	char *
PLATON_FUNC(strdel)(s)
	char *s;
{
#if 1
	return (char *) memmove(s, s + 1, strlen(s));
#else
	register int i;

	for (i = 0; s[i] != '\0'; i++)
		s[i] = s[i + 1];

	return s;
#endif
}

	char *
PLATON_FUNC(strrmlf)(s)
	char *s;
{
	register char *p_lf;

	while ((p_lf = strchr(s, '\n')) != NULL)
		PLATON_FUNC(strdel)(p_lf);

	return s;
}

	char *
PLATON_FUNC(strrmcr)(s)
	char *s;
{
	register char *p_cr;

	while ((p_cr = strchr(s, '\r')) != NULL)
		PLATON_FUNC(strdel)(p_cr);

	return s;
}

	char *
PLATON_FUNC(str_left_trim)(s)
	char *s;
{
	register char *pos;

	for (pos = s; *pos != '\0' && isspace(*pos); pos++) ;

	if (pos > s)
		memmove((void *) s, (void *) pos, strlen(pos) + 1);

	return s;
}

	char *
PLATON_FUNC(str_right_trim)(s)
	char *s;
{
	register char *pos;

	for (pos = s + (strlen(s) - 1); pos >= s && isspace(*pos); pos--) ;

	*(pos + 1) = '\0';

	return s;
}

	char *
PLATON_FUNC(str_trim_whitechars)(s)
	char *s;
{
	register char *pos, *start;

	for (pos = s, start = NULL; ; pos++) {
		if (isspace(*pos)) {
			if (start == NULL)
				start = pos;
		}
		else {
			if (start != NULL) {
				memmove(start + 1, pos, strlen(pos) + 1);
				*start = ' ';

				pos = start + 1;
				start = NULL;
			}
		}

		if (*pos == '\0')
			break;
	}

	return s;
}

	char *
PLATON_FUNC(strins)(str, ins)
	char *str;
	char *ins;
{
	register int ins_len = strlen(ins);

	memmove(str + ins_len, str, strlen(str) + 1);
	strncpy(str, ins, ins_len);

	return str;
}

	char *
PLATON_FUNC(strrev)(str)
	char *str;
{
	register int i, c, len = strlen(str);

	/* Code borrowed from PHP: Hypertext Preprocessor, http://www.php.net/ */
	for (i = 0; i < len - 1 - i; i++) {
		c = str[i];
		str[i] = str[len - 1 - i];
		str[len - 1 - i] = c;
	}

	return str;
}

	int
PLATON_FUNC(strrcmp)(s1, s2)
	const char *s1;
	const char *s2;
{
	register char *x1, *x2;

	x1 = strchr(s1,'\0');
	x2 = strchr(s2,'\0');

	while (x1 > s1 && x2 > s2) {
		x1--;
		x2--;
		if (strcmp(x1,x2))
			return strcmp(x1,x2);
	}

	return strlen(s1) - strlen(s2);
}

#if defined(SELF) || defined(SELFTEST) || defined(SELF_STRPLUS) || defined(SELF_STRPLUS2)

#include <stdio.h>

#define TESTSTR1 "___AAA_BBBB__C_DaDaD____"
#define TESTSEP1 '_'
#define TESTSEP2 "BB"

#define TESTSTR2 " \t  AAA\nBBBB__C   D\taDa D \t   \t"

	static void
strins_selftest(void)
{
	char long_str[80];

#define SEARCH_STR	"56"
#define INSERT_STR	"<-now-goes-56->"

	strcpy(long_str, "1234567890");
	printf("  long_str before: %s\n", long_str);

	printf("  Now we're searching '%s' and want to insert '%s' before it.\n",
			SEARCH_STR, INSERT_STR);

	strins(strstr(long_str, SEARCH_STR), INSERT_STR);
	printf("  long_str  after: %s\n", long_str);

	return;
}

	int
main(argc, argv)
	int argc;
	char **argv;
{
	char *str2;

	puts("Entering str_white_str() selftest:");

	if (argc > 2) {
		int size;

		str2 = PLATON_FUNC(str_white_str)(argv[1], argv[2], &size);
		/* str2 = str_white_str("telnet \t atlantis.sk 	5678", "t  a", &size);
		*/
		printf("  ptr = [%s], size = %d\n", str2, size);

		return 0;
	}

	printf("  Usage: %s <string> <substring>\n\n", argv[0]);

	str2 = strdup(TESTSTR2);

	printf("strcnt(\"%s\", '%c') = %d\n",
			TESTSTR1, TESTSEP1,
			PLATON_FUNC(strcnt)(TESTSTR1, TESTSEP1));

	printf("strcnt_str(\"%s\", \"%s\") = %d\n",
			TESTSTR1, TESTSEP2,
			PLATON_FUNC(strcnt_str)(TESTSTR1, TESTSEP2));

	printf("strcnt_sepstr(\"%s\", \"%s\") = %d\n",
			TESTSTR1, TESTSEP2,
			PLATON_FUNC(strcnt_sepstr)(TESTSTR1, TESTSEP2));

	printf("str_trim_whitechars(\"%s\") = \"%s\"\n",
			TESTSTR2,
			PLATON_FUNC(str_trim_whitechars)(str2));

	printf("strdel(\"%s\") = ", str2);
	printf("\"%s\"\n", PLATON_FUNC(strdel)(str2));

	printf("strrev(\"%s\") = ", str2);
	printf("\"%s\"\n", PLATON_FUNC(strrev)(str2));
	strrev(str2); /* Reversing back, just for sure */
	free(str2);

#if ! defined(SELF) && ! defined(SELFTEST) && ! defined(SELF_STRPLUS)
	{ /* stristr() selftest */
		char *ptr    = "Reply-To";
		char *search = "reply-to";
		char *output = stristr(ptr, search);
		printf("stristr(\"%s\", \"%s\") = \"%s\"\n", ptr, search, output);
		printf("  (\"%s\" == \"%s\") == %d\n", ptr, output, ptr == output);
	}
#endif


	puts("\nEntering strins_selftest():");
	strins_selftest();

	return 0;
}

#endif /* #if defined(SELF) || defined(SELFTEST) || defined(SELF_STRPLUS) || defined(SELF_STRPLUS2) */

