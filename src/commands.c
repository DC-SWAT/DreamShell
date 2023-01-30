/*****************************
 * DreamShell ##version##    *
 * commands.c                *
 * DreamShell CMD            *
 * (c)2004-2023 SWAT         *
 * http://www.dc-swat.ru     *
 ****************************/


#include "ds.h"
#include "cmd_elf.h"
#include "zlib/zlib.h"
#include "network/net.h"

static Item_list_t *cmds;


/* print command list/desc */
static int builtin_help(int argc, char *argv[]) {

	Cmd_t *c;
	Item_t *i;

	char ac[512];
	int cnt = 0;

	if (argc > 1) {

		if((c = GetCmdByName(argv[1])) == NULL) return CMD_ERROR;

		ds_printf("%s - %s\n", c->command, c->desc);
		return CMD_OK;

	} else {

		ds_printf("\n Internal commands:\n");
		SLIST_FOREACH(i, cmds, list) {

			//ds_printf("%s, ", b->command);
			c = (Cmd_t *) i->data;

			if(!cnt) {

				strcpy(ac, "  ");

			} else {

				if(cnt % 6) {

					strcat(ac, ", ");

				} else {
					ds_printf("%s\n", ac);
					strcpy(ac, "  ");
				}

			}

			strcat(ac, c->command);
			cnt++;
			//ds_printf("%s, ", c->command);
		}

		ds_printf("%s\n", ac);
		file_t hnd;
		char dir[MAX_FN_LEN];

		snprintf(dir, MAX_FN_LEN, "%s/cmds", getenv("PATH"));
		hnd = fs_open(dir, O_RDONLY | O_DIR);

		if(hnd != FILEHND_INVALID) {

			dirent_t *ent;
			cnt = 0;

			memset_sh4(ac, 0, sizeof(ac));
			ds_printf("\n External commands:\n");
			while ((ent = fs_readdir(hnd)) != NULL) {

				if(ent->attr != O_DIR) {

					if(!cnt) {

						strcpy(ac, "  ");

					} else {

						if(cnt % 6) {

							strcat(ac, ", ");

						} else {
							ds_printf("%s\n", ac);
							strcpy(ac, "  ");
						}
					}

					strcat(ac, ent->name);
					cnt++;
					//ds_printf("%s, ", ent->name);
				}

			}

			fs_close(hnd);
			ds_printf("%s\n", ac);
		}

		ds_printf(" \nEnter 'help command_name' for a description.\n");
		return CMD_OK;
	}

	return CMD_ERROR;
}



int CallCmd(int argc, char *argv[]) {

	Cmd_t *cmd;
	if((cmd = GetCmdByName(argv[0])) != NULL) {

		int r = CMD_OK;

		if(cmd->handler != NULL) {
			r = cmd->handler(argc, argv);
		}
		return r;

	} else {
		return CallExtCmd(argc, argv);
	}

	return CMD_NOT_EXISTS;
}



int CheckExtCmdType(const char *fn) {

	file_t fd;
	uint8 data[2];

	fd = fs_open(fn, O_RDONLY);

	if(fd == FILEHND_INVALID) {
		return CMD_TYPE_UKNOWN;
	}

	fs_read(fd, data, sizeof(data));
	fs_close(fd);

	if(data[0] == 0x7F && data[1] == 0x45) {
		return CMD_TYPE_ELF;
	} else if(data[0] == 0x23 && data[0] == 0x44) {
		return CMD_TYPE_DSC;
	} else if(data[0] == 0x2D && data[1] == 0x2D) {
		return CMD_TYPE_LUA;
	} else if(data[1] == 0xD0) {
		return CMD_TYPE_BIN;
	} else {
		return CMD_TYPE_UKNOWN;
	}
}



int CallExtCmd(int argc, char *argv[]) {

	dirent_t *ent;
	file_t hnd;
	char dir[MAX_FN_LEN];
	char fn[MAX_FN_LEN];
	int r = CMD_NOT_EXISTS;

	snprintf(dir, MAX_FN_LEN, "%s/cmds", getenv("PATH"));
	hnd = fs_open(dir, O_RDONLY | O_DIR);

	if(hnd < 0) {
		ds_printf("DS_ERROR: Can't find cmds path\n");
		return r;
	}

	while ((ent = fs_readdir(hnd)) != NULL) {

		if(!strncmp(ent->name, argv[0], MAX_FN_LEN) && ent->attr != O_DIR) {
			snprintf(fn, MAX_FN_LEN, "%s/%s", dir, ent->name);
			r = CallCmdFile(fn, argc, argv);
			break;
		}
	}

	fs_close(hnd);
	return r;
}


int CallCmdFile(const char *fn, int argc, char *argv[]) {

	cmd_elf_prog_t *cmd = NULL;
	uint8 *buff;
	lua_State *L;
	file_t fd;
	int type = -1, r = CMD_NOT_EXISTS;

	type = CheckExtCmdType(fn);

	switch(type) {

	case CMD_TYPE_ELF:

		cmd = cmd_elf_load(fn);

		if(cmd == NULL) {
			break;
		}

		int (*cmd_main)(int argc, char *argv[]) = (int (*)(int argc, char *argv[]))cmd->main;
		r = cmd_main(argc, argv);
		cmd_elf_free(cmd);
		break;

	case CMD_TYPE_LUA:

		L = NewLuaThread();

		if (L == NULL) {
			ds_printf("DS_ERROR_LUA: Invalid state.. giving up\n");
			r = CMD_ERROR;
			break;
		}

		LuaPushArgs(L, argv);
		lua_setglobal(L, "argv");
		LuaDo(LUA_DO_FILE, fn, L);
		//lua_close(L);
		r = CMD_OK;
		break;

	case CMD_TYPE_DSC:

		r = dsystem_script(fn);
		break;

	case CMD_TYPE_BIN:

		fd = fs_open(fn, O_RDONLY);

		if(fd == FILEHND_INVALID) {
			break;
		}

		r = fs_total(fd);
		buff = (uint8* )memalign(32, r);

		if(!buff) {
			fs_close(fd);
			return CMD_ERROR;
		}

		if(fs_read(fd, buff, r) < 0) {
			fs_close(fd);
			return CMD_ERROR;
		}

		fs_close(fd);
		arch_exec(buff, r);
		break;

	default:
		break;
	}

	return r;
}


static void FreeCmd(void *cmd) {
	free(cmd);
}


void ShutdownCmd() {
	listDestroy(cmds, (listFreeItemFunc *) FreeCmd);
}



Cmd_t *AddCmd(const char *cmd, const char *helpmsg, CmdHandler *handler) {

	Cmd_t *c;
	Item_t *i;

	c = (Cmd_t *) calloc(1, sizeof(Cmd_t));
	if(c == NULL)
		return NULL;

	//ds_printf("Adding cmd %s %s\n", cmd, helpmsg);

	c->command = cmd;
	c->desc = helpmsg;
	c->handler = handler;

	if((i = listAddItem(cmds, LIST_ITEM_CMD, c->command, c, sizeof(Cmd_t))) == NULL) {
		FreeCmd(c);
		return NULL;
	}
	return c;
}



void RemoveCmd(Cmd_t *cmd) {

	if(cmd == NULL) {
		return;
	}

	listRemoveItem(cmds, listGetItemByName(cmds, cmd->command), (listFreeItemFunc *) FreeCmd);
}



Item_list_t *GetCmdList() {
	return cmds;
}


Cmd_t *GetCmdByName(const char *name) {

	if(name == NULL) {
		return NULL;
	}

	//SemWait(cmd_mutex);
	Item_t *i = listGetItemByName(cmds, name);
	//SemSignal(cmd_mutex);

	if(i != NULL) {
		return (Cmd_t *) i->data;
	}

	return NULL;
}

