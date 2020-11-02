/****************************
 * DreamShell ##version##   *
 * main.c                   *
 * DreamShell main          *
 * Created by SWAT          *
 * http://www.dc-swat.ru    *
 ***************************/

#include "ds.h"
#include "fs.h"

//#define EMU
//#define DEBUG

#ifdef EMU
#	define DEBUG 1
#endif

extern uint8 romdisk[];
KOS_INIT_FLAGS(INIT_IRQ | INIT_THD_PREEMPT/* | INIT_NET | INIT_MALLOCSTATS | INIT_NO_DCLOAD*/);
KOS_INIT_ROMDISK(romdisk);

static uint32 ver_int = 0;
static const char *build_str[4] = {"Alpha", "Beta", "RC", "Release"};
static mutex_t input_mutex = MUTEX_INITIALIZER;
void gdb_init();

uint32 GetVersion() {
	return ver_int;
}

void SetVersion(uint32 ver) {
	
	char ver_str[32];
	uint ver_bld;
	
	if(ver == 0) {
		ver_int = DS_MAKE_VER(VER_MAJOR, VER_MINOR, VER_MICRO, VER_BUILD);
	} else {
		ver_int = ver;
	}
	
	ver_bld = (uint)DS_VER_BUILD(ver_int);
	
	if(DS_VER_BUILD_TYPE(ver_bld) == DS_VER_BUILD_TYPE_RELEASE) {

		snprintf(ver_str, sizeof(ver_str), "%d.%d.%d %s",
					(uint)DS_VER_MAJOR(ver_int),
					(uint)DS_VER_MINOR(ver_int),
					(uint)DS_VER_MICRO(ver_int),
					DS_VER_BUILD_TYPE_STR(ver_bld));
	} else {

		snprintf(ver_str, sizeof(ver_str), "%d.%d.%d.%s.%d",
					(uint)DS_VER_MAJOR(ver_int),
					(uint)DS_VER_MINOR(ver_int),
					(uint)DS_VER_MICRO(ver_int),
					DS_VER_BUILD_TYPE_STR(ver_bld),
					DS_VER_BUILD_NUM(ver_bld));
	}

	setenv("VERSION", ver_str, 1);
}

char *GetVersionString() {
	return getenv("VERSION");
}

const char *GetVersionBuildTypeString(int type) {
	return build_str[type];
}

static int is_emulator() {
#ifndef EMU
	return 0;
#else
	return 1;
#endif
}

static uint8 *get_board_id() {
	uint8 *(*sc)(int, int, int, int) = NULL;
	uint32 *scv = (uint32 *)&sc;
	*scv = *((uint32 *)0x8c0000b0);
	return sc(0, 0, 0, 3);
}

