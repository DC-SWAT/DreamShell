/* KallistiOS ##version##

   string.h
   (c)2000 Dan Potter

   $Id: string.h,v 1.3 2002/07/27 00:52:08 bardtx Exp $

*/

#ifndef __STRING_H
#define __STRING_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>

void   bcopy(const void * src, void * dest, size_t count);
void   bzero(void *s, size_t n);
char * index(const char *p, int ch);
void * memchr(const void *s, uint8 c, size_t n);
int    memcmp(const void * cs,const void * ct,size_t count);
void * memcpy(void *p1, const void *p2, size_t count);
void * memmove(void *p1, const void *p2, size_t count);
void * memset(void *p1, uint32 val, size_t count);
void * memscan(void * addr, int c, size_t size);
char * rindex(const char *p, int ch);
char * strcat(char * dest, const char * src);
char * strchr(const char * s, int c);
int    strcmp(const char * cs,const char * ct);
int    strcoll(const char *s1, const char *s2);
char * strcpy(char * dest,const char *src);
size_t strcspn(const char *s1, const char *s2);
char * strdup(const char * src);
char * strerror(int errnum);
int    stricmp(const char *cs, const char *ct);
size_t strlen(const char * s);
size_t strnlen(const char *s, size_t maxlen);
char * strncat(char *dest, const char *src, size_t count);
char * strncpy(char * dest,const char *src, size_t count);
int    strnicmp(const char *cs, const char *ct, int cnt);
int    strncmp(const char * cs,const char * ct,size_t count);
size_t strnlen(const char * s, size_t count);
char * strpbrk(const char * cs,const char * ct);
char * strrchr(const char * s, int c);
char * strsep(char **stringp, const char *delim);
size_t strspn(const char *s, const char *accept);
char * strstr(const char * s1,const char * s2);
char * strtok(char * s,const char * ct);
size_t strxfrm(char *s1, const char *s2, size_t n);
long   strtol(const char * nptr, char ** endptr, int base);
char * _strupr(char * string);

void memcpy4(uint32 *p1, const uint32 *p2, uint32 count);
void memset4(uint32 *p1, uint32 val, uint32 count);
void memcpy2(uint16 *p1, const uint16 *p2, uint32 count);
void memset2(uint16 *p1, uint32 val, uint32 count);
int strncasecmp(const char *s1, const char *s2, int n);
int strcasecmp(const char *s1, const char *s2);

int check_digit (char c);
int hex_to_int(char c);

__END_DECLS

#endif	/* __STRING_H */


