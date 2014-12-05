/* DreamShell ##version##

   module.c - Bios flasher app module
   Copyright (C)2013 Yev
   Copyright (C)2013-2014 SWAT

*/

#include "ds.h"
#include "drivers/bflash.h"
#include <time.h>
#include <stdbool.h>
#include <drivers/sh7750_regs.h>

DEFAULT_MODULE_EXPORTS(app_bios_flasher);

typedef enum OperationState
{
	eNone = -1,
	eErasing = 0,
	eWriting,
	eReading,
	eCompare
}OperationState_t;

typedef enum OperationError
{
	eSuccess = 0,
	eUnknownFail = -1,
	eDetectionFail = -2,
	eFileFail = -3,
	eErasingFail = -4,
	eWritingFail = -5,
	eReadingFail = -6,
	eDataMissmatch = 1
}OperationError_t;

static const uint16 CHUNK_SIZE = 32 * 1024;

#define UPDATE_GUI(state, progress, gui) do { if(gui) gui(state, progress); } while(0)

typedef void BiosFlasher_OperationCallback(OperationState_t state, float progress);

int BiosFlasher_WriteBiosFileToFlash(const char* filename, BiosFlasher_OperationCallback guiClbk);
int BiosFlasher_CompareBiosWithFile(const char* filename, BiosFlasher_OperationCallback guiClbk);
int BiosFlasher_WriteFromBiosToFile(const char* filename, BiosFlasher_OperationCallback guiClbk);
void BiosFlasher_EnableMainPage();
void BiosFlasher_EnableChoseFilePage();
void BiosFlasher_EnableProgressPage();
void BiosFlasher_EnableResultPage(int result, const char* fileName);
void BiosFlasher_EnableSettingsPage();
void BiosFlasher_OnOperationProgress(OperationState_t state, float progress);

struct
{
	App_t* m_App;

	GUI_Widget *m_LabelProgress;
	GUI_Widget *m_LabelProgressDesc;
	GUI_Widget *m_ProgressBar;
	GUI_Widget *filebrowser;
	GUI_Widget *pages;
	
	GUI_Surface *m_ItemNormal;
	GUI_Surface *m_ItemSelected;

	char* m_SelectedBiosFile;
	OperationState_t m_CurrentOperation;
	unsigned int m_ScrollPos;
	unsigned int m_MaxScroll;
	
	bool have_args;
}self;

struct
{
	size_t m_DataStart;
	size_t m_DataLength;
}settings;


void BiosFlasher_ResetSelf()
{
	memset(&self, 0, sizeof(self));
	self.m_CurrentOperation = eNone;
	memset(&settings, 0, sizeof(settings));
}

// BIOS 
bflash_dev_t* BiosFlasher_DetectFlashChip()
{
	bflash_manufacturer_t* mfr = 0;
	bflash_dev_t* dev = 0;
	
	if (bflash_detect(&mfr, &dev) < 0)
	{
		dev = (bflash_dev_t*)malloc(sizeof(*dev));
		memset(dev, 0, sizeof(*dev));
		dev->name = strdup("Unknown");
	}

	return dev;
}