static char *fix_spaces(char *str)
{
	if(!str) return NULL;
	
	int i, len = (int) strlen(str);
	
	for(i=0; i<len; i++)
	{
		if(str[i] == '\\') str[i] = ' ';
	}
	
	return str;
}

/* Execute a single command input by the user or a script */
int dsystem(const char *buff) {
	int argc, ret = 0;
	char *argv[16];
	char *str = (char*)buff;

	/* we don't care if the user just hit enter */
	if (buff[0] == '\0') return 0;

	/* seperate the string into args */
	for (argc = 0; argc < 16;) {
		if ((argv[argc] = fix_spaces(strsep(&str, " \t\n"))) == NULL)
			break;
		if (*argv[argc] != '\0')
			argc++;
	}
	/* try to run the command as a builtin */

	ret = CallCmd(argc, argv);

	if(ret == CMD_NOT_EXISTS) {
		ds_printf("DS_ERROR: '%s' - Command not found\n", buff);
	}

	return ret;
}

int system(const char *buff) {
	return dsystem(buff);
}


int dsystemf(const char *fmt, ...) {
	char buff[256];
	va_list args;
	va_start(args, fmt);
	vsnprintf(buff, sizeof(buff), fmt, args);
	va_end(args);
	return dsystem(buff);
}


int dsystem_script(const char *fn) {
	char buff[256];
	FILE *f;
	int r = CMD_OK;

	f = fopen(fn, "rb");

	if (!f) {
		ds_printf("DS_ERROR: Can't Open DScript: %s\n", fn);
		return 0;
	}

	while (fgets(buff, sizeof(buff), f)) {

		if (buff[0] == 0 || buff[0] == '#')
			continue;
		if (buff[strlen(buff)-1] == '\n')
			buff[strlen(buff)-1] = 0;
		if (buff[strlen(buff)-1] == '\r')
			buff[strlen(buff)-1] = 0;

		r = dsystem(buff);
	}

	fclose(f);
	return r;
}


int dsystem_buff(const char *buff) {
	char *b, *bf;
	int r = CMD_OK;

	bf = (char*)buff;

	while((b = strsep(&bf, "\n")) != NULL) {
		if (b[0] == 0 || b[0] == '#') continue;
		r = dsystem(b);
	}

	return r;
}


/* list the contents of the directory */
static int builtin_ls(int argc, char *argv[]) {

	int lflag = 0;

	if(argc > 1 && !strncmp(argv[1], "-l", 2)) {
		lflag = 1;
	}

	dirent_t *ent;
	file_t fd;
	char dir[MAX_FN_LEN];
	int dcnt, fcnt;

	getcwd(dir, MAX_FN_LEN);
	fd = fs_open(dir, O_RDONLY | O_DIR);

	if (fd == FILEHND_INVALID) {
		ds_printf("DS_ERROR: Couldn't open %s\n", dir);
		return CMD_ERROR;
	}

	dcnt = fcnt = 0;
	ds_printf("DS_PROCESS: Reading %s\n", dir);

	while ((ent = fs_readdir(fd)) != NULL) {
		if (lflag)
			ds_printf("%12d bytes %s\n", ent->size, ent->name);
		else
			ds_printf("%s\n", ent->name);
		if (ent->size < 0)
			dcnt++;
		else
			fcnt++;
	}

	ds_printf("Total: %d files, %d dirs\n", fcnt, dcnt);
	fs_close(fd);

	return CMD_OK;
}



/* change the current directory */
static int builtin_cd(int argc, char *argv[]) {

	if(argc < 2) {
		ds_printf("Usage: cd dir\n");
		return CMD_NO_ARG;
	}

	if(fs_chdir(argv[1]) < 0) {
		ds_printf("DS_ERROR: Can't change directory to %s", argv[1]);
		return CMD_ERROR;
	}

	setenv("PWD", fs_getwd(), 1);
	return CMD_OK;
}


/* print the current directory */
static int builtin_pwd(int argc, char *argv[]) {
	ds_printf("%s\n", fs_getwd());
	return CMD_OK;
}


int builtin_env(int argc, char *argv[]) {

	if(argc < 2) {
		ds_printf("Usage: %s variable [value]\n", argv[0]);
		return CMD_NO_ARG;
	} else if(argc > 2) {
		setenv(argv[1], argv[2], argc > 3 ? atoi(argv[3]) : 1);
	} else {
		ds_printf("%s\n", getenv(argv[1]));
	}

	return CMD_OK;
}


/* clear the screen */
static int builtin_clear(int argc, char *argv[]) {
	ConsoleInformation *console = GetConsole();
	Clear_History(console);
	CON_UpdateConsole(console);
	return CMD_OK;
}

/* echo, heh */
static int builtin_echo(int argc, char *argv[]) {

	int i;
	char buff[256];

	if(argc == 1) {
		return CMD_NO_ARG;
	}

	memset_sh4(buff, 0, sizeof(buff));
	strncpy(buff, argv[1], sizeof(buff));

	for(i = 2; i < argc; i++) {
		strcat(buff, " ");
		strcat(buff, argv[i]);
	}

	strcat(buff, "\n");
	ds_printf(buff);
	return CMD_OK;
}

/* copy files and directories */
static int builtin_cp(int argc, char *argv[]) {

	if (argc < 3) {
		ds_printf("Usage: cp src_path dest_path\n");
		return CMD_NO_ARG;
	}

	int verbose = 0;

	if(argc > 3) {
		verbose = atoi(argv[3]);
	}

//	struct	stat st;
//
//	if (fs_stat(argv[1], &st, 0) < 0) {
//		ds_printf("DS_ERROR: Can't open %s\n", argv[1]);
//		return CMD_ERROR;
//	}
//
//	if(st.st_mode & S_IFDIR) {
//
//		if(CopyDirectory(argv[1], argv[2]))
//			return CMD_OK;
//		else
//			return CMD_ERROR;
//
//	} else if(st.st_mode & S_IFREG) {
//
//		if(CopyFile(argv[1], argv[2]))
//			return CMD_OK;
//		else
//			return CMD_ERROR;
//
//	} else {
//		ds_printf("DS_ERROR: Allow copy only files and directories\n");
//		return CMD_ERROR;
//	}

	if(FileExists(argv[1])) {

		if(CopyFile(argv[1], argv[2], verbose))
			return CMD_OK;
		else
			return CMD_ERROR;

	} else if(DirExists(argv[1])) {

		if(CopyDirectory(argv[1], argv[2], verbose))
			return CMD_OK;
		else
			return CMD_ERROR;

	} else {
		ds_printf("DS_ERROR: Can't open %s\n", argv[1]);
		return CMD_ERROR;
	}

	return CMD_OK;
}



/* unlink files */
static int builtin_rm(int argc, char *argv[]) {

	if (argc == 1) {
		ds_printf("Usage: rm file\n");
		return CMD_NO_ARG;
	}

	if (fs_unlink(argv[1]) == -1) {
		ds_printf("DS_ERROR: Error unlinking %s.\n", argv[1]);
		return CMD_ERROR;
	}

	return CMD_OK;
}


/* create a directory */
static int builtin_mkdir(int argc, char *argv[]) {

	if (argc == 1) {
		ds_printf("Usage: %s dirname\n", argv[0]);
		return CMD_NO_ARG;
	}

	if (fs_mkdir(argv[1]) < 0) {
		ds_printf("DS_ERROR: Error making directory %s (maybe not supported)\n", argv[1]);
		return CMD_ERROR;
	}

	return CMD_OK;
}



static int builtin_rmdir(int argc, char *argv[]) {

	if (argc == 1) {
		ds_printf("Usage: %s dirname\n", argv[0]);
		return CMD_NO_ARG;
	}

	if (fs_rmdir(argv[1]) < 0) {
		ds_printf("DS_ERROR: Error deleting directory %s (maybe not supported)\n", argv[1]);
		return CMD_ERROR;
	}

	return CMD_OK;
}


