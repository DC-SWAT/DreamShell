/****************************
 * DreamShell ##version##   *
 * main.c                   *
 * DreamShell main          *
 * (c)2004-2023 SWAT        *
 * http://www.dc-swat.ru    *
 ***************************/

#include "ds.h"
#include "fs.h"
#include "vmu.h"
#include "profiler.h"
#include "network/net.h"

extern uint8 romdisk[];
KOS_INIT_FLAGS(INIT_IRQ | INIT_THD_PREEMPT | INIT_EXPORT);
KOS_INIT_ROMDISK(romdisk);

static uint32 ver_int = 0;
static const char *build_str[4] = {"Alpha", "Beta", "RC", "Release"};

static uint32 net_inited = 0;

void gdb_init();
uint32 _fs_dclsocket_get_ip(void);


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

const char *GetVersionBuildTypeString(int type) {
	return build_str[type];
}

static uint8 *get_board_id() {
	uint8 *(*sc)(int, int, int, int) = NULL;
	uint32 *scv = (uint32 *)&sc;
	*scv = *((uint32 *)0x8c0000b0);
	return sc(0, 0, 0, 3);
}


int InitNet(uint32 ipl) {

#if defined(DS_DEBUG)
	net_inited = 1;
#endif

	if(net_inited) {
		return 0;
	}

	union {
		uint32 ipl;
		uint8 ipb[4];
	} ip;
	ip.ipl = ipl;

	/* Check if the dcload-ip console is up, and if so, disable it,
		otherwise we'll crash when we attempt to bring up the BBA */
	if(dcload_type == DCLOAD_TYPE_IP) {
		/* Grab the IP address from dcload before we disable dbgio... */
		ip.ipl = _fs_dclsocket_get_ip();
		dbglog(DBG_INFO, "dc-load says our IP is %d.%d.%d.%d\n", ip.ipb[3],
				ip.ipb[2], ip.ipb[1], ip.ipb[0]);
		dbgio_disable();
	}

	int rv = net_init(ip.ipl);     /* Enable networking (and drivers) */

	if(rv < 0) {
		dbgio_enable();
		return -1;
	}
	if(dcload_type == DCLOAD_TYPE_IP) {
		fs_dclsocket_init_console();

		if(!fs_dclsocket_init()) {
			dbgio_dev_select("fs_dclsocket");
			dbgio_enable();
			dbglog(DBG_INFO, "fs_dclsocket console support enabled\n");
		} else {
			dbgio_enable();
		}
	}
	if (net_default_dev != NULL) {
		char ip_str[64];
		memset(ip_str, 0, sizeof(ip_str));
		snprintf(ip_str, sizeof(ip_str), "%d.%d.%d.%d",
			net_default_dev->ip_addr[0], net_default_dev->ip_addr[1],
			net_default_dev->ip_addr[2], net_default_dev->ip_addr[3]);
		setenv("NET_IPV4", ip_str, 1);

		dbglog(DBG_INFO, "Network IPv4 address: %s\n", ip_str);
	}
	net_inited = 1;
	return rv;
}

void ShutdownNet() {
	if(dcload_type == DCLOAD_TYPE_IP) {
		dbgio_set_dev_ds();
	}
	net_shutdown();
	net_inited = 0;
	setenv("NET_IPV4", "0.0.0.0", 1);
}

