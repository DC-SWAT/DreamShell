#ifndef systime_h_
#define systime_h_

#define TCK  10000                           /* Timer Clock  */
#define PIV  ((MCK/TCK/16)-1)               /* Periodic Interval Value */

extern volatile unsigned long systime_value;

extern void systime_init(void);
extern unsigned long systime_get(void);
extern unsigned long systime_get_ms(void);

#endif


