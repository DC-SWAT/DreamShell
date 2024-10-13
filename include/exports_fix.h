/** 
 * \file    exports_fix.h
 * \brief   DreamShell exports fix
 * \date    2023, 2024
 * \author  SWAT www.dc-swat.ru
 */

#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <kos.h>
#include <kos/net.h>

extern uint32 _arch_old_sr, _arch_old_vbr, _arch_old_stack, _arch_old_fpscr, start;
extern volatile uint32 jiffies;
extern void _fini(void);

int asprintf(char **restrict strp, const char *restrict fmt, ...);
int vasprintf(char **restrict strp, const char *restrict fmt, va_list ap);

char *index(const char *string, int c);
char *rindex(const char *string, int c);

void bcopy(const void *s1, void *s2, size_t n);
void bzero(void *s, size_t n);

int usleep(useconds_t usec);
int putenv(char *string);
char *mktemp(char *template);
int getw(FILE *fp);
int putw(int c, FILE *fp);

/* Need for libppp */
#define packed __attribute__((packed))
typedef struct {
    uint8   dest[6];
    uint8   src[6];
    uint8   type[2];
} packed eth_hdr_t;

int net_ipv4_input(netif_t *src, const uint8 *pkt, size_t pktsize,
                   const eth_hdr_t *eth);

/* GCC exports */
extern uint32 __fixdfdi;
extern uint32 __fixunsdfdi;
extern uint32 __floatundidf;
extern uint32 __unorddf2;
extern uint32 __floatdidf;
extern uint32 __divdf3;
extern uint32 __ledf2;
extern uint32 __moddi3;
extern uint32 __udivsi3_i4i;
extern uint32 __sdivsi3_i4i;
extern uint32 __udivdi3;
extern uint32 __divdi3;
extern uint32 __fixsfdi;
extern uint32 __floatdisf;
extern uint32 __fixunssfdi;
extern uint32 __floatundisf;
extern uint32 __movmem_i4_even;
extern uint32 __movmem_i4_odd;
extern uint32 __movmemSI12_i4;
extern uint32 __ctzsi2;
extern uint32 __clzsi2;
extern uint32 __umoddi3;
extern uint32 __extendsfdf2;
extern uint32 __muldf3;
extern uint32 __floatunsidf;
extern uint32 __truncdfsf2;
extern uint32 __gtdf2;
extern uint32 __fixdfsi;
extern uint32 __floatsidf;
extern uint32 __subdf3;
extern uint32 __ltdf2;
extern uint32 __adddf3;
extern uint32 __gedf2;
extern uint32 __eqdf2;
extern uint32 __ashldi3;
extern uint32 __unordsf2;
extern uint32 __lshrdi3;
extern uint32 __ashrdi3;
extern uint32 _Unwind_Resume;
extern uint32 _Unwind_DeleteException;
extern uint32 _Unwind_GetRegionStart;
extern uint32 _Unwind_GetLanguageSpecificData;
extern uint32 _Unwind_GetIPInfo;
extern uint32 _Unwind_SetGR;
extern uint32 _Unwind_SetIP;
extern uint32 _Unwind_RaiseException;
extern uint32 _Unwind_GetTextRelBase;
extern uint32 _Unwind_Resume_or_Rethrow;
