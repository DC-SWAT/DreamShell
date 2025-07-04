/* DreamShell ##version##

   module.c - VMU Manager app module
   Copyright (C)2014-2015 megavolt85
   Copyright (C)2024 SWAT

*/

#include "ds.h"
#include "fs_vmd.h"
#include <stdbool.h>

DEFAULT_MODULE_EXPORTS(app_vmu_manager);

#define MAPLE_FUNC_GUN     0x81000000
#define PACK_NYBBLE_RGB565(nybble) ((((nybble & 0x0f00)>>8)*2)<<11) + \
	((((nybble & 0x00f0)>>4)*4)<<5) + ((((nybble & 0x000f)>>0)*2)<<0)

#define VMU_ICON_WIDTH  32
#define VMU_ICON_HEIGHT 32
#define VMU_ICON_SIZE   (VMU_ICON_WIDTH * VMU_ICON_HEIGHT * 2)
#define LEFT_FM  0
#define RIGHT_FM  1

typedef enum {
	DS_VMS = 0,
	DS_VMD,
	DS_VMI,
	DS_DCI
} save_type_t;

typedef struct vm_root {
	uint16_t size;
	uint16_t partition;
	uint16_t sys_block;
	uint16_t fat_block;
	uint16_t fat_cnt;
	uint16_t file_info_block;
	uint16_t file_info_cnt;
	uint8_t  vol_icon;
	uint8_t  reserved;
	uint16_t extra_block;
	uint16_t extra_cnt;
	uint16_t exe_block;
	uint16_t exe_cnt;
} vm_root_t;

void VMU_Manager_ItemContextClick(dirent_fm_t *fm_ent);
int VMU_Manager_Dump(GUI_Widget *widget);
void VMU_Manager_addfileman(GUI_Widget *widget);

static struct {
	App_t* m_App;
	GUI_Widget *pages;
	GUI_Widget *button_dump;
	GUI_Widget *button_home;
	GUI_Widget *cd_c;
	GUI_Widget *sd_c;
	GUI_Widget *hdd_c;
	GUI_Widget *pc_c;
	GUI_Widget *format_c;
	GUI_Widget *dst_vmu;
	GUI_Widget *vmu[4][2];
	GUI_Widget *img_cont[4];
	GUI_Widget *name_device;
	GUI_Widget *free_mem;
	GUI_Widget *filebrowser;
	GUI_Widget *filebrowser2;
	GUI_Widget *sicon;
	GUI_Widget *save_name;
	GUI_Widget *save_size;
	GUI_Widget *save_descshort;
	GUI_Widget *save_desclong;
	GUI_Widget *progressbar_container;
	GUI_Widget *progressbar;
	GUI_Widget *folder_name;
	GUI_Surface *progres_img;
	GUI_Surface *progres_img_b;
	GUI_Surface *confirmimg[3];
	GUI_Surface *m_ItemNormal;
	GUI_Surface *m_ItemSelected;
	GUI_Surface *m_ItemNormal2;
	GUI_Surface *m_ItemSelected2;
	GUI_Surface *logo;
	GUI_Surface *dump_icon;
	GUI_Surface *vmuicon;
	SDL_Surface *vmu_icon;

	GUI_Surface *controller;
	GUI_Surface *treamcast;
	GUI_Surface *arcade;
	GUI_Surface *asciipad;
	GUI_Surface *whell;
	GUI_Surface *fishrod;
	GUI_Surface *twin;
	GUI_Surface *maracas;
	GUI_Surface *popnmusic;
	GUI_Surface *missionstick;
	GUI_Surface *panther;
	GUI_Surface *densha;
	GUI_Surface *x360;
	GUI_Surface *psx;
	GUI_Surface *lightgun;
	GUI_Surface *lightgunus;
	GUI_Surface *treamcastgun;
	GUI_Surface *keyboard;
	GUI_Surface *keyboardjp;
	GUI_Surface *mouse;
	GUI_Surface *dreameye;
	GUI_Surface *mic;
	GUI_Surface *dreameyemic;
	GUI_Surface *vibro_pack;
	GUI_Surface *vmu_d;

	GUI_Widget *vmu_page;
	GUI_Widget *vmu_container;

	GUI_Widget *confirm;
	GUI_Widget *image_confirm;
	GUI_Widget *confirm_text;
	GUI_Widget *drection;

	kthread_t *thd;
	int thread_kill;
	bool have_args;

	int vmu_freeblock;
	int vmu_freeblock2;
	int direction_flag;
	char* home_path;
	char* m_SelectedFile;
	char* m_SelectedPath;
	char desc_short[17];
	char desc_long[33];
} self;

static struct {
	uint32 checksum;
	char title[32];
	char creator[32];
	char date[8];
	char version[2];
	char numfiles[2];
	char source[8];
	char name[12];
	uint16 type;
	char reserved[2];
	uint32 size;
} vmi_t;

static struct {
	char type[1];
	uint8 copyprotect;
	uint16 firstblock;
	char name[12];
	uint16 year;
	uint8 month;
	uint8 day;
	uint8 hours;
	uint8 mins;
	uint8 secs;
	char weekday[1];
	uint16 size; // size in blocks
	uint16 headeroffset;
	uint32 unused;
	uint8 vmsheader[1024];
} dci_t;

static void* vmu_dev(const char* path) {

	int port = path[5] - 'A';
	int slot = path[6] - '0';
	
	if (port < 0 || port > 3 || slot < 1 || slot > 2) {
		return NULL;
	}
	
	return maple_enum_dev(port, slot);
}

static void rmdir_recursive(const char* folder) {

	file_t d;
	dirent_t *de;
	char dst[NAME_MAX];

	d = fs_open(folder, O_DIR);

	while ((de = fs_readdir(d))) {
		if (strcmp(de->name ,".") == 0 || strcmp(de->name ,"..") == 0) {
			continue;
		}
		
		snprintf(dst, sizeof(dst), "%s/%s", folder, de->name);

		if (de->attr == O_DIR) {
			rmdir_recursive(dst);
		}
		else {
			fs_unlink(dst);
		}
	}
	
	fs_close(d);
	fs_rmdir(folder);
}

static void free_blocks(const char *path , int n) {
	maple_device_t *vmucur = vmu_dev(path);

	if(vmucur == NULL) {
		return;
	}

	if(n == 0) {
		self.vmu_freeblock = vmufs_free_blocks(vmucur);
	}
	else {
		self.vmu_freeblock2 = vmufs_free_blocks(vmucur);
	}
}

static void addbutton() {
	GUI_WidgetSetEnabled(self.button_dump, 0);
	GUI_ContainerRemove(self.vmu_page, self.filebrowser2);
	
	if(!DirExists("/pc")) GUI_WidgetSetEnabled(self.pc_c, 0);
	if(!DirExists("/sd")) GUI_WidgetSetEnabled(self.sd_c, 0);
	if(!DirExists("/ide")) GUI_WidgetSetEnabled(self.hdd_c, 0);	
	
	self.direction_flag = 0;
	
	GUI_ContainerAdd(self.vmu_page, self.sd_c);
	GUI_ContainerAdd(self.vmu_page, self.hdd_c);
	GUI_ContainerAdd(self.vmu_page, self.cd_c);
	GUI_ContainerAdd(self.vmu_page, self.pc_c);	
	GUI_ContainerAdd(self.vmu_page, self.dst_vmu);
	GUI_ContainerAdd(self.vmu_page, self.format_c);
	GUI_WidgetMarkChanged(self.m_App->body); 
}

static void disable_high(int fm)
{	
	int i;
	GUI_Widget *panel, *w;

	if(fm == RIGHT_FM) {
		panel = GUI_FileManagerGetItemPanel(self.filebrowser2);
		
		for(i = 0; i < GUI_ContainerGetCount(panel); i++) {
			w = GUI_FileManagerGetItem(self.filebrowser2, i);
			GUI_ButtonSetNormalImage(w, self.m_ItemNormal2);
			GUI_ButtonSetHighlightImage(w, self.m_ItemNormal2);	
		}
	}
	else {	
		panel = GUI_FileManagerGetItemPanel(self.filebrowser);
		
		for(i = 0; i < GUI_ContainerGetCount(panel); i++) {
			w = GUI_FileManagerGetItem(self.filebrowser, i);
			GUI_ButtonSetNormalImage(w, self.m_ItemNormal);
			GUI_ButtonSetHighlightImage(w, self.m_ItemNormal);	
		}			
	}
}