/* create a path */
static int builtin_mkpath(int argc, char *argv[]) {

	if (argc == 1) {
		ds_printf("Usage: %s path\n", argv[0]);
		return CMD_NO_ARG;
	}

	if (mkpath(argv[1]) < 0) {
		ds_printf("DS_ERROR: Error making path %s\n", argv[1]);
		return CMD_ERROR;
	}

	return CMD_OK;
}


/* cat text files to the screen */
static int builtin_cat(int argc, char *argv[]) {

	if(argc == 1) {
		ds_printf("Usage: cat filename\n");
		return CMD_NO_ARG;
	}


	FILE *f;
	char buff[128];

	f = fopen(argv[1], "rt");
	if (!f) {
		ds_printf("DS_ERROR: Error opening %s\n", argv[1]);
		return CMD_ERROR;
	}

	while (fgets(buff, 128, f)) {
		ds_printf(buff);
	}

	fclose(f);
	return CMD_OK;
}


static int builtin_romdisk(int argc, char *argv[]) {

	if(argc < 3) {
		ds_printf("Usage: romdisk -flag args...\n"
		          "Flags: \n"
		          " -m filename dir  -Mount Romdisk Image\n"
		          " -u dir           -Unmount Romdisk Image\n\n"
		          "Example: romdisk -m /cd/romdisk.dsr /rd2\n"
		          "         romdisk -u /rd2\n");
		return CMD_NO_ARG;
	}

	if (!strncmp(argv[1], "-m", 2)) {

		void * datar = NULL;

		ds_printf("DS_PROCESS: Loading the romdisk image...\n");
		if (fs_load(argv[2], &datar) <= 0) {
			ds_printf("DS_ERROR: Error! Invalid romdisk source image '%s'\n", argv[2]);
			if (datar != NULL)
				free(datar);
			return CMD_ERROR;
		}

		/* Attempt to mount it */
		ds_printf("DS_PROCESS: Mounting on %s...\n", argv[2]);
		if (fs_romdisk_mount(argv[3], (const uint8 *)datar, 1) < 0) {
			ds_printf("DS_ERROR: Error! Can't mount romdisk image\n");
			free(datar);
			return CMD_ERROR;
		}
		ds_printf("DS_OK: Romdisk Success!\n");
		return CMD_ERROR;
	}

	if(!strncmp(argv[1], "-u", 2)) {
		ds_printf("DS_PROCESS: Unmounting %s ...\n", argv[2]);

		if(fs_romdisk_unmount(argv[2]) < 0) {
			ds_printf("DS_ERROR: Can't unmounting romdisk  %s\n", argv[2]);
		}
		ds_printf("DS_OK: Complete.\n");
	}
	return CMD_OK;
}



static int builtin_exec(int argc, char *argv[]) {

	if(argc < 3) {
		ds_printf("Usage: %s option args\n"
		          "Options: \n"
		          " -b, --binary   -Binary file type\n"
		          " -e, --elf      -Elf file type\n "
		          "Arguments: \n"
		          " -f, --file     -File for executing\n"
		          " -a, --addr     -Executing address\n\n"
		          "Examples: %s -b -f /cd/prog.bin\n"
		          "          %s -b -f /cd/prog.bin -a 0xac010000\n\n",
		          argv[0], argv[0], argv[0]);
		return CMD_NO_ARG;
	}

	CFG_CONTEXT con;
	int binary = 0, elf = 0, ret = 0;
	char *file = NULL;
	uint32 addr = 0;//0xac010000;

	struct cfg_option options[] = {
		{"binary", 'b', NULL, CFG_BOOL, (void *) &binary, 0},
		{"elf",    'e', NULL, CFG_BOOL, (void *) &elf,  0},
		{"file",   'f', NULL, CFG_STR,  (void *) &file,  0},
		{"addr",   'a', NULL, CFG_ULONG,(void *) &addr, 0},
		{"return", 'r', NULL, CFG_BOOL, (void *) &ret, 0},
		CFG_END_OF_LIST
	};

	con = cfg_get_context(options);
	cfg_set_context_flag(con, CFG_IGNORE_UNKNOWN);

	if (con == NULL) {
		ds_printf("DS_ERROR: Not enough memory\n");
		return CMD_ERROR;
	}

	cfg_set_cmdline_context(con, 1, -1, argv);

	if (cfg_parse(con) != CFG_OK) {
		ds_printf("DS_ERROR: Error parsing command line: %s\n", cfg_get_error_str(con));
		cfg_free_context(con);
		return CMD_ERROR;
	}

	cfg_free_context(con);

	if(!file) {
		ds_printf("DS_ERROR: Not specified file to execute.\n");
		return CMD_ERROR;
	}

	if(binary) {

		uint32 len = 0;
		void *ptr = NULL;

		file_t f = fs_open(file, O_RDONLY);

		if (f < 0) {
			ds_printf("DS_ERROR: Can't open %s\n", file);
			return CMD_ERROR;
		}

		ds_printf("DS_PROCESS: Loading %s in to memory...\n", file);

		len = fs_total(f);
		ptr = (uint8*) memalign(32, len);

		if(ptr == NULL) {
			ds_printf("DS_ERROR: Not enough memory\n");
			return CMD_ERROR;
		}

		memset_sh4(ptr, 0, len);
		fs_read(f, ptr, len);
		fs_close(f);
		ds_printf("DS_PROCESS: Executing...\n");

		if(!addr) arch_exec(ptr, len);
		else arch_exec_at(ptr, len, addr);

		ds_printf("DS_ERROR: Exec failed.\n");

		return CMD_OK;
	}

	if(elf) {

		cmd_elf_prog_t *cmd = cmd_elf_load(file);

		if(cmd == NULL) {
			ds_printf("DS_ERROR: Can't load %s\n", file);
			return CMD_ERROR;
		}

		int (*cmd_main)(int argc, char *argv[]) = (int (*)(int argc, char *argv[]))cmd->main;
		int r = cmd_main(argc, argv);
		cmd_elf_free(cmd);
		ds_printf("DS_OK: Program return: %d\n", r);
		return CMD_OK;
	}

	return CMD_OK;
}


static int builtin_periphs(int argc, char *argv[]) {
	int p, u;
	maple_device_t *dev;
	ds_printf("DS: Attached maple peripheral info:\n");

	for (p = 0; p < MAPLE_PORT_COUNT; p++) {
		for (u = 0; u < MAPLE_UNIT_COUNT; u++) {
			dev = maple_enum_dev(p, u);
			if (dev) {
				ds_printf("%c%c: %s (%08lx: %s)\n",
				          'A' + p, '0' + u,
				          dev->info.product_name,
				          dev->info.functions,
				          maple_pcaps(dev->info.functions));
			}
		}
	}
	return CMD_OK;
}


static int builtin_sleep(int argc, char *argv[]) {

	if (argc < 2) {
		ds_printf("Usage: sleep msec\n");
		return CMD_NO_ARG;
	}

	thd_sleep(atoi(argv[1]));
	return CMD_OK;
}


static int builtin_rename(int argc, char *argv[]) {

	if(argc < 3) {
		ds_printf("Usage: rename src dst\n");
		return CMD_NO_ARG;
	}

	if(fs_rename(argv[1], argv[2]) == -1) {
		ds_printf("DS_ERROR: Renaming Failed.\n");
		return CMD_ERROR;
	}

	return CMD_OK;
}