int InitDS() {

	char fn[NAME_MAX], bf[32];
	int tmpi = 0;
	uint8 *tmpb = NULL;
	Settings_t *settings;
#ifdef DS_EMU
	int emu = 1;
#else
	int emu = 0;
#endif

	SetVersion(0);

#if defined(DS_DEBUG) && DS_DEBUG == 2
	gdb_init();
#elif defined(USE_DS_EXCEPTIONS)
	expt_init();
#endif

#ifdef DS_DEBUG
	uint64 t_start = timer_ms_gettime64();
	dbglog_set_level(DBG_KDEBUG);
#endif

	if(!emu) {
		InitIDE();
		InitSDCard();

		if(is_custom_bios()) {
			InitRomdisk();
		}
	} else {
		setenv("EMU", "Unknown", 1);
	}

	SearchRoot();
	settings = GetSettings();

	InitVideoHardware();
	ShowLogo();

	if(settings->root[0] != 0 && DirExists(settings->root)) {
		setenv("PATH", settings->root, 1);
		if(	!strncmp(getenv("PATH"), "/sd", 3) || 
			!strncmp(getenv("PATH"), "/ide", 4) || 
			!strncmp(getenv("PATH"), "/pc", 3)) {
			setenv("TEMP", getenv("PATH"), 1);
		} else {
			setenv("TEMP", "/ram", 1);
		}
	}
	setenv("HOST", "DreamShell", 1);
	setenv("OS", getenv("HOST"), 1);
	setenv("USER", getenv("HOST"), 1);
	setenv("ARCH", hardware_sys_mode(&tmpi) == HW_TYPE_SET5 ? "Set5.xx" : "Dreamcast", 1);

	setenv("NET_IPV4", "0.0.0.0", 1);

	setenv("SDL_DEBUG", "0", 1);
	setenv("SDL_VIDEODRIVER", "dcvideo", 1);

	vmu_draw_string(getenv("HOST"));
	dbglog(DBG_INFO, "Initializing DreamShell Core...\n");

	SetConsoleDebug(1);

	setenv("HOME", getenv("PATH"), 1);
	setenv("$PATH", getenv("PATH"), 1);
	setenv("LUA_PATH", getenv("PATH"), 1);
	setenv("LUA_CPATH", getenv("PATH"), 1);
	setenv("PWD", fs_getwd(), 1);
	setenv("APP", (settings->app[0] != 0 ? settings->app : "Main"), 1);

	/* If used custom BIOS and syscalls is not installed, setting up it */
	if(is_custom_bios() && is_no_syscalls()) {
		tmpb = (uint8 *)0x8c000068;
	} else {

		/* Getting board ID */
		tmpb = get_board_id();

		if(strncmp(getenv("PATH"), "/cd", 3)) {
			/* Relax GD drive =) */
			cdrom_spin_down();
		}
	}

	memset(fn, 0, sizeof(fn));

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

	snprintf(fn, NAME_MAX, "%s/gui/cursors/default.png", getenv("PATH"));
	SetActiveMouseCursor(CreateMouseCursor(fn, NULL));
	
	TTF_Init();
	InitGUI();
	snd_stream_init();

	snprintf(fn, NAME_MAX, "%s/fonts/bitmap/console.png", getenv("PATH"));
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

#ifdef DS_DEBUG
	uint64 t_end = timer_ms_gettime64();
	dbglog(DBG_INFO, "Initializing time: %ld ms\n", (uint32)(t_end - t_start));
#endif

	if(settings->startup[0] == '/') {

		snprintf(fn, NAME_MAX, "%s%s", getenv("PATH"), settings->startup);
		LuaDo(LUA_DO_FILE, fn, GetLuaState());

	} else if(settings->startup[0] == '#') {

		dsystem_buff(settings->startup);

	} else if(settings->startup[0] != 0) {

		LuaDo(LUA_DO_STRING, settings->startup, GetLuaState());

	} else {
		snprintf(fn, NAME_MAX, "%s/lua/startup.lua", getenv("PATH"));
		LuaDo(LUA_DO_FILE, fn, GetLuaState());
	}

#ifdef DS_DEBUG
	t_end = timer_ms_gettime64();
	dbglog(DBG_INFO, "Startup time: %ld ms\n", (uint32)(t_end - t_start));
#else
	if(!emu) {
		SetConsoleDebug(0);
		dbgio_set_dev_ds();
	}
#endif

	HideLogo();

#ifdef DS_PROF
	if(dcload_type == DCLOAD_TYPE_IP) {
		profiler_init("/pc");
	} else {
		profiler_init(getenv("PATH"));
	}
	profiler_start();
#endif
	return 0;
}

void ShutdownDS() {
#ifdef DS_PROF
	profiler_stop();
	profiler_clean_up();
#endif
	dbglog(DBG_INFO, "Shutting down DreamShell Core...\n");

	char fn[NAME_MAX];
	snprintf(fn, NAME_MAX, "%s/lua/shutdown.lua", getenv("PATH"));
	LuaDo(LUA_DO_FILE, fn, GetLuaState());

	ShutdownCmd();
	ShutdownVideoThread();
	ShutdownApps();
	ShutdownConsole();
	ShutdownModules();
	ShutdownEvents();
	ShutdownLua();
	ShutdownNet();

	expt_shutdown();
	g1_ata_shutdown();
}


int main(int argc, char **argv) {

	SDL_Event event;
	maple_device_t *dev;
	int key;

	if(InitDS()) {
		return -1;
	}

	while(1) {

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
			break;
		}
	}

	ShutdownDS();
	return 0;
}
