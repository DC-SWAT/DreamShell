/**
 * This file is part of DreamShell ISO Loader
 * Copyright (C)2019 megavolt85
 * 
 * This file is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License version 2 as
 * published by the Free Software Foundation.
 */

#include "syscalls.h"

void gd_release()
{
#ifdef LOG	
	LOGFF(0);
#endif
}

void gd_pause(uint32_t *params, GDS *my_gds)
{
#ifdef LOG	
	LOGFF(0);
#endif
	(void) params;
	my_gds->drv_stat = STAT_PAUSE;
}

void gd_play2()
{
#ifdef LOG	
	LOGFF(0);
#endif
}

void gd_play(uint32_t *params, GDS *my_gds)
{
#ifdef LOG	
	LOGFF(0);
#endif
	
	(void) params;
	my_gds->drv_stat = STAT_PLAY;
}

void gd_seek()
{
#ifdef LOG	
	LOGFF(0);
#endif
}

void gd_stop(uint32_t *params, GDS *my_gds)
{
#ifdef LOG	
	LOGFF(0);
#endif
	(void) params;
	my_gds->drv_stat = STAT_PAUSE;
}
/*
int gdGdcChangeDataType(uint32_t *param)
{
#ifdef LOG	
	LOGFF(0);
#endif
	
	if (allocate_GD() != 0)
	{
		return 4;
	}

	GDS *my_gds = get_GDS();
	
	switch (param[0]) 
	{
		case 0:
		{
			if (param[3] == 2048)
			{
				release_GD();
				return 0;
			}
			break;
		}
		
		case 1:
		{
			param[1] = 0x2000;
			param[2] = (my_gds->drv_media == 0x20) ? 0x800 : 0x400;
			param[3] = 0x800;
			
			release_GD();
			return 0;
			break;
		}
	}
	
	release_GD();
	return -1;
}
*/

static int res_count = 0;

void gdGdcChangeCD(int disc_num)
{
#ifdef LOG	
	LOGFF("%d\n", disc_num);
#endif
	res_count = 1;
	disc_id[0] = disc_num;
}

int gdGdcGetCmdStat(int gd_chn, int *status)
{
#ifdef LOG	
	LOGFF(0);
#endif
	GDS *my_gds;

	if (allocate_GD() != 0) 
	{
#if 0//def LOG
		LOGF("CMD_STAT_WAITING\n");
#endif
		return CMD_STAT_WAITING; /* CMD_STAT_WAITING */
	}

	my_gds = get_GDS();

	status[0] = 0;
	status[1] = 0;
	status[2] = 0;
	status[3] = 0;

	if (gd_chn == 0) 
	{
#if 0//def LOG
		LOGF("gd_chn=0\n");
#endif
		if (my_gds->gd_cmd_stat == CMD_STAT_NO_ACTIVE) 
		{
			release_GD();
			return CMD_STAT_NO_ACTIVE; /* CMD_STAT_NO_ACTIVE */
		}
		release_GD();
		return CMD_STAT_PROCESSING; /* CMD_STAT_PROCESSING */
	}

	if (my_gds->gd_chn != gd_chn) 
	{
#if 0 //def LOG		
		LOGF("gd_chn error\n");
#endif
		status[0] = CMD_STAT_ERROR;
		release_GD();
		return CMD_STAT_FAILED; /* CMD_STAT_FAILED */
	}

	switch (my_gds->gd_cmd_stat) 
	{
		case CMD_STAT_NO_ACTIVE:
#if 0 //def LOG
			LOGF("CMD_STAT_NO_ACTIVE\n");
#endif
			release_GD();
			return CMD_STAT_NO_ACTIVE; /* CMD_STAT_NO_ACTIVE */
			break;

		case CMD_STAT_PROCESSING:
		case CMD_STAT_COMPLETED:
			status[2] = my_gds->transfered;
			status[3] = my_gds->ata_status;
			release_GD();
#if 0 //def LOG
			LOGF("CMD_STAT_PROCESSING\n");
#endif
			return CMD_STAT_PROCESSING; /* CMD_STAT_PROCESSING */
			break;

		case CMD_STAT_ABORTED:
			if (my_gds->gd_cmd_err != GDCMD_OK) 
			{
				status[0] = my_gds->gd_cmd_err;
				status[1] = my_gds->gd_cmd_err2;
				status[2] = my_gds->transfered;
				status[3] = my_gds->ata_status;
				release_GD();
				return CMD_STAT_FAILED; /* CMD_STAT_FAILED */
			}
#if 0 //def LOG
			LOGF("CMD_STAT_COMPLETED\n");
#endif
			status[2] = my_gds->transfered;
			status[3] = my_gds->ata_status;
			my_gds->ata_status = 0;
			my_gds->gd_cmd_stat = CMD_STAT_NO_ACTIVE;
			release_GD();
			return CMD_STAT_COMPLETED; /* CMD_STAT_COMPLETED */
			break;

		case CMD_STAT_WAITING:
			if (my_gds->gd_cmd_err != GDCMD_OK) 
			{
				status[0] = my_gds->gd_cmd_err;
				status[1] = my_gds->gd_cmd_err2;
				status[2] = my_gds->transfered;
				status[3] = my_gds->ata_status;
				release_GD();
				return CMD_STAT_FAILED; /* CMD_STAT_FAILED */
			}
			status[2] = my_gds->transfered;
			status[3] = my_gds->ata_status;
			release_GD();
#if 0 //def LOG
			LOGF("CMD_STAT_ABORTED\n");
#endif
			return CMD_STAT_ABORTED; /* CMD_STAT_ABORTED */
			break;

		default:
			break;
	}
#if 0 //def LOG
	LOGF("CMD_STAT_NO_ACTIVE\n");
#endif
	release_GD();
	return CMD_STAT_NO_ACTIVE; /* CMD_STAT_NO_ACTIVE */
}

