#ifndef serial_h_
#define serial_h_

void uart0_init (void);

int uart0_putc(int ch); 
int uart0_putchar (int ch); /* replaces \n with \r\n */

int uart0_puts   ( char *s ); /* uses putc */
int uart0_prints ( char* s ); /* uses putchar */

int uart0_kbhit( void );
int uart0_getc ( void );

#endif