static void clr_statusbar() {
	GUI_PictureSetImage(self.sicon, self.logo);
	GUI_LabelSetText(self.save_name, "   ");
	GUI_LabelSetText(self.save_size, "   ");
	GUI_LabelSetText(self.save_descshort, "   ");
	GUI_LabelSetText(self.save_desclong, "   ");	
}

static int Confirm_Window() {
	maple_device_t *controller_dev;
	maple_device_t *mouse_dev;
	int y, rv = CMD_ERROR;

	SDL_GetMouseState(NULL, &y);
	SDL_WarpMouse(0,0);
	GUI_ContainerAdd(self.vmu_page, self.confirm);
	GUI_WidgetMarkChanged(self.vmu_page);

	while(1) {
		mouse_dev = maple_enum_type(0, MAPLE_FUNC_MOUSE);
		controller_dev = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);

		if(mouse_dev) {
			mouse_cond_t *state = (mouse_cond_t *)maple_dev_status(mouse_dev);

			if(state) {
				if(state->buttons & MOUSE_LEFTBUTTON) {
					rv = CMD_OK;
					break;
				}
				else if(state->buttons & MOUSE_RIGHTBUTTON) {
					break;
				}
				else if(state->buttons & MOUSE_SIDEBUTTON) {
					rv = CMD_NO_ARG;
					break;
				}
			}
		}

		if(controller_dev) {
			cont_state_t *state = (cont_state_t *)maple_dev_status(controller_dev);

			if(state) {
				if(state->buttons & CONT_A) {
					rv = CMD_OK;
					break;
				}
				else if(state->buttons & CONT_B) {
					break;
				}
				else if(state->buttons & CONT_X) {
					rv = CMD_NO_ARG;
					break;
				}
			}
		}

		if(!mouse_dev && !controller_dev) {
			continue;
		}

		thd_sleep(50);
	}

	GUI_ContainerRemove(self.vmu_page, self.confirm);	
	GUI_WidgetMarkChanged(self.m_App->body);

	if (y > 320) y = 300;
	SDL_WarpMouse(270, y);
	UnlockVideo();

	return rv;
}

static void show_slots(int port) {
	int slot;
	maple_device_t *dev;

	for(slot = 1; slot < 3; ++slot) {
		dev = maple_enum_dev(port, slot);

		if (dev == NULL) {
			GUI_ButtonSetDisabledImage(self.vmu[port][slot - 1], self.vmu_d);
			GUI_WidgetSetEnabled(self.vmu[port][slot - 1], 0);
		}
		else if(dev->info.functions & MAPLE_FUNC_MEMCARD) {
			GUI_ButtonSetDisabledImage(self.vmu[port][slot - 1], self.vmu_d);
			GUI_WidgetSetEnabled(self.vmu[port][slot - 1], 1);
		}
		//replace inactive memory card slot with microphone if installed
		else if(dev->info.functions & MAPLE_FUNC_MICROPHONE) {
			//check if this is Dreameye microphone or not
			GUI_ButtonSetDisabledImage(self.vmu[port][slot - 1], !strncmp(dev->info.product_name, "MicDevice for Dreameye", 22) ? self.dreameyemic : self.mic);
			GUI_WidgetSetEnabled(self.vmu[port][slot - 1], 0);
		}
		//replace inactive memory card slot with vibration pack if installed
		else if(dev->info.functions & MAPLE_FUNC_PURUPURU) {
			GUI_ButtonSetDisabledImage(self.vmu[port][slot - 1], self.vibro_pack);
			GUI_WidgetSetEnabled(self.vmu[port][slot - 1], 0);
		}
	}
}

static void dev_widget_set_img(GUI_Widget *w, GUI_Surface *s) {
	GUI_ProgressBarSetImage2(w, s);
	GUI_ProgressBarSetPosition(w, 1.0);
}

static void show_port(int port, maple_device_t *dev) {
	if(!dev) {
		GUI_ProgressBarSetPosition(self.img_cont[port], 0.0);	
		GUI_WidgetSetEnabled(self.vmu[port][0], 0);
		GUI_WidgetSetEnabled(self.vmu[port][1], 0);
		//return normal inactive slots when controller disconnect
		GUI_ButtonSetDisabledImage(self.vmu[port][0], self.vmu_d);
		GUI_ButtonSetDisabledImage(self.vmu[port][1], self.vmu_d);
		return;
	}
	
	uint32_t functions = dev->info.functions;
	char *name = dev->info.product_name;
	
	if(functions & (MAPLE_FUNC_LIGHTGUN | MAPLE_FUNC_ARGUN)) {
        if (dev->info.standby_power == 0xA0 && dev->info.max_power == 0xFA) {
            // Treamcast gun
            dev_widget_set_img(self.img_cont[port], self.treamcastgun);
        }
        else if (dev->info.area_code == 1) {
            // NTSC-U or 3rd party lightgun
            dev_widget_set_img(self.img_cont[port], self.lightgunus);
        }
        else {
            // NTSC-J or PAL lightgun
            dev_widget_set_img(self.img_cont[port], self.lightgun);
        }
    }
    else if(functions & MAPLE_FUNC_KEYBOARD) {
        if (dev->info.area_code == 2) {
            dev_widget_set_img(self.img_cont[port], self.keyboardjp);
        }
        else {
            dev_widget_set_img(self.img_cont[port], self.keyboard);
        }
    }
	else if(functions & MAPLE_FUNC_MOUSE) {
		dev_widget_set_img(self.img_cont[port], self.mouse);
	}
	else if(functions & MAPLE_FUNC_CONTROLLER) {
		if (!strncmp(name, "Arcade Stick", 12)) {
			dev_widget_set_img(self.img_cont[port], self.arcade);
		}
		else if (!strncmp(name, "ASCII STICK", 11)) {
			// six button controller, megadrive style
			dev_widget_set_img(self.img_cont[port], self.asciipad);
		}
		else if (!strncmp(name, "Racing Controller", 17)) {
			// racing whell
			dev_widget_set_img(self.img_cont[port], self.whell);
		}
		else if (!strncmp(name, "Dreamcast Fishing Controller", 28)) {
			// fishing rod
			dev_widget_set_img(self.img_cont[port], self.fishrod);
		}
		else if (!strncmp(name, "Twin Stick", 10)) {
			dev_widget_set_img(self.img_cont[port], self.twin);
		}
		else if (!strncmp(name, "Maracas Controller", 18)) {
			dev_widget_set_img(self.img_cont[port], self.maracas);
		}
		else if (!strncmp(name, "pop'n music controller", 22)) {
			// pop'n music or dance mat
			dev_widget_set_img(self.img_cont[port], self.popnmusic);
		}
		else if (!strncmp(name, "ASCII ANALOG STICK", 18)) {
			// Ascii Mission Stick
			dev_widget_set_img(self.img_cont[port], self.missionstick);
		}
		else if (!strncmp(name, "TAITO 001 Controller", 20)) {
			// Densha de Go!! Controller
			dev_widget_set_img(self.img_cont[port], self.densha);
		}
		else if (!strncmp(name, "XBOX360 Controller", 18)) {
            // usb4maple with x360 controller
            dev_widget_set_img(self.img_cont[port], self.x360);
        }
        else if (!strncmp(name, "PlayStation", 11)) {
            // usb4maple with PS controller
            dev_widget_set_img(self.img_cont[port], self.psx);
        }
		else if (!strncmp(name, "Dreamcast Camera", 16)) {
			// DreamEYE
			dev_widget_set_img(self.img_cont[port], self.dreameye);
		}
		else if (dev->info.function_data[1] == 0x400 && !strncmp(name, "Dreamcast Controller", 20)) {
            dev_widget_set_img(self.img_cont[port], self.treamcast);
        }
        else {
            dev_widget_set_img(self.img_cont[port], self.controller);
        }
	}
	
	show_slots(port);
}