int BiosFlasher_WriteBiosFileToFlash(const char* filename, BiosFlasher_OperationCallback guiClbk)
{
	size_t i = 0;
	uint8* data 	= 0;
	file_t pFile 	= 0;

	UPDATE_GUI(eReading, 0.0f, guiClbk);
	// Detect writible flash
	bflash_dev_t *dev = NULL;
	bflash_manufacturer_t *mrf = NULL;
	if( bflash_detect(&mrf, &dev) < 0   ||
		!(dev->flags & F_FLASH_PROGRAM) ) 
	{
		ds_printf("DS_ERROR: flash chip detection error.\n");
		return eDetectionFail;
	}

	// Read bios file from file to memory
	pFile = fs_open(filename, O_RDONLY);
	if (pFile == FILEHND_INVALID)
	{
		ds_printf("DS_ERROR: Can't open bios file: %s.\n", filename);
		return eFileFail;
	}
	
	size_t fileSize = fs_total(pFile);
	if(fileSize > dev->size * 1024) 
	{
		ds_printf("DS_ERROR: The firmware larger than a flash chip (%d KB vs %d KB).\n", (fileSize / 1024), dev->size);
		fs_close(pFile);
		return eFileFail;
	}

	data = (uint8 *) memalign(32, fileSize);
	if(data == NULL) 
	{
		ds_printf("DS_ERROR: Not enough memory\n");
		fs_close(pFile);
		return eUnknownFail;
	}
	
	size_t readLen = fs_read(pFile, data, fileSize);
	if (fileSize != readLen)
	{
		ds_printf("DS_ERROR: File wasn't loaded fully to memory\n");
		fs_close(pFile);
		return eFileFail;
	}
	fs_close(pFile);

	EXPT_GUARD_BEGIN;

		// Erasing
		if (dev->flags & F_FLASH_ERASE_SECTOR || 
			dev->flags & F_FLASH_ERASE_ALL) 
		{
			for (i = 0; i < dev->sec_count; ++i) 
			{		
				UPDATE_GUI(eErasing, (float)i / dev->sec_count, guiClbk);

				if (bflash_erase_sector(dev, dev->sectors[i]) < 0) 
				{
					ds_printf("DS_ERROR: Can't erase flash\n");
					free(data);
					EXPT_GUARD_RETURN eErasingFail;
				}
			}
		}

		// Writing
		size_t offset = 0;
		if (fileSize >= settings.m_DataStart + settings.m_DataLength)
		{
			offset = settings.m_DataStart;
			fileSize = (settings.m_DataLength > 0) ? offset + settings.m_DataLength : fileSize - offset;
		}

		size_t chunkCount = fileSize / CHUNK_SIZE;
		for (i = 0; i <= chunkCount; ++i)
		{
			UPDATE_GUI(eWriting, (float)i / chunkCount, guiClbk);

			size_t dataPos = i * CHUNK_SIZE + offset;
			size_t dataLen = (dataPos + CHUNK_SIZE > fileSize) ? fileSize - dataPos : CHUNK_SIZE;
//			ScreenWaitUpdate();
			LockVideo();
			int result = bflash_write_data(dev, dataPos, data + dataPos, dataLen);
			UnlockVideo();
			
			if (result < 0)
			{
				ds_printf("DS_ERROR: Can't write flash\n");
				free(data);
				EXPT_GUARD_RETURN eWritingFail;
			}
		}

	EXPT_GUARD_CATCH;
	
		ds_printf("DS_ERROR: Fatal error\n");
		free(data);
		fs_close(pFile);
		EXPT_GUARD_RETURN eFileFail;
		
	EXPT_GUARD_END;

	free(data);
	fs_close(pFile);

	return 0;
}

int BiosFlasher_CompareBiosWithFile(const char* filename, BiosFlasher_OperationCallback guiClbk)
{
	uint8* data 	= 0;
	file_t pFile 	= 0;

	UPDATE_GUI(eReading, 0.0f, guiClbk);
	// Detect writible flash
	bflash_dev_t *dev = NULL;
	bflash_manufacturer_t *mrf = NULL;
	if (bflash_detect(&mrf, &dev) < 0) 
	{
		ds_printf("DS_ERROR: flash chip detection error.\n");
		return eDetectionFail;
	}
	
	// Read bios file from file to memory
	pFile = fs_open(filename, O_RDONLY);
	if (pFile == FILEHND_INVALID)
	{
		ds_printf("DS_ERROR: Can't open bios file: %s.\n", filename);
		return eFileFail;
	}
	
	size_t fileSize = fs_total(pFile);
	if (fileSize > dev->size * 1024) 
	{
		ds_printf("DS_ERROR: The firmware larger than a flash chip (%d KB vs %d KB).\n", (fileSize / 1024), dev->size);
		fs_close(pFile);
		return eDataMissmatch;
	}

	data = (uint8 *) memalign(32, CHUNK_SIZE);
	if(data == NULL) 
	{
		ds_printf("DS_ERROR: Not enough memory\n");
		fs_close(pFile);
		return eUnknownFail;
	}

	int ret = eSuccess;
	int i = 0;
	size_t chunkCount = fileSize / CHUNK_SIZE;
	size_t dataPos, dataLen, readLen;
	
	for (i = 0; i <= chunkCount; ++i)
	{

		dataPos = i * CHUNK_SIZE;
		dataLen = (dataPos + CHUNK_SIZE > fileSize) ? fileSize - dataPos : CHUNK_SIZE;

		UPDATE_GUI(eReading, (float)i / chunkCount, guiClbk);
		
		readLen = fs_read(pFile, data, dataLen);
		if (readLen != dataLen)
		{
			ds_printf("DS_ERROR: Part of file wasn't loaded to memory\n");
			free(data);
			fs_close(pFile);
			return eFileFail;
		}

		if (memcmp(data, (uint8*)BIOS_FLASH_ADDR + dataPos, dataLen) != 0)
		{
			ret = eDataMissmatch;
			break;
		}
	}

	free(data);
	fs_close(pFile);

	return ret;
}

