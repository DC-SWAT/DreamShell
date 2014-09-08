#ifdef HAVE_CONFIG_H
#  include "config.h"
#endif

#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "strplus.h"
#include "strdyn.h"

	void
PLATON_FUNC(strdyn_free)(ar)
	char **ar;
{
	register int i;

	for (i = 0; ar[i] != NULL; i++)
		free(ar[i]);

	free(ar);
}

	void
PLATON_FUNC(strdyn_safe_free)(ar)
	char **ar;
{
	if (ar == NULL)
		return;

	PLATON_FUNC(strdyn_free)(ar);

	return;
}

	int
PLATON_FUNC(strdyn_get_size)(ar)
	char **ar;
{
	register int i;

	for (i = 0; ar[i] != NULL; i++)
		;

	return i;
}

	char **
PLATON_FUNC(strdyn_create)(void)
{
	register char **ar;

	if ((ar = (char **) malloc(1 * sizeof(char *))) == NULL)
		return NULL;

	ar[0] = NULL;

	return ar;
}

char **
PLATON_FUNC(strdyn_create_va)(
		char *s1,
		...)
{
	register char **ar;

	if ((ar = PLATON_FUNC(strdyn_create)()) == NULL)
		return NULL;

	if (s1 != NULL) {
		register char *s;
		va_list ap;

		if ((ar = PLATON_FUNC(strdyn_add)(ar, s1)) == NULL)
			return NULL;

		va_start(ap, s1);

		while ((s = va_arg(ap, char *)) != NULL)
			if ((ar = PLATON_FUNC(strdyn_add)(ar, s)) == NULL)
				return NULL;

		va_end(ap);
	}

	return ar;
}

	char **
PLATON_FUNC(strdyn_create_ar)(ar)
	char **ar;
{
	register int i;
	register char **new_ar;

	if ((new_ar = (char**) malloc((PLATON_FUNC(strdyn_get_size)(ar) + 1)
					* sizeof(char*))) == NULL)
			return NULL;

	for (i = 0; ar[i] != NULL; i++)
		new_ar[i] = strdup(ar[i]);

	new_ar[i] = NULL;

	return new_ar;
}

char **
PLATON_FUNC(strdyn_safe_create_ar)(ar)
	char **ar;
{
	if (ar == NULL)
		return NULL;

	return PLATON_FUNC(strdyn_create_ar)(ar);
}

char **
PLATON_FUNC(strdyn_add)(ar, s)
	char       **ar;
	const char *s;
{
	register int count;

	if (ar == NULL)
		if ((ar = PLATON_FUNC(strdyn_create)()) == NULL)
			return NULL;

	count = PLATON_FUNC(strdyn_get_size)(ar);

	if ((ar = (char **) realloc(ar, (count + 2) * sizeof(char *))) == NULL)
		return NULL;

	ar[count] = strdup(s);
	ar[count + 1] = NULL;

	return ar;
}

char **
PLATON_FUNC(strdyn_add_va)(
		char **ar,
		...)
{
	register char *s;
	va_list ap;

	if (ar == NULL)
		if ((ar = PLATON_FUNC(strdyn_create)()) == NULL)
			return NULL;

	va_start(ap, ar);

	while ((s = va_arg(ap, char *)) != NULL)
		if ((ar = PLATON_FUNC(strdyn_add)(ar, s)) == NULL)
			return NULL;

	va_end(ap);

	return ar;
}

char **
PLATON_FUNC(strdyn_add_ar)(ar, s_ar)
	char         **ar;
	char * const *s_ar;
{
	register int k;

	for (k = 0; s_ar[k] != NULL; k++)
		if ((ar = PLATON_FUNC(strdyn_add)(ar, s_ar[k])) == NULL)
			return NULL;

	return ar;
}

char **
PLATON_FUNC(strdyn_remove_idx)(ar, idx)
	char **ar;
	int  idx;
{
	register int i;

	for (i = 0; ar[i] != NULL; i++) {
		if (i == idx)
			free(ar[i]);

		if (i >= idx)
			ar[i] = ar[i + 1];
	}

	if ((ar = (char**) realloc(ar, i * sizeof(char*))) == NULL)
		return NULL;

	return ar;
}