static void *maple_scan(void *arg)
{
	(void)arg;
	self.thread_kill = 0;
	int port;
	maple_device_t *dev;

	while(self.thread_kill == 0) {
		for(port = 0; port < 4; ++port) {

			dev = maple_enum_dev(port, 0);
			show_port(port, dev);
		}
		
		if(GUI_CardStackGetIndex(self.pages) != 0) {
			break;
		}
		
		thd_sleep(500);
	}
	return NULL;
}

static void* GetElement(const char *name, ListItemType type, int from)
{
	Item_t *item;
	item = listGetItemByName(from ? self.m_App->elements : self.m_App->resources, name);

	if(item != NULL && item->type == type) {
		return item->data;
	}

	dbgio_printf("Resource not found: %s\n", name);
	return NULL;
}

void Vmu_Manager_Init(App_t* app)
{
	self.m_App = app;

	if (self.m_App == NULL) {
		ds_printf("DS_ERROR: Can't find app named: %s\n", "Vmu Manager");
		return;
	}

	int x,y;
	self.direction_flag = 0;
	self.pages	= (GUI_Widget *) GetElement("pages", LIST_ITEM_GUI_WIDGET, 1);
	self.button_home = (GUI_Widget *) GetElement("home_but", LIST_ITEM_GUI_WIDGET, 1);
	self.cd_c  = (GUI_Widget *) GetElement("/cd", LIST_ITEM_GUI_WIDGET, 1);
	self.sd_c  = (GUI_Widget *) GetElement("/sd", LIST_ITEM_GUI_WIDGET, 1);
	self.hdd_c = (GUI_Widget *) GetElement("/ide", LIST_ITEM_GUI_WIDGET, 1);
	self.pc_c  = (GUI_Widget *) GetElement("/pc", LIST_ITEM_GUI_WIDGET, 1);
	self.format_c  = (GUI_Widget *) GetElement("format-c", LIST_ITEM_GUI_WIDGET, 1);
	self.dst_vmu  = (GUI_Widget *) GetElement("dst-vmu", LIST_ITEM_GUI_WIDGET, 1);
	self.vmu_container  = (GUI_Widget *) GetElement("vmu-container", LIST_ITEM_GUI_WIDGET, 1);
	self.folder_name  = (GUI_Widget *) GetElement("folder-name", LIST_ITEM_GUI_WIDGET, 1);

	self.vmu[0][0]  = (GUI_Widget *) GetElement("A1", LIST_ITEM_GUI_WIDGET, 1);
	self.vmu[0][1]  = (GUI_Widget *) GetElement("A2", LIST_ITEM_GUI_WIDGET, 1);
	self.vmu[1][0]  = (GUI_Widget *) GetElement("B1", LIST_ITEM_GUI_WIDGET, 1);
	self.vmu[1][1]  = (GUI_Widget *) GetElement("B2", LIST_ITEM_GUI_WIDGET, 1);
	self.vmu[2][0]  = (GUI_Widget *) GetElement("C1", LIST_ITEM_GUI_WIDGET, 1);
	self.vmu[2][1]  = (GUI_Widget *) GetElement("C2", LIST_ITEM_GUI_WIDGET, 1);
	self.vmu[3][0]  = (GUI_Widget *) GetElement("D1", LIST_ITEM_GUI_WIDGET, 1);
	self.vmu[3][1]  = (GUI_Widget *) GetElement("D2", LIST_ITEM_GUI_WIDGET, 1);

	self.img_cont[0] = (GUI_Widget *) GetElement("contA", LIST_ITEM_GUI_WIDGET, 1);
	self.img_cont[1] = (GUI_Widget *) GetElement("contB", LIST_ITEM_GUI_WIDGET, 1);
	self.img_cont[2] = (GUI_Widget *) GetElement("contC", LIST_ITEM_GUI_WIDGET, 1);
	self.img_cont[3] = (GUI_Widget *) GetElement("contD", LIST_ITEM_GUI_WIDGET, 1);

	self.save_name = (GUI_Widget *) GetElement("save-name", LIST_ITEM_GUI_WIDGET, 1);
	self.save_size = (GUI_Widget *) GetElement("save-size", LIST_ITEM_GUI_WIDGET, 1);
	self.save_descshort = (GUI_Widget *) GetElement("desc-short", LIST_ITEM_GUI_WIDGET, 1);
	self.save_desclong = (GUI_Widget *) GetElement("desc-long", LIST_ITEM_GUI_WIDGET, 1);

	self.name_device = (GUI_Widget *) GetElement("name-device", LIST_ITEM_GUI_WIDGET, 1);
	self.free_mem = (GUI_Widget *) GetElement("free-mem", LIST_ITEM_GUI_WIDGET, 1);

	self.sicon = (GUI_Widget *) GetElement("vmu-icon", LIST_ITEM_GUI_WIDGET, 1);
	self.button_dump = (GUI_Widget *) GetElement("dump-button", LIST_ITEM_GUI_WIDGET, 1);

	self.filebrowser = (GUI_Widget *) GetElement("file_browser", LIST_ITEM_GUI_WIDGET, 1);
	self.filebrowser2 = (GUI_Widget *) GetElement("file_browser2", LIST_ITEM_GUI_WIDGET, 1);
	self.m_ItemNormal = (GUI_Surface *) GetElement("item-normal", LIST_ITEM_GUI_SURFACE, 0);
	self.m_ItemSelected	= (GUI_Surface *) GetElement("item-selected", LIST_ITEM_GUI_SURFACE, 0);
	self.m_ItemNormal2 = (GUI_Surface *) GetElement("item-normal2", LIST_ITEM_GUI_SURFACE, 0);
	self.m_ItemSelected2	= (GUI_Surface *) GetElement("item-selected2", LIST_ITEM_GUI_SURFACE, 0);
	self.logo = (GUI_Surface *) GetElement("logo", LIST_ITEM_GUI_SURFACE, 0);
	self.dump_icon = (GUI_Surface *) GetElement("dump_icon", LIST_ITEM_GUI_SURFACE, 0);
	self.controller = (GUI_Surface *) GetElement("controller", LIST_ITEM_GUI_SURFACE, 0);
	self.treamcast = (GUI_Surface *) GetElement("treamcast", LIST_ITEM_GUI_SURFACE, 0);
	self.arcade = (GUI_Surface *) GetElement("arcadestick", LIST_ITEM_GUI_SURFACE, 0);
	self.asciipad = (GUI_Surface *) GetElement("asciipad", LIST_ITEM_GUI_SURFACE, 0);
	self.densha = (GUI_Surface *) GetElement("densha", LIST_ITEM_GUI_SURFACE, 0);
	self.fishrod = (GUI_Surface *) GetElement("fishingrod", LIST_ITEM_GUI_SURFACE, 0);
	self.maracas = (GUI_Surface *) GetElement("maracas", LIST_ITEM_GUI_SURFACE, 0);
	self.missionstick = (GUI_Surface *) GetElement("missionstick", LIST_ITEM_GUI_SURFACE, 0);
	self.panther = (GUI_Surface *) GetElement("pantherdc", LIST_ITEM_GUI_SURFACE, 0);
	self.popnmusic = (GUI_Surface *) GetElement("popnmusic", LIST_ITEM_GUI_SURFACE, 0);
	self.twin = (GUI_Surface *) GetElement("twinstick", LIST_ITEM_GUI_SURFACE, 0);
	self.whell = (GUI_Surface *) GetElement("whell", LIST_ITEM_GUI_SURFACE, 0);
	self.x360 = (GUI_Surface *) GetElement("x360", LIST_ITEM_GUI_SURFACE, 0);
	self.psx = (GUI_Surface *) GetElement("psx", LIST_ITEM_GUI_SURFACE, 0);
	self.lightgun = (GUI_Surface *) GetElement("lightgun", LIST_ITEM_GUI_SURFACE, 0);
	self.lightgunus = (GUI_Surface *) GetElement("lightgunus", LIST_ITEM_GUI_SURFACE, 0);
	self.treamcastgun = (GUI_Surface *) GetElement("treamcastgun", LIST_ITEM_GUI_SURFACE, 0);
	self.keyboard = (GUI_Surface *) GetElement("keyboard", LIST_ITEM_GUI_SURFACE, 0);
	self.keyboardjp = (GUI_Surface *) GetElement("keyboardjp", LIST_ITEM_GUI_SURFACE, 0);
	self.mouse = (GUI_Surface *) GetElement("mouse", LIST_ITEM_GUI_SURFACE, 0);
	self.dreameye = (GUI_Surface *) GetElement("dreameye", LIST_ITEM_GUI_SURFACE, 0);
	self.mic = (GUI_Surface *) GetElement("mic", LIST_ITEM_GUI_SURFACE, 0);
	self.dreameyemic = (GUI_Surface *) GetElement("dreameyemic", LIST_ITEM_GUI_SURFACE, 0);
	self.vibro_pack = (GUI_Surface *) GetElement("vibro_pack", LIST_ITEM_GUI_SURFACE, 0);
	self.vmu_d = (GUI_Surface *) GetElement("vmu_d", LIST_ITEM_GUI_SURFACE, 0);

	self.progres_img = (GUI_Surface *) GetElement("progressbar", LIST_ITEM_GUI_SURFACE, 0);
	self.progres_img_b = (GUI_Surface *) GetElement("progressbar_back", LIST_ITEM_GUI_SURFACE, 0);

	self.confirmimg[0] = (GUI_Surface *) GetElement("confirmimg", LIST_ITEM_GUI_SURFACE, 0);
	self.confirmimg[1] = (GUI_Surface *) GetElement("confirmimg0", LIST_ITEM_GUI_SURFACE, 0);
	self.image_confirm = (GUI_Widget *) GetElement("image-confirm", LIST_ITEM_GUI_WIDGET, 1);
	self.confirm = (GUI_Widget *) GetElement("confirm", LIST_ITEM_GUI_WIDGET, 1);
	self.confirm_text = (GUI_Widget *) GetElement("confirm-text", LIST_ITEM_GUI_WIDGET, 1);

	self.drection = (GUI_Widget *) GetElement("drection", LIST_ITEM_GUI_WIDGET, 1);

	self.progressbar = (GUI_Widget *) GetElement("progressbar", LIST_ITEM_GUI_WIDGET, 1);
	self.progressbar_container = (GUI_Widget *) GetElement("progressbar_container", LIST_ITEM_GUI_WIDGET, 1);

	Item_t *i;
	i = listGetItemByName(self.m_App->elements, "vmu_page");
	self.vmu_page = (GUI_Widget*) i->data;

	GUI_FileManagerSetItemContextClick(self.filebrowser, (GUI_CallbackFunction*) VMU_Manager_ItemContextClick);
	GUI_FileManagerSetItemContextClick(self.filebrowser2, (GUI_CallbackFunction*) VMU_Manager_ItemContextClick);
	
	GUI_ContainerRemove(self.vmu_page, self.progressbar_container);
	GUI_ContainerRemove(self.vmu_page, self.filebrowser2);
	GUI_ContainerRemove(self.vmu_page, self.confirm);

	if(!DirExists("/pc")) GUI_WidgetSetEnabled(self.pc_c, 0);
	if(!DirExists("/sd")) GUI_WidgetSetEnabled(self.sd_c, 0);
	if(!DirExists("/ide")) GUI_WidgetSetEnabled(self.hdd_c, 0);

	GUI_WidgetSetEnabled(self.button_dump, 0);

	for(x = 0; x < 4; ++x)
	{
		for(y = 0; y < 2; ++y)
		{
			GUI_WidgetSetEnabled(self.vmu[x][y], 0);
		}
	}

	/* Disabling scrollbar for file browsers */
	GUI_FileManagerRemoveScrollbar(self.filebrowser);
	GUI_FileManagerRemoveScrollbar(self.filebrowser2);

	if (GUI_CardStackGetIndex(self.pages) == 0) {
		GUI_WidgetSetEnabled(self.button_home, 0);
	}

	self.m_SelectedFile = NULL;
	self.m_SelectedPath = NULL;
	self.home_path = NULL;
	self.thd = thd_create(1, maple_scan, NULL);
	fs_vmd_init();

	if (app->args != 0)
	{
		/* TODO
		char *path = getFilePath(app->args);
		if (path) {
			GUI_FileManagerSetPath(self.filebrowser2, path);
			free(path);
		}
		*/
		self.have_args = true;
	}
	else {
		self.have_args = false;
	}
}