int BiosFlasher_WriteFromBiosToFile(const char* filename, BiosFlasher_OperationCallback guiClbk)
{
	uint8* data 	= 0;
	file_t pFile 	= 0;

	UPDATE_GUI(eReading, 0.0f, guiClbk);
	// Detect writible flash
	bflash_dev_t *dev = NULL;
	bflash_manufacturer_t *mrf = NULL;
	if (bflash_detect(&mrf, &dev) < 0) 
	{
		ds_printf("DS_ERROR: flash chip detection error.\n");
		return eDetectionFail;
	}

	// Read bios from file to memory
	pFile = fs_open(filename, O_WRONLY | O_TRUNC | O_CREAT);
	if (pFile == FILEHND_INVALID)
	{
		ds_printf("DS_ERROR: Can't open bios file: %s.\n", filename);
		return eFileFail;
	}

	size_t fileSize = dev->size * 1024;
	size_t offset = 0;
	if (fileSize >= settings.m_DataStart + settings.m_DataLength)
	{
		offset = settings.m_DataStart;
		fileSize = (settings.m_DataLength > 0) ? offset + settings.m_DataLength : fileSize - offset;
	}

	data = memalign(32, fileSize);
	if(!data) {
		ds_printf("DS_ERROR: Not enough memory\n");
		fs_close(pFile);
		return eUnknownFail;
	}
	
	/* 
	 * We can't use G1 bus simultaneously for IDE and BIOS, 
	 * so need copy BIOS data to RAM 
	 */
	memcpy(data, (uint8*)BIOS_FLASH_ADDR, fileSize);

	int i = 0;
	size_t chunkCount = fileSize / CHUNK_SIZE;
	for (i = 0; i <= chunkCount; ++i)
	{
		UPDATE_GUI(eReading, (float)i / chunkCount, guiClbk);

		size_t dataPos = i * CHUNK_SIZE + offset;
		size_t dataLen = (dataPos + CHUNK_SIZE > fileSize) ? fileSize - dataPos : CHUNK_SIZE;
		size_t writeLen = fs_write(pFile, data + dataPos, dataLen);
		if (writeLen != dataLen)
		{
			ds_printf("DS_ERROR: Can't write data to file\n");
			fs_close(pFile);
			free(data);
			return eFileFail;
		}
	}

	free(data);
	fs_close(pFile);

	return 0;
}

char* BiosFlasher_GenerateBackupName()
{
	char tmp[128];
	
	time_t t;
	struct tm* ti;
	time(&t);
	ti = localtime(&t);

	sprintf(tmp, "backup_%04d-%02d-%02d_%02d-%02d-%02d.bios", 
			1900 + ti->tm_year, 1 + ti->tm_mon, ti->tm_mday, 
			ti->tm_hour, ti->tm_min, ti->tm_sec);

	ds_printf("Backup filename: %s\n", tmp);

	char* filename = strdup(tmp);
	return filename;
}

// HELPERS
void* BiosFlasher_GetElement(const char *name, ListItemType type, int from) 
{
	Item_t *item;
	item = listGetItemByName(from ? self.m_App->elements : self.m_App->resources, name);

	if(item != NULL && item->type == type) {
		return item->data;
	}

	ds_printf("Resource not found: %s\n", name);
	return NULL;
}

GUI_Widget* BiosFlasher_GetWidget(const char* name)
{
	return (GUI_Widget *) BiosFlasher_GetElement(name, LIST_ITEM_GUI_WIDGET, 1);
}

int BiosFlasher_GetNumFilesCurrentDir()
{
	GUI_Widget *w = GUI_FileManagerGetItemPanel(self.filebrowser);
	return GUI_ContainerGetCount(w);
}

// HANDLERS
void BiosFlasher_ActivateScrollButtons(int currScrollPos, int maxScroll)
{
	GUI_WidgetSetEnabled(BiosFlasher_GetWidget("btn_scroll_left"), 0);
	GUI_WidgetSetEnabled(BiosFlasher_GetWidget("btn_scroll_right"), 0);
	if (currScrollPos > 0)
	{
		GUI_WidgetSetEnabled(BiosFlasher_GetWidget("btn_scroll_left"), 1);
	}
	if (currScrollPos < maxScroll)
	{
		GUI_WidgetSetEnabled(BiosFlasher_GetWidget("btn_scroll_right"), 1);
	}
}

