
/**
 * This file is part of DreamShell ISO Loader
 * Copyright (C)2011-2022 SWAT
 * Copyright (C)2019 megavolt85
 * 
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#ifdef LOG
#include "arch/types.h"
#include "string.h"
#include "stdio.h"
#include "stdarg.h"

/* SCIF registers */
#define SCIFREG08(x) *((volatile uint8 *)(x))
#define SCIFREG16(x) *((volatile uint16 *)(x))
#define SCSMR2	SCIFREG16(0xffeb0000)
#define SCBRR2	SCIFREG08(0xffe80004)
#define SCSCR2	SCIFREG16(0xffe80008)
#define SCFTDR2	SCIFREG08(0xffe8000C)
#define SCFSR2	SCIFREG16(0xffe80010)
#define SCFRDR2	SCIFREG08(0xffe80014)
#define SCFCR2	SCIFREG16(0xffe80018)
#define SCFDR2	SCIFREG16(0xffe8001C)
#define SCSPTR2	SCIFREG16(0xffe80020)
#define SCLSR2	SCIFREG16(0xffe80024)

#define BAUDRATE	500000
/* Initialize the SCIF port; baud_rate must be at least 9600 and
   no more than 57600. 115200 does NOT work for most PCs. */
// recv trigger to 1 byte
int scif_init() 
{
	int i;
	/* int fifo = 1; */

	/* Disable interrupts, transmit/receive, and use internal clock */
	SCSCR2 = 0;

	/* Enter reset mode */
	SCFCR2 = 0x06;
	
	/* 8N1, use P0 clock */
	SCSMR2 = 0;
	
	/* If baudrate unset, set baudrate, N = P0/(32*B)-1 */
	SCBRR2 = (uint8)(50000000 / (32 * BAUDRATE)) - 1;

	/* Wait a bit for it to stabilize */
	for (i=0; i<10000; i++)
		__asm__("nop");

	/* Unreset, enable hardware flow control, triggers on 8 bytes */
	SCFCR2 = 0x48;
	
	/* Disable manual pin control */
	SCSPTR2 = 0;
	
	/* Disable SD */
//	SCSPTR2 = PTR2_RTSIO | PTR2_RTSDT;
	
	/* Clear status */
	(void)SCFSR2;
	SCFSR2 = 0x60;
	(void)SCLSR2;
	SCLSR2 = 0;
	
	/* Enable transmit/receive */
	SCSCR2 = 0x30;

	/* Wait a bit for it to stabilize */
	for (i=0; i<10000; i++)
		__asm__("nop");

	return 0;
}

/* Read one char from the serial port (-1 if nothing to read) */
int scif_read() 
{
	int c;

	if (!(SCFDR2 & 0x1f))
		return -1;

	// Get the input char
	c = SCFRDR2;

	// Ack
	SCFSR2 &= ~0x92;

	return c;
}

/* Write one char to the serial port (call serial_flush()!) */
int scif_write(int c) 
{
	int timeout = 100000;

	/* Wait until the transmit buffer has space. Too long of a failure
	   is indicative of no serial cable. */
	while (!(SCFSR2 & 0x20) && timeout > 0)
		timeout--;
	
	if (timeout <= 0)
		return -1;
	
	/* Send the char */
	SCFTDR2 = c;
	
	/* Clear status */
	SCFSR2 &= 0xff9f;

	return 1;
}

/* Flush all FIFO'd bytes out of the serial port buffer */
int scif_flush() 
{
	int timeout = 100000;

	SCFSR2 &= 0xbf;

	while (!(SCFSR2 & 0x40) && timeout > 0)
		timeout--;
	if (timeout <= 0)
		return -1;

	SCFSR2 &= 0xbf;

	return 0;
}

/* Send an entire buffer */
int scif_write_buffer(const uint8 *data, int len, int xlat) 
{
	int rv, i = 0, c;
	
	while (len-- > 0) 
	{
		c = *data++;
		
		if (xlat) 
		{
			if (c == '\n') 
			{
				if (scif_write('\r') < 0)
					return -1;
				i++;
			}
		}
		rv = scif_write(c);
		
		if (rv < 0)
			return -1;
		
		i += rv;
	}
	
	if (scif_flush() < 0)
		return -1;

	return i;
}






#define do_div(n, base) ({ int32 __res; __res = ((unsigned long) n) % (unsigned) base; n = ((unsigned long) n) / (unsigned) base; __res; })

