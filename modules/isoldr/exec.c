/* KallistiOS ##version##

   exec.c
   Copyright (C) 2002 Dan Potter
   Copyright (C) 2013-2014 SWAT
*/

#include <kos.h>
#include <arch/exec.h>
#include <kos/dbgio.h>
#include <arch/rtc.h>
#include <dc/spu.h>
#include <dc/sound/sound.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

void _atexit_call_all();

#if __GNUC__ >= 4
void init(void);
void fini(void);
#endif

/* Pull these in from execasm.s */
extern uint32 _isoldr_exec_template[];
extern uint32 _isoldr_exec_template_values[];
extern uint32 _isoldr_exec_template_end[];

/* Pull this in from startup.s */
extern uint32 _arch_old_sr, _arch_old_vbr, _arch_old_stack, _arch_old_fpscr;

void isoldr_arch_auto_shutdown() {
	fs_dclsocket_shutdown();
	net_shutdown();

	irq_disable();
	snd_shutdown();
	timer_shutdown();

	/* hardware_shutdown() */
	la_shutdown();
	bba_shutdown();
	maple_shutdown();
	cdrom_shutdown();
	spu_dma_shutdown();
	spu_shutdown();
	//vid_shutdown();
	pvr_shutdown();
	library_shutdown();
	fs_dcload_shutdown();
	fs_vmu_shutdown();
	vmufs_shutdown();
	fs_iso9660_shutdown();
	fs_ramdisk_shutdown();
	fs_romdisk_shutdown();
	fs_pty_shutdown();
	fs_shutdown();
	thd_shutdown();
	rtc_shutdown();
}

static void isoldr_arch_shutdown() {
    /* Run dtors */
    _atexit_call_all();

#if __GNUC__ < 4
    arch_dtors();
#else
# if __GNUC__ < 8
    fini();
# endif
#endif

    dbglog(DBG_CRITICAL, "Shutting down DreamShell kernel\n");

    /* Turn off UBC breakpoints, if any */
    ubc_disable_all();

    /* Do auto-shutdown */
    isoldr_arch_auto_shutdown();

    /* Shut down IRQs */
    irq_shutdown();
}


/* Replace the currently running image with whatever is at
   the pointer; note that this call will never return. */
void isoldr_exec_at(const void *image, uint32 length, uint32 address, uint32 params_len) {
    /* Find the start/end of the trampoline template and make a stack
       buffer of that size */
    uint32  tstart = (uint32)_isoldr_exec_template,
            tend = (uint32)_isoldr_exec_template_end;
    uint32  tcount = (tend - tstart) / 4;
    uint32  buffer[tcount];
    uint32  * values;
    uint32 i;

    assert((tend - tstart) % 4 == 0);

    /* Turn off interrupts */
    irq_disable();

    /* Flush the data cache for the source area */
    dcache_flush_range((uint32)image, length);

    /* Copy over the trampoline */
    for(i = 0; i < tcount; i++)
        buffer[i] = _isoldr_exec_template[i];

    /* Plug in values */
    values = buffer + (_isoldr_exec_template_values - _isoldr_exec_template);
    values[0] = (uint32)image;      /* Source */
    values[1] = address;            /* Destination */
    values[2] = length / 4;         /* Length in uint32's */
    values[3] = _arch_old_stack;    /* Patch in old R15 */
    values[4] = params_len;         /* Params size */

    /* Flush both caches for the trampoline area */
    dcache_flush_range((uint32)buffer, tcount * 4);
    icache_flush_range((uint32)buffer, tcount * 4);

    /* Shut us down */
    isoldr_arch_shutdown();

    /* Reset our old SR, VBR, and FPSCR */
    __asm__ __volatile__("ldc	%0,sr\n"
                         : /* no outputs */
                         : "z"(_arch_old_sr)
                         : "memory");
    __asm__ __volatile__("ldc	%0,vbr\n"
                         : /* no outputs */
                         : "z"(_arch_old_vbr)
                         : "memory");
    __asm__ __volatile__("lds	%0,fpscr\n"
                         : /* no outputs */
                         : "z"(_arch_old_fpscr)
                         : "memory");

    /* Jump to the trampoline */
    {
        typedef void (*trampoline_func)() __noreturn;
        trampoline_func trampoline = (trampoline_func)buffer;

        trampoline();
    }
}

