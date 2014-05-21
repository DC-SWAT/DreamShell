
STARTUP(crt0.o)

SECTIONS
{
	.text 0x00000000 : {
		*(.text)
	}

	__rodata_start = ALIGN(0x4);
	.rodata : {
		*(.rodata)
	}

	__data_start = ALIGN(0x4);
	.data : {
		*(.data)
	}

	__bss_start = ALIGN(0x4);
	.bss : {
		*(.bss)
		*(COMMON)
	}

	__bss_end__ = ALIGN(0x4);
	__bss_end = __bss_end__;
	_end = __bss_end__;
	end = _end;

	__stack = 0x1FF000;
	__brk = __stack;
	_brk = __brk;
	brk = __brk;

	__fiq_stack = 0x1FFFFC;
	__io_init = __fiq_stack;
}