int check_digit (char c) 
{
	if((c>='0') && (c<='9')) 
	{
		return 1;
	}
	
	return 0;
}

static int32 skip_atoi (const char **s)
{
    int32   i;

    i = 0;

    while (check_digit (**s))
        i = i*10 + *((*s)++) - '0';

    return i;
}

char* printf_number (char *str, long num, int32 base, int32 size, int32 precision, int32 type)
{
    int32       i;
    char        c;
    char        sign;
    char        tmp[66];
    const char *digits = "0123456789abcdefghijklmnopqrstuvwxyz";

    if (type & N_LARGE)
        digits = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZ";

    if (type & N_LEFT)
        type &= ~N_ZEROPAD;

    if (base < 2 || base > 36)
        return 0;

    c = (type & N_ZEROPAD) ? '0' : ' ';

    sign = 0;

    if (type & N_SIGN)
    {
        if (num < 0)
        {
            sign = '-';
            num = -num;
            size--;
        }
        else if (type & N_PLUS)
        {
            sign = '+';
            size--;
        }
        else if (type & N_SPACE)
        {
            sign = ' ';
            size--;
        }
    }

    if (type & N_SPECIAL)
    {
        if (base == 16)
            size -= 2;
        else if (base == 8)
            size--;
    }

    i = 0;

    if (num == 0)
        tmp[i++] = '0';
    else while (num != 0)
        tmp[i++] = digits[do_div (num,base)];

    if (i > precision)
        precision = i;
    size -= precision;

    if (!(type & (N_ZEROPAD + N_LEFT)))
    {
        while (size-- > 0)
            *str++ = ' ';
    }

    if (sign)
        *str++ = sign;

    if (type & N_SPECIAL)
    {
        if (base == 8)
        {
            *str++ = '0';
        }
        else if (base == 16)
        {
            *str++ = '0';
            *str++ = digits[33];
        }
    }

    if (!(type & N_LEFT))
    {
        while (size-- > 0)
            *str++ = c;
    }

    while (i < precision--)
        *str++ = '0';

    while (i-- > 0)
        *str++ = tmp[i];

    while (size-- > 0)
        *str++ = ' ';

    return str;
}