static int builtin_read(int argc, char *argv[]) {

	void *buff;
	FILE *f;
	FILE *fs;
	int siz = 0;
	int s_byte = 0;

	if(argc < 5) {
		ds_printf("Usage: %s infile outfile size offset\n", argv[0]);
		ds_printf("Example: %s in.bin out.bin 512 64\n", argv[0]);
		return CMD_NO_ARG;
	}

	//ds_printf("DS_PROCESS: Reading file...\n");

	siz = atoi(argv[3]);

	if(argc > 4)
		s_byte = atoi(argv[4]);


	if((buff = malloc(siz)) == NULL) {
		ds_printf("DS_ERROR: The not enough memory.\n");
		return CMD_ERROR;
	}

	f = fopen(argv[1], "rb");

	if(!f) {
		ds_printf("DS_ERROR: Can't open %s\n", argv[1]);
		return CMD_ERROR;
	}

	//fseek(f, 0, SEEK_END);
	if(s_byte)
		fseek(f, s_byte, SEEK_SET);

	fread(buff, siz, 1, f);
	fclose(f);

	fs = fopen(argv[2], "wb");

	if(!fs) {
		ds_printf("DS_ERROR: Can't open %s\n", argv[2]);
		return CMD_ERROR;
	}

	//ds_printf("DS_PROCESS: Reading complete, writing...\n");
	fwrite(buff, siz, 1, fs);
	fclose(fs);
	free(buff);
	//ds_printf("DS_OK: Complete.\n");
	return CMD_OK;
}


static int builtin_mstats(int argc, char *argv[]) {

	struct mallinfo mi = mallinfo();
	//struct mallinfo pmi = pvr_int_mallinfo();
	ds_printf(	"Memory usage:\n"
	            "Max system bytes      = %10lu\n"
	            "System bytes          = %10lu\n"
	            "In use bytes          = %10lu\n"
	            "Free chunks           = %10lu\n"
	            "Fastbin blocks        = %10lu\n"
	            "Mmapped regions       = %10lu\n"
	            "Total allocated space = %10lu\n"
	            "Total free space      = %10lu\n"
	            "PVR Mem avaible       = %10lu\n"
	            "Current sbrk          = %10lx\n",
	            (uint32)(mi.usmblks),
	            (uint32)(mi.arena + mi.hblkhd),
	            (uint32)(mi.uordblks + mi.hblkhd),
	            (uint32) mi.ordblks,
	            (uint32) mi.smblks,
	            (uint32) mi.hblks,
	            (uint32) mi.uordblks,
	            (uint32) mi.fordblks,
	            (uint32) pvr_mem_available(),
	            (uint32) (sbrk(0))
	         );
	//ds_printf("PVR System bytes = %10lu\n",(uint32)(pmi.arena + pmi.hblkhd));
	//ds_printf("PVR In use bytes = %10lu\n",(uint32)(pmi.uordblks + pmi.hblkhd));

	return CMD_OK;
}


static int builtin_statf(int argc, char *argv[]) {

	if(argc == 1) {
		ds_printf("Usage: statf path\n");
		return CMD_NO_ARG;
	}

	struct stat st;
	/*time_t t;
	struct tm tm;
	const char *days[] = { "Sun", "Mon", "Tue", "Wed", "Thu", "Fri", "Sat" };
	const char *months[] = { "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul",
		"Aug", "Sep", "Oct", "Nov", "Dec" };*/

	if(fs_stat(argv[1], &st, 0) < 0) {
		ds_printf("DS_ERROR: Couldn't get stat from %s, error: %d\n", argv[1], errno);
		return CMD_ERROR;
	}

	if(st.st_mode & S_IFDIR) {
		ds_printf("Type: Standard directory, size %d\n", st.st_size);
	} else if(st.st_mode & S_IFSOCK) {
		ds_printf("Type: Socket\n");
	} else if(st.st_mode & S_IFIFO) {
		ds_printf("Type: FIFO\n");
	} else if(st.st_mode & S_IFBLK) {
		ds_printf("Type: Block special\n");
	} else if(st.st_mode & S_IFCHR) {
		ds_printf("Type: Character device\n");
	} else if(st.st_mode & S_IFREG) {
		ds_printf("Type: Regular file, size %d\n", st.st_size);
	}

	ds_printf("Mode: %08lx\nAccess time: %d\nModified time: %d\nCreated time: %d\n",
	          st.st_mode, st.st_atime, st.st_mtime, st.st_ctime);

	return CMD_OK;
}


/* Dreamcast functions */
static int builtin_dc(int argc, char *argv[]) {

	if(argc == 1) {
		ds_printf("Usage: dc -flag arguments(if needed)\n"
		          "Flags:\n"
		          "-menu    Kernel \"exit to menu\" call\n"
		          "-exit    Generic kernel \"exit\" point\n"
		          "-return  Kernel \"return\" point\n"
		          "-abort   Kernel \"abort\" point\n"
		          "-reboot  Kernel \"reboot\" call\n"
		          "-main    Kernel C-level entry point\n");
		return CMD_NO_ARG;

	} else if (!strncmp(argv[1], "-menu", 5)) {
		arch_menu();
	} else if (!strncmp(argv[1], "-exit", 5)) {
		arch_exit();
	} else if (!strncmp(argv[1], "-return", 7)) {
		arch_return();
	} else if (!strncmp(argv[1], "-abort", 6)) {
		arch_abort();
	} else if (!strncmp(argv[1], "-reboot", 7)) {
		arch_reboot();
	} else if (!strncmp(argv[1], "-main", 5)) {
		arch_main();
	} else {
		ds_printf("DS_ERROR: flag '%s' is not supported\n", argv[1]);
		return CMD_ERROR;
	}

	return CMD_OK;
}


static void ctaddr(uint32 ad) {
	//uint32 *mem = ad;
	int irqd;
	irqd = irq_disable();
	void (*ctm)() = (void (*)())ad;
	//void (*ctm)()((uint32*)ctm) = (uint32 *)ad;
	//{ void (*ctm)(); ((uint32*)ctm) = (uint32 *)ad; ctm(); }
	ctm();
	irq_restore(irqd);
}



static int builtin_addr(int argc, char *argv[]) {

	if(argc < 2) {
		ds_printf("Usage: addr -flag address args...\n"
		          "Flags: \n"
		          "  -c addr -s(if needed)  -Call to addr(flag -s for arch_shutdown)\n"
		          "  -r addr size filename  -Read data from addr and write to file\n"
		          "  -w addr filename       -Read file to addr\n"
		          "  -v addr                -Valid addr?\n");
		return CMD_NO_ARG;
	}

	file_t fd;
	int irqd, len;
	uint32 ad = strtoul(argv[2], NULL, 16);

	if(!strncmp(argv[1], "-c", 2)) {
		ds_printf("DS_INF: Calling to 0x%x ...\n", ad);

		if(!strncmp(argv[3], "-s", 2)) {
			arch_shutdown();
			ctaddr(ad);
			ds_printf("DS_OK: Complete.\n");
			return CMD_OK;
		}
		ctaddr(ad);
		ds_printf("DS_OK: Complete.\n");
		return CMD_OK;
	}

	if(!strncmp(argv[1], "-v", 2)) {
		if(arch_valid_address(ad)) {
			ds_printf("DS_INF: 0x%x not valid.\n",ad);
			return CMD_OK;
		}

		ds_printf("DS_INF: 0x%x is valid.\n" ,ad);
		return CMD_OK;
	}


	if(!strncmp(argv[1], "-r", 2)) {

		len = atoi(argv[3]);
		fd = fs_open(argv[4], O_CREAT | O_WRONLY);

		if(fd == -1) {
			ds_printf("DS_ERROR: Can't open %s\n", argv[4]);
			return CMD_ERROR;
		}

		ds_printf("DS_PROCESS: Reading addr = 0x%x, size = %d\n", len);
		fs_write(fd, (char*)ad, len);
		fs_close(fd);
		ds_printf("DS_OK: Complete.\n");
		return CMD_ERROR;
	}


	if(!strncmp(argv[1], "-w", 2)) {

		fd = fs_open(argv[3], O_RDONLY);

		if(fd == -1) {
			ds_printf("DS_ERROR: Can't open %s\n", argv[3]);
			return CMD_ERROR;
		}

		len = fs_total(fd);
		fs_read(fd, (char*)ad, len);
		fs_close(fd);

		ds_printf("DS_PROCESS: Loading %s (%i bytes) at 0x%x\n", argv[3], len, (uint32)ad);

		irqd = irq_disable();
		//flush_icache_range((uint32)ad, len);
		dcache_flush_range((uint32)ad, len);
		irq_restore(irqd);

		ds_printf("DS_OK: Complete.\n");
		return CMD_ERROR;
	}

	return CMD_OK;
}


