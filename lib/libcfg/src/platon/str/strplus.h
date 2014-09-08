/**************************************************************************
 *                                                                        *
 *  Advanced string locate, counting, removing, altering                  *
 *  and comparing functions                                               *
 *                                                                        *
 *  Programmed by Ondrej Jombik <nepto@platon.sk>                         *
 *  Copyright (c) 1997-2000 Condy software inc.                           *
 *  Copyright (c) 2001-2004 Platon Software Development Group             *
 *  All rights reserved.                                                  *
 *                                                                        *
 *  Updates: 16.4.2000, 8.11.2000, 5.10.2001, 21.10.2001, 5.12.2001       *
 *  20.12.2001 - str_white_str() added (thanks to <rbarlik@yahoo.com>)    *
 *   4. 2.2002 - strins() added                                           *
 *  28. 2.2002 - str_white_str() bugfix                                   *
 *  24. 9.2003 - stristr() rewritten                                      *
 *                                                                        *
 **************************************************************************/

/**
 * Advanced string locate, counting, removing, inserting
 * and comparing functions
 *
 * @file	platon/str/strplus.h
 * @author	Ondrej Jombik <nepto@platon.sk>
 * @version	\$Platon: libcfg+/src/platon/str/strplus.h,v 1.27 2004/01/12 06:03:09 nepto Exp $
 * @date	1997-2004
 */

#ifndef _PLATON_STR_STRPLUS_H
#define _PLATON_STR_STRPLUS_H

#ifndef PLATON_FUNC
# define PLATON_FUNC(_name) _name
#endif
#ifndef PLATON_FUNC_STR
# define PLATON_FUNC_STR(_name) #_name
#endif