static void reset_selected() {
	if(self.m_SelectedFile != NULL) {
		free(self.m_SelectedFile);
		free(self.m_SelectedPath);
		self.m_SelectedFile = NULL;
		self.m_SelectedPath = NULL;
	}
}

void VMU_Manager_EnableMainPage() {
	int x, y;

	self.thread_kill = 1;

	for(x = 0; x < 4; ++x)
	{
		for(y = 0; y < 2; ++y)
		{
			if(GUI_ContainerContains(self.vmu_container, self.vmu[x][y]) == 0){
				GUI_ContainerAdd(self.vmu_container,self.vmu[x][y]);
			}
		}
	}

	reset_selected();
	disable_high(RIGHT_FM);
	disable_high(LEFT_FM);
	clr_statusbar();
	self.direction_flag = 0;
	GUI_LabelSetText(self.drection, "SELECT SOURCE VMU");
	GUI_WidgetSetEnabled(self.button_home, 0);
	ScreenFadeOutEx(NULL, 1);
	GUI_CardStackShowIndex(self.pages, 0);
	ScreenFadeIn();
	self.thd = thd_create(1, maple_scan, NULL);
}

void VMU_Manager_vmu(GUI_Widget *widget) {
	char vpath[NAME_MAX];

	ScreenFadeOutEx(NULL, 1);
	GUI_CardStackShowIndex(self.pages, 1);
	GUI_WidgetSetEnabled(self.button_home, 1);

	snprintf(vpath, NAME_MAX, "/vmu/%s", GUI_ObjectGetName(widget));

	if(self.direction_flag == 0) {
		GUI_FileManagerSetPath(self.filebrowser, vpath);
		GUI_ContainerRemove(self.vmu_container, widget);
		addbutton();
	}
	else {
		GUI_WidgetSetEnabled(self.button_dump, 0);
		VMU_Manager_addfileman(widget);	
	}
	
	ScreenFadeIn();
}

void VMU_Manager_info_bar(GUI_Widget *widget) {
	char str[16], path[16];

	GUI_LabelSetText(self.name_device, GUI_ObjectGetName( (GUI_Object *)widget));
	snprintf(path, sizeof(path), "/vmu/%s", GUI_ObjectGetName((GUI_Object *) widget));

	if(self.direction_flag == 0) {
		self.vmu_freeblock = vmufs_free_blocks(vmu_dev(path));
		snprintf(str, sizeof(str), "%s %d %s", "free", self.vmu_freeblock, "blocks");
	}
	else {
		self.vmu_freeblock2 = vmufs_free_blocks(vmu_dev(path));
		snprintf(str, sizeof(str), "%s %d %s", "free", self.vmu_freeblock2, "blocks");
	}

	if(self.vmu_freeblock >= 0) {
		GUI_LabelSetText(self.free_mem, str);
	}
}

void VMU_Manager_info_bar_clr(GUI_Widget *widget) {
	GUI_LabelSetText(self.name_device, "");
	GUI_LabelSetText(self.free_mem, "");
	
#ifdef VMDEBUR
	dbgio_printf("onmouseout");
#endif
}