void BiosFlasher_OnLeftScrollPressed(GUI_Widget *widget)
{
	if (--self.m_ScrollPos < 0) self.m_ScrollPos = 0;
	GUI_PanelSetYOffset(GUI_FileManagerGetItemPanel(self.filebrowser), self.m_ScrollPos * 300);
	BiosFlasher_ActivateScrollButtons(self.m_ScrollPos, self.m_MaxScroll);
}

void BiosFlasher_OnRightScrollPressed(GUI_Widget *widget)
{
	if (++self.m_ScrollPos > self.m_MaxScroll) self.m_ScrollPos = self.m_MaxScroll;
	GUI_PanelSetYOffset(GUI_FileManagerGetItemPanel(self.filebrowser), self.m_ScrollPos * 300);
	BiosFlasher_ActivateScrollButtons(self.m_ScrollPos, self.m_MaxScroll);
}

void BiosFlasher_ItemClick(dirent_fm_t *fm_ent) 
{
	dirent_t *ent = &fm_ent->ent;
	GUI_Widget *fmw = (GUI_Widget*)fm_ent->obj;
	
	if(ent->attr == O_DIR) 
	{
		GUI_FileManagerChangeDir(fmw, ent->name, ent->size);

		self.m_ScrollPos = 0;
		self.m_MaxScroll = BiosFlasher_GetNumFilesCurrentDir() / 10;
		GUI_PanelSetYOffset(GUI_FileManagerGetItemPanel(fmw), self.m_ScrollPos * 300);
		BiosFlasher_ActivateScrollButtons(self.m_ScrollPos, self.m_MaxScroll);

		return;
	}

	if (IsFileSupportedByApp(self.m_App, ent->name))
	{
		if(self.m_SelectedBiosFile) {
			free(self.m_SelectedBiosFile);
		}
		
		self.m_SelectedBiosFile = strdup(ent->name);
		
		int i;
		GUI_Widget *panel, *w;
		panel = GUI_FileManagerGetItemPanel(fmw);
		
		for(i = 0; i < GUI_ContainerGetCount(panel); i++) {
			
			w = GUI_FileManagerGetItem(fmw, i);
			
			if(i != fm_ent->index) {
				GUI_ButtonSetNormalImage(w, self.m_ItemNormal);
				GUI_ButtonSetHighlightImage(w, self.m_ItemNormal);
			} else {
				GUI_ButtonSetNormalImage(w, self.m_ItemSelected);
				GUI_ButtonSetHighlightImage(w, self.m_ItemSelected);
			}
		}
	}
}

void BiosFlasher_OnWritePressed(GUI_Widget *widget)
{
	self.m_CurrentOperation = eWriting;

	BiosFlasher_EnableChoseFilePage();
}

void BiosFlasher_OnReadPressed(GUI_Widget *widget)
{
	self.m_CurrentOperation = eReading;

	BiosFlasher_EnableChoseFilePage();
}

void BiosFlasher_OnComparePressed(GUI_Widget *widget)
{
	self.m_CurrentOperation = eCompare;

	BiosFlasher_EnableChoseFilePage();
}

void BiosFlasher_OnDetectPressed(GUI_Widget *widget)
{
	ScreenFadeOut();
	thd_sleep(200);
	BiosFlasher_EnableMainPage();	// Redetect bios again
	ScreenFadeIn();
}

void BiosFlasher_OnBackPressed(GUI_Widget *widget)
{
	BiosFlasher_EnableMainPage();
}

void BiosFlasher_OnSettingsPressed(GUI_Widget *widget)
{
	BiosFlasher_EnableSettingsPage();
}

void BiosFlasher_OnConfirmPressed(GUI_Widget *widget)
{
	if (self.m_SelectedBiosFile == 0 && 
		self.m_CurrentOperation != eReading)
	{
		ds_printf("Select bios file!\n");
		return;
	}

	BiosFlasher_EnableProgressPage();

	int result = eSuccess;
	char* fileName = 0;
	char filePath[MAX_FN_LEN];
	memset(filePath, 0, MAX_FN_LEN);

	switch(self.m_CurrentOperation)
	{
		case eWriting:
			snprintf(filePath, MAX_FN_LEN, "%s/%s", GUI_FileManagerGetPath(self.filebrowser), self.m_SelectedBiosFile);	// TODO
			result = BiosFlasher_WriteBiosFileToFlash(filePath, &BiosFlasher_OnOperationProgress);
		break;

		case eReading:
			fileName = BiosFlasher_GenerateBackupName();
			snprintf(filePath, MAX_FN_LEN, "%s/%s", GUI_FileManagerGetPath(self.filebrowser), fileName);	// TODO
			result = BiosFlasher_WriteFromBiosToFile(filePath, &BiosFlasher_OnOperationProgress);
		break;

		case eCompare:
			snprintf(filePath, MAX_FN_LEN, "%s/%s", GUI_FileManagerGetPath(self.filebrowser), self.m_SelectedBiosFile);	// TODO
			result = BiosFlasher_CompareBiosWithFile(filePath, &BiosFlasher_OnOperationProgress);
		break;

		case eNone:
		case eErasing:
		break;
	};
	
	BiosFlasher_EnableResultPage(result, fileName);
	free(fileName);
}