#ifdef __cplusplus
extern "C" {
#endif

	/**
	 * @name Locate functions
	 */
	/**@{*/

	/**
	 * Macro that locates end of substring
	 *
	 * @param		__s1	where to search (haystack)
	 * @param		__s2	what to search (needle)
	 * @return		pointer to character after __s2
	 * @warning		use only if you are sure, that substring is located in string;
	 *				otherwise use strestr() function
	 */
#define strendstr(__s1,__s2) (strstr(__s1,__s2) + strlen(__s2))

	/**
	 * Locates a end of substring
	 *
	 * @param		s1		where to search (haystack)
	 * @param		s2		what to search (needle)
	 * @return		pointer to character after s2 or NULL if not found
	 */
	char *PLATON_FUNC(strestr)(const char *s1, const char *s2);

	/**
	 * Locates a substring case-insensitive
	 *
	 * @param		s1		where to search (haystack)
	 * @param		s2		what to search (needle)
	 * @return		pointer to s2 in s1 or NULL if not found
	 */
	char *PLATON_FUNC(stristr)(const char *s1, const char *s2);

#define strcasestr(__s1, __s2) stristr(__s1, __s2) /**< alias to stristr() */

	/**
	 * Searches substr in str with special whitespaces handling
	 *
	 * @param	str		where to search (haystack)
	 * @param	substr	what to search (needle)
	 * @retval	size	size of matched patern if found, undefined if not found
	 * @return	pointer to substr in str on NULL if not found
	 * @author	Rastislav 'Crasher' Barlik <rbarlik@yahoo.com>\n
	 *			(patched by: Ondrej Jombik <nepto@platon.sk> [28/2/2002])
	 *
	 * This function works just like classical strstr() call, with following
	 * advanced feature. Every white char in substr can be substitued with one
	 * or more white chars in str. In size, if not NULL was passed, will be the
	 * length of matched patern.
	 */
	char *PLATON_FUNC(str_white_str)(char *str, char *substr, int *size);

	/* TODO: remove this (???) */
	/** alias to str_white_str() */
#define strwhitestr(str, substr, size) str_white_str(str, substr, size)

	/**
	 * Function str_white_str() without usage of matched pattern size.
	 *
	 * This function works just like str_white_str(), but third return value
	 * parameter (pattern length) is unused.
	 */
#define strwstr(str, substr) str_white_str(str, substr, NULL)

	/**@}*/
	/**
	 * @name Counting functions
	 */
	/**@{*/

	/**
	 * Counts number of characters in string
	 *
	 * @param	str		input string
	 * @param	c		character to count
	 * @return	number of c occurences in str
	 */
	int PLATON_FUNC(strcnt)(const char *str, const int c);

	/**
	 * Counts number of substrings in string
	 *
	 * @param	str		input string
	 * @param	substr	substring to count
	 * @return	number of substr occurences in str
	 *
	 * Note that in this function strings may overlay. For separate strings
	 * counting use strcnt_sepstr() function.
	 */
	int PLATON_FUNC(strcnt_str)(const char *str, const char *substr);

	/**
	 * Count number of separate substrings in string
	 *
	 * @param	str		input string
	 * @param	substr	substring to count
	 * @return	number of separate substr occurences in str
	 *
	 * Note that in this function will be only not overlayed strings counted.
	 * For counting overlayed strings use strcnt_str() function. Also note,
	 * that counting is performed from beginning of string. Result count may,
	 * but MUST NOT be the highest number of separate substr substrings in str.
	 */
	int PLATON_FUNC(strcnt_sepstr)(const char *str, const char *substr);

	/**@}*/
	/**
	 * @name Removing functions
	 */
	/**@{*/

	/**
	 * Deletes a one character
	 *
	 * @param	s	where to delete one character
	 * @return	modified string
	 */
	char *PLATON_FUNC(strdel)(char *s);

	/**
	 * Removes all occurences of LF (Line Feed)
	 *
	 * @param	s	string where to remove all LF (\\n) characters
	 * @return	modified string
	 */
	char *PLATON_FUNC(strrmlf)(char *s);

	/**
	 * Removes all occurences of CR (Carriage Return)
	 *
	 * @param	s	string where to remove all CR (\r) characters
	 * @return	modified string
	 */
	char *PLATON_FUNC(strrmcr)(char *s);

	/** alias that removes all occurences of LF and CR characters */
#define strrmeol(__s)	strrmcr(strrmlf(__s))
#define strrmcrlf(__s)	strrmeol(__s)	/**< alias to strrmeol() */
#define strrmlfcr(__s)	strrmeol(__s)	/**< alias to strrmeol() */

	/**
	 * Removes white characters from the beginning of string
	 *
	 * @param	s	string
	 * @param	modified string
	 */
	char *PLATON_FUNC(str_left_trim)(char *s);

	/**
	 * Removes white characters from the end of string
	 *
	 * @param	s	string
	 * @param	modified string
	 */
	char *PLATON_FUNC(str_right_trim)(char *s);

#define ltrim(s) PLATON_FUNC(str_left_trim)(s) /**< alias to str_left_trim() */
#define rtrim(s) PLATON_FUNC(str_right_trim)(s)/**< alias to str_right_trim()*/

	/**
	 * Removes white characters from beginning and end of string
	 *
	 * @param	s	string
	 * @return	modified string
	 */
#define trim(s) rtrim(ltrim(s))

#define strtrim(s)	trim(s) /**< alias to trim() */
#define str_trim(s)	trim(s) /**< alias to trim() */

	/**
	 * Substitute every group of whitespaces for one space
	 *
	 * @param	s	string
	 * @return	modified string
	 */
	char *PLATON_FUNC(str_trim_whitechars)(char *s);

	/**@}*/
	/**
	 * @name Altering functions
	 */
	/**@{*/

	/**
	 * Inserts string into string
	 *
	 * @param	str		where to insert
	 * @param	ins		what to insert
	 * @return	modified string
	 *
	 * This function inserts string ins at position str. Note that there MUST
	 * be enough memory allocated in str to avoid memory ovelaping after str.
	 */
	char *PLATON_FUNC(strins)(char *str, char *ins);

	/**
	 * Reverse string
	 *
	 * @param	str		where to insert
	 * @return	reversed string
	 *
	 * This function reverses passed string.
	 */
	char *PLATON_FUNC(strrev)(char *str);

	/**@}*/
	/**
	 * @name Comparing functions
	 */
	/**@{*/

	/**
	 * Compares two string in reverse order
	 *
	 * @param	s1	first string to compare
	 * @param	s2	second string to compare
	 * @return	0 if strings are indetical, strcmp() difference of the shortest
	 *			different substrings otherwise
	 */
	int PLATON_FUNC(strrcmp)(const char *s1, const char *s2);

	/**@}*/

#ifdef __cplusplus
}
#endif

#endif /* #ifndef _PLATON_STR_STRPLUS_H */

