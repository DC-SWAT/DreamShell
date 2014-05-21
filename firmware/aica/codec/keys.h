#ifndef _KEYS_H_
#define _KEYS_H_

void process_keys(void);
long get_key_press( long key_mask );
long get_key_rpt( long key_mask );
long get_key_short( long key_mask );
long get_key_long( long key_mask );
void key_init( void );

#define KEY0		19
#define KEY1		20

#endif /* _KEYS_H_ */