int InitDS() {
	
	char fn[MAX_FN_LEN], bf[32];
	int tmpi = 0, emu = 0;
	uint8 *tmpb = NULL;
	Settings_t *settings;
	
	SetVersion(0);
	settings = GetSettings();

	InitVideoHardware();
	ShowLogo();
	
	setenv("HOST", "DreamShell", 1);
	setenv("OS", getenv("HOST"), 1);
	setenv("USER", getenv("HOST"), 1);
	setenv("ARCH", hardware_sys_mode(&tmpi) == HW_TYPE_SET5 ? "Set5.xx" : "Dreamcast", 1);

	emu = is_emulator();
	
	if(emu) {
		setenv("EMU", "Unknown", 1); // TODO Emu name
	}

	setenv("SDL_DEBUG", "0", 1);
	setenv("SDL_VIDEODRIVER", "dcvideo", 1);
	
	//setenv("SDL_VIDEO_YUV_DIRECT", "1", 1);
	//setenv("SDL_VIDEO_YUV_HWACCEL", "1", 1);

//#if !defined(EMU) && defined(DEBUG)
//	gdb_init();
//#endif

	vmu_draw_string(getenv("HOST"));
	dbglog(DBG_INFO, "Initializing DreamShell Core...\n");

#ifdef DEBUG
	uint64 t_start = timer_ms_gettime64();
	dbglog_set_level(DBG_KDEBUG);
#endif

	expt_init();
	SetConsoleDebug(1);

	if(!emu) {
		
		tmpi = 1;
		
		if(!InitIDE()) {
			tmpi = 0;
		}
		
		if(!InitSDCard()) {
			tmpi = 0;
		}
		
		if(tmpi && is_custom_bios()) {
			InitRomdisk();
		}
	}

	if(settings->root[0] != 0 && DirExists(settings->root)) {
		setenv("PATH", settings->root, 1);
	} else {
		SearchRoot(0);
	}

	setenv("HOME", getenv("PATH"), 1);
	setenv("$PATH", getenv("PATH"), 1);
	setenv("LUA_PATH", getenv("PATH"), 1);
	setenv("LUA_CPATH", getenv("PATH"), 1);
	setenv("PWD", fs_getwd(), 1);
	setenv("APP", (settings->app[0] != 0 ? settings->app : "Main"), 1);
	
	/* If used custom BIOS and syscalls is not installed, setting up it */
	if(is_custom_bios() && is_no_syscalls()) {
//		if(!setup_syscalls()) {
//			tmpb = get_board_id();
//		} else {
			tmpb = (uint8 *)0x8c000068;
//		}
	} else {
		
		/* Getting board ID */
		tmpb = get_board_id();
		
		if(!tmpi && strncasecmp(getenv("PATH"), "/cd", 3)) {
			/* Relax GD drive =) */
			cdrom_spin_down();
		}
	}
	
	memset(fn, 0, MAX_FN_LEN);
	
	for(tmpi = 0; tmpi < 8; tmpi++) {
		sprintf(bf, "%02X", tmpb[tmpi]);
		strcat(fn, bf);
	}
	
	setenv("BOARD_ID", fn, 1);
	IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
	InitEvents();
	
	/* Initializing video */
	if(!InitVideo(settings->video.tex_width, settings->video.tex_height, settings->video.bpp)) {
		dbglog(DBG_ERROR, "Can't init video: %dx%dx\n", settings->video.tex_width, settings->video.tex_height);
		arch_reboot();
	}

	SetScreenMode(settings->video.virt_width, settings->video.virt_height, 0.0f, 0.0f, 1.0f);

	snprintf(fn, MAX_FN_LEN, "%s/gui/cursors/default.png", getenv("PATH"));
	SetActiveMouseCursor(CreateMouseCursor(fn, NULL));
	
	TTF_Init();
	InitGUI();
	snd_stream_init();

	snprintf(fn, MAX_FN_LEN, "%s/fonts/bitmap/console.png", getenv("PATH"));
	InitCmd(); 
	InitConsole(fn, NULL, 40, 0, 0, GetScreenWidth(), GetScreenHeight(), 255);

	if(InitModules()) {
		dbglog(DBG_ERROR, "Can't init modules\n");
		return -1;
	}
	
	if(InitApps()) {
		dbglog(DBG_ERROR, "Can't init apps\n");
		return -1;
	}

	if(InitLua()) {
		dbglog(DBG_ERROR, "Can't init lua library\n");
		return -1;
	}

#ifdef DEBUG
	uint64 t_end = timer_ms_gettime64();
	dbglog(DBG_INFO, "Initializing time: %ld ms\n", (uint32)(t_end - t_start));
#else
	if(!emu) {
		SetConsoleDebug(0);
		dbgio_set_dev_ds();
	}
#endif

	if(settings->startup[0] == '/') {
		
		snprintf(fn, MAX_FN_LEN, "%s%s", getenv("PATH"), settings->startup);
		LuaDo(LUA_DO_FILE, fn, GetLuaState());
		
	} else if(settings->startup[0] == '#') {

		dsystem_buff(settings->startup);

	} else if(settings->startup[0] != 0) {

		LuaDo(LUA_DO_STRING, settings->startup, GetLuaState());

	} else {
		
		snprintf(fn, MAX_FN_LEN, "%s/lua/startup.lua", getenv("PATH"));
		LuaDo(LUA_DO_FILE, fn, GetLuaState());
	}
	
#ifdef DEBUG
	t_end = timer_ms_gettime64();
	dbglog(DBG_INFO, "Startup time: %ld ms\n", (uint32)(t_end - t_start));
#endif
	
	HideLogo();
	return 0;
}

void ShutdownDS() {
	char fn[MAX_FN_LEN];
	snprintf(fn, MAX_FN_LEN, "%s/lua/shutdown.lua", getenv("PATH"));
	LuaDo(LUA_DO_FILE, fn, GetLuaState());
	
	ShutdownCmd();
	ShutdownVideoThread();
	ShutdownApps();
	ShutdownConsole();
	ShutdownModules();
	ShutdownEvents();
	ShutdownLua();
	
	expt_shutdown();
}

void LockInput() { 
	mutex_lock(&input_mutex);
}

void UnlockInput() {
	mutex_unlock(&input_mutex);
}

int InputIsLocked() {
	return mutex_is_locked(&input_mutex);
}

int InputMustLock() {
	kthread_t *ct = thd_get_current();
	return ct->tid != 1;
}


int main(int argc, char **argv) {

	SDL_Event event;
	maple_device_t *dev;
	int key;

	if(InitDS()) {
		return -1;
	}

	while(1) {

		LockInput();

		while(SDL_PollEvent(&event)) {

			if((dev = maple_enum_type(0, MAPLE_FUNC_KEYBOARD))) {

				key = kbd_queue_pop(dev, 1);

				/* ASCII? */
				if(!(key & 0xFFFFFF00)) {
					event.key.keysym.unicode = key;
				}
			}

			ProcessInputEvents(&event);
		}

		UnLoadOldApps();
		GUI_ClearTrash();

		if(event.type == SDL_QUIT) {
			UnlockInput();
			break;
		}

		UnlockInput();
	}

	ShutdownDS();
	return 0;
}
