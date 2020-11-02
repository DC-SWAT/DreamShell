#include "patch.h"

void edc_bcopy(unsigned char *src, unsigned char *dest, int len)
{
  if (dest < src)
    while (len--)
      *dest++ = *src++;
  else
    {
      unsigned char *lasts = src + (len-1);
      unsigned char *lastd = dest + (len-1);
      while (len--)
        *(unsigned char *)lastd-- = *(unsigned char *)lasts--;
    }
}