int gdGdcGetDrvStat(uint32_t *status)
{
	GDS *my_gds = get_GDS();
#if 0 //def LOG	
	LOGFF("%d\n", my_gds->drv_stat);
#endif
	if (allocate_GD() != 0) 
	{
		return 4;
	}
	
	if (res_count)
	{
		res_count++;
		
		if (res_count == 2)
		{
			my_gds->drv_media = TYPE_CDDA;
			my_gds->drv_stat = STAT_OPEN;
		}
		else if (res_count > 30)
		{
			my_gds->drv_media = TYPE_GDROM;
			my_gds->drv_stat = STAT_PAUSE;
			res_count = 0;
			release_GD();
			return 2;
		}
		else
		{
			status[0] = my_gds->drv_stat;
			status[1] = my_gds->drv_media;
			release_GD();
			return 1;
		}
	}
	
	status[0] = my_gds->drv_stat;
	status[1] = my_gds->drv_media;
	release_GD();
	return 0;
	
	/*
#ifdef LOG	
	LOGFF(0);
#endif
	
	if (allocate_GD() != 0) 
	{
		return 4;
	}

	GDS *my_gds = get_GDS();
	
	if (my_gds->dma_in_progress == 0) 
	{
		if (!(IN8(G1_ATA_ALTSTATUS) & G1_ATA_SR_BSY)) 
		{
			my_gds->drv_stat = STAT_PAUSE;
			status[0] = STAT_PAUSE;
		}
		else
		{
			release_GD();
			return 1;
		}
	}
	else
	{
		my_gds->drv_stat = STAT_PLAY;
		status[0] = STAT_PLAY;
	}
	
	status[1] = my_gds->drv_media;
	release_GD();
	
	return 0;
	*/
}
/*
void gdGdcG1DmaEnd(uint32_t func, uint32_t param)
{
#ifdef LOG	
	LOGFF("%08X %d\n", func, param);
#endif
	OUT32(NORMAL_INT_STAT, 0x4000);
	
#if 0 //def LOG
	scif_init();
#endif	
	
	if (!func) 
	{
		return;
	}

	void (*callback)() = (void (*)())(func);
	callback(param);
}
*/
int gdGdcReqDmaTrans(int gd_chn, uint32_t *params)	/* _8c0032a2 */
{
#ifdef LOG	
	LOGFF("%08X %d\n", params[0], params[1]);
#endif
	GDS *my_gds;

	my_gds = get_GDS();

	if (gd_chn != my_gds->gd_chn) 
	{
		return -1;
	}

	if (my_gds->gd_cmd_stat != CMD_STAT_WAITING) 
	{
		return -1;
	}
	
	if (params[1] > my_gds->requested) 
	{
		return -1;
	}

	OUT32(G1_ATA_DMA_PRO, G1_ATA_DMA_PRO_SYSMEM);
	OUT32(G1_ATA_DMA_ADDRESS, params[0]);
	OUT32(G1_ATA_DMA_LENGTH, params[1]);
	OUT8(G1_ATA_DMA_DIRECTION, 1);

	my_gds->requested -= params[1];

	OUT8(G1_ATA_DMA_ENABLE, 1);
	OUT8(G1_ATA_DMA_STATUS, 1);

	return 0;
}

int gdGdcCheckDmaTrans(int gd_chn, uint32_t *size)
{
#ifdef LOG	
	LOGFF(0);
#endif
	GDS *my_gds;

	my_gds = get_GDS();

	if (gd_chn != my_gds->gd_chn) 
	{
		return -1;
	}

	if (my_gds->gd_cmd_stat != CMD_STAT_WAITING) 
	{
		return -1;
	}
	
	if(IN8(G1_ATA_DMA_STATUS)) 
	{
		*size = IN32(G1_ATA_DMA_LEND);
		return 1;
	}

	*size = my_gds->requested;
	return 0;
}

int gdGdcReadAbort(int gd_chn)
{
#ifdef LOG	
	LOGFF(0);
#endif
	GDS *my_gds;

	my_gds = get_GDS();

	if (gd_chn != my_gds->gd_chn) 
	{
		return -1;
	}

	if (my_gds->cmd_abort) 
	{
		return -1;
	}

	switch (my_gds->gd_cmd) 
	{
		case 16:
		case 17:
		case 20:
		case 21:
		case 22:
		case 27:
		case 28:
		case 29:
		case 32:
		case 33:
		case 34:
		case 37:
		case 38:
		case 39:
			switch (my_gds->gd_cmd_stat) 
			{
				case CMD_STAT_PROCESSING:
				case CMD_STAT_COMPLETED:
				case CMD_STAT_WAITING:
					my_gds->cmd_abort = 1;
					return 0;
					break;
					
				default:
					return 0;
					break;
			}
			break;

		default:
			break;
	}
	
	return -1;
}

void gdGdcReset()
{
#ifdef LOG	
	LOGFF(0);
#endif
	GDS *my_gds;
	
	my_gds = get_GDS();
	
	if (IN8(G1_ATA_DMA_STATUS)) 
	{
		OUT8(G1_ATA_DMA_ENABLE, 0);
		
		while (IN8(G1_ATA_DMA_STATUS));
		
		my_gds->dma_in_progress = 0;
	}
	
	OUT8(G1_ATA_COMMAND_REG, 8);
	
	while (IN8(G1_ATA_ALTSTATUS) & G1_ATA_SR_BSY);
}

void gdGdcSetPioCallback(void *callback, int callback_param)
{
#ifdef LOG	
	LOGFF("%08X %08X\n", callback, callback_param);
#endif
	GDS *my_gds;

	my_gds = get_GDS();

	if (callback) 
	{
		my_gds->callback = callback;
		my_gds->callback_param = callback_param;
	}
	else 
	{
		my_gds->callback = 0;
		my_gds->callback_param = 0;
	}
}

int gdGdcCheckPioTrans(int gd_chn, int *size)
{
#ifdef LOG	
	LOGFF(0);
#endif
	GDS *my_gds;

	my_gds = get_GDS();

	if (gd_chn != my_gds->gd_chn) 
	{
		return -1;
	}

	if (my_gds->gd_cmd_stat != CMD_STAT_WAITING) 
	{
		return -1;
	}
	
	if (my_gds->piosize) 
	{
		*size = my_gds->transfered;
		return 1;
	}

	*size = my_gds->requested;

	return 0;
}

