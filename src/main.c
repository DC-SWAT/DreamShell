/****************************
 * DreamShell ##version##   *
 * main.c                   *
 * DreamShell main          *
 * (c)2004-2025 SWAT        *
 * http://www.dc-swat.ru    *
 ***************************/

#include "ds.h"
#include "fs.h"
#include "vmu.h"
#include "profiler.h"
#include "network/net.h"
#include "sfx.h"
#include <dc/syscalls.h>
#include <dc/net/w5500_adapter.h>

KOS_INIT_FLAGS(INIT_DEFAULT | INIT_EXPORT);

static uint32 ver_int = 0;
static const char *build_str[4] = {"Alpha", "Beta", "RC", "Release"};
static int net_inited = -1;

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
	}
	else {
		ver_int = ver;
	}

	ver_bld = (uint)DS_VER_BUILD(ver_int);

	if(DS_VER_BUILD_TYPE(ver_bld) == DS_VER_BUILD_TYPE_RELEASE) {
		snprintf(ver_str, sizeof(ver_str), "%d.%d.%d %s",
					(uint)DS_VER_MAJOR(ver_int),
					(uint)DS_VER_MINOR(ver_int),
					(uint)DS_VER_MICRO(ver_int),
					getenv("ARCH"));
	}
	else {
		snprintf(ver_str, sizeof(ver_str), "%d.%d.%d.%s.%d %s",
					(uint)DS_VER_MAJOR(ver_int),
					(uint)DS_VER_MINOR(ver_int),
					(uint)DS_VER_MICRO(ver_int),
					DS_VER_BUILD_TYPE_STR(ver_bld),
					DS_VER_BUILD_NUM(ver_bld),
					getenv("ARCH"));
	}
	setenv("VERSION", ver_str, 1);
	snprintf(ver_str, sizeof(ver_str), "%d.%d.%d",
				(uint)DS_VER_MAJOR(ver_int),
				(uint)DS_VER_MINOR(ver_int),
				(uint)DS_VER_MICRO(ver_int));

	setenv("VERSION_SHORT", ver_str, 1);
}

const char *GetVersionBuildTypeString(int type) {
	return build_str[type];
}