char **
PLATON_FUNC(strdyn_remove_str)(ar, s)
	char **ar;
	char *s;
{
	register int idx;

	idx = PLATON_FUNC(strdyn_search)(ar, s);

	if (idx < 0)
		return ar;

	return PLATON_FUNC(strdyn_remove_idx)(ar, idx);
}

char **
PLATON_FUNC(strdyn_remove_str_all)(ar, s)
	char **ar;
	char *s;
{
	char **new_ar = NULL;

	while (new_ar != ar) {
		if (new_ar != NULL)
			ar = new_ar;

		if ((new_ar = PLATON_FUNC(strdyn_remove_str)(ar, s)) == NULL)
			return NULL;
	}

	return ar;
}

char **
PLATON_FUNC(strdyn_remove_empty)(ar)
	char **ar;
{
	register int i, j;

	for (i = 0; ar[i] != NULL; ) {

		if (strlen(ar[i]) == 0) {
			free(ar[i]);

			for (j = i; ar[j] != NULL; j++)
				ar[j] = ar[j + 1];
		}
		else
			i++;
	}

	if ((ar = (char**) realloc(ar, (i + 1) * sizeof(char*))) == NULL)
		return NULL;

	return ar;
}

char **
PLATON_FUNC(strdyn_remove_all)(ar)
	char **ar;
{
	register int i;

	for (i = 0; ar[i] != NULL; i++)
		free(ar[i]);

	if ((ar = (char**) realloc(ar, /* 1 * */ sizeof(char*))) == NULL)
		return NULL;

	ar[0] = NULL;

	return ar;
}

char **
PLATON_FUNC(strdyn_explode_chr)(str, sep)
	char *str;
	int sep;
{
	char sep_str[2];

	sep_str[0] = (char) sep;
	sep_str[1] = '\0';

	return PLATON_FUNC(strdyn_explode_str)(str, sep_str);
}

char **
PLATON_FUNC(strdyn_explode2_chr)(str, sep)
	char *str;
	int sep;
{

	return PLATON_FUNC(strdyn_remove_empty)(PLATON_FUNC(strdyn_explode_chr)(str, sep));
}

char **
PLATON_FUNC(strdyn_explode_str)(str, sep)
	char *str;
	char *sep;
{
	register char **ar;
	register char *s;
	register int ar_size, s_size, sep_size, i;

	if (str == NULL || sep == NULL)
		return NULL;

	ar_size = PLATON_FUNC(strcnt_sepstr)(str, sep);

	if ((ar = (char**) malloc((ar_size + 2) * sizeof(char*))) == NULL)
		return NULL;

	sep_size = strlen(sep);

	for (s = str, i = 0; i < ar_size; i++, s += s_size + sep_size) {

		s_size = strstr(s, sep) - s;

		if ((ar[i] = (char*) malloc((s_size + 1) * sizeof(char))) == NULL)
			return NULL;

		strncpy(ar[i], s, s_size);
		ar[i][s_size] = '\0';
	}

	if ((ar[ar_size] = strdup(s)) == NULL)
		return NULL;

	ar[ar_size + 1] = NULL;

	return ar;

}

char **
PLATON_FUNC(strdyn_explode2_str)(str, sep)
	char *str;
	char *sep;
{
	return PLATON_FUNC(strdyn_remove_empty)(PLATON_FUNC(strdyn_explode_str)(str, sep));
}

char **
PLATON_FUNC(strdyn_explode_ar)(str, sep)
	char *str;
	char **sep;
{
	/* WARNING: Unefective recursion used! */
	/* TODO: Various code optimalizations. */

	char **ar, **ar1;

	if ((ar1 = PLATON_FUNC(strdyn_explode_str)(str, sep[0])) == NULL)
		return NULL;

	if (sep[1] != NULL) {
		char **ar2;
		register int i;

		if ((ar = PLATON_FUNC(strdyn_create)()) == NULL) {
			PLATON_FUNC(strdyn_free)(ar1);
			return NULL;
		}

		for (i = 0; i < strdyn_count(ar1); i++) {
			if ((ar2 = PLATON_FUNC(strdyn_explode_ar)(ar1[i], sep + 1)) == NULL) {
				PLATON_FUNC(strdyn_free)(ar1);
				PLATON_FUNC(strdyn_free)(ar);
				return NULL;
			}

			if ((ar = PLATON_FUNC(strdyn_add_ar)(ar, ar2)) == NULL) {
				PLATON_FUNC(strdyn_free)(ar1);
				PLATON_FUNC(strdyn_free)(ar);
				PLATON_FUNC(strdyn_free)(ar2);
				return NULL;
			}

			PLATON_FUNC(strdyn_free)(ar2);
		}

		PLATON_FUNC(strdyn_free)(ar1);
	}
	else
		ar = ar1;

	return ar;
}