int gdGdcReqPioTrans(int gd_chn, uint32_t *params)	/* _8c003374 */
{
#ifdef LOG	
	LOGFF("%08X %d\n", params[0], params[1]);
#endif
	GDS *my_gds;

	my_gds = get_GDS();

	if (gd_chn != my_gds->gd_chn) 
	{
		return -1;
	}

	if (my_gds->gd_cmd_stat != CMD_STAT_WAITING) 
	{
		return -1;
	}
	
	if(params[1] > my_gds->requested) 
	{
		return -1;
	}

	my_gds->requested -= params[1];
	my_gds->pioaddr = (short *) params[0];
	my_gds->piosize = params[1];
	
	return 0;
}

/*void Init_GDS()
{
#ifdef LOG	
	LOGFF(0);
#endif
	
	GDS *my_gds = get_GDS();
	
	my_gds->gd_cmd = 0;
	my_gds->gd_cmd_err = GDCMD_OK;
	my_gds->gd_cmd_err2 = 0;
	my_gds->transfered = 0;
	my_gds->ata_status = 0;
	my_gds->drv_stat = 0;
	my_gds->cmd_abort = 0;
	my_gds->requested = 0;
	my_gds->gd_chn = 1;
	my_gds->dma_in_progress = 0;
	my_gds->need_reinit = 0;
	my_gds->callback = 0;
	my_gds->callback_param = 0;
	my_gds->pioaddr = 0;
	my_gds->piosize = 0;
	my_gds->param[0] = 0;
	my_gds->param[1] = 0;
	my_gds->param[2] = 0;
	my_gds->param[3] = 0;
	my_gds->gd_cmd_stat = CMD_STAT_NO_ACTIVE;
	
	while(1) 
	{
		if (my_gds->gd_cmd_stat == CMD_STAT_COMPLETED) 
		{
			my_gds->gd_cmd_stat = CMD_STAT_PROCESSING;
			my_gds->gd_cmd_err = GDCMD_OK;
			my_gds->gd_cmd_err2 = 0;
			my_gds->transfered = 0;
			
			if (my_gds->need_reinit == 1 && my_gds->gd_cmd != 24)
			{
				my_gds->gd_cmd_err = GDCMD_NOT_INITED;
			}
			else
			{
				gd_do_cmd(my_gds->param, my_gds, my_gds->gd_cmd);
			}
			
			
			// 8с003742 сюда мы попадаем после отработки команды
			
			my_gds->gd_cmd_stat = CMD_STAT_ABORTED;
			my_gds->cmd_abort = 0;

			if (my_gds->gd_cmd_err == GDCMD_NOT_INITED) 
			{
				my_gds->need_reinit = 1;
			}
		}
		
		Exit_to_game(); // эта функция вернёт нас в игру
		// 8с003756 сюда мы попадаем при первой отработке любой комнды gdGdcExecServer'ом
	}

}*/


int dma_abort(GDS *my_gds)
{
	if (IN8(G1_ATA_DMA_STATUS)) 
	{
		OUT8(G1_ATA_DMA_ENABLE, 0);
		
		while (IN8(G1_ATA_DMA_STATUS));
	}
	
	my_gds->dma_in_progress = 0;
	
	return lock_gdsys(0);
}

void g1_ata_wait_nbsy(GDS *my_gds)
{
	while( IN8(G1_ATA_ALTSTATUS) & G1_ATA_SR_BSY )
	{
		my_gds->ata_status = 4;
		Exit_to_game();
	}
	
	my_gds->ata_status = 0;
}

void g1_ata_wait_bsydrq(GDS *my_gds)
{
	while (1) 
	{
		if (!(IN8(G1_ATA_ALTSTATUS) & 0x88)) 
		{
			my_gds->ata_status = 0;
			return;
		} 
		else 
		{
			my_gds->ata_status = 2;
			Exit_to_game();
		}
	}
}

void wait_gddma_irq(GDS *my_gds)
{
	do
	{
		my_gds->ata_status = 1;
		Exit_to_game();
	} 
	while (!(IN32(EXT_INT_STAT) & 1));
	
	my_gds->ata_status = 0;
}

void transfer_loop(int dma, GDS *my_gds)
{
	while (1)
	{
		if(my_gds->cmd_abort != 0)
		{
			break;
		}
		
		my_gds->ata_status = 1;
		
		Exit_to_game();
		
		if (dma)
		{
			my_gds->transfered = IN32(G1_ATA_DMA_LEND);
		}
		
		if((IN32(EXT_INT_STAT) & 1) != 0)
		{
			break;
		}
	} 
	
	my_gds->ata_status = 0;
}

void gd_cmd_abort(GDS *my_gds)
{
#ifdef LOG	
	LOGFF(0);
#endif	
	dma_abort(my_gds);
	
	my_gds->requested = 0;
	
	OUT8(G1_ATA_DEVICE_SELECT, 0x10);
	OUT8(G1_ATA_FEATURES, 0);
	
	g1_ata_wait_nbsy(my_gds);
	
	OUT8(G1_ATA_COMMAND_REG, 0);
	
	g1_ata_wait_bsydrq(my_gds);
}

uint8_t gd_read_abort(int *param, GDS *my_gds)
{
#ifdef LOG	
	LOGFF(0);
#endif
	(void) param;
	dma_abort(my_gds);
	
	my_gds->requested = 0;
	
	OUT8(G1_ATA_DEVICE_SELECT, 0x10);
	OUT8(G1_ATA_FEATURES, 0);
	OUT8(G1_ATA_COMMAND_REG, 0);
	 
	wait_gddma_irq(my_gds);
	 
	return IN8(G1_ATA_STATUS_REG);
}

