/*
  Copyright (C) 2006 Andreas Schwarz <andreas@andreas-s.net>

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.
*/

#ifndef _DAC_H_
#define _DAC_H_

#define AIC_REG_LINVOL		0x00
#define AIC_REG_RINVOL		0x01
#define AIC_REG_LOUTVOL		0x02
#define AIC_REG_ROUTVOL		0x03
#define AIC_REG_AN_PATH		0x04
#define AIC_REG_DIG_PATH		0x05
#define AIC_REG_POWER		0x06
#define AIC_REG_DIG_FORMAT	0x07
#define AIC_REG_SRATE		0x08
#define AIC_REG_DIG_ACT		0x09
#define AIC_REG_RESET		0x0F

#define AIC_USB (1 << 0)
#define AIC_BOSR (1 << 1)
#define AIC_SR0 (1 << 2)
#define AIC_SR1 (1 << 3)
#define AIC_SR2 (1 << 4)
#define AIC_SR3 (1 << 5)

#define AIC_MICB	(1 << 0)
#define AIC_MICM	(1 << 1)
#define AIC_INSEL	(1 << 2)
#define AIC_BYP		(1 << 3)
#define AIC_DAC		(1 << 4)
#define AIC_STE		(1 << 5)
#define AIC_STA0	(1 << 6)
#define AIC_STA1	(1 << 7)
#define AIC_STA2	(1 << 8)

#define MAX_BUFFERS 3
#define DAC_BUFFER_MAX_SIZE 2400
extern short dac_buffer[MAX_BUFFERS][DAC_BUFFER_MAX_SIZE];
extern int dac_buffer_size[MAX_BUFFERS];
extern unsigned long current_srate;
extern unsigned int underruns;

void dac_reset();
int dac_get_writeable_buffer();
int dac_get_readable_buffer();
int dac_readable_buffers();
int dac_writeable_buffers();
int dac_busy_buffers();
int adc_busy_buffers();
int dac_fill_dma();

void dac_enable_dma();
void dac_disable_dma();
int dac_next_dma_empty();
int dac_first_dma_empty();
int adc_next_dma_empty();
int adc_first_dma_empty();
void dac_set_first_dma(short *buffer, int n);
void dac_set_next_dma(short *buffer, int n);
int dma_endtx(void);
void dac_write_reg(unsigned char reg, unsigned short value);
int dac_set_srate(unsigned long srate);
void dac_init(void);

#endif /* _DAC_H_ */
