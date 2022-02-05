/* 
 * DreamShell ISO Loader
 * dcload support
 * (c)2009-2022 SWAT <http://www.dc-swat.ru>
 */

#include <main.h>
#include <arch/irq.h>
#include "dcload.h"

#define dclsc(...) ({                                  \
		int rv, old;                                   \
		old = irq_disable();                           \
		do {} while ((*(vuint32 *)0xa05f688c) & 0x20); \
		rv = dcloadsyscall(__VA_ARGS__);               \
		irq_restore(old);                              \
		rv;                                            \
})

/* Printk replacement */
int dcload_write_buffer(const uint8 *data, int len) {
	return dclsc(DCLOAD_WRITE, 1, data, len);
}

int dcload_reinit() {
	return dclsc(DCLOAD_REINIT, 0, 0, 0);
}

#ifdef HAVE_GDB
size_t dcload_gdbpacket(const char* in_buf, size_t in_size, char* out_buf, size_t out_size) {
	/* we have to pack the sizes together because the dcloadsyscall handler
		can only take 4 parameters */
	return dclsc(DCLOAD_GDBPACKET, in_buf, (in_size << 16) | (out_size & 0xffff), out_buf);
}
#endif

int dcload_type = DCLOAD_TYPE_NONE;

int dcload_init() {

	if (dcload_type != DCLOAD_TYPE_NONE) {
		return 0;
	}

	if(*DCLOADMAGICADDR != DCLOADMAGICVALUE) {
		return -1;
	}

	dcload_reinit();

	/* Give dcload the 64k it needs to compress data (if on serial) */
	if(dclsc(DCLOAD_ASSIGNWRKMEM, (uint32 *)0x8cfb0000) == -1) {
		dcload_type = DCLOAD_TYPE_IP;
		printf("dc-load-ip initialized\n");
	} else {
		dcload_type = DCLOAD_TYPE_SER;
		printf("dc-load-serial initialized\n");
	}

	return 0;
}