void lba_tranlator(int dma, GDS *my_gds)
{
	uint32_t lba;
	uint16_t seccnt = (uint16_t) my_gds->param[1] << 2;
//	uint8_t cmd;
	my_gds->currentLBA = my_gds->param[0] + my_gds->param[1];
	
	if ( /*my_gds->drv_media == 0x20 ||((*/my_gds->param[0] < 45150 /*) && my_gds->dtrkLBA[0])*/ )
	{
		lba = ((my_gds->param[0] - 150) << 2) + my_gds->dtrkLBA[0];
		my_gds->currentLBA += 0x01000000;
	}
	else if (my_gds->dtrkLBA[2] && my_gds->param[0] >= (my_gds->TOC.entry[((my_gds->TOC.last >> 16) & 0xFF)-1] & 0x00FFFFFF))
	{
		lba = ((my_gds->param[0] - (my_gds->TOC.entry[((my_gds->TOC.last >> 16) & 0xFF)-1] & 0x00FFFFFF)) << 2) + my_gds->dtrkLBA[2];
		my_gds->currentLBA += (my_gds->TOC.last << 8);
	}
	else // track03
	{
		lba = ((my_gds->param[0] - 45150) << 2) + my_gds->dtrkLBA[1];
		my_gds->currentLBA += 0x03000000;
	}
	
//	if (seccnt > 256 || lba >= 0x10000000)
//	{	
//		cmd = (dma == 0) ? ATA_CMD_READ_SECTORS_EXT : ATA_CMD_READ_DMA_EXT;

		OUT8(G1_ATA_DEVICE_SELECT, 0xF0);
		
		do {} while ((IN8(G1_ATA_STATUS_REG) & (G1_ATA_SR_DRDY | G1_ATA_SR_BSY)) !=  G1_ATA_SR_DRDY);
		
		//OUT8(G1_ATA_CTL, 0x80);
		OUT8(G1_ATA_SECTOR_COUNT, (uint8_t)(seccnt >> 8));
		OUT8(G1_ATA_LBA_LOW, lba >> 24);
		OUT8(G1_ATA_LBA_MID, 0);
		OUT8(G1_ATA_LBA_HIGH, 0);
//	}
/*	else
	{
		cmd = (dma == 0) ? ATA_CMD_READ_SECTORS : ATA_CMD_READ_DMA;

		OUT8(G1_ATA_DEVICE_SELECT, (0xF0 | ((lba >> 24) & 0xF)));
		
		do {} while ((IN8(G1_ATA_STATUS_REG) & (G1_ATA_SR_DRDY | G1_ATA_SR_BSY)) !=  G1_ATA_SR_DRDY);
	}*/

	//OUT8(G1_ATA_CTL, 0);
		
	OUT8(G1_ATA_SECTOR_COUNT, (seccnt & 0xff));
	OUT8(G1_ATA_LBA_LOW,  (lba & 0xff));
	OUT8(G1_ATA_LBA_MID,  ((lba >> 8) & 0xff));
	OUT8(G1_ATA_LBA_HIGH, ((lba >> 16) & 0xff));
	
	OUT8(G1_ATA_COMMAND_REG, dma ? ATA_CMD_READ_DMA_EXT:ATA_CMD_READ_SECTORS_EXT);
	
	//do {} while ((IN8(G1_ATA_ALTSTATUS) & 0xC0) != 0x40);
}
/*
void pio_read_internal(uint16_t *buffer, GDS *my_gds)
{
#ifdef LOG	
	LOGFF("%08X %d %d %08X\n", my_gds->param[2], my_gds->param[1], my_gds->param[0], my_gds->param[3]);
#endif
	
	lba_tranlator(0, my_gds);
	
	my_gds->ata_status = 1;
	
	uint32_t len = my_gds->param[1] << 2;
	
	for (uint32_t i = 0; i < len; i++)
	{		
		if (my_gds->cmd_abort != 0)
		{
			break;
		}
		
		for(int n = 0; n < 4; n++)
			IN8(G1_ATA_ALTSTATUS);
		
		do {} while((IN8(G1_ATA_STATUS_REG) & (G1_ATA_SR_BSY | G1_ATA_SR_DRQ)) != G1_ATA_SR_DRQ);
		
		for (uint32_t j = 0; j < 256; j++)
		{
			*buffer++ = IN16(G1_ATA_DATA);
		}
		
		my_gds->transfered += 512;
		
		if (i && !(i % 4))
		{
			Exit_to_game();
		}
	}
	
	IN8(G1_ATA_STATUS_REG);
	my_gds->ata_status = 0;
}
*/


void pio_read_internal(short *buffer, GDS *my_gds)
{
#ifdef LOG	
	LOGFF("%08X %d %d %08X\n", my_gds->param[2], my_gds->param[1], my_gds->param[0], my_gds->param[3]);
#endif
	uint8_t status, status2;
	
	lba_tranlator(0, my_gds);
#if 0 //def LOG	
	LOGF("lba translated\n");
#endif
	do
	{
		transfer_loop(0, my_gds);
#if 0 //def LOG
		LOGF("transfer_loop\n");
#endif
		if(my_gds->cmd_abort == 0)
		{
			status = IN8(G1_ATA_STATUS_REG);
		}
		else
		{
			my_gds->cmd_abort = 2;
			status = gd_read_abort(0, my_gds);
		}
		
		status2 = status & G1_ATA_SR_DRQ;
		
		if (status2 == G1_ATA_SR_DRQ)
		{
#if 0 //def LOG
			LOGF("read sector\n");
#endif
			OUT8(G1_ATA_CTL, 2);
			
			int sec_cnt;
			
			if ((my_gds->requested - my_gds->transfered) >= 0xF000)
			{
				sec_cnt = 119;
			}
			else
			{
				sec_cnt = ((my_gds->requested - my_gds->transfered) >> 9) - 1;
			}
			
			for (int j = 0; j <= sec_cnt; j++)
			{
				if (j == sec_cnt)
				{
					OUT8(G1_ATA_CTL, 0);
				}
				
				if (j)
					do {} while((IN8(G1_ATA_ALTSTATUS) & (G1_ATA_SR_BSY | G1_ATA_SR_DRQ)) != G1_ATA_SR_DRQ);
				
				for (int i = 0; i < 256; i++)
				{
					buffer[i] = IN16(G1_ATA_DATA);
				}
				
				buffer = &buffer[256];
				my_gds->transfered += 512;
			}	
		}
		
		//g1_ata_wait_nbsy(my_gds);
	}
	while(status2 == G1_ATA_SR_DRQ);
	
	(void) IN8(G1_ATA_STATUS_REG);
#if 0 //def LOG
	LOGFF("done\n");
#endif
}