static int builtin_lua(int argc, char *argv[]) {

	if(argc == 1) {
		ds_printf("%.80s  %.80s\n\n", LUA_RELEASE, LUA_COPYRIGHT);
		ds_printf("Usage: lua file.lua argv...\n");
		return CMD_NO_ARG;
	}

	lua_State *L = GetLuaState();//NewLuaThread();

	if (L == NULL) {
		ds_printf("DS_ERROR: LUA: Invalid state.. giving up\n");
		return CMD_ERROR;
	}

	LuaPushArgs(L, argv+1);
	lua_setglobal(L, "argv");
	LuaDo(LUA_DO_FILE, argv[1], L);
	//lua_close(L);
	return CMD_OK;
}


static int builtin_dsc(int argc, char *argv[]) {

	if(argc < 2) {
		ds_printf("Usage: dsc filename.dsc\n");
		return CMD_NO_ARG;
	}

	dsystem_script(argv[1]);
	return CMD_OK;
}


static int builtin_screenshot(int argc, char *argv[]) {

	if(argc < 3) {
		ds_printf("Usage: %s filename format(bmp,ppm)\n", argv[0]);
		return CMD_NO_ARG;
	}

	if(argc > 2 && !strncmp(argv[2], "ppm", 3)) {

		if(vid_screen_shot(argv[1]) < 0) {
			ds_printf("DS_ERROR: Can't save PPM screenshot to %s\n", argv[1]);
			return CMD_ERROR;
		}

	} else if(argc > 2 && !strncmp(argv[2], "bmp", 3)) {
		
		if(SDL_SaveBMP(GetScreen(), argv[1]) < 0) {
			ds_printf("DS_ERROR: Can't save BMP screenshot to %s\n", argv[1]);
			return CMD_ERROR;
		}
	} else {

		SDL_Surface *tmp = SDL_PNGFormatAlpha(GetScreen());
		if(SDL_SavePNG(tmp, argv[1]) < 0) {
			SDL_FreeSurface(tmp);
			ds_printf("DS_ERROR: Can't save PNG screenshot to %s\n", argv[1]);
			return CMD_ERROR;
		}
		SDL_FreeSurface(tmp);
	}

	ds_printf("DS_OK: Screenshot saved to %s\n", argv[1]);
	return CMD_OK;
}


static int builtin_module(int argc, char *argv[]) {

	if(argc == 1) {
		ds_printf("Usage: module option args...\n\n"
		          "Options: \n"
		          " -o, --open         -Open module\n"
		          " -c, --close        -Close module\n"
		          " -p, --printlist    -Print modules list\n\n"
		          "Arguments: \n"
		          " -n, --name         -Module name\n"
		          " -f, --file         -Module file\n\n"
		          "Examples: module -o -f /cd/module.klf\n"
		          "          module --close --name modulename\n");
		return CMD_NO_ARG;
	}

	int mopen = 0, mclose = 0, printlist = 0;
	char *file = NULL, *name = NULL;

	struct cfg_option options[] = {
		{"open",      'o', NULL, CFG_BOOL, (void *) &mopen,     0},
		{"close",     'c', NULL, CFG_BOOL, (void *) &mclose,    0},
		{"printlist", 'p', NULL, CFG_BOOL, (void *) &printlist, 0},
		{"name",      'n', NULL, CFG_STR,  (void *) &name,      0},
		{"file",      'f', NULL, CFG_STR,  (void *) &file,      0},
		CFG_END_OF_LIST
	};

	CMD_DEFAULT_ARGS_PARSER(options);

	if(mopen || mclose) {

		if(mopen && file == NULL) {
			ds_printf("DS_ERROR: Too few arguments. (file) \n");
			return CMD_NO_ARG;
		}

		if(mclose && name == NULL) {
			ds_printf("DS_ERROR: Too few arguments. (module name) \n");
			return CMD_NO_ARG;
		}
	}


	if(printlist) {
		PrintModuleList(ds_printf);
	}

	if(mopen) {
		klibrary_t *mdl = OpenModule(file);

		if(mdl != NULL) {
			ds_printf("DS_OK: Opened module \"%s\"\n",
			          mdl->lib_get_name());
		}
		return CMD_OK;
	}

	if(mclose) {
		if(CloseModule(library_lookup(name))) {
			ds_printf("DS_OK: Module \"%s\" closed.\n", name);
			return CMD_OK;
		} else {
			ds_printf("DS_ERROR: Can't close module: %s\n", name);
			return CMD_ERROR;
		}
	}

	if(!printlist) {
		ds_printf("DS_ERROR: There is no option.\n");
		return CMD_NO_ARG;
	}

	return CMD_OK;
}


static int builtin_thread(int argc, char *argv[]) {

	if(argc == 1) {
		ds_printf("Usage: %s option\n\n"
		          "Options: \n"
		          " -l, --list     -Print list of threads\n"
		          " -q, --queue    -Print list of threads queue\n\n"
		          "Example: %s --list\n", argv[0], argv[0]);
		return CMD_NO_ARG;
	}

	int printlist = 0, printlist_queue = 0;

	struct cfg_option options[] = {
		{"list",  'l', NULL, CFG_BOOL, (void *) &printlist, 0},
		{"queue", 'q', NULL, CFG_BOOL, (void *) &printlist_queue, 0},
		CFG_END_OF_LIST
	};

	CMD_DEFAULT_ARGS_PARSER(options);

	if(printlist) {

		thd_pslist(ds_printf);

	} else if(printlist_queue) {

		thd_pslist_queue(ds_printf);
	}

	return CMD_OK;
}


static int builtin_event(int argc, char *argv[]) {

	if(argc == 1) {
		ds_printf("Usage: event option args...\n\n"
		          "Options: \n"
		          " -r, --remove        -Remove event\n"
		          " -s, --state         -Set event state (0 or 1)\n"
		          " -p, --printlist     -Print list of events\n\n"
		          "Arguments: \n"
		          " -m, --name          -Event name\n\n"
		          "Examples: event -s 0 -n VKB\n"
		          "          event --remove --name VKB\n");
		return CMD_NO_ARG;
	}

	int remove = 0, printlist = 0, state = -1;
	char *name = NULL;

	struct cfg_option options[] = {
		{"remove",    'd', NULL,    CFG_BOOL, (void *) &remove,   0},
		{"state",  's', NULL,    CFG_INT, (void *) &state, 0},
		{"printlist",  'p', NULL,    CFG_BOOL, (void *) &printlist, 0},
		{"name",      'n', NULL,    CFG_STR,  (void *) &name,      0},
		CFG_END_OF_LIST
	};

	CMD_DEFAULT_ARGS_PARSER(options);

	if(remove) {
		if(name == NULL) {
			ds_printf("DS_ERROR: Too few arguments. (event name) \n");
			return CMD_NO_ARG;
		}
		RemoveEvent(GetEventByName(name));
		return CMD_OK;
	}


	if(printlist) {

		Item_list_t *events = GetEventList();
		Event_t *e;
		Item_t *i;

		ds_printf("\n Name     State    Type\n");
		ds_printf("-------------------------\n\n");

		SLIST_FOREACH(i, events, list) {

			e = (Event_t *) i->data;
			ds_printf(" %s     %s     %s\n", e->name, e->state ? "Sleep" : "Active", e->type == EVENT_TYPE_INPUT ? "Input" : "Video");
		}

		ds_printf("\n-------------------------\n");
		ds_printf(" End of list.\n\n");
	}


	if(state > -1) {

		if(name == NULL) {
			ds_printf("DS_ERROR: Too few arguments. (event name) \n");
			return CMD_NO_ARG;
		}

		SetEventState(GetEventByName(name), state);
		return CMD_OK;
	}

	if(!printlist) ds_printf("DS_ERROR: There is no option.\n");
	return CMD_OK;
}