void BiosFlasher_OnSaveSettingsPressed(GUI_Widget *widget)
{
	const char* dataStart = GUI_TextEntryGetText(BiosFlasher_GetWidget("start_addresss"));
	settings.m_DataStart = atol(dataStart);
	const char* dataLength = GUI_TextEntryGetText(BiosFlasher_GetWidget("data_length"));
	settings.m_DataLength = atol(dataLength);

	// TODO: Validate start address and length here

	BiosFlasher_EnableMainPage();
}

void BiosFlasher_OnSupportedPressed(GUI_Widget *widget) {
	dsystem("bflash -l");
	ShowConsole();
}

void BiosFlasher_OnExitPressed(GUI_Widget *widget) {
	
	vuint32 * portac = (vuint32 *)PCTRA;
	*portac |= PCTRA_PBOUT(7);
	thd_sleep(100);
	*portac = PCTRA_PBINP(7);
	
	(void)widget;
	App_t *app = NULL;
	
	if(self.have_args == true) {
		
		app = GetAppByName("File Manager");
		
		if(!app || !(app->state & APP_STATE_LOADED)) {
			app = NULL;
		}
	}
	
	if(!app) {
		app = GetAppByName("Main");
	}
	
	OpenApp(app, NULL);
}

void BiosFlasher_OnOperationProgress(OperationState_t state, float progress)
{
	char msg[64];	// TODO: Check overflow
	memset(msg, 0, sizeof(msg));
	switch(state)
	{
		case eErasing:
			sprintf(msg, "Erasing: %d%%", (int)(progress * 100));
		break;

		case eWriting:
			sprintf(msg, "Writing: %d%%", (int)(progress * 100));
		break;

		case eReading:
			sprintf(msg, "Reading: %d%%", (int)(progress * 100));
		break;

		case eNone:
		case eCompare:
		break;
	};

	GUI_LabelSetText(self.m_LabelProgress, msg);
	GUI_ProgressBarSetPosition(self.m_ProgressBar, progress);
}

void BiosFlasher_EnableMainPage()
{
	
	if (GUI_CardStackGetIndex(self.pages) != 0)
	{
		ScreenFadeOut();
		thd_sleep(200);
		GUI_CardStackShowIndex(self.pages, 0);
		ScreenFadeIn();
	}

	bflash_dev_t* dev = BiosFlasher_DetectFlashChip();

	char info[64];
	sprintf(info, "Model: %s", dev->name); 
	GUI_LabelSetText(BiosFlasher_GetWidget("label_flash_name"), info);
	sprintf(info, "Size: %dKb", dev->size); 
	GUI_LabelSetText(BiosFlasher_GetWidget("label_flash_size"), info);
	sprintf(info, "Voltage: %dV", (dev->flags & F_FLASH_LOGIC_3V) ? 3 : 5); 
	GUI_LabelSetText(BiosFlasher_GetWidget("label_flash_voltage"), info);
	sprintf(info, "Access: %s", (dev->flags & F_FLASH_PROGRAM) ? "Read/Write" : "Read only"); 
	GUI_LabelSetText(BiosFlasher_GetWidget("label_flash_access"), info);
}

void BiosFlasher_EnableChoseFilePage()
{
	ScreenFadeOut();
	thd_sleep(200);
	GUI_CardStackShowIndex(self.pages, 1);
	ScreenFadeIn();

	self.m_ScrollPos = 0;
	self.m_MaxScroll = BiosFlasher_GetNumFilesCurrentDir() / 10;
	GUI_PanelSetYOffset(GUI_FileManagerGetItemPanel(self.filebrowser), self.m_ScrollPos * 300);
	BiosFlasher_ActivateScrollButtons(self.m_ScrollPos, self.m_MaxScroll);
}