void pio_stream_internal(GDS *my_gds)
{
#ifdef LOG	
	LOGFF("%08X %08X %08X %08X\n", my_gds->param[2], my_gds->param[1], my_gds->param[0], my_gds->param[3]);
#endif	
	uint8_t status, status2;
	
	lba_tranlator(0, my_gds);
	
	do
	{
		transfer_loop(0, my_gds);
		
		if(my_gds->cmd_abort == 0)
		{
			status = IN8(G1_ATA_STATUS_REG);
		}
		else
		{
read_abort:		
			my_gds->cmd_abort = 2;
			status = gd_read_abort(0, my_gds);
		}
		
		status2 = status & G1_ATA_SR_DRQ;
		
		if (status2 == G1_ATA_SR_DRQ)
		{
			OUT8(G1_ATA_CTL, 2);
			
			int len = 2048;
			
			/*if ((my_gds->requested - my_gds->transfered) >= 0xF000)
			{
				len = 0xF000;
			}
			else
			{
				len = my_gds->requested - my_gds->transfered;
			}*/
			
			while (len >= 1)
			{
				if (len == 512)
				{
					OUT8(G1_ATA_CTL, 0);
				}
				
				if (len && !(len % 512))
				{
					do {} while((IN8(G1_ATA_ALTSTATUS) & (G1_ATA_SR_BSY | G1_ATA_SR_DRQ)) != G1_ATA_SR_DRQ);
				}
				
				if(my_gds->piosize > 1)
				{
					*my_gds->pioaddr = IN16(G1_ATA_DATA);
					my_gds->pioaddr++;
					my_gds->transfered += 2;
					my_gds->piosize -= 2;
					len -= 2;
				}
				else
				{
					if(my_gds->callback != 0)
					{
						void (*callback)() = (void (*)())(my_gds->callback);
						callback(my_gds->callback_param);
					}
					
					while (my_gds->piosize == 0)
					{
						if(my_gds->cmd_abort != 0)
							goto read_abort;
						
						Exit_to_game();
					}
				}
			}
		}
		
		//g1_ata_wait_nbsy(my_gds);
	}
	while(status2 == G1_ATA_SR_DRQ);
	
	(void) IN8(G1_ATA_STATUS_REG);
}

void gd_dmaread_stream2(uint32_t *params, GDS *my_gds)
{
#ifdef LOG	
	LOGFF("%08X %d %d %08X\n", params[2], params[1], params[0], params[3]);
#endif
	/* dma abort, stop cdda unlock gdsys */
	dma_abort(my_gds);
	
	if (lock_gdsys(1)) 
	{
#if 0 //def LOG	
		LOGFF("ERROR, can't lock gdsys\n");
#endif
		my_gds->gd_cmd_err = GDCMD_GDSYS_LOCKED;
		return;
	}
	
	my_gds->gd_cmd_stat = CMD_STAT_WAITING;
	my_gds->requested = params[1] << 11;
	
	lba_tranlator(1, my_gds);
	
	my_gds->dma_in_progress = 1;
	
	transfer_loop(1, my_gds);
	
	my_gds->transfered = IN32(G1_ATA_DMA_LEND);
	
	if(my_gds->cmd_abort != 0)
	{		
		if ( !my_gds->requested && (my_gds->transfered + 512) < IN32(G1_ATA_DMA_LENGTH))
		{
			wait_gddma_irq(my_gds);
			
			my_gds->dma_in_progress = 0;
		}
		else
		{
			my_gds->cmd_abort = 2;
			
			uint8_t stat = gd_read_abort(0, my_gds);
			
			if (stat & G1_ATA_SR_ERR)
			{
				if (IN8(G1_ATA_ERROR) & G1_ATA_ER_ABRT)
				{
					return;
				}
			}
			
			wait_gddma_irq(my_gds);
			
			(void) IN8(G1_ATA_STATUS_REG);
			
			g1_ata_wait_bsydrq(my_gds);

			return;
		}
	}
	
	(void) IN8(G1_ATA_STATUS_REG);
	
	dma_abort(my_gds);
	
/*	g1_ata_wait_bsydrq(my_gds);
	
	lock_gdsys(0);*/
}

