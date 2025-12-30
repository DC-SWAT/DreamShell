/* DreamShell ##version##

   module.c - PolarSSL module
   Copyright (C) 2025 SWAT 
*/

#include <ds.h>
#include <polarssl/version.h>

DEFAULT_MODULE_EXPORTS(polarssl);

/* Stubs required by polarssl/timing.c */
unsigned int alarm(unsigned int seconds) {
    (void)seconds;
    return 0;
}
