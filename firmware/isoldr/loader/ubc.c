/**
 * DreamShell ISO Loader
 * SH4 UBC
 * (c)2013-2020 SWAT <http://www.dc-swat.ru>
 * Based on Netplay VOOT code by Scott Robinson <scott_vo@quadhome.com>
 */

#include <main.h>
#include <ubc.h>
#include <exception.h>

extern void ubc_wait(void);

static void* ubc_handler(register_stack *stack, void *current_vector) {
	
	if(ubc_is_channel_break(UBC_CHANNEL_A)) {
		
		ubc_clear_channel(UBC_CHANNEL_A);
		
//		LOGF("UBC_A: Enabling widescreen\n");

		/* Enable horizontal scaling */
//		*((vuint32*)0xa05f80f4) = 0x10000;
		
		/* Enable pixel double */
//		*((vuint32*)0xa05f80e8) |= 0x100;
		
//		UBC_R_BBRA = UBC_BBR_OPERAND | UBC_BBR_WRITE;
//		ubc_wait();

		ubc_clear_break(UBC_CHANNEL_B);
		LOGF("UBC: A\n");
		dump_regs(stack);
	}

	if(ubc_is_channel_break(UBC_CHANNEL_B)) {
//		(void)stack;
		ubc_clear_break(UBC_CHANNEL_B);
		LOGF("UBC: B\n");
		dump_regs(stack);
//		
//		ubc_clear_channel(UBC_CHANNEL_A);
//
//		/* TODO force VGA */
//		vuint32 *regs = *(vuint32*)0xa05f8000;
//		
//		/* VBlank IRQ */
//		uint32 val = regs[0x33];
//		uint16 sint2 = val & 0x0000ffff;
//		regs[0x33] = (val & 0xffff0000) | (sint2 << 1);
//		
//#ifdef LOG
//		WriteLog("UBC_B: %08lx\n", *(vuint32*)0xa0702c00);
//#endif
	}

	return current_vector;
}


void ubc_init() {
	
	UBC_R_BBRA  = UBC_R_BBRB  = 0;
	UBC_R_BAMRA = UBC_R_BAMRB = UBC_BAMR_NOASID;
	UBC_R_BRCR  = UBC_BRCR_UBDE | UBC_BRCR_PCBA | UBC_BRCR_PCBB;

	/* set DBR here */
	dbr_set(ubc_handler_lowlevel);
	
	exception_table_entry entry;
	entry.type    = EXP_TYPE_GEN;
	entry.code    = EXP_CODE_UBC;
	entry.handler = ubc_handler;

	exception_add_handler(&entry, NULL);
}


int ubc_configure_channel(ubc_channel channel, uint32 breakpoint, uint16 options) {

    switch (channel) {
        case UBC_CHANNEL_A:
            UBC_R_BARA = breakpoint;
            UBC_R_BBRA = options;
            break;
        case UBC_CHANNEL_B:
            UBC_R_BARB = breakpoint;
            UBC_R_BBRB = options;
            break;
        default :
            return 0;
    }

    ubc_wait();
    return 1;
}

void ubc_clear_channel(ubc_channel channel) {

    switch (channel) {
        case UBC_CHANNEL_A:
            UBC_R_BBRA = 0;
            ubc_clear_break(channel);
            break;
        case UBC_CHANNEL_B:
            UBC_R_BBRB = 0;
            ubc_clear_break(channel);
            break;
        default:
            return;
    }

    ubc_wait();
}

void ubc_clear_break(ubc_channel channel) {

    switch (channel) {
        case UBC_CHANNEL_A:
            UBC_R_BRCR &= ~(UBC_BRCR_CMFA);
            break;
        case UBC_CHANNEL_B:
            UBC_R_BRCR &= ~(UBC_BRCR_CMFB);
            break;
        default:
            break;
    }
}

int ubc_is_channel_break(ubc_channel channel) {

    switch (channel) {
        case UBC_CHANNEL_A :
            return !!(UBC_R_BRCR & UBC_BRCR_CMFA);
            break;
        case UBC_CHANNEL_B:
            return !!(UBC_R_BRCR & UBC_BRCR_CMFB);
            break;
        default:
            return 0;
    }
}

