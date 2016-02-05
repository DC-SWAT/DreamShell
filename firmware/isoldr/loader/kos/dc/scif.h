/* KallistiOS ##version##

   dc/scif.h
   Copyright (C)2000,2001,2004 Dan Potter

*/

#ifndef __DC_SCIF_H
#define __DC_SCIF_H

#include <sys/cdefs.h>
__BEGIN_DECLS

#include <arch/types.h>

/* Set serial parameters; this is not platform independent like I want
   it to be, but it should be generic enough to be useful. */
void scif_set_parameters(int baud, int fifo);

// The rest of these are the standard dbgio interface.
int scif_set_irq_usage(int on);
int scif_detected();
int scif_init();
int scif_shutdown();
int scif_read();
int scif_write(int c);
int scif_flush();
int scif_write_buffer(const uint8 *data, int len, int xlat);
int scif_read_buffer(uint8 *data, int len);

__END_DECLS

#endif  /* __DC_SCIF_H */