static int builtin_app(int argc, char *argv[]) {

	if(argc == 1) {
		ds_printf("Usage: app option args...\n\n"
		          "Options: \n"
		          " -a, --add          -Add app\n"
		          " -r, --remove       -Remove app\n"
		          " -o, --open         -Open app\n"
		          " -c, --close        -Close app\n"
		          " -s, --sleep        -Sleep app\n"
		          " -u, --unload       -Unload old apps\n"
		          " -p, --printlist    -Print apps list\n\n"
		          "Arguments: \n"
		          " -n, --name         -App name\n"
		          " -f, --file         -App file\n"
		          " -i, --id           -App ID\n"
		          " -g, --args         -App args\n"
		          "Examples: app -a -f /cd/apps/test/app.xml\n"
		          "          app --remove --id 1\n");
		return CMD_OK;
	}

	int add = 0, remove = 0, open = 0, close = 0, sleep = 0, printlist = 0, id = 0, unload = 0;
	char *file = NULL, *name = NULL, *arg = NULL;
	App_t *a;

	struct cfg_option options[] = {
		{"add",      'a', NULL,    CFG_BOOL, (void *) &add,      0},
		{"remove",   'r', NULL,    CFG_BOOL, (void *) &remove,   0},
		{"open",     'o', NULL,    CFG_BOOL, (void *) &open,   0},
		{"close",    'c', NULL,    CFG_BOOL, (void *) &close,   0},
		{"sleep",    's', NULL,    CFG_BOOL, (void *) &sleep,   0},
		{"unload",   'u', NULL,    CFG_BOOL, (void *) &unload,      0},
		{"printlist",'p', NULL,    CFG_BOOL, (void *) &printlist, 0},
		{"name",     'n', NULL,    CFG_STR,  (void *) &name,      0},
		{"file",     'f', NULL,    CFG_STR,  (void *) &file,     0},
		{"id",       'i', NULL,    CFG_INT,  (void *) &id,     0},
		{"args",     'g', NULL,    CFG_STR+CFG_LEFTOVER_ARGS,  (void *) &arg,     0},
		CFG_END_OF_LIST
	};

	CMD_DEFAULT_ARGS_PARSER(options);

	if(remove || open || close) {

		if(name == NULL && id == 0) {
			ds_printf("DS_ERROR: Too few arguments. (app ID/Name) \n");
			return CMD_NO_ARG;
		}
	}

	if(printlist) {

		Item_list_t *apps = GetAppList();
		Item_t *i;

		ds_printf("\n ID   Name    Version    File    State\n\n");

		SLIST_FOREACH(i, apps, list) {
			a = (App_t *) i->data;
			ds_printf(" %d    %s    %s    %s    0x%x\n", a->id, a->name, a->ver, a->fn, a->state);
		}

		ds_printf("\n End of list.\n\n");
		return CMD_OK;
	}


	if(add) {

		if(file == NULL) {
			ds_printf("DS_ERROR: Too few arguments. (filename) \n");
			return CMD_NO_ARG;
		}

		a = AddApp(file);

		if(a == NULL) {
			ds_printf("DS_ERROR: Can't add app\n");
			return CMD_ERROR;
		}

		ds_printf("DS_OK: App '%s' added, id = %d\n", a->name, a->id);
		return CMD_OK;
	}

	if(unload) {
		UnLoadOldApps();
		ds_printf("DS_OK: Old apps unloaded.\n");
		return CMD_OK;
	}

	if(id) {
		a = GetAppById(id);
	} else {
		a = GetAppByName(name);
	}

	if(a == NULL) {
		ds_printf("DS_ERROR: Can't find app '%s' (%d)\n", id ? "Unknown" : a->name, id);
		return CMD_ERROR;
	}

	if(sleep) {
		ds_printf("DS_INF: Temporarily it is not supported.\n");
		return CMD_OK;
	}

	if(remove) {

		if(!RemoveApp(a)) {
			ds_printf("DS_ERROR: Can't remove app '%s' with id = %d \n", a->name, a->id);
			return CMD_ERROR;
		}

		ds_printf("DS_OK: App removed.\n", name);
		return CMD_OK;
	}

	if(open) {

		if(!OpenApp(a, arg)) {
			ds_printf("DS_ERROR: Can't open app '%s' with id = %d\n", a->name, a->id);
			return CMD_ERROR;
		}

		//ds_printf("DS_OK: App opened.\n");
		return CMD_OK;
	}

	if(close) {

		if(!CloseApp(a, 1)) {
			ds_printf("DS_ERROR: Can't close app '%s' with id = %d\n", a->name, a->id);
			return CMD_ERROR;
		}

		//ds_printf("DS_OK: App closed.\n");
		return CMD_OK;
	}

	ds_printf("DS_ERROR: There is no option.\n");
	return CMD_ERROR;
}


static int builtin_cmd(int argc, char *argv[]) {

	if(argc == 1) {
		ds_printf("Usage: cmd option args...\n\n"
		          "Options: \n"
		          //" -a, --add          -Add new command\n"
		          " -r, --remove       -Remove command\n"
		          " -s, --shutdown     -Shutdown all commands\n\n"
		          "Arguments: \n"
		          " -n, --name         -Command name\n"
		          //" -h, --helpmsg      -Help message for - 'help cmd_name'\n"
		          "Examples: cmd --remove --name luacmd\n");
		return CMD_NO_ARG;
	}

	int remove = 0, shutdown = 0;
	char *cmd = NULL;

	struct cfg_option options[] = {
		//{"add",       'a', NULL,    CFG_BOOL, (void *) &add,      0},
		{"remove",    'r', NULL,    CFG_BOOL, (void *) &remove,   0},
		{"shutdown",  's', NULL,    CFG_BOOL, (void *) &shutdown, 0},
		{"name",      'n', NULL,    CFG_STR,  (void *) &cmd,      0},
		//{"helpmsg",   'h', NULL,    CFG_STR+CFG_MULTI+CFG_LEFTOVER_ARGS,  (void *) &premsg,      0},
		CFG_END_OF_LIST
	};

	CMD_DEFAULT_ARGS_PARSER(options);

	if(remove) {
		if(cmd) RemoveCmd(GetCmdByName(cmd));
		else ds_printf("DS_ERROR: Too few arguments.\n");
		return CMD_OK;
	}

	if(shutdown) {
		ShutdownCmd();
		return CMD_OK;
	}

	ds_printf("DS_ERROR: There is no option.\n");
	return CMD_NO_ARG;
}



