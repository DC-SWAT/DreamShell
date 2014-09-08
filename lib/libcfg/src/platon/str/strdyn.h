#ifndef _PLATON_STR_STRDYN_H
#define _PLATON_STR_STRDYN_H

/*
 * TODO list:
 *
 * 1. Functions for removing from array.
 * 2. Optimalizations (in strdyn_explode_ar() and similar).
 */

#ifndef PLATON_FUNC
# define PLATON_FUNC(_name) _name
#endif
#ifndef PLATON_FUNC_STR
# define PLATON_FUNC_STR(_name) #_name
#endif

#define strdyn_count(ar)           PLATON_FUNC(strdyn_get_size)(ar)
#define strdyn_duplicate(ar)       PLATON_FUNC(strdyn_create_ar)(ar)
#define strdyn_safe_duplicate(ar)  PLATON_FUNC(strdyn_safe_create_ar)(ar)
#define strdyn_remove(ar, s)       PLATON_FUNC(strdyn_remove_str)(ar, s)
#define strdyn_intersect(ar1, ar2) PLATON_FUNC(strdyn_conjunct)(ar1, ar2)
#define strdyn_join(ar1, ar2)      PLATON_FUNC(strdyn_consolide)(ar1, ar2)
#define strdyn_union(ar1, ar2)     PLATON_FUNC(strdyn_consolide)(ar1, ar2)
#define strdyn_explode(str, sep)   PLATON_FUNC(strdyn_explode_str)(str, sep)
#define strdyn_explode2(str, sep)  PLATON_FUNC(strdyn_explode2_str)(str, sep)
#define strdyn_implode(str, sep)   PLATON_FUNC(strdyn_implode_str)(str, sep)
#define strdyn_implode2(str, sep)  PLATON_FUNC(strdyn_implode2_str)(str, sep)
#define strdyn_cmp(ar, s)          PLATON_FUNC(strdyn_compare)(ar, s)
#define strdyn_casecmp(ar, s)      PLATON_FUNC(strdyn_casecompare)(ar, s)
#define strdyn_str(s, ar)          PLATON_FUNC(strdyn_str2)(s, ar, NULL)


#ifdef __cplusplus
extern "C" {
#endif

	void PLATON_FUNC(strdyn_free)(char **ar);
	void PLATON_FUNC(strdyn_safe_free)(char **ar);
	int  PLATON_FUNC(strdyn_get_size)(char **ar);
	char **PLATON_FUNC(strdyn_create)(void);
	char **PLATON_FUNC(strdyn_create_va)(char *s1, ...);
	char **PLATON_FUNC(strdyn_create_ar)(char **ar);
	char **PLATON_FUNC(strdyn_safe_create_ar)(char **ar);
	char **PLATON_FUNC(strdyn_add)(char **ar, const char *s);
	char **PLATON_FUNC(strdyn_add_va)(char **ar, ...);
	char **PLATON_FUNC(strdyn_add_ar)(char **ar, char * const *s_ar);
	char **PLATON_FUNC(strdyn_remove_idx)(char **ar, int idx);
	char **PLATON_FUNC(strdyn_remove_str)(char **ar, char *s);
	char **PLATON_FUNC(strdyn_remove_str_all)(char **ar, char *s);
	char **PLATON_FUNC(strdyn_remove_empty)(char **ar);
	char **PLATON_FUNC(strdyn_remove_all)(char **ar);
	char **PLATON_FUNC(strdyn_explode_chr)(char *str, int sep);
	char **PLATON_FUNC(strdyn_explode2_chr)(char *str, int sep);
	char **PLATON_FUNC(strdyn_explode_str)(char *str, char *sep);
	char **PLATON_FUNC(strdyn_explode2_str)(char *str, char *sep);
	char **PLATON_FUNC(strdyn_explode_ar)(char *str, char **sep);
	char **PLATON_FUNC(strdyn_explode2_ar)(char *str, char **sep);
	char *PLATON_FUNC(strdyn_implode_chr)(char **ar, int sep);
	char *PLATON_FUNC(strdyn_implode2_chr)(char **ar, int sep);
	char *PLATON_FUNC(strdyn_implode_str)(char **ar, char *sep);
	char *PLATON_FUNC(strdyn_implode2_str)(char **ar, char *str);
	char **PLATON_FUNC(strdyn_conjunct)(char **ar1, char **ar2);
	char **PLATON_FUNC(strdyn_consolide)(char **ar1, char **ar2);
	int  PLATON_FUNC(strdyn_search)(char **ar, char *s);
	int  PLATON_FUNC(strdyn_casesearch)(char **ar, char *s);
	int  PLATON_FUNC(strdyn_compare)(char **ar, char *s);
	int  PLATON_FUNC(strdyn_casecompare)(char **ar, char *s);
	int  PLATON_FUNC(strdyn_compare_all)(char **ar, char *s);
	char *PLATON_FUNC(strdyn_str2)(char *s, char **ar, int *idx);

#ifdef __cplusplus
}
#endif

#endif /* _PLATON_STR_STRDYN_H */