void gd_dmaread(uint32_t *params, GDS *my_gds)	/* _8c001b2c */
{
#ifdef LOG	
	LOGFF("%08X %d %d %d\n", params[2], params[1], params[0], params[3]);
#endif
	dma_abort(my_gds);
	
	if (lock_gdsys(1)) 
	{
		my_gds->gd_cmd_err = GDCMD_GDSYS_LOCKED;
		return;
	}
	
	OUT32(G1_ATA_DMA_PRO, G1_ATA_DMA_PRO_SYSMEM);
	OUT32(G1_ATA_DMA_ADDRESS, params[2]);
	OUT32(G1_ATA_DMA_LENGTH, params[1] << 11);
	OUT8(G1_ATA_DMA_DIRECTION, 1);
	
	lba_tranlator(1, my_gds);

	my_gds->dma_in_progress = 1;
	
	OUT8(G1_ATA_DMA_ENABLE, 1);
	OUT8(G1_ATA_DMA_STATUS, 1);
	
	transfer_loop(1, my_gds);
	
/*	if (my_gds->cmd_abort != 0) 
	{
		if ((my_gds->transfered + 2048) > IN32(G1_ATA_DMA_LENGTH)) 
		{
			wait_gddma_irq(my_gds);
		} 
		else 
		{
			my_gds->cmd_abort = 2;
			uint8_t stat = gd_read_abort(0, my_gds);
			
			if (stat & G1_ATA_SR_ERR) 
			{
				if (IN8(G1_ATA_ERROR) & G1_ATA_ER_ABRT)
					return;
			}
			
			wait_gddma_irq(my_gds);
			(void) IN8(G1_ATA_STATUS_REG);
			g1_ata_wait_bsydrq(my_gds);
			return;
		}
	}*/
	
	(void) IN8(G1_ATA_STATUS_REG);
	dma_abort(my_gds);
	
/*	g1_ata_wait_bsydrq(my_gds);
	
	lock_gdsys(0);*/
}
/*
void gd_set_mode()
{
#ifdef LOG	
	LOGFF(0);
#endif
}

void gd_req_mode()
{
#ifdef LOG	
	LOGFF(0);
#endif
}
*/
/*
void dummy_read(GDS *my_gds)
{
	OUT8(G1_ATA_DEVICE_SELECT, 0xF0);
		
	do {} while ((IN8(G1_ATA_STATUS_REG) & (G1_ATA_SR_DRDY | G1_ATA_SR_BSY)) !=  G1_ATA_SR_DRDY);
	
	OUT8(G1_ATA_CTL, 0);
	
	OUT8(G1_ATA_SECTOR_COUNT, 1);
	OUT8(G1_ATA_LBA_LOW, 0xff);
	OUT8(G1_ATA_LBA_MID,  0);
	OUT8(G1_ATA_LBA_HIGH, 0);

	OUT8(G1_ATA_COMMAND_REG, ATA_CMD_READ_SECTORS);
	
	transfer_loop(0, my_gds);
	
	do {} while((IN8(G1_ATA_ALTSTATUS) & (G1_ATA_SR_BSY | G1_ATA_SR_DRQ)) != G1_ATA_SR_DRQ);
				
	for (int i = 0; i < 256; i++)
	{
		(void) IN16(G1_ATA_DATA);
	}
	
	g1_ata_wait_nbsy(my_gds);
	
	transfer_loop(0, my_gds);
}*/
/*
void gd_cd_open_tray()
{
#ifdef LOG	
	LOGFF("WARNING\n");
#endif
}
*/
/*
void gd_gettoc()
{
#ifdef LOG	
	LOGFF("WARNING\n");
#endif
}
*/
void gd_gettoc2(uint32_t *params, GDS *my_gds)
{
#ifdef LOG	
	LOGFF(0);
#endif
	
	if(my_gds->drv_media == TYPE_GDROM /*&& !params[0]*/)
	{	
		if (!params[0])
		{
			toc_t *toc = (toc_t *) params[1];
			
			toc->entry[0] = my_gds->TOC.entry[0];
			toc->entry[1] = my_gds->TOC.entry[1];
			
			for (int i = 2; i < 99; i++)
			{
				toc->entry[i] = (uint32_t) -1;
			}
			
			toc->first = 0x41010000;
			toc->last  = 0x01020000;
			toc->leadout_sector = 0x01001A2C;
		}
		else
		{
			uint8_t *src = (uint8_t *) my_gds->TOC.entry;
			uint8_t *dst = (uint8_t *) params[1];
			
			for (int i = 0; i < 408; i++)
			{
				dst[i] = src[i];
			}
		}
		return;
	}
	
	my_gds->gd_cmd_stat = CMD_STAT_ERROR;
}

void gd_gettracks(int *param, GDS *my_gds)
{
#ifdef LOG	
	LOGFF(0);
#endif
	//dummy_read(my_gds);

	uint8_t *buf = (uint8_t *) param[2];
	uint32_t lba = my_gds->TOC.leadout_sector;
	
	buf[0] = STAT_PAUSE;
	buf[1] = 0;
	buf[2] = 1;
	buf[3] = (lba >> 16) & 0xFF;
	buf[4] = (lba >> 8) & 0xFF;
	buf[5] = lba & 0xFF;
	my_gds->transfered = 6;
}

void gd_getstat(uint32_t *params, GDS *my_gds)
{
	//dummy_read(my_gds);

	uint8_t track = ((uint8_t *) &my_gds->currentLBA)[3];
	uint32_t lba = my_gds->currentLBA & 0xFFFFFF;
	
	*((uint32_t *)params[0]) = STAT_PAUSE;
	*((uint32_t *)params[1]) = track;
	*((uint32_t *)params[2]) = lba + 0x41000000;
	*((uint32_t *)params[3]) = 1;
}

static uint8_t scd_all[100] = 
{
	0x00, 0x15, 0x00, 0x64, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x40, 0x40,
	0x00, 0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x40, 0x00, 0x00, 0x00, 0x40, 0x40, 0x00, 0x00, 0x00, 0x40,
	0x40, 0x00, 0x40, 0x00, 0x40, 0x00, 0x00, 0x00, 0x40, 0x40, 0x40, 0x40,
	0x40, 0x40, 0x40, 0x40
};

static uint8_t scd_media[24] = 
{
	0x00, 0x15, 0x00, 0x18, 0x02, 0x00, 0x00, 0x00, 0x00, 0x30, 0x30, 0x30,
	0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x00, 0x00
};

static uint8_t scd_isrc[24] = 
{
	0x00, 0x15, 0x00, 0x18, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x30, 0x00, 0x00, 0x00
};

