#ifndef _PLATON_STR_STRCTYPE_H
#define _PLATON_STR_STRCTYPE_H

#ifndef PLATON_FUNC
# define PLATON_FUNC(_name) _name
#endif
#ifndef PLATON_FUNC_STR
# define PLATON_FUNC_STR(_name) #_name
#endif

#  define strisalnum(s)  PLATON_FUNC(strctype_fnc)(s, isalnum)
#  define strisalpha(s)  PLATON_FUNC(strctype_fnc)(s, isalpha)
#  define strisascii(s)  PLATON_FUNC(strctype_fnc)(s, isascii)
#  define strisblank(s)  PLATON_FUNC(strctype_fnc)(s, isblank)
#  define striscntrl(s)  PLATON_FUNC(strctype_fnc)(s, iscntrl)
#  define strisdigit(s)  PLATON_FUNC(strctype_fnc)(s, isdigit)
#  define strisgraph(s)  PLATON_FUNC(strctype_fnc)(s, isgraph)
#  define strislower(s)  PLATON_FUNC(strctype_fnc)(s, islower)
#  define strisprint(s)  PLATON_FUNC(strctype_fnc)(s, isprint)
#  define strispunct(s)  PLATON_FUNC(strctype_fnc)(s, ispunct)
#  define strisspace(s)  PLATON_FUNC(strctype_fnc)(s, isspace)
#  define strisupper(s)  PLATON_FUNC(strctype_fnc)(s, isupper)
#  define strisxdigit(s) PLATON_FUNC(strctype_fnc)(s, isxdigit)
#  define strlwr(s) PLATON_FUNC(strtolower)(s)
#  define strupr(s) PLATON_FUNC(strtoupper)(s)

#ifdef __cplusplus
extern "C" {
#endif

	char *PLATON_FUNC(strtolower)(char *s);
	char *PLATON_FUNC(strtoupper)(char *s);
	int   PLATON_FUNC(strctype_fnc)(const char *s, int (*fnc)(int));

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _PLATON_STR_STRCTYPE_H */