int InitNet(uint32 ipl) {

	if(net_inited > 0) {
		ds_printf("DS_OK: Network already initialized.\n");
		return 0;
	}
	union {
		uint32 ipl;
		uint8 ipb[4];
	} ip;
	ip.ipl = ipl;

	/* Check if the dcload-ip console is up, and if so, disable it,
		otherwise we'll crash when we attempt to bring up the BBA */
	if(ipl == 0 && dcload_type == DCLOAD_TYPE_IP) {
		/* Grab the IP address from dcload before we disable dbgio... */
		ip.ipl = _fs_dclsocket_get_ip();
		dbglog(DBG_INFO, "dc-load says our IP is %d.%d.%d.%d\n", ip.ipb[3],
				ip.ipb[2], ip.ipb[1], ip.ipb[0]);
		dbgio_dev_select("scif");
	}

	if(net_inited < 0) {
		/* Trying initialize Ethernet devices */
		la_init();
		bba_init();
		w5500_adapter_init(NULL, true);
	}

	/* Trying initialize network stack */
	int rv = net_init(ip.ipl);

	if(rv < 0) {
		net_inited = 0;
		return -1;
	}
	if(dcload_type == DCLOAD_TYPE_IP) {

		if(!fs_dclsocket_init()) {
			dbglog(DBG_INFO, "fs_dclsocket console support enabled\n");
			fs_dclsocket_init_console();
			dbgio_dev_select("fs_dclsocket");
		}
	}
	if(net_default_dev != NULL) {
		char ip_str[64];
		memset(ip_str, 0, sizeof(ip_str));
		snprintf(ip_str, sizeof(ip_str), "%d.%d.%d.%d",
			net_default_dev->ip_addr[0], net_default_dev->ip_addr[1],
			net_default_dev->ip_addr[2], net_default_dev->ip_addr[3]);
		setenv("NET_IPV4", ip_str, 1);
		ds_printf("DS_OK: Network initialized, IPv4 address: %s\n", ip_str);
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
	int tmpi;
	uint8_t *tmpb;
	uint64_t tmpd;
	Settings_t *settings;
#ifdef DS_EMU
	int emu = 1;
#else
	int emu = 0;
#endif

#if defined(DS_DEBUG) && DS_DEBUG == 2
	gdb_init();
#elif defined(USE_DS_EXCEPTIONS)
	expt_init();
#endif

#ifdef DS_DEBUG
	uint64 t_start = timer_ms_gettime64();
	dbglog_set_level(DBG_DEBUG);
#endif

	if(!emu) {
		InitIDE();
		InitSDCard();
	}
	else {
		setenv("EMU", "Unknown", 1);
	}

	SearchRoot();
	settings = GetSettings();

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

	tmpi = hardware_sys_mode(NULL);
	switch(tmpi) {
		case HW_TYPE_SET5:
			setenv("ARCH", "Set5.xx", 1);
			break;
		case HW_TYPE_NAOMI:
			// TODO: Add NAOMI 2 detection
			setenv("ARCH", "NAOMI", 1);
			break;
		case HW_TYPE_RETAIL:
			setenv("ARCH", "Dreamcast", 1);
			break;
		default:
			setenv("ARCH", "Unknown", 1);
			break;
	}

	if(tmpi != HW_TYPE_RETAIL) {
		// FIXME: Get some real ID for NAOMI.
		tmpb = (uint8_t *)0xa021a056;
	}
	else {
		tmpd = syscall_sysinfo_id();
		tmpb = (uint8_t *)&tmpd;
		if(strncmp(getenv("PATH"), "/cd", 3)) {
			/* Relax GD drive =) */
			cdrom_spin_down();
		}
	}
	memset(fn, 0, sizeof(fn));

	for(tmpi = 0; tmpi < 8; tmpi++) {
		snprintf(bf, sizeof(bf), "%02X", tmpb[tmpi]);
		strcat(fn, bf);
	}
	setenv("BOARD_ID", fn, 1);

	SetVersion(0);
	snprintf(bf, sizeof(bf), "%s v%s", getenv("HOST"), getenv("VERSION_SHORT"));
	setenv("TITLE", bf, 1);

	setenv("NET_IPV4", "0.0.0.0", 1);

	setenv("SDL_DEBUG", "0", 1);
	setenv("SDL_VIDEODRIVER", "dcvideo", 1);

	snd_stream_init();
	ds_sfx_play(DS_SFX_STARTUP);

	InitVideoHardware();
	ShowLogo();

	dbglog(DBG_INFO, "Initializing DreamShell Core...\n");
	vmu_draw_string(getenv("TITLE"));

	SetConsoleDebug(1);

	setenv("HOME", getenv("PATH"), 1);
	setenv("$PATH", getenv("PATH"), 1);
	setenv("LUA_PATH", getenv("PATH"), 1);
	setenv("LUA_CPATH", getenv("PATH"), 1);
	setenv("PWD", fs_getwd(), 1);
	setenv("APP", (settings->app[0] != 0 ? settings->app : "Main"), 1);

	IMG_Init(IMG_INIT_JPG | IMG_INIT_PNG);
	InitEvents();

	/* Initializing video */
	if(!InitVideo(settings->video.tex_width, settings->video.tex_height, settings->video.bpp)) {
		dbglog(DBG_ERROR, "Can't init video: %dx%dx\n", settings->video.tex_width, settings->video.tex_height);
		arch_reboot();
	}

	SetScreenMode(settings->video.virt_width, settings->video.virt_height, 0.0f, 0.0f, 1.0f);

	TTF_Init();
	InitGUI();

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

	/* Preload some sfx to avoid delay on first input */
	ds_sfx_preload();

	if(settings->network.startup_connect_eth) {
		strcpy(fn, "net --init");
	}
	else if(settings->network.startup_connect_ppp) {
		snprintf(fn, sizeof(fn), "%s/modules/ppp.klf", getenv("PATH"));

		if(OpenModule(fn)) {
			strcpy(fn, "ppp --init");
		}
		else {
			fn[0] = '\0';
		}
	}
	else {
		fn[0] = '\0';
	}

	if(fn[0] != '\0') {
		if(settings->network.startup_ntp) {
			strcat(fn, " && ntp --sync");
		}
		strcat(fn, " &");
		setenv("STARTUP_CMD", fn, 1);
	}

#ifdef DS_DEBUG
	uint64 t_end = timer_ms_gettime64();
	dbglog(DBG_INFO, "Initializing time: %ld ms\n", (uint32)(t_end - t_start));
#endif

	if(settings->startup[0] == '/') {
		snprintf(fn, NAME_MAX, "%s%s", getenv("PATH"), settings->startup);
		LuaDo(LUA_DO_FILE, fn, GetLuaState());
	}
	else if(settings->startup[0] == '#') {
		dsystem_buff(settings->startup);
	}
	else if(settings->startup[0] != 0) {
		LuaDo(LUA_DO_STRING, settings->startup, GetLuaState());
	}
	else {
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

	EXPT_GUARD_BEGIN;
		ShutdownCmd();
		ShutdownVideoThread();
		ShutdownConsole();
		ShutdownEvents();
		ShutdownNet();
	EXPT_GUARD_CATCH;
		dbglog(DBG_ERROR, "DS_ERROR: Shutting down failed\n");
	EXPT_GUARD_END;

	EXPT_GUARD_BEGIN_NEXT;
		ShutdownApps();
	EXPT_GUARD_CATCH;
		dbglog(DBG_ERROR, "DS_ERROR: Shutting down failed on apps\n");
	EXPT_GUARD_END;

	EXPT_GUARD_BEGIN_NEXT;
		ShutdownModules();
	EXPT_GUARD_CATCH;
		dbglog(DBG_ERROR, "DS_ERROR: Shutting down failed on modules\n");
	EXPT_GUARD_END;

	EXPT_GUARD_BEGIN_NEXT;
		ShutdownLua();
	EXPT_GUARD_CATCH;
		dbglog(DBG_ERROR, "DS_ERROR: Shutting down failed on lua\n");
	EXPT_GUARD_END;

	expt_shutdown();
	g1_ata_shutdown();
}

#define KBD_ATTACHED		(1 << 0)
#define KBD_DETACHED		(1 << 1)

static volatile uint8_t kbd_changed = 0;

static void close_vkb_module(void) {
	if(maple_enum_type(0, MAPLE_FUNC_KEYBOARD)) {
		if(GetModuleByName("vkb"))  {
			CloseModule(GetModuleByName("vkb"));
		}
	}
}

static void open_vkb_module(void) {
	if(!maple_enum_type(0, MAPLE_FUNC_KEYBOARD)) {
		if(!GetModuleByName("vkb"))  {
			char vkb_patch[32];
			snprintf(vkb_patch, 32, "%s/modules/vkb.klf", getenv("PATH"));
			OpenModule(vkb_patch);
		}
	}
}

static void keyboard_attach_cb(maple_device_t *dev) {
	(void) dev;
	kbd_changed |= KBD_ATTACHED;
}

static void keyboard_detach_cb(maple_device_t *dev) {
	(void) dev;
	kbd_changed |= KBD_DETACHED;
}

int main(int argc, char **argv) {
	SDL_Event event;

	if(InitDS()) {
		return -1;
	}

	maple_attach_callback(MAPLE_FUNC_KEYBOARD, keyboard_attach_cb);
	maple_detach_callback(MAPLE_FUNC_KEYBOARD, keyboard_detach_cb);
	memset(&event, 0, sizeof(event));

	while(1) {

		while(SDL_PollEvent(&event)) {
			ProcessInputEvents(&event);
		}

		if (kbd_changed) {
			if (kbd_changed & KBD_ATTACHED) {
				close_vkb_module();
			}
			else {
				open_vkb_module();
			}
			kbd_changed = 0;
		}
		UnLoadOldApps();
		GUI_ClearTrash();

		if(event.type == SDL_QUIT) {
			ds_printf("DS_INFO: SDL_QUIT called\n");
			break;
		}
		thd_sleep(10);
	}

	ShutdownDS();
	return 0;
}