int vsnprintf (char *buf, int size, const char *fmt, va_list args)
{
    int           len;
    unsigned long   num;
    int           i;
    int           base;
    char           *str;
    const char     *s;
    int           flags;          /* NOTE: Flags to printf_number (). */

    int           field_width;    /* NOTE: Width of output field. */
    int           precision;      /* NOTE: min. # of digits for integers; max number of chars for from string. */
    int             qualifier;      /* NOTE: 'h', 'l', or 'L' for integer fields. */

    for (str = buf; *fmt && ((str - buf) < (size - 1)); ++fmt)
    {
        /* STAGE: If it's not specifying an option, just pass it on through. */

        if (*fmt != '%')
        {
            *str++ = *fmt;
            continue;
        }
            
        /* STAGE: Process the flags. */

        flags = 0;

        repeat:
            ++fmt;                  /* NOTE: This also skips first '%' */
            switch (*fmt)
            {
                case '-' :  flags |= N_LEFT; goto repeat;
                case '+' :  flags |= N_PLUS; goto repeat;
                case ' ' :  flags |= N_SPACE; goto repeat;
                case '#' :  flags |= N_SPECIAL; goto repeat;
                case '0' :  flags |= N_ZEROPAD; goto repeat;
            }
        
        /* STAGE: Get field width. */

        field_width = -1;

        if (check_digit (*fmt))
        {
            field_width = skip_atoi (&fmt);
        }
        else if (*fmt == '*')
        {
            ++fmt;

            /* STAGE: Specified on the next argument. */

            field_width = va_arg (args, int);

            if (field_width < 0)
            {
                field_width = -field_width;
                flags |= N_LEFT;
            }
        }

        /* STAGE: Get the precision. */

        precision = -1;

        if (*fmt == '.')
        {
            ++fmt;    

            if (check_digit (*fmt))
            {
                precision = skip_atoi (&fmt);
            }
            else if (*fmt == '*')
            {
                ++fmt;

                /* STAGE: Specified on the next argument */

                precision = va_arg (args, int);
            }

            /* STAGE: Paranoia on the precision value. */

            if (precision < 0)
                precision = 0;
        }

        /* STAGE: Get the conversion qualifier. */

        qualifier = -1;

        if (*fmt == 'h' || *fmt == 'l' || *fmt == 'L')
        {
            qualifier = *fmt;

            ++fmt;
        }

        /* NOTE: The default base. */

        base = 10;


        /* STAGE: Handle all the other formatting types. */

        switch (*fmt)
        {
            /* STAGE: Character type. */
            case 'c' :
            {
                if (!(flags & N_LEFT))
                {
                    while (--field_width > 0)
                        *str++ = ' ';
                }

                *str++ = (unsigned char) va_arg (args, int);

                while (--field_width > 0)
                    *str++ = ' ';

                continue;
            }

            /* STAGE: String type. */
            case 's' :
            {
                s = va_arg (args, char *);

                if (!s)
                    s = "<NULL>";

                len = strnlen (s, precision);

                if (!(flags & N_LEFT))
                {
                    while (len < field_width--)
                        *str++ = ' ';
                }

                for (i = 0; i < len; ++i)
                    *str++ = *s++;

                while (len < field_width--)
                    *str++ = ' ';

                continue;
            }

            case 'p' :
            {
                if (field_width == -1)
                {
                    field_width = 2 * sizeof (void *);
                    flags |= N_ZEROPAD;
                }

                str = printf_number (str, (unsigned long) va_arg (args, void *), 16, field_width, precision, flags);

                continue;
            }

            case 'n' :
            {
                if (qualifier == 'l')
                {
                    long   *ip;
                    
                    ip = va_arg (args, long *);

                    *ip = (str - buf);
                }
                else
                {
                    int    *ip;
                    
                    ip = va_arg (args, int *);

                    *ip = (str - buf);
                }

                continue;
            }

            /* NOTE: Integer number formats - set up the flags and "break". */

            /* STAGE: Octal. */
            case 'o' :
                base = 8;
                break;

            /* STAGE: Uppercase hexidecimal. */
            case 'X' :
                flags |= N_LARGE;
                base = 16;
                break;
            /* STAGE: Lowercase hexidecimal. */
            case 'x' :
                base = 16;
                break;

            /* STAGE: Signed decimal/integer. */
            case 'd' :
            case 'i' :
                flags |= N_SIGN;
            /* STAGE: Unsigned decimal/integer. */
            case 'u' :
                break;

            default :
            {
                if (*fmt != '%')
                    *str++ = '%';

                if (*fmt)
                    *str++ = *fmt;
                else
                    --fmt;

                continue;
            }
        }

        /* STAGE: Handle number formats with the various modifying qualifiers. */

        if (qualifier == 'l')
        {
            num = va_arg (args, unsigned long);
        }
        else if (qualifier == 'h')
        {
            /*
                NOTE: The int16 should work, but the compiler promotes the
                datatype and complains if I don't handle it directly.
            */

            if (flags & N_SIGN)
            {
                // num = va_arg (args, int16);
                num = va_arg (args, int32);
            }
            else
            {
                // num = va_arg (args, uint16);
                num = va_arg (args, uint32);
            }
        }
        else if (flags & N_SIGN)
        {
            num = va_arg (args, int);
        }
        else
        {
            num = va_arg (args, unsigned int);
        }

        /* STAGE: And after all that work, actually convert the integer into text. */

        str = printf_number (str, num, base, field_width, precision, flags);
    }

    *str = '\0';

    return str - buf;
}


size_t strnlen(const char *s, size_t maxlen) 
{
	const char *e;
	size_t n;

	for (e = s, n = 0; *e && n < maxlen; e++, n++)
		;
	return n;
}

static int PutLog(char *buff) 
{
	int len = strnlen(buff, 128);

	scif_write_buffer((uint8 *)buff, len, 1);

	return len;
}

int WriteLog(const char *fmt, ...) 
{
	char buff[128];
	va_list args;
	int i;

	va_start(args, fmt);
	i = vsnprintf(buff, sizeof(buff), fmt, args);
	va_end(args);

	PutLog(buff);
	return i;
}

int WriteLogFunc(const char *func, const char *fmt, ...) 
{
	PutLog((char *)func);
	
	if(fmt == NULL) 
	{
		PutLog("\n");
		return 0;
	}
	
	PutLog(": ");
	
	char buff[128];
	va_list args;
	int i;

	va_start(args, fmt);
	i = vsnprintf(buff, sizeof(buff), fmt, args);
	va_end(args);

	PutLog(buff);
	return i;
}

#endif /* LOG */