static int builtin_console(int argc, char *argv[]) {

	if(argc == 1) {
		ds_printf("Usage: console option args...\n"
		          "Options: \n"
		          " -s, --show          -Show console\n"
		          " -h, --hide          -Hide console\n"
		          " -b, --background    -Set background\n"
		          " -p, --position      -Set position\n"
		          " -r, --resize        -Resize console\n"
		          " -a, --alpha         -Console alpha\n"
		          " -u, --update        -Update console(redraw)\n");
		ds_printf("Arguments: \n"
		          " -w, --width         -Console width\n"
		          " -h, --height        -Console hight\n"
		          " -x, --xpos          -Console/Background x position\n"
		          " -y, --ypos          -Console/Background y position\n"
		          " -t, --transparency  -Alpha transparency\n"
		          " -f, --file          -Background image file\n\n");
		ds_printf("Examples: console -r -x 0 -y 0 -w 480 -h 272\n"
		          "          console -p -x 0 -y 0\n");
		return CMD_NO_ARG;
	}

	int show = 0, hide = 0, toggle = 0, back = 0, pos = 0, resize = 0, alpha = 0, update = 0;
	int w = 0, h = 0, x = 0, y = 0, t = 0;
	char *file = NULL;
	char fn[MAX_FN_LEN];

	struct cfg_option options[] = {
		{"show",         's', NULL,    CFG_BOOL, (void *) &show,   0},
		{"hide",         'h', NULL,    CFG_BOOL, (void *) &hide,   0},
		{"toggle",       'g', NULL,    CFG_BOOL, (void *) &toggle, 0},
		{"background",   'b', NULL,    CFG_BOOL, (void *) &back,   0},
		{"position",     'p', NULL,    CFG_BOOL, (void *) &pos,    0},
		{"resize",       'r', NULL,    CFG_BOOL, (void *) &resize, 0},
		{"alpha",        'a', NULL,    CFG_BOOL, (void *) &alpha,  0},
		{"update",       'u', NULL,    CFG_BOOL, (void *) &update, 0},
		{"width",        'w', NULL,    CFG_INT,  (void *) &w,      0},
		{"height",       'h', NULL,    CFG_INT,  (void *) &h,      0},
		{"xpos",         'x', NULL,    CFG_INT,  (void *) &x,      0},
		{"ypos",         'y', NULL,    CFG_INT,  (void *) &y,      0},
		{"transparency", 't', NULL,    CFG_INT,  (void *) &t,      0},
		{"file",         'f', NULL,    CFG_STR,  (void *) &file,   0},
		CFG_END_OF_LIST
	};

	CMD_DEFAULT_ARGS_PARSER(options);

	if(show) {
		ShowConsole();
		return CMD_OK;
	}

	if(hide) {
		HideConsole();
		return CMD_OK;
	}

	if(toggle) {
		ToggleConsole();
		return CMD_OK;
	}

	if(back) {
		if(file) {
			realpath(file, fn);
			CON_Background(GetConsole(), fn, x, y);
		} else {
			ds_printf("DS_ERROR: Too few arguments.\n");
			return CMD_NO_ARG;
		}

		return CMD_OK;
	}

	if(pos) {
		CON_Position(GetConsole(), x, y);
		return CMD_OK;
	}

	if(resize) {
		SDL_Rect rect;
		rect.x = x;
		rect.y = y;
		rect.w = w;
		rect.h = h;
		CON_Resize(GetConsole(), rect);
		return CMD_OK;
	}

	if(alpha) {
		CON_Alpha(GetConsole(), t);
		return CMD_OK;
	}

	if(update) {
		CON_UpdateConsole(GetConsole());
		return CMD_OK;
	}

	ds_printf("DS_ERROR: There is no option.\n");
	return CMD_NO_ARG;
}



int builtin_gzip(int argc, char *argv[]) {

	if(argc < 3) {
		ds_printf("Usage: %s option infile outfile\n"
		          "Options: \n"
		          " -s      -Show real file size\n"
		          " -9      -Compress with mode from 0 to 9\n"
		          " -d      -Decompress\n\n", argv[0]);
		ds_printf("Examples: %s -9 /cd/file.dat /ram/file.dat.gz\n"
		          "          %s -d /ram/file.dat.gz /ram/file.dat\n", argv[0], argv[0]);
		return CMD_NO_ARG;
	}


	char src[MAX_FN_LEN];
	char dst[MAX_FN_LEN];
	char buff[512];
	int len = 0;

	realpath(argv[2], src);

	if(argc < 4) {

		char tmp[MAX_FN_LEN];
		relativeFilePath_wb(tmp, src, strrchr(src, '/'));
		sprintf(dst, "%s.gz", tmp);

	} else {
		realpath(argv[3], dst);
	}

	if(!strncmp("-s", argv[1], 2)) {

		uint32 size = gzip_get_file_size(src);
		ds_printf("DS_OK: File size is %ld bytes\n", size);
		return CMD_OK;

	} else if(!strncmp("-d", argv[1], 2)) {

		ds_printf("DS_PROCESS: Decompressing '%s' ...\n", src);
		gzFile s = NULL;
		file_t d = -1;

		if((s = gzopen(src, "rb")) == NULL) {
			ds_printf("DS_ERROR: Can't open file: %s\n", src);
			return CMD_ERROR;
		}

		d = fs_open(dst, O_CREAT | O_WRONLY);

		if(d < 0) {
			ds_printf("DS_ERROR: Can't create file: %s\n", dst);
			gzclose(s);
			return CMD_ERROR;
		}

		while((len = gzread(s, buff, sizeof(buff))) > 0) {
			fs_write(d, buff, len);
		}

		gzclose(s);
		fs_close(d);
		ds_printf("DS_OK: File decompressed.");

	} else if(argv[1][1] >= '0' && argv[1][1] <= '9') {

		char mode[3];
		sprintf(mode, "wb%c", argv[1][1]);
		ds_printf("DS_PROCESS: Compressing '%s' with mode '%c' ...\n", src, argv[1][1]);

		gzFile d = NULL;
		file_t s = -1;

		if((d = gzopen(dst, mode)) == NULL) {
			ds_printf("DS_ERROR: Can't create file: %s\n", dst);
			return CMD_ERROR;
		}

		s = fs_open(src, O_RDONLY);

		if(s < 0) {
			ds_printf("DS_ERROR: Can't open file: %s\n", src);
			gzclose(d);
			return CMD_ERROR;
		}

		while((len = fs_read(s, buff, sizeof(buff))) > 0) {

			if(gzwrite(d, buff, len) <= 0) {
				ds_printf("DS_ERROR: Error writing to file: %s\n", dst);
				gzclose(d);
				fs_close(s);
				return CMD_ERROR;
			}
		}

		gzclose(d);
		fs_close(s);
		ds_printf("DS_OK: File compressed.");

	} else {
		return CMD_NO_ARG;
	}

	return CMD_OK;
}


static int builtin_speedtest(int argc, char *argv[]) {

	uint8 *buff = (uint8*)0x8c400000;
	size_t buff_size = 0x10000;
	int rd = 0, size = 0, cnt = 0, rc = 0;
	int64 time_before = 0, time_after = 0;
	uint32 t = 0;
	float speed = 0.0f;
	file_t fd;

	if (argc < 2) {
		ds_printf("Usage: speedtest file [buffsize]\nInfo: if file exists, then testing read,"
		          "\n otherwise testing write (need 8 Mb free space!).\n");
		return CMD_NO_ARG;
	}

	fd = fs_open(argv[1], O_RDONLY);

	if(fd != FILEHND_INVALID) {

		rd = 1;

	} else {

		fd = fs_open(argv[1], O_CREAT | O_WRONLY);

		if (fd == FILEHND_INVALID) {
			ds_printf("DS_ERROR: Can't open %s for write: %d\n", argv[1], errno);
			return CMD_ERROR;
		}
	}

	if(argc > 2) {
		buff_size = atoi(argv[2]);
	}

	ds_printf("DS_PROCESS: Testing %s speed...\n", (rd ? "read" : "write"));

	if(rd) {

		size = fs_total(fd);
		buff = (uint8 *) memalign(32, buff_size);

		if(buff == NULL) {
			fs_close(fd);
			ds_printf("DS_ERROR: No free memory\n");
			return CMD_ERROR;
		}

		ShutdownVideoThread();
		time_before = timer_ms_gettime64();

		while(cnt < size) {

			rc = fs_read(fd, buff, buff_size);

			if(rc < 0) {
				fs_close(fd);
				InitVideoThread();
				ds_printf("DS_ERROR: Can't read file: %d\n", errno);
				return CMD_ERROR;
			}

			cnt += rc;
		}

	} else {

		if(!strncmp(argv[1], "/ram", 4)) {
			size = 0x400000;
		} else {
			size = 0x800000;
		}

		/* Pre-allocate clusters */
		fs_seek(fd, size, SEEK_SET);
		fs_seek(fd, 0, SEEK_SET);

		ShutdownVideoThread();
		time_before = timer_ms_gettime64();

		while(cnt < size) {

			if(fs_write(fd, buff, buff_size) != buff_size) {
				fs_close(fd);
				InitVideoThread();
				ds_printf("DS_ERROR: Can't write to file: %d\n", errno);
				return CMD_ERROR;
			}

			buff += buff_size;
			cnt += buff_size;
		}
	}

	time_after = timer_ms_gettime64();
	InitVideoThread();

	t = (uint32)(time_after - time_before);
	speed = size / ((float)t / 1000);

	if(rd) {
		free(buff);
	}

	fs_close(fd);

	ds_printf("DS_OK: Complete!\n"
	          " Test: %s\n Time: %d ms\n"
	          "Speed: %.2f Kbytes/s (%.2f Mbit/s)\n"
	          " Size: %d Kb\n Buff: %d Kb\n",
	          (rd ? "read" : "write"), t,
	          speed / 1024,
	          ((speed / 1024) / 1024) * 8,
	          size / 1024,
	          buff_size / 1024);

	return CMD_OK;
}