void gd_getscd(uint32_t *param, GDS *my_gds)
{
#ifdef LOG	
	LOGFF("%08X %08X %08X\n", param[0], param[1], param[2]);
#endif
	//dummy_read(my_gds);
	
	uint8_t *buf = (uint8_t *)param[2];

	switch(param[0] & 0xF) 
	{
		case SCD_REQ_ALL_SUBCODE:
		{
			for (int i = 0; i < scd_all[SCD_DATA_SIZE_INDEX]; i++)
			{
				buf[i] = scd_all[i];
			}
			
			buf[1] = SCD_AUDIO_STATUS_PAUSED;
			my_gds->transfered = scd_all[SCD_DATA_SIZE_INDEX];
			break;
		}
		case SCD_REQ_Q_SUBCODE:
		{
			uint8_t track = ((uint8_t*) &my_gds->currentLBA)[3];
			uint32_t lba = my_gds->currentLBA & 0xFFFFFF;
			uint32_t offset = lba - 150;
			
			buf[0] = 0x00;						// Reserved
			buf[1] = SCD_AUDIO_STATUS_NO_INFO;	// Audio Status
			buf[2] = 0x00;						// DATA Length MSB
			buf[3] = 0x0E;						// DATA Length LSB
			
			buf[4] = 0x41;					// Track flags (CTRL and ADR)
			buf[5] = track;					// Track NO
			buf[6] = track;						// 00: Pause area 01-99: Index number
			
			buf[7] = (uint8_t)(offset >> 16);
			buf[8] = (uint8_t)(offset >> 8);
			buf[9] =  (uint8_t) offset;
			
			buf[10] = 0x00;                     // Reserved
			
			buf[11] = (uint8_t)(lba >> 16);
			buf[12] = (uint8_t)(lba >> 8);
			buf[13] =  (uint8_t) lba;

			my_gds->transfered = 0xE;
			break;
		}
		case SCD_REQ_MEDIA_CATALOG:
		{
			for (int i = 0; i < scd_media[SCD_DATA_SIZE_INDEX]; i++)
			{
				buf[i] = scd_media[i];
			}
			
//			buf[0] = 0x00;                      // Reserved
			buf[1] = SCD_AUDIO_STATUS_PAUSED;	// Audio Status
//			buf[2] = 0x00;                      // DATA Length MSB
//			buf[3] = 0x18;                      // DATA Length LSB
//			buf[4] = 0x02;                      // Format Code
			my_gds->transfered = scd_media[SCD_DATA_SIZE_INDEX];
			break;
		}
		case SCD_REQ_ISRC:
		{
			for (int i = 0; i < scd_isrc[SCD_DATA_SIZE_INDEX]; i++)
			{
				buf[i] = scd_isrc[i];
			}
			
//			buf[0] = 0x00;                      // Reserved
			buf[1] = SCD_AUDIO_STATUS_PAUSED;	// Audio Status
//			buf[2] = 0x00;                      // DATA Length MSB
//			buf[3] = 0x18;                      // DATA Length LSB
//			buf[4] = 0x03;                      // Format Code
			my_gds->transfered = scd_isrc[SCD_DATA_SIZE_INDEX];
			break;
		}
		// case SCD_REQ_RESERVED:
		// 	break;
			
		default:
			break;
	}
	
	if (res_count > 1)
	{
		my_gds->gd_cmd_err = GDCMD_NOT_INITED;
	}
}

void gd_init(int *param, GDS *my_gds)
{
	(void) param;
#ifdef LOG	
	scif_init();
	LOGFF("dt = %02X\n", my_gds->drv_media);
#endif
	
	if (disc_id[0])
	{
		my_gds->dtrkLBA[1] = disc_id[disc_id[0]];
	}
	
	my_gds->cmd_abort = 0;
	my_gds->requested = 0;
	
	dma_abort(my_gds);
	
	uint8_t stat = IN8(G1_ATA_STATUS_REG);
	
	if (stat & G1_ATA_SR_BSY)
	{
		gd_cmd_abort(my_gds);
	}
	else if (stat & G1_ATA_SR_DRQ)
	{
		if (IN32(EXT_INT_STAT) & 1)
		{
			(void) IN8(G1_ATA_STATUS_REG);
		}
		
		gd_read_abort(0, my_gds);
	}
	
	disc_id[0] = 0;
	my_gds->need_reinit = 0;
	my_gds->drv_media = TYPE_GDROM;
	my_gds->gd_cmd_err = 0;
	my_gds->gd_cmd_err2 = 0;
	my_gds->transfered = 0x198;
	my_gds->ata_status = 0;
}

typedef struct
{
	uint32_t off;
	uint32_t sz;
} fl_inf;

static fl_inf fl_parts[5] =
{
	{ 0x1a000, 0x02000 },
	{ 0x18000, 0x02000 },
	{ 0x1c000, 0x04000 },
	{ 0x10000, 0x08000 },
	{ 0		 , 0x10000 }
};


int flashrom_info(int part, uint32_t *buffer)
{
#ifdef LOG	
	LOGFF(0);
#endif
	if (part < 0 || part >= 5)
		return -1;
	
	buffer[0] = fl_parts[part].off;
	buffer[1] = fl_parts[part].sz;
	
	return 0;
}

uint8_t region_str[3][6] =
{
	{ "00000" },
	{ "00110" },
	{ "00211" }
};
	
int flashrom_read(uint32_t offset, uint8_t *dst, uint32_t size)
{
#ifdef LOG
	LOGFF("%08X %d\n", offset, size);
#endif
	if (flashrom_lock() != 0)
		return -1;
	
	uint8_t *src;
	
	if (offset == 0x1A000 && *((uint8_t *)0x8C000020) != 0)
	{
		src = region_str[*((uint8_t *)0x8C000020) - 1];
		#ifdef LOG
		LOGF("%s\n", (char *) src);
		#endif
	}
	else
	{
		src = (uint8_t *) (0xA0200000 + offset);
	}
	
	for (; size; size--)
	{
		*dst++ = *src++;
	}
	
	flashrom_unlock();
	
	return 0;
}

int gd_do_bioscall_tmp(int nu1, int nu2, int nu3, int cmd)
{
	(void) nu1;
	(void) nu2;
	(void) nu3;
	
	switch (cmd)
	{
		case 0:
			return 0;
			break;
		
		case 1:
		case 4:
			return 4;
			break;
		
		default:
			break;
	}
	
	return -1;
}

static uint32_t saved_irq = 0;