void BiosFlasher_EnableProgressPage()
{
	ScreenFadeOut();
	thd_sleep(200);
	GUI_CardStackShowIndex(self.pages, 2);
	ScreenFadeIn();
}

void BiosFlasher_EnableResultPage(int result, const char* fileName)
{
	ScreenFadeOut();
	thd_sleep(200);
	GUI_CardStackShowIndex(self.pages, 3);
	ScreenFadeIn();
	
	char title[32];
	char msg[64];	// TODO: Check overflow
	memset(title, 0, sizeof(title));
	memset(msg, 0, sizeof(msg));
	switch(self.m_CurrentOperation)
	{
		case eWriting:
			if (result == eSuccess)
			{
				sprintf(title, "Done");
				sprintf(msg, "Writing successful");
			}
			else
			{
				sprintf(title, "Error");
				sprintf(msg, "Writing fail. Status code: %d", result);
			}
		break;

		case eReading:
			if (result == eSuccess)
			{
				sprintf(title, "Done");
				sprintf(msg, "Reading successful. File stored at file %s", fileName);
			}
			else
			{
				sprintf(title, "Error");
				sprintf(msg, "Reading fail. Status code: %d", result);
			}
		break;

		case eCompare:
			if (result == eSuccess)
			{
				sprintf(title, "Done");
				sprintf(msg, "Comare successful. Flash data match");
			}
			else if (result == eDataMissmatch)
			{
				sprintf(title, "Done");
				sprintf(msg, "Comare successful. Flash data missmatch");
			}
			else
			{
				sprintf(title, "Error");
				sprintf(msg, "Comaring fail. Status code: %d", result);
			}
		break;

		case eNone:
		case eErasing:
		break;
	};

	GUI_LabelSetText(BiosFlasher_GetWidget("result_title_text"), title);
	GUI_LabelSetText(BiosFlasher_GetWidget("result_desc_text"), msg);
}

void BiosFlasher_EnableSettingsPage()
{
	ScreenFadeOut();
	thd_sleep(200);
	GUI_CardStackShowIndex(self.pages, 4);
	ScreenFadeIn();
}

void BiosFlasher_Init(App_t* app) 
{

	int i;
	GUI_Widget *w;
	BiosFlasher_ResetSelf();
	self.m_App = app;
	
	vuint32 * portac = (vuint32 *)PCTRA;
	*portac |= PCTRA_PBOUT(7);
	thd_sleep(100);
	*portac = PCTRA_PBINP(7);
	
	if (self.m_App != 0)
	{
		self.m_LabelProgress		= (GUI_Widget *) BiosFlasher_GetElement("progress_text", LIST_ITEM_GUI_WIDGET, 1);
		self.m_LabelProgressDesc = (GUI_Widget *) BiosFlasher_GetElement("progress_desc", LIST_ITEM_GUI_WIDGET, 1);
		self.m_ProgressBar		= (GUI_Widget *) BiosFlasher_GetElement("progressbar", LIST_ITEM_GUI_WIDGET, 1);
		self.filebrowser			= (GUI_Widget *) BiosFlasher_GetElement("file_browser", LIST_ITEM_GUI_WIDGET, 1);
		self.pages					= (GUI_Widget *) BiosFlasher_GetElement("pages", LIST_ITEM_GUI_WIDGET, 1);
		
		self.m_ItemNormal			= (GUI_Surface *) BiosFlasher_GetElement("item-normal", LIST_ITEM_GUI_SURFACE, 0);
		self.m_ItemSelected		= (GUI_Surface *) BiosFlasher_GetElement("item-selected", LIST_ITEM_GUI_SURFACE, 0);
		
		/* Disabling scrollbar on filemanager */
		for(i = 3; i > 0; i--) {
			w = GUI_ContainerGetChild(self.filebrowser, i);
			GUI_ContainerRemove(self.filebrowser, w);
		}

		if (app->args != 0)
		{
			char *name = getFilePath(app->args);
			self.have_args = true;
			
			if(name) {
				
				GUI_FileManagerSetPath(self.filebrowser, name);
				free(name);
				name = strrchr(app->args, '/');
				
				if(name) {
					name++;
					self.m_SelectedBiosFile = strdup(name);
				}
			}
			
			BiosFlasher_OnWritePressed(NULL);
		}
		else
		{
			self.have_args = false;
			BiosFlasher_EnableMainPage();
		}
	}
	else 
	{
		ds_printf("DS_ERROR: Can't find app named: %s\n", "BiosFlasher");
	}
}

