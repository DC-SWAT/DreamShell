/*  printf.c

    $Id: printf.c,v 1.3 2002/06/29 12:57:04 quad Exp $

DESCRIPTION

    An implementation of printf.

CREDIT

    I stole it from somewhere. I can't remember where though. Sorry!

TODO

    Cleanup the printf implementation. See if it's possible to compress its
    size - remove textualization modes we don't need.

*/

#include <arch/types.h>
#include <string.h>
#include <stdio.h>

#define do_div(n, base) ({ int32 __res; __res = ((unsigned long) n) % (unsigned) base; n = ((unsigned long) n) / (unsigned) base; __res; })

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
/*
int snprintf (char *buf, int size, const char *fmt, ...)
{
    va_list args;
    int     i;

    va_start (args, fmt);
    i = vsnprintf (buf,size,fmt,args);
    va_end (args);

    return i;
}
*/