void hide_gd_vector(int restore)
{
	uint32_t oldsr = irq_disable();
	
	uint32_t reg = 0xA05F6904;
	uint32_t shift;
	uint32_t tmp;
	
	if (restore)
	{
		gd_vector2 = 0x8C0010F0;
		
		for (int i = 0; i < 3; i++)
		{
			shift = saved_irq;
			reg += 12;
			tmp = shift >> i;
			OUT32(reg, IN32(reg) | (tmp & 0x4000));
			
			reg += 4;
			OUT32(reg, IN32(reg) | (tmp & 1));
		}
		
		(void) IN32(reg);
		
		reg = 0xA05F6900;
		
		OUT32(reg, IN32(reg) & 0x4000);
	}
	else
	{
		gd_vector2 = (uint32_t) &gd_do_bioscall_tmp;
		tmp = 0;
		
		for (int i = 0; i < 3; i++)
		{
			reg += 12;
			tmp |= ((IN32(reg) & 0x4000) << i);
			
			OUT32(reg, IN32(reg) & 0xFFFFBFFF);
			
			reg += 4;
			tmp |= ((IN32(reg) & 1) << i);
			
			OUT32(reg, IN32(reg) & 0xFFFFFFFE);
		}
		
		(void) IN32(reg);
		saved_irq = tmp;
		
		gd_gdrom_syscall(0, 0, 0, 3);
	}
	
	irq_restore(oldsr);
}

/*
int sys_do_bioscall(int param)
{
#ifdef LOG
	LOGFF("%d\n", param);
#endif	
	return 0;
}
*/

static int req_id;

int syBtCheckDisc()
{
#ifdef LOG	
	LOGFF(0);
#endif
	
	int disc = (int) disc_type;
	
	if (disc < 0)
	{
		gd_gdrom_syscall(0, 0, 0, 2);
	}
	
	switch (disc)
	{
		case -8:
				disc = 0;
			break;
		
		case -7:
		case -4:
		{
			static uint32_t stat[4];
			(void) stat;
			
			int ret = gd_gdrom_syscall(req_id, stat, 0, 1);
			
			if (ret == CMD_STAT_NO_ACTIVE || ret == CMD_STAT_COMPLETED)
			{
				disc -= 1;
			}
			
			break;
		}
		case -6:
			break;
		
		case -5:
		{
			static uint32_t cmd[4] = { 45150, 7, 0x8C008100, 0 };
			req_id = gd_gdrom_syscall(17, cmd, 0, 0);
			
			disc = -7;
			
			break;
		}
		case -3:
			break;
		
		case -2:
		{
			req_id = gd_gdrom_syscall(24, 0, 0, 0);
			
			disc = -4;
			
			break;
		}
		case -1:
			break;
		
		case 0:
		{
			hide_gd_vector(0); // hide 
			display_cable |= 1;
			disc = -2;
			break;
		}
	}
	
	if (disc >= 0)
	{
		hide_gd_vector(1); // restore
	}
	
	disc_type = (short) disc;
	
	return disc;
}

#ifdef LOG	
void syBtExit(uint32_t pr, uint32_t *stack)
{
	LOGFF("pr = %08X\n", pr);
	for (int i = 0; i < 32; i++)
	{
		if (i == 8 || i == 16 || i == 24)
		{
			LOGF("\n");
		}
		LOGF("%08X ", stack[i]);
	}
	LOGF("\n");
}

#else

uint32_t reset_regs[12] =
{
	0xA05F6808, 0xA05F6820, 0xA05F6C14, 0xA05F7414, 
	0xA05F7814, 0xA05F7834, 0xA05F7854, 0xA05F7874, 
	0xA05F7C14, 0xFFA0001C, 0xFFA0002C, 0xFFA0003C
};

void syBtExit()
{
//#ifdef LOG	
//	LOGFF(0);
//#endif
	flush_cache();
	
	OUT32(0xFF000010, 0);
	OUT32(0xFF00001C, 0x929);
	
	uint32_t addr1 = 0xA05F6938;
	uint32_t addr2 = 0xFFD0000C;
	
	for (int i = 3; i; i--)
	{
		OUT32(addr1, 0);
		addr1 -= 4;
		OUT32(addr1, 0);
		addr1 -= 4;
		OUT32(addr1, 0);
		addr1 -= 8;
		
		OUT32(addr2, 0);
		addr2 -= 4;
	}
	
	OUT32(addr2, 0);
	(void) IN32(addr1);
	addr1 -= 8;
	(void) IN32(addr1);
	
	OUT32(0xA05F8044, (IN32(0xA05F8044) & 0xFFFFFFFE));
	
	for (int i = 0; i < 12; i++)
	{
		OUT32(reset_regs[i], (IN32(reset_regs[i]) & 0xFFFFFFFE));
		
		for (int j = 0; j < 127; j++)
		{
			if (!(IN32(reset_regs[i]) & 0xFFFFFFFE))
			{
				break;
			}
		}
	}
	
	
	GDS *my_gds = get_GDS();
	
	if (my_gds->dsLBA)
	{
		uint32_t *addr = (uint32_t *) 0x8C010000;
		my_gds->requested = ((my_gds->dsseccnt>>11)<<11)+2048;
		my_gds->transfered = 0;
		my_gds->param[0] = 150;
		my_gds->param[1] = (my_gds->dsseccnt >> 11)+1;
		my_gds->param[2] = (uint32_t) addr;
		my_gds->param[3] = 0;
		my_gds->dtrkLBA[0] = my_gds->dsLBA;
		
		lba_tranlator(0, my_gds);
		
		for (uint32_t j = 0; j < (my_gds->param[1]<<2); j++)
		{
			do {} while((IN8(G1_ATA_ALTSTATUS) & G1_ATA_SR_BSY));
			do {} while(!(IN8(G1_ATA_STATUS_REG) & G1_ATA_SR_DRQ));
			
			for (uint32_t i = 0; i < 256; i++)
			{
				((uint16_t *)addr)[i+(j*256)] = IN16(G1_ATA_DATA);
			}
		}
		
		OUT8(G1_ATA_DEVICE_SELECT, 0x10);
		OUT8(G1_ATA_COMMAND_REG, 8);
		OUT8(G1_ATA_DEVICE_SELECT, 0);
		
		for (uint32_t i = 0; i < (my_gds->dsseccnt >> 2); i++)
		{
			if(addr[i] == 0xA05F74E4)
				addr[i] = 0xA05F74EC;
		}
		
		typedef void (*trampoline_func)();
        trampoline_func trampoline = (trampoline_func) 0x8C010000;

        trampoline();
	}
	
	OUT32(0xA05F6890, 0x7611);
	while(1);
}
#endif