char **
PLATON_FUNC(strdyn_explode2_ar)(str, sep)
	char *str;
	char **sep;
{
	return PLATON_FUNC(strdyn_remove_empty)(PLATON_FUNC(strdyn_explode_ar)(str, sep));
}

char *
PLATON_FUNC(strdyn_implode_chr)(ar, sep)
	char **ar;
	int sep;
{
	char sep_str[2];

	sep_str[0] = (char) sep;
	sep_str[1] = '\0';

	return PLATON_FUNC(strdyn_implode_str)(ar, sep_str);
}

char *
PLATON_FUNC(strdyn_implode2_chr)(ar, sep)
	char **ar;
	int sep;
{
	register char **new_ar;
	register char *s;

	new_ar = PLATON_FUNC(strdyn_remove_empty)(strdyn_duplicate(ar));

	s = PLATON_FUNC(strdyn_implode_chr)(new_ar, sep);

	PLATON_FUNC(strdyn_free)(new_ar);

	return s;
}

char *
PLATON_FUNC(strdyn_implode_str)(ar, sep)
	char **ar;
	char *sep;
{
	register int i, str_size, sep_size;
	register char *str, *s;

	sep_size = strlen(sep);

	for (i = 0, str_size = 0; ar[i] != NULL; i++)
		str_size += strlen(ar[i]) + sep_size;

	str_size -= sep_size;

	if ((str = (char*) malloc((str_size + 1) * sizeof(char))) == NULL)
		return NULL;

	for (i = 0,	s = str; ar[i] != NULL; i++) {
		strcpy(s, ar[i]);
		s += strlen(ar[i]);

		if (ar[i + 1] != NULL)
			strcpy(s, sep);
		s += sep_size;
	}

	return str;
}

char *
PLATON_FUNC(strdyn_implode2_str)(ar, str)
	char **ar;
	char *str;
{
	register char **new_ar;
	register char *s;

	new_ar = PLATON_FUNC(strdyn_remove_empty)(strdyn_duplicate(ar));

	s = PLATON_FUNC(strdyn_implode_str)(new_ar, str);

	PLATON_FUNC(strdyn_free)(new_ar);

	return s;
}

char **
PLATON_FUNC(strdyn_conjunct)(ar1, ar2)
	char **ar1;
	char **ar2;
{
	register int i;
	register char **ar;

	if ((ar = PLATON_FUNC(strdyn_create)()) == NULL)
		return NULL;

	for (i = 0; ar2[i] != NULL; i++) {
		if (! PLATON_FUNC(strdyn_compare)(ar1, ar2[i])) {
			if ((ar = PLATON_FUNC(strdyn_add)(ar, ar2[i])) == NULL)
				return NULL;
		}
	}

	return ar;
}

char **
PLATON_FUNC(strdyn_consolide)(ar1, ar2)
	char **ar1;
	char **ar2;
{
	register int i;
	register char **ar;

	if ((ar = PLATON_FUNC(strdyn_create)()) == NULL)
		return NULL;

	for (i = 0; ar1[i] != NULL; i++) {
		if (PLATON_FUNC(strdyn_compare)(ar, ar1[i])) {
			if ((ar = PLATON_FUNC(strdyn_add)(ar, ar1[i])) == NULL)
				return NULL;
		}
	}

	for (i = 0; ar2[i] != NULL; i++) {
		if (PLATON_FUNC(strdyn_compare)(ar, ar2[i])) {
			if ((ar = PLATON_FUNC(strdyn_add)(ar, ar2[i])) == NULL)
				return NULL;
		}
	}

	return ar;
}

