/** 
 *  \file       vmu.h
 *	\brief      DreamShell VMU utils
 *	\date       2013
 *	\author     SWAT
 *	\copyright  http://www.dc-swat.ru
 */

#ifndef _DS_VMU_H_
#define _DS_VMU_H_

/**
 * Draw string on VMU LCD
 */
void vmu_draw_string(const char *str);
void vmu_draw_string_xy(const char *str, int x, int y);

#endif /* _DS_VMU_H_ */