static int builtin_net(int argc, char *argv[]) {

	if(argc == 1) {
		ds_printf("Usage: %s option args...\n"
		          "Options: \n"
		          " -i, --init        -Initialize network\n"
		          " -s, --shutdown    -Shutdown network\n", argv[0]);
		ds_printf("Arguments: \n"
		          " -a, --ip          -IP address\n\n");
		ds_printf("Example: %s --init\n", argv[0]);
		return CMD_NO_ARG;
	}

	int initnet = 0, shutnet = 0;
	char *ip_str = NULL;
	int ipi[4];

	union {
		uint32 ipl;
		uint8 ipb[4];
	} ip;

	struct cfg_option options[] = {
		{"init",     'i', NULL, CFG_BOOL, (void *) &initnet, 0},
		{"shutdown", 's', NULL, CFG_BOOL, (void *) &shutnet, 0},
		{"ip",       'a', NULL, CFG_STR,  (void *) &ip_str,  0},
		CFG_END_OF_LIST
	};

	CMD_DEFAULT_ARGS_PARSER(options);

	if(initnet) {

		ip.ipl = 0;

		if(ip_str) {
			sscanf(ip_str, "%d.%d.%d.%d", &ipi[3], &ipi[2], &ipi[1], &ipi[0]);
			ip.ipb[0] = (uint8)ipi[0] & 0xff;
			ip.ipb[1] = (uint8)ipi[1] & 0xff;
			ip.ipb[2] = (uint8)ipi[2] & 0xff;
			ip.ipb[3] = (uint8)ipi[3] & 0xff;
			ds_printf("DS_PROCESS: Initializing network with IP %s\n", ip_str);
		} else {
			ds_printf("DS_PROCESS: Initializing network\n");
		}

		if(InitNet(ip.ipl) < 0) {
			ds_printf("DS_ERROR: Can't initialize network\n");
			return CMD_ERROR;
		}
		return CMD_OK;
	}

	if(shutnet) {
		ShutdownNet();
		return CMD_OK;
	}
	return CMD_NO_ARG;
}


static int builtin_callfunc(int argc, char *argv[]) {

	if(argc == 1) {
		ds_printf("Usage: %s function_name arg\n", argv[0]);
		ds_printf("Example: %s ScreenFadeOut\n");
		return CMD_NO_ARG;
	}

	int r = 0;
	uint32 func_addr = GET_EXPORT_ADDR(argv[1]);

	if(func_addr > 0 && func_addr != 0xffffffff) {
		EXPT_GUARD_BEGIN;
		void (*cb_func)(void *) = (void (*)(void *))func_addr;
		cb_func(argc > 2 ? argv[1] : NULL);
		r = 1;
		EXPT_GUARD_CATCH;
		r = 0;
		EXPT_GUARD_END;
	}

	ds_printf("DS_%s: %s at 0x%08lx\n", (r ? "OK" : "ERROR"), argv[1], func_addr);
	return r ? CMD_OK : CMD_ERROR;
}


/* Setup all our builtins */
int InitCmd() {

	if((cmds = listMake()) != NULL) {

		/* table of the builtin commands, their help mesg,  and their handler funcs */
		AddCmd("help",      "This message", (CmdHandler *) builtin_help);
		AddCmd("ls",        "List contents of directories(flag -l for print files size)", (CmdHandler *) builtin_ls);
		AddCmd("cd",        "Change directory", (CmdHandler *) builtin_cd);
		AddCmd("pwd",       "Print the current directory(flag -get for store dir, flag -go to change stored dir)", (CmdHandler *) builtin_pwd);
		AddCmd("env",       "Show or set environment variables", (CmdHandler *) builtin_env);
		AddCmd("clear",     "Clear the console screen",	(CmdHandler *) builtin_clear);
		AddCmd("echo",      "Echo text to the console",	(CmdHandler *) builtin_echo);
		AddCmd("cat",       "Display text files to the console",	(CmdHandler *) builtin_cat);
		AddCmd("cp",        "Copy files and directories", (CmdHandler *) builtin_cp);
		AddCmd("rm",        "Remove files",	(CmdHandler *) builtin_rm);
		AddCmd("rename",    "Rename Files", (CmdHandler *) builtin_rename);
		AddCmd("mkdir",     "Create a directory", (CmdHandler *) builtin_mkdir);
		AddCmd("rmdir",     "Delete a directory", (CmdHandler *) builtin_rmdir);
		AddCmd("mkpath",    "Create a path", (CmdHandler *) builtin_mkpath);
		AddCmd("exec",      "Load and exec another program", (CmdHandler *) builtin_exec);
		AddCmd("romdisk",   "Mount/unmount a romdisk image", (CmdHandler *) builtin_romdisk);
		AddCmd("periphs",   "Info about attached peripherals", (CmdHandler *) builtin_periphs);
		AddCmd("mstats",    "View Memory usage", (CmdHandler *) builtin_mstats);
		AddCmd("sleep",     "Sleep in milliseconds", (CmdHandler *) builtin_sleep);
		AddCmd("read",      "Reading from file", (CmdHandler *) builtin_read);
		AddCmd("stat",      "Print file(or dir) statistic", (CmdHandler *) builtin_statf);
		AddCmd("gzip",      "Gzip archiver", (CmdHandler *) builtin_gzip);
		AddCmd("cmd",       "Command manager", (CmdHandler *) builtin_cmd);
		AddCmd("screenshot","Screenshot", (CmdHandler *) builtin_screenshot);
		AddCmd("dc",        "Dreamcast system functions", (CmdHandler *) builtin_dc);
		AddCmd("addr",      "Address manager", (CmdHandler *) builtin_addr);
		AddCmd("dsc",       "CMD script interpreter", (CmdHandler *) builtin_dsc);
		AddCmd("lua",       "Lua script interpreter", (CmdHandler *) builtin_lua);
		AddCmd("net",       "Network", builtin_net);
		AddCmd("module",    "Modules manager", (CmdHandler *) builtin_module);
		AddCmd("thread",    "Threads manager", (CmdHandler *) builtin_thread);
		AddCmd("event",     "Events manager", (CmdHandler *) builtin_event);
		AddCmd("app",       "Apps manager", (CmdHandler *) builtin_app);
		AddCmd("console",   "Console manager", (CmdHandler *) builtin_console);
		AddCmd("speedtest", "Testing r/w speed of device", (CmdHandler *) builtin_speedtest);
		AddCmd("callfunc",  "Call any exported function", (CmdHandler *) builtin_callfunc);
		//ds_printf("Command list initialised.\n");
		return 1;
	}

	return 0;
}