void VMU_Manager_Exit(GUI_Widget *widget) {
	(void)widget;
	App_t *app = NULL;

	self.thread_kill = 1;
	thd_join(self.thd, NULL);
	fs_vmd_shutdown();

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

static void copy_save(const char *src_fn, const char *dest_fn) {
	size_t cnt, size, cur = 0, buf_size;
	file_t src_fd, dest_fd;
	uint8_t *buff;
	int src_flag = O_RDONLY, dst_flag = O_WRONLY | O_CREAT | O_TRUNC;
	
	if (!strncmp(src_fn, "/vmu", 4)) {
		src_flag |= O_META;
	}
	
	if (!strncmp(dest_fn, "/vmu", 4)) {
		dst_flag |= O_META;
	}
	
	src_fd = fs_open(src_fn, src_flag);
	
	if (src_fd == FILEHND_INVALID) {
		ds_printf("DS_ERROR: Can't open %s for read\n", src_fn);
		return;
	}
	
	dest_fd = fs_open(dest_fn, dst_flag);
	
	if (dest_fd == FILEHND_INVALID) {
		ds_printf("DS_ERROR: Can't open %s for write\n", dest_fn);
		fs_close(src_fd);
		return;
	}
	
	size = fs_total(src_fd) / 1024;

	if(size >= 256) {
		buf_size = 32 * 1024;
	} else if(size < 32 && size > 8) {
		buf_size = 1024;
	} else if(size <= 8) {
		buf_size = 512;
	} else {
		buf_size = 16 * 1024;
	}

	buff = (uint8 *) memalign(32, buf_size);

	if(buff == NULL) {
		ds_printf("DS_ERROR: No memory: %d bytes\n", buf_size); 
		return;
	}

	while ((cnt = fs_read(src_fd, buff, buf_size)) > 0) {
		
		cur += cnt;
		
		if(fs_write(dest_fd, buff, cnt) < 0) {
			break;
		}
	}

	fs_close(dest_fd);
	fs_close(src_fd);
	free(buff);
}

void VMU_Manager_ItemClick(dirent_fm_t *fm_ent) {
	
	dirent_t *ent = &fm_ent->ent;
	file_t f;
	int xx, yy;
	int flag = CMD_ERROR;
	save_type_t flag_type = DS_VMS; // vms 0 ; vmd 1 ; vmi 2 ; dci 3
	uint8 nyb;
	static char tmp[64];
	static char src[NAME_MAX];
	static char dst[NAME_MAX];
	static char size[64];
	static char text[1024];
	uint16 *tmpbuf = NULL;
	static uint8_t buf[1024];
	static uint8_t icon[512];
	static uint16_t pal[16];
	GUI_Widget *fmw = (GUI_Widget*)fm_ent->obj;
	int i;
	GUI_Widget *panel, *w;
	
	if(ent->attr == O_DIR) { // This is FOLDER
		if (strcmp(GUI_ObjectGetName(fmw), "file_browser") == 0 && fm_ent->index == 0) {
			GUI_ContainerContains(self.vmu_page, self.filebrowser2);
			reset_selected();
			disable_high(RIGHT_FM);
			disable_high(LEFT_FM);
			clr_statusbar();
			GUI_FileManagerScan(self.filebrowser);
			return;
		}

		if(fm_ent->index == 0 && strlen(self.home_path) >= strlen(GUI_FileManagerGetPath(self.filebrowser2))) {
			reset_selected();
			self.home_path = NULL;
			clr_statusbar();
			addbutton();
			return;
		}
		
		disable_high(RIGHT_FM);
		disable_high(LEFT_FM);
		reset_selected();
		clr_statusbar();
		GUI_FileManagerChangeDir(fmw, ent->name, ent->size);
		return;
	}

	int name_len = strlen(ent->name);
	
	if( strcmp(self.m_SelectedFile,ent->name) == 0 &&
		strcmp(self.m_SelectedPath,GUI_FileManagerGetPath(fmw)) == 0) {		// file selected
		
		if((strcmp(GUI_ObjectGetName(fmw), "file_browser2") == 0 && 
									ent->name[name_len - 3] == 'v' && 
									ent->name[name_len - 2] == 'm' && 
									ent->name[name_len - 1] == 'd') && 
						strcmp(self.m_SelectedFile, ent->name) == 0 && 
						strcmp(self.m_SelectedPath, GUI_FileManagerGetPath(fmw)) == 0) {	/* RESTORE DUMP*/
			
			GUI_LabelSetText(self.confirm_text, "Restore dump. WARNING all data on VMU lost");
			
			GUI_PictureSetImage(self.image_confirm, self.confirmimg[1]);
			flag = Confirm_Window();
			GUI_PictureSetImage(self.image_confirm, self.confirmimg[0]);
			
			sprintf(src,"%s/%s",GUI_FileManagerGetPath(fmw),ent->name);
			
			if (flag == CMD_OK && FileSize(src) == (2 << 16)) {
				if ( VMU_Manager_Dump(GUI_FileManagerGetItem(self.filebrowser2, fm_ent->index)) == CMD_OK) {
					free_blocks(GUI_FileManagerGetPath(self.filebrowser) , 0);
					GUI_FileManagerScan(self.filebrowser);
				}
			}
			else if (flag == CMD_NO_ARG) {
				self.home_path = "/vmd";
				
				fs_vmd_vmdfile(src);
				GUI_WidgetSetEnabled(self.button_dump, 0);	
				GUI_FileManagerSetPath(self.filebrowser2, self.home_path);
				GUI_FileManagerScan(self.filebrowser2);
			}
			
			reset_selected();
			clr_statusbar();
			return;				
		}
		else if(strcmp(self.m_SelectedFile,ent->name) == 0 &&
				strcmp(self.m_SelectedPath,GUI_FileManagerGetPath(fmw)) == 0 && 
				GUI_ContainerContains(self.vmu_page, self.filebrowser2) == 1) {	// copy file

			if(strcmp(GUI_ObjectGetName(fmw), "file_browser2") == 0) {		// copy file to vmu

				if ((ent->name[strlen(ent->name) - 3] == 'v' || ent->name[strlen(ent->name) - 3] == 'V') && 
					(ent->name[strlen(ent->name) - 2] == 'm' || ent->name[strlen(ent->name) - 2] == 'M') && 
					(ent->name[strlen(ent->name) - 1] == 'i' || ent->name[strlen(ent->name) - 1] == 'I')) {

					if((vmi_t.size/512) > self.vmu_freeblock) {
						return;
					}

					sprintf(src, "%s/%1.8s.vms", GUI_FileManagerGetPath(fmw), vmi_t.source);
					if(!FileExists(src)) sprintf(src, "%s/%1.8s.VMS", GUI_FileManagerGetPath(fmw), vmi_t.source);
					
					if(!FileExists(src)) {
						reset_selected();
						disable_high(RIGHT_FM);
						clr_statusbar();
						return;
					}
					
					sprintf(dst, "%s/%12.12s", GUI_FileManagerGetPath(self.filebrowser), vmi_t.name);
				}
				else if  ((ent->name[strlen(ent->name) - 3] == 'd' || ent->name[strlen(ent->name) - 3] == 'D') && 
						  (ent->name[strlen(ent->name) - 2] == 'c' || ent->name[strlen(ent->name) - 2] == 'C') && 
						  (ent->name[strlen(ent->name) - 1] == 'i' || ent->name[strlen(ent->name) - 1] == 'I')) {

					if(((ent->size-32)/512) > self.vmu_freeblock) {
						return;
					}

					sprintf(src, "%s/%s", GUI_FileManagerGetPath(fmw), ent->name);
					sprintf(dst, "%s/%12.12s", GUI_FileManagerGetPath(self.filebrowser), dci_t.name);
					flag_type = DS_DCI;
				}
				else {
					if((ent->size/512) > self.vmu_freeblock) {
						return;
					}

					sprintf(src, "%s/%s", GUI_FileManagerGetPath(fmw), ent->name);
					sprintf(dst, "%s/%12.12s", GUI_FileManagerGetPath(self.filebrowser), ent->name);
				}

				if (FileExists(dst) != 0) {

					sprintf(text, "Overwrite %s", dst);
					GUI_LabelSetText(self.confirm_text, text);
					flag = Confirm_Window();
				}
				else {
					flag = CMD_OK;
				}

				if (flag == CMD_OK) {
					GUI_ProgressBarSetImage1(self.progressbar, self.progres_img_b);
					GUI_ProgressBarSetImage2(self.progressbar, self.progres_img);
					GUI_ProgressBarSetPosition(self.progressbar, 1.0);
					GUI_ContainerAdd(self.vmu_page, self.progressbar_container);
					GUI_WidgetMarkChanged(self.vmu_page);
					LockVideo();
#ifdef VMDEBUG
					dbgio_printf("src: %s\ndst: %s\n", src, dst);
					thd_sleep(2000);
#else
					if(flag_type == DS_DCI) {
						uint8 *tmpvmsbuf = NULL, *vmsbuf = NULL;
						uint32 cursize = ent->size - 32;

						tmpvmsbuf = (uint8* )calloc(1,cursize);
						vmsbuf = (uint8* )calloc(1,cursize);
						f = fs_open(src, O_RDONLY);
						
						if(f == FILEHND_INVALID) {
							GUI_ContainerRemove(self.vmu_page, self.progressbar_container);
							GUI_WidgetMarkChanged(self.vmu_page);
							UnlockVideo();
							return;
						}

						fs_seek(f,32,SEEK_SET);
						fs_read(f, tmpvmsbuf, cursize);
						fs_close(f);

						for (xx = 0; xx < cursize; xx += 4) {
							for (yy = 3; yy >= 0; yy--) vmsbuf[xx + (3-yy)] = tmpvmsbuf[xx + yy];
						}
						
						f = fs_open(dst, O_WRONLY | O_META);
						
						if(f == FILEHND_INVALID) {
							GUI_ContainerRemove(self.vmu_page, self.progressbar_container);
							GUI_WidgetMarkChanged(self.vmu_page);
							UnlockVideo();
							return;
						}

						fs_write(f, vmsbuf, cursize);
						fs_close(f);
						free(tmpvmsbuf);
						free(vmsbuf);
					}
					else {
						copy_save(src, dst);
					}
#endif
					free_blocks(GUI_FileManagerGetPath(self.filebrowser),0);
					GUI_FileManagerScan(self.filebrowser);
				}
			}
			else if(strncmp(self.m_SelectedPath, "/cd",3) != 0 || strncmp(self.m_SelectedPath, "/vmd",4) != 0) { // copy file from vmu
				sprintf(src, "%s/%s", GUI_FileManagerGetPath(fmw), ent->name);

				if(strncmp(GUI_FileManagerGetPath(self.filebrowser2), "/vmu",4) == 0) {
					if((ent->size/512) > self.vmu_freeblock2) return;

					sprintf(dst, "%s/%s", GUI_FileManagerGetPath(self.filebrowser2), ent->name);

					if (FileExists(dst) != 0) {
						sprintf(text, "Overwrite %s", dst);
						GUI_LabelSetText(self.confirm_text, text);

						if(Confirm_Window() == CMD_ERROR) {
							reset_selected();
							clr_statusbar();
							return;
						}
					}
				}
				else {
					sprintf(dst, "%s/%12.12s.vms", GUI_FileManagerGetPath(self.filebrowser2), ent->name);

					for(i=1;i<99;i++) {
						if(!FileExists(dst)) break;
						sprintf(dst, "%s/%12.12s.%02d.vms", GUI_FileManagerGetPath(self.filebrowser2), ent->name, i);
					}
				}

				GUI_ProgressBarSetImage1(self.progressbar, self.progres_img_b);
				GUI_ProgressBarSetImage2(self.progressbar, self.progres_img);
				GUI_ProgressBarSetPosition(self.progressbar, 1.0);
				GUI_ContainerAdd(self.vmu_page, self.progressbar_container);
				GUI_WidgetMarkChanged(self.vmu_page);
				LockVideo();
#ifdef VMDEBUG
				dbgio_printf("src: %s\ndst: %s\n", src, dst);
				thd_sleep(2000);
#else					
				copy_save(src, dst);
#endif	
				if(strncmp(GUI_FileManagerGetPath(self.filebrowser2), "/vmu",4) == 0) free_blocks(GUI_FileManagerGetPath(self.filebrowser2),1);

				GUI_FileManagerScan(self.filebrowser2);
				disable_high(LEFT_FM);
			}				
		}

		reset_selected();
		GUI_ContainerRemove(self.vmu_page, self.progressbar_container);
		GUI_WidgetMarkChanged(self.vmu_page);			
		clr_statusbar();
		if (VideoIsLocked()) UnlockVideo();
		return;
	}

	if((strcmp(GUI_ObjectGetName(fmw), "file_browser2") == 0 && 
							ent->name[name_len - 3] == 'v' && 
							ent->name[name_len - 2] == 'm' && 
							ent->name[name_len - 1] == 'd')) {		// This is VMD file
		flag_type = DS_VMD;
	}
	else if((strcmp(GUI_ObjectGetName(fmw), "file_browser2") == 0 && 
							(ent->name[name_len - 3] == 'v' || ent->name[name_len - 3] == 'V') && 
							(ent->name[name_len - 2] == 'm' || ent->name[name_len - 2] == 'M') && 
							(ent->name[name_len - 1] == 'i' || ent->name[name_len - 1] == 'I'))) {	// This is VMI file
		flag_type = DS_VMI;
	}
	else if(strcmp(GUI_ObjectGetName(fmw), "file_browser2") == 0 && 
							(ent->name[name_len - 3] == 'd' || ent->name[name_len - 3] == 'D') && 
							(ent->name[name_len - 2] == 'c' || ent->name[name_len - 2] == 'C') && 
							(ent->name[name_len - 1] == 'i' || ent->name[name_len - 1] == 'I')) {	// This is DCI file
		flag_type = DS_DCI;
	}
	else if((strcmp(GUI_ObjectGetName(fmw), "file_browser2") == 0 && 
							ent->name[name_len - 3] != 'v' && 
							ent->name[name_len - 2] != 'm' && 
							ent->name[name_len - 1] != 's' && 
							strncmp(GUI_FileManagerGetPath(self.filebrowser2), "/vm", 3) != 0)) {	// This is VMS file and not vmd folder
		disable_high(RIGHT_FM);
		return;
	}

	/* file not selected */
	if(self.m_SelectedFile != NULL) {
		free(self.m_SelectedFile);
		free(self.m_SelectedPath);
	}
	self.m_SelectedFile = strdup(ent->name);
	self.m_SelectedPath = strdup(GUI_FileManagerGetPath(fmw));

	panel = GUI_FileManagerGetItemPanel(fmw);

	for(i = 0; i < GUI_ContainerGetCount(panel); i++) {

		w = GUI_FileManagerGetItem(fmw, i);
		if(strcmp(GUI_ObjectGetName(fmw), "file_browser") == 0) {
			if(i != fm_ent->index) {
				GUI_ButtonSetNormalImage(w, self.m_ItemNormal);
				GUI_ButtonSetHighlightImage(w, self.m_ItemNormal);
			}
			else {
				GUI_ButtonSetNormalImage(w, self.m_ItemSelected);
				GUI_ButtonSetHighlightImage(w, self.m_ItemSelected);
			}
		}
		else {
			if(i != fm_ent->index) {
				GUI_ButtonSetNormalImage(w, self.m_ItemNormal2);
				GUI_ButtonSetHighlightImage(w, self.m_ItemNormal2);
			}
			else {
				GUI_ButtonSetNormalImage(w, self.m_ItemSelected2);
				GUI_ButtonSetHighlightImage(w, self.m_ItemSelected2);
			}
		}
	}

	if(strcmp(GUI_ObjectGetName(fmw), "file_browser") == 0) {
		disable_high(RIGHT_FM);
	}
	else {
		disable_high(LEFT_FM);
	}
		
	if(flag_type != DS_VMD) {
		if(!(tmpbuf = (uint16* )calloc(1, 2048))) {
			return;
		}

		sprintf(tmp, "%s/%s", GUI_FileManagerGetPath(fmw), ent->name);

		if(flag_type == DS_VMI) {
			f = fs_open(tmp, O_RDONLY);
			fs_read(f, &vmi_t, 108);
			fs_close(f);
			sprintf(tmp, "%s/%1.8s.vms", GUI_FileManagerGetPath(fmw), vmi_t.source);
			if(!FileExists(tmp)) sprintf(tmp, "%s/%1.8s.VMS", GUI_FileManagerGetPath(fmw), vmi_t.source);
		}
#ifdef VMDEBUG
		dbgio_printf("VMS filename: %s flag_type: %d\n", tmp, flag_type);
#endif		
		if(flag_type != DS_DCI) {
			if(flag_type == DS_VMS) {
				f = fs_open(tmp, O_RDONLY | O_META);
			}
			else {
				f = fs_open(tmp, O_RDONLY);
			}
			
			if(f == FILEHND_INVALID) {
				dbgio_printf("flag_type != 3 FILEHND_INVALID\n");
				reset_selected();
				disable_high(RIGHT_FM);
				free(tmpbuf);
				return;
			}
			fs_read(f, buf, 1024);
			fs_close(f);
		}
		else {
			f = fs_open(tmp, O_RDONLY);
			fs_read(f, &dci_t, 1056);
			fs_close(f);

			for (xx = 0; xx < 1024; xx += 4) {
				for (yy = 3; yy >= 0; yy--) buf[xx + (3-yy)] = dci_t.vmsheader[xx + yy];
			}		
		}

		memcpy(self.desc_short, buf, 16);
		self.desc_short[16]=0;
		memcpy(self.desc_long, buf+0x10, 32);
		self.desc_long[32]=0;
		if (strcmp(ent->name, "ICONDATA_VMS") == 0) {
			free(tmpbuf);
			return;
		}
		else {
			memcpy(pal, buf+0x60, 32);
			memcpy(icon, buf+0x80, 512);
		}

		for (yy = 0; yy < VMU_ICON_WIDTH; yy++) {
			for (xx = 0; xx < VMU_ICON_HEIGHT; xx += 2) {
				nyb=(icon[yy*16 + xx/2] & 0xf0) >> 4;
				tmpbuf[xx+yy*32]=PACK_NYBBLE_RGB565(pal[nyb]);
				nyb=(icon[yy*16 + xx/2] & 0x0f) >> 0;
				tmpbuf[xx+1+yy*32]=PACK_NYBBLE_RGB565(pal[nyb]);
			}
		}
		self.vmu_icon = SDL_CreateRGBSurfaceFrom(tmpbuf, 32, 32, 16, 64, 0, 0, 0, 0);
		self.vmuicon = GUI_SurfaceFrom("vmuicon", self.vmu_icon);
		free(tmpbuf);
	}
	else {	// is DS_VMD
		sprintf(self.desc_short, "VMU Dump");
		sprintf(self.desc_long, "Dreamshell VMU Dump file");
	}

	if(flag_type == DS_VMI) {
		sprintf(size, "%ld  Block(s)", vmi_t.size / 512);
	}
	else if(flag_type == DS_DCI) {
		sprintf(size, "%d  Block(s)", (ent->size - 32) / 512);
	}
	else {
		sprintf(size, "%d  Block(s)", ent->size / 512);
	}

#ifdef VMDEBUG	
	dbgio_printf("name: %s\n", ent->name);
	dbgio_printf("size: %s\n", size);
	dbgio_printf("descshort: %s\n", self.desc_short);
	dbgio_printf("desclong: %s\n", self.desc_long);
#endif

	if(flag_type == DS_VMD) {
		GUI_PictureSetImage(self.sicon, self.dump_icon);
	}
	else if(self.vmuicon) {
		GUI_PictureSetImage(self.sicon, self.vmuicon);
		GUI_ObjectDecRef((GUI_Object *)self.vmuicon);
	}

	GUI_LabelSetText(self.save_name, ent->name);
	GUI_LabelSetText(self.save_size, size);
	GUI_LabelSetText(self.save_descshort, self.desc_short);
	GUI_LabelSetText(self.save_desclong, self.desc_long);
}

void VMU_Manager_addfileman(GUI_Widget *widget) {
	file_t f;
	static char path[NAME_MAX];

	if(strcmp(GUI_ObjectGetName(widget),"/cd") != 0 && strlen(GUI_ObjectGetName(widget)) > 2) {
		if((f = fs_open(GUI_ObjectGetName(widget),O_DIR)) == FILEHND_INVALID) {
			fs_mkdir(GUI_ObjectGetName(widget));	
		}
		else {
			fs_close(f);
		}
		GUI_WidgetSetEnabled(self.button_dump, 1);
	}
	else if(strlen(GUI_ObjectGetName(widget)) != 2) {
		if(!DirExists("/cd")) {
			GUI_WidgetSetEnabled(self.cd_c, 0);
			return;
		}
	}
	
	if(self.direction_flag == 1) {
		sprintf(path,"/vmu/%s", GUI_ObjectGetName(widget));
		self.home_path = path;
	}
	else {
		self.home_path = (char *)GUI_ObjectGetName(widget);
	}

	reset_selected();
	disable_high(RIGHT_FM);
	disable_high(LEFT_FM);
	clr_statusbar();

	GUI_ContainerRemove(self.vmu_page, self.sd_c);
	GUI_ContainerRemove(self.vmu_page, self.hdd_c);
	GUI_ContainerRemove(self.vmu_page, self.cd_c);
	GUI_ContainerRemove(self.vmu_page, self.pc_c);
	GUI_ContainerRemove(self.vmu_page, self.format_c);
	GUI_ContainerRemove(self.vmu_page, self.dst_vmu);
	GUI_FileManagerSetPath(self.filebrowser2, self.home_path);
	GUI_ContainerAdd(self.vmu_page, self.filebrowser2);
	GUI_WidgetMarkChanged(self.vmu_page);
}

void VMU_Manager_ItemContextClick(dirent_fm_t *fm_ent) {
	dirent_t *ent = &fm_ent->ent;
	GUI_Widget *fmw = (GUI_Widget*)fm_ent->obj;
	char text[1024];
	char path[NAME_MAX];

	if(ent->attr == O_DIR) {
		if (strcmp(GUI_ObjectGetName(fmw), "file_browser") == 0) {
			VMU_Manager_ItemClick(fm_ent);
			return;
		}
		else if( strncmp(GUI_FileManagerGetPath(fmw),"/cd",3) == 0 || 
				 strncmp(GUI_FileManagerGetPath(fmw),"/vm",3) == 0) {
			
			reset_selected();
			disable_high(RIGHT_FM);
			clr_statusbar();		 
			return;
		}
		else {
			reset_selected();
			disable_high(RIGHT_FM);
			clr_statusbar();
			
			if (fm_ent->index == 0) {
				GUI_CardStackShowIndex(self.pages, 2);
			}
			else {
				sprintf(text, "Delete folder: %s ?", ent->name);
				sprintf(path, "%s/%s",GUI_FileManagerGetPath(self.filebrowser2), ent->name);
				GUI_LabelSetText(self.confirm_text, text);
				if(Confirm_Window() == CMD_OK){
					rmdir_recursive(path);
					//fs_rmdir(path);
					GUI_FileManagerScan(fmw);
				}
			}
		}
		return;
	}
	else if(!self.m_SelectedFile) {
		VMU_Manager_ItemClick(fm_ent);
	}
	else if(strcmp(self.m_SelectedFile,ent->name) != 0 || strcmp(self.m_SelectedPath,GUI_FileManagerGetPath(fmw)) != 0) {
		VMU_Manager_ItemClick(fm_ent);
	}
	else if(strncmp(self.m_SelectedPath,"/vmd",4) != 0 || strncmp(self.m_SelectedPath,"/cd",3) != 0) {
		/* Delete file */
		
		sprintf(text, "Delete %s/%s", GUI_FileManagerGetPath(fmw), ent->name);
		GUI_LabelSetText(self.confirm_text, text);
		
		if(Confirm_Window() == CMD_OK) {
			sprintf(text, "%s/%s", GUI_FileManagerGetPath(fmw), ent->name);
			fs_unlink(text);
			
			if(strncmp(GUI_FileManagerGetPath(fmw) , "/vmu", 4) == 0) {
				if(strcmp(GUI_ObjectGetName(fmw), "file_browser") == 0) {
					free_blocks(text,0);
				}
				else {
					free_blocks(text,1);
				}
			}
		}	

		reset_selected();
		clr_statusbar();
		GUI_FileManagerScan(fmw);
		GUI_WidgetMarkChanged(self.vmu_page);
	}	
	return;
}

int VMU_Manager_Dump(GUI_Widget *widget) {
	maple_device_t *dev = NULL;
	uint8 *vmdata;
	file_t f;
	char src[NAME_MAX] , dst[NAME_MAX], addr[NAME_MAX];
	int i,dumpflg;
	double progress = 0.0;
	
	strcpy(addr,GUI_FileManagerGetPath(self.filebrowser));
	
	if((dev = vmu_dev(addr)) == NULL) return CMD_ERROR;
	
	dumpflg = strcmp(GUI_ObjectGetName(widget),"dump-button");
	
	if(dumpflg == 0) {			/*DUMP*/
		GUI_ProgressBarSetImage1(self.progressbar, self.progres_img_b);
		GUI_ProgressBarSetImage2(self.progressbar, self.progres_img);
		GUI_ProgressBarSetPosition(self.progressbar, progress);
		GUI_ContainerAdd(self.vmu_page, self.progressbar_container);

		sprintf(dst, "%s/vmu001.vmd", GUI_FileManagerGetPath(self.filebrowser2));

		for(i = 2; i < 999; i++) {
			if(!FileExists(dst)) break;
			sprintf(dst, "%s/vmu%03d.vmd", GUI_FileManagerGetPath(self.filebrowser2), i);
		}
			
#ifdef VMDEBUG
		dbgio_printf("dst name: %s\n", dst);
#endif
		f = fs_open(dst, O_WRONLY | O_CREAT | O_TRUNC);

		if(f < 0) {
			dbgio_printf("DS_ERROR: Can't open %s\n", dst);
			return CMD_ERROR;
		}

		vmdata = (uint8 *) calloc(1,512);

		for (i = 0; i < 256; i++) {
			if (vmu_block_read(dev, i, vmdata) < 0) {
				dbgio_printf("DS_ERROR: Failed to read block %d\n", i);
				fs_close(f);
				free(vmdata);
				GUI_ContainerRemove(self.vmu_page, self.progressbar_container);
				GUI_WidgetMarkChanged(self.vmu_page);
				return CMD_ERROR;
			}
			fs_write(f, vmdata, 512);
			progress = (double) ceil(i*10/256)/10 + 0.1;
			GUI_ProgressBarSetPosition(self.progressbar, progress);
			GUI_WidgetMarkChanged(self.progressbar_container);
		}

		fs_close(f);
		free(vmdata);

		GUI_ContainerRemove(self.vmu_page, self.progressbar_container);
		GUI_FileManagerScan(self.filebrowser2);
	}
	else {					/*RESTORE*/
		progress = 1.0;
		GUI_ProgressBarSetImage1(self.progressbar, self.progres_img);
		GUI_ProgressBarSetImage2(self.progressbar, self.progres_img_b);
		GUI_ProgressBarSetPosition(self.progressbar, progress);
		GUI_ContainerAdd(self.vmu_page, self.progressbar_container);

		sprintf(src, "%s/%s", self.m_SelectedPath , self.m_SelectedFile);

#ifdef VMDEBUG
		dbgio_printf("src name: %s\n", src);
		dbgio_printf("SelectedPath: %s\nSelectedFile: %s\n", self.m_SelectedPath,self.m_SelectedFile);
#endif			
		f = fs_open(src, O_RDONLY);

		if(f < 0) {
			dbgio_printf("DS_ERROR: Can't open %s\n", src);
			return CMD_ERROR; 
		}

		vmdata = (uint8 *) calloc(1,512);

		i = 0; 

		while(fs_read(f, vmdata, 512) > 0) {
#ifdef VMDEBUG	
			thd_sleep(10);
#else		
			if(vmu_block_write(dev, i, vmdata) < 0) {
				dbgio_printf("DS_ERROR: Failed to write block %d\n", i);
				fs_close(f);
				free(vmdata);
				GUI_ContainerRemove(self.vmu_page, self.progressbar_container);
				GUI_WidgetMarkChanged(self.vmu_page);
				return CMD_ERROR;
			}
#endif
			i++;
			progress = (double) ceil((255-i)*10/256)/10 + 0.1;
			GUI_ProgressBarSetPosition(self.progressbar, progress);
			GUI_WidgetMarkChanged(self.progressbar_container);
		}

		fs_close(f);
		free(vmdata);
		GUI_ContainerRemove(self.vmu_page, self.progressbar_container);
	}
	
	GUI_WidgetMarkChanged(self.vmu_page);
	return CMD_OK;
}

void VMU_Manager_format(GUI_Widget *widget) {

	static uint8_t tmp_buf[512];
	maple_device_t *vmu = vmu_dev(GUI_FileManagerGetPath(self.filebrowser));
	
	if (!vmu) {
		return;
	}
	
	GUI_LabelSetText(self.confirm_text, "ERASE VMU. WARNING all data on VMU lost");
	if(Confirm_Window() == CMD_ERROR) return;

	reset_selected();
	disable_high(LEFT_FM);
	clr_statusbar();
	
	GUI_ProgressBarSetImage1(self.progressbar, self.progres_img_b);
	GUI_ProgressBarSetImage2(self.progressbar, self.progres_img);
	GUI_ProgressBarSetPosition(self.progressbar, 0.0);
	GUI_ContainerAdd(self.vmu_page, self.progressbar_container);
	GUI_WidgetMarkChanged(self.vmu_page);
	
	memset(tmp_buf, 0, 512);
	
	for (int i = 1; i < 14; i++) {
		vmu_block_write(vmu, 240+i, tmp_buf);
		GUI_ProgressBarSetPosition(self.progressbar, (double) i / 10);
		GUI_WidgetMarkChanged(self.progressbar_container);
	}
	
	uint16_t *tmp_ptr = (uint16_t *) tmp_buf;
	
	for (int i = 0; i < 241; i++)
	{
		*tmp_ptr++ = 0xFFFC;
	}
	
	*tmp_ptr++ = 0xFFFA;
	
	for (int i = 0; i < 12; i++)
	{
		*tmp_ptr++ = 0xF1 + i;
	}
	
	*tmp_ptr++ = 0xFFFA;
	*tmp_ptr++ = 0xFFFA;
	
	vmu_block_write(vmu, 254, tmp_buf);
	
	GUI_ProgressBarSetPosition(self.progressbar, 0.93);
	GUI_WidgetMarkChanged(self.progressbar_container);
	
	tmp_ptr = (uint16_t *) tmp_buf;
	
	memset(tmp_buf, 0, 512);
	
	for (int i = 0; i < 8; i++)
	{
		*tmp_ptr++ = 0x5555;
	}
	
	*tmp_ptr++ = 0xFF01;
	*tmp_ptr++ = 0xFFFF;
	*tmp_ptr++ = 0x00FF;
	
	tmp_ptr = (uint16_t *) &tmp_buf[0x30];
	
	*tmp_ptr++ = 0x2420;
	*tmp_ptr++ = 0x2503;
	*tmp_ptr++ = 0x0022;
	*tmp_ptr++ = 0x0038;
	
	vm_root_t *root = (vm_root_t *) &tmp_buf[0x40];
	
	root->size = 255;
	root->partition = 0;
	root->sys_block = 255;
	root->fat_block = 254;
	root->fat_cnt = 1;
	root->file_info_block = 253;
	root->file_info_cnt = 13;
	root->vol_icon = 0;
	root->reserved = 0;
	root->extra_block = 200;
	root->extra_cnt = 31;
	root->exe_block = 0;
	root->exe_cnt = 128;
	
	vmu_block_write(vmu, 255, tmp_buf);
	
	GUI_ProgressBarSetPosition(self.progressbar, 1.0);
	GUI_WidgetMarkChanged(self.progressbar_container);
	
	GUI_ContainerRemove(self.vmu_page, self.progressbar_container);
	free_blocks(GUI_FileManagerGetPath(self.filebrowser), 0);
	GUI_FileManagerScan(self.filebrowser);
	GUI_WidgetMarkChanged(self.vmu_page);
}

void VMU_Manager_sel_dst_vmu(GUI_Widget *widget) {
	self.direction_flag = 1;
	GUI_LabelSetText(self.drection, "SELECT DESTINATION VMU");
	ScreenFadeOutEx(NULL, 1);
	GUI_CardStackShowIndex(self.pages, 0);
	ScreenFadeIn();
	self.thd = thd_create(1, maple_scan, NULL);
}

void VMU_Manager_make_folder(GUI_Widget *widget) {
	
	char newfolder[NAME_MAX];
	char textentry[NAME_MAX];
	
	if (strcmp(GUI_ObjectGetName(widget),"confirm-no") == 0) GUI_CardStackShowIndex(self.pages, 1);
	strcpy(textentry,GUI_TextEntryGetText(self.folder_name));
	if (strlen(textentry) < 1) return;
	
	sprintf(newfolder,"%s/%s",GUI_FileManagerGetPath(self.filebrowser2),textentry);
	fs_mkdir(newfolder);
	GUI_CardStackShowIndex(self.pages, 1);
}

void VMU_Manager_clr_name(GUI_Widget *widget) {
	
	GUI_TextEntrySetText(widget, "");
}