int
PLATON_FUNC(strdyn_search)(ar, s)
	char **ar;
	char *s;
{
	register int i;

	for (i = 0; ar[i] != NULL; i++)
		if (! strcmp(ar[i], s))
			return i;

	return -1;
}

int
PLATON_FUNC(strdyn_casesearch)(ar, s)
	char **ar;
	char *s;
{
	register int i;

	for (i = 0; ar[i] != NULL; i++)
		if (! strcasecmp(ar[i], s))
			return i;

	return -1;
}

int
PLATON_FUNC(strdyn_compare)(ar, s)
	char **ar;
	char *s;
{
	return PLATON_FUNC(strdyn_search)(ar, s) < 0 ? -1 : 0;
}

int
PLATON_FUNC(strdyn_casecompare)(ar, s)
	char **ar;
	char *s;
{
	return PLATON_FUNC(strdyn_casesearch)(ar, s) < 0 ? -1 : 0;
	return PLATON_FUNC(strdyn_casesearch)(ar, s) < 0 ? -1 : 0;
}

int
PLATON_FUNC(strdyn_compare_all)(ar, s)
	char **ar;
	char *s;
{
	register int i;

	for (i = 0; ar[i] != NULL; i++)
		if (strcmp(ar[i], s))
			return -1;

	return 0;
}

char *
PLATON_FUNC(strdyn_str2)(s, ar, idx)
	char *s;
	char **ar;
	int  *idx;
{
	register char *ret, *tmp_s;
	register int i;

	for (ret = NULL, i = 0; ar[i] != NULL; i++)
		if ((tmp_s = strstr(s, ar[i])) != NULL
				&& (ret == NULL || tmp_s < ret)) {
			ret = tmp_s;

			if (idx != NULL)
				*idx = i;
		}

	return ret;
}

#if defined(SELF) || defined(SELFTEST) || defined(SELF_STRDYN)

#define TESTSTR1 "___AAA_BBBB__C_DaDaD____"
#define TESTSEP1 '_'
#define TESTSEP2 "__"

	int
main(void)
{

	register int i;
	char **ar1, **ar2, **ar_join, **ar_intersect;
	char *s;

	ar2 = strdyn_create_va("A", "B", "C", "D", NULL);

	i = 0;
	while (ar2[i] != NULL) {
		printf("ar2[%d] = \"%s\"\n", i, ar2[i]);
		i++;
	}

	printf("strdyn_explode2_chr(\"%s\", '%c') = ar1\n", TESTSTR1, TESTSEP1);
	ar1 = strdyn_explode2_chr(TESTSTR1, TESTSEP1);

	puts("strdyn_free(ar1)");
	strdyn_free(ar1);

	printf("strdyn_explode_str(\"%s\", \"%s\") = ar1\n", TESTSTR1, TESTSEP2);
	ar1 = strdyn_explode_str(TESTSTR1, TESTSEP2);

	for (i = 0; ar1[i] != NULL; i++)
		printf("ar1[%d] = \"%s\"\n", i, ar1[i]);

	s = strdyn_implode2_chr(ar1, TESTSEP1);
	printf("strdyn_implode2_chr(ar1, '%c') = \"%s\"\n", TESTSEP1, s);

	puts("free(s)");
	free(s);

	s = strdyn_implode2_str(ar1, TESTSEP2);
	printf("strdyn_implode2_str(ar1, \"%s\") = \"%s\"\n", TESTSEP2, s);

	puts("free(s)");
	free(s);

	ar_join = strdyn_join(ar1, ar2);
	ar_intersect = strdyn_intersect(ar1, ar2);

	i = 0;
	while (ar_join[i] != NULL) {
		printf("ar_join[%d] = \"%s\"\n", i, ar_join[i]);
		i++;
	}

	i = 0;
	while (ar_intersect[i] != NULL) {
		printf("ar_intersect[%d] = \"%s\"\n", i, ar_intersect[i]);
		i++;
	}

	puts("strdyn_free(ar1)");
	strdyn_free(ar1);

	puts("strdyn_free(ar2)");
	strdyn_free(ar2);

	puts("strdyn_free(ar_join)");
	strdyn_free(ar_join);

	puts("strdyn_free(ar_intersect)");
	strdyn_free(ar_intersect);


	return 0;
}

#endif

