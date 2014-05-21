/** 
 * \file    aica.h
 * \brief   Definitions for AICA sound system
 * \date    2013-2014
 * \author  SWAT www.dc-swat.ru
 */


#ifndef _DS_AICA_H
#define _DS_AICA_H

/* Initialize AICA with custom firmware */
int snd_init_firmware(const char *filename);

/* Initialize AICA with KOS firmware */
int snd_init();

/* Shutdown AICA */
int snd_shutdown();

#endif /* _DS_AICA_H */
