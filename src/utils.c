/****************************
 * DreamShell ##version##   *
 * utils.c                  *
 * DreamShell Utils         *
 * (c)2004-2024 SWAT        *
 * http://www.dc-swat.ru    *
 ***************************/

#include "ds.h"
#include "video.h"
#include <dc/sound/sound.h>
#include "drivers/aica_cmd_iface.h"
#include <sys/time.h>


int snd_init_firmware(const char *filename) {
	file_t f;
	size_t sz;
	uint8 *buff;
	
	f = fs_open(filename, O_RDONLY);
	
	if(f < 0) {
		ds_printf("DS_ERROR: Can't open firmware %s\n", filename);
		return -1;
	}
	
	sz = fs_total(f);
	ds_printf("DS_PROCESS: Loading firmware %s (%d bytes) into SPU RAM\n", filename, sz);
	
	buff = (uint8*)malloc(sz);
	fs_read(f, buff, sz);
	fs_close(f);
	
	spu_disable();
	spu_memset(0, 0, AICA_RAM_START);
	spu_memload(0, buff, sz);

	/* Enable the AICA and give it a few ms to start up */
	spu_enable();
	timer_spin_sleep(10);

	/* Initialize the RAM allocator */
	snd_mem_init(AICA_RAM_START);
	return 0;
}


int flashrom_get_region_only() {
	
	int start, size;
	uint8 region[6] = { 0 };
	region[2] = *(uint8*)0x0021A002;

	/* Find the partition */
	if(flashrom_info(FLASHROM_PT_SYSTEM, &start, &size) < 0) {
		
		dbglog(DBG_ERROR, "%s: can't find partition %d\n", __func__, FLASHROM_PT_SYSTEM);
		
	} else {

		/* Read the first 5 characters of that partition */
		if(flashrom_read(start, region, 5) < 0) {
			dbglog(DBG_ERROR, "%s: can't read partition %d\n", __func__, FLASHROM_PT_SYSTEM);
		}
	}

	if(region[2] == 0x58 || region[2] == 0x30) {
		return FLASHROM_REGION_JAPAN;
	} else if(region[2] == 0x59 || region[2] == 0x31) {
		return FLASHROM_REGION_US;
	} else if(region[2] == 0x5A || region[2] == 0x32) {
		return FLASHROM_REGION_EUROPE;
	} else {
		dbglog(DBG_ERROR, "%s: Unknown region code %02x\n", __func__, region[2]);
		return FLASHROM_REGION_UNKNOWN;
	}
}

int is_hacked_bios() {
	return (*(uint16 *)0xa0000000) == 0xe6ff;
}

int is_custom_bios() {
	return (*(uint16 *)0xa0000004) == 0x4318;
}

int is_no_syscalls() {
	return (*(uint16 *)0xac000100) != 0x2f06;
}


uint32 gzip_get_file_size(const char *filename) {
	
	file_t fd;
	uint32 len;
	uint32 size;
	
	fd = fs_open(filename, O_RDONLY);
	
	if(fd < 0)
		return 0;
	
	if(fs_seek(fd, -4, SEEK_END) <= 0) {
		fs_close(fd);
		return 0;
	}
	
	len = fs_read(fd, &size, sizeof(size));
	fs_close(fd);
	
	if(len != 4) {
		return 0;
	}
	
	return size;
}


void readstr( FILE *f, char *string ) {
	/* Loop Until End Of Line Is Reached */
	do {
		/* Gets A String Of 255 Chars Max From f (File) */
		fgets(string, 255, f);
	} while ( ( string[0] == '/' ) || ( string[0] == '\n' ) );

	return;
}


int FileSize(const char *fn) {

	int size;
	file_t f = fs_open(fn, O_RDONLY); 

	if(f == FILEHND_INVALID) {
		return 0; 
	}

	size = fs_total(f);
	fs_close(f);
	return size;
}

int FileExists(const char *fn) {
	file_t f = fs_open(fn, O_RDONLY);

	if(f == FILEHND_INVALID) {
		return 0;
	}

	fs_close(f);
	return 1;
}

int DirExists(const char *dir) {
	file_t f = fs_open(dir, O_DIR | O_RDONLY);

	if(f == FILEHND_INVALID) {
		return 0;
	}

	fs_close(f);
	return 1;
}

int CopyFile(const char *src_fn, const char *dest_fn, int verbose) {
	
	size_t cnt, size, cur = 0, buf_size;
	file_t src_fd, dest_fd;
	uint8 *buff;
	
	if(verbose) {
		ds_printf("DS_PROCESS: Copying file: %s to %s\n", src_fn, dest_fn);
	}

	src_fd = fs_open(src_fn, O_RDONLY);

	if (src_fd == FILEHND_INVALID) {
		ds_printf("DS_ERROR: Can't open %s for read\n", src_fn);
		return 0;
	}

	dest_fd = fs_open(dest_fn, O_WRONLY | O_CREAT);

	if (dest_fd == FILEHND_INVALID) {
		ds_printf("DS_ERROR: Can't open %s for write\n", dest_fn);
		fs_close(src_fd);
		return 0;
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
		return 0;
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
	return 1;
}

int CopyDirectory(const char *src_path, const char *dest_path, int verbose) {
	
	file_t fd = fs_open(src_path, O_RDONLY | O_DIR);

	if(fd != FILEHND_INVALID) {
		
		char Path[NAME_MAX], *EndPtr = Path;
		char PathDest[NAME_MAX], *EndDestPtr = PathDest;
		dirent_t *e;
		
		strcpy(Path, src_path);
		Path[strlen(src_path)] = '/';
		EndPtr += (strlen(src_path)+1);
		
		strcpy(PathDest, dest_path);
		
		if(!DirExists(PathDest)) {
			
			if(verbose) {
				ds_printf("DS_PROCESS: Making directory: %s\n", PathDest);
			}
			
			if(fs_mkdir(PathDest)) {
				ds_printf("DS_ERROR: Can't make directory %s\n", PathDest);
				fs_close(fd);
				return 0;
			}
		}
		
		PathDest[strlen(dest_path)] = '/';
		EndDestPtr += (strlen(dest_path)+1);

		while((e = fs_readdir(fd)) != NULL) 
		{
			if (!strcmp(e->name, ".") || !strcmp(e->name, ".."))
				continue;
			
			strcpy(EndPtr, e->name);
			strcpy(EndDestPtr, e->name);
			
			if(e->attr == O_DIR) {
				
				if(verbose) {
					ds_printf("DS_PROCESS: Making directory: %s\n", PathDest);
				}
				
				if(!fs_mkdir(PathDest)) {
					CopyDirectory(Path, PathDest, verbose);
				} else {
					ds_printf("DS_ERROR: Can't make directory %s\n", PathDest);
					fs_close(fd);
					return 0;
				}
			} else {
				if(!CopyFile(Path, PathDest, verbose)) {
					fs_close(fd);
					return 0;
				}
			}
		}
		
		fs_close(fd);
		return 1;
	}
	
	return 0;
}


int PeriphExists(const char *name) {
	int p, u;
	maple_device_t *dev;

	for (p = 0; p < MAPLE_PORT_COUNT; p++) {
		for (u = 0; u < MAPLE_UNIT_COUNT; u++) {
			dev = maple_enum_dev(p, u);
			if (dev) {
				/*
                ds_printf("%c%c: %s (%08lx: %s)\n",
					'A' + p, '0' + u,
					dev->info.product_name,
					dev->info.functions,
					maple_pcaps(dev->info.functions));
				*/
//				ds_printf("Piriph: [%s] [%s] [%s]\n", 
//						maple_pcaps(dev->info.functions), dev->info.product_name, dev->drv->name);
						
				if(!strcasecmp(maple_pcaps(dev->info.functions), name)) 
					return 1;
			}
		}
	}
	return 0;
}

int hex_to_int(char c) {
	
	if(c>='0' && c <='9') {
		return c-'0';
	}
	
	if(c>='a' && c <='f') {
		return 10+(c-'a');
	}
	
	if(c>='A' && c <='F') {
		return 10+(c-'A');
	}
	
	return -1;
}

char *strtolower(char *str) {
	int x;
	static char buf[256];
	
	if(str == NULL)
		return NULL;
	
	strcpy(buf, str);
	
	for(x = 0; x < strlen(buf); x++) {
		if(buf[x] >= 'A' && buf[x] <= 'Z') {
			buf[x] += ('a'-'A');
		}
	}
	
	return buf;
}

static char *splitnext(char **pos) {
	char *a, *d, *s;

	s = *pos;
	while (*s == ' ' || *s == '\t')
		s++;
	a = d = s;

	while (*s && *s != ' ' && *s != '\t') {
		if (*s == '"') {
			s++;
			while (*s && *s != '"') {
				if (*s == '\\')
					s++;
				if (*s)
					*(d++) = *(s++);
			}
			if (*s == '"')
				s++;
		} else {
			if (*s == '\\')
				s++;
			*(d++) = *(s++);
		}
	}
	while (*s == ' ' || *s == '\t')
		s++;
	*d = 0;
	*pos = s;
	return a;
}


int splitline(char **argv, int max, char *line) {
	char *s;
	int i = 0;

	s = line;
	
	while(*s && i < max) {
		argv[i] = splitnext(&s);
		i++;
	}
	
	if(!argv[0])
		return(0);
		
	return i;
}


char *substring(const char* str, size_t begin, size_t len)  { 
	if (str == 0 || strlen(str) == 0 || strlen(str) < begin || strlen(str) < (begin+len)) 
		return 0; 

	return strndup(str + begin, len); 
}


int access(const char *filename, int mode) {
	if(FileExists(filename) || DirExists(filename))
		return 0;
	else
		return -1;
}

int execv(const char *file, char *const *argv) {
	int argc = 1, ret = CMD_OK;
	
	while(*argv[argc-1]) {
		argc++;
	}

	ret = CallCmdFile(file, argc, (char**)argv);
	
	return ret == CMD_OK ? 0 : -1;
}

int execvp(const char *file, char *const *argv) {
	char fn[NAME_MAX];
	sprintf(fn, "%s/%s", getenv("PATH"), file);

	if(!FileExists(fn)) {
		sprintf(fn, "%s/cmds/%s", getenv("PATH"), file);
	}

	return execv(fn, argv);
}


int fchmod(int filedes, mode_t mode) {
	return 0;
}

int fchown(int filedes, uid_t owner, gid_t group) {
	return 0;
}

int chown(const char *path, uid_t owner, gid_t group) {
	return 0;
}

int ftruncate(int file, off_t length) {
	return 0;
}

int utimes(const char *filename, const struct timeval times[2]) {
	return 0;
}

gid_t geteuid(void) {
	return 1;
}

int gethostname(char *name, size_t len) {
	strncpy(name, getenv("HOST"), len);
	return 0;
}

//int sethostname(const char *name, size_t len) {
//	setenv("HOST", name, 1);
//	return 0;
//}

static int do_mkdir(const char *path) {
	
    struct stat st;

    if (fs_stat(path, &st, 0) < 0) {
		
        /* Directory does not exist. EEXIST for race condition */
        if (fs_mkdir(path) != 0/* && errno != EEXIST*/)
            return -1;
			
    } else if (!(st.st_mode & S_IFDIR)) {
        errno = ENOTDIR;
        return -1;
    }

    return 0;
}

/**
 * mkpath - ensure all directories in path exist
 * Algorithm takes the pessimistic view and works top-down to ensure
 * each directory in path exists, rather than optimistically creating
 * the last element and working backwards.
 */
int mkpath(const char *path) {
    char *pp;
    char *sp;
    int  status;
    char *copypath = strdup(path);

    status = 0;
    pp = copypath;
	
    while (status == 0 && (sp = strchr(pp, '/')) != 0)
    {
        if (sp != pp)
        {
            /* Neither root nor double slash in path */
            *sp = '\0';
            status = do_mkdir(copypath);
            *sp = '/';
        }
        pp = sp + 1;
    }
	
    if (status == 0)
        status = do_mkdir(path);
		
    free(copypath);
    return (status);
}



/* read a pathname based on the current directory and turn it into an abs one, we
   don't check for validity, or really do anything except handle ..'s and .'s */
int makeabspath_wd(char *buff, char *path, char *dir, size_t size) {

	int numtxts;
	int i;
	char *txts[32];		/* max of 32...should be nuff */
	char *rslash;

	numtxts = 0;

	/* check if path is already absolute */
	if (path[0] == '/') {
		strncpy(buff, path, size);
		return 0;
	}

	/* split the path into tokens */
	for (numtxts = 0; numtxts < 32;) {
		if ((txts[numtxts] = strsep(&path, "/")) == NULL)
			break;
		if (*txts[numtxts] != '\0')
			numtxts++;
	}

	/* start from the current directory */
	strncpy(buff, dir, size);

	for (i = 0; i < numtxts; i++) {
		if (strcmp(txts[i], "..") == 0) {
			if ((rslash = strrchr(buff, '/')) != NULL)
				*rslash = '\0';
		} else if (strcmp(txts[i], ".") == 0) {
			/* do nothing */
		} else {
			if (buff[strlen(buff) - 1] != '/')
				strncat(buff, "/", size - 1 - strlen(buff));
			strncat(buff, txts[i], size - 1 - strlen(buff));
		}
	}

	/* make sure it's not empty */
	if (buff[0] == '\0') {
		buff[0] = '/';
		buff[1] = '\0';
	}

	return 0;
}


// Deprecated
int makeabspath(char *buff, char *path, size_t size) {
    //return makeabspath_wd(buff, path, cwd, size);
	realpath(path, buff);
	return 1;
}

const char *relativeFilePath(const char *rel, const char *file) {
    
    if(file[0] == '/') return file;
    
    char *rslash, *dir, *ret;
    char fn[NAME_MAX];
    char buff[NAME_MAX];

    if((rslash = strrchr(rel, '/')) != NULL) {
               
        strncpy(buff, file, NAME_MAX);
        dir = substring(rel, 0, strlen(rel) - strlen(rslash));
        makeabspath_wd(fn, buff, dir, NAME_MAX);
        
        fn[strlen(fn)] = '\0';
        ret = strdup(fn);
        //ds_printf("Directory: '%s' File: '%s' Out: '%s'", dir, rslash, ret, fn);
        free(dir);
        
    } else {
        return file;
    }
    
    return ret;
}


int relativeFilePath_wb(char *buff, const char *rel, const char *file) {
    
    if(file[0] == '/') {
       strncpy(buff, file, NAME_MAX);
       return 1;
    }
/*    
    char *rslash, *dir;

    if((rslash = strrchr(rel, '/')) != NULL) {
        dir = substring(rel, 0, strlen(rel) - strlen(rslash));
        makeabspath_wd(buff, file, dir, NAME_MAX);
        ds_printf("Directory: '%s' File: '%s' Out: '%s'", dir, rel, buff);
*/

    makeabspath_wd(buff, (char*)file, getFilePath(rel), NAME_MAX);
    //ds_printf("Directory: '%s' File: '%s' Out: '%s'", path, rel, buff);
    return 1;
}


char *getFilePath(const char *file) {
     
    char *rslash;
    
    if((rslash = strrchr(file, '/')) != NULL) {
        return substring(file, 0, strlen(file) - strlen(rslash));
    }
    
    return NULL;
}


void dbgio_hex_dump(char *desc, void *addr, int len) {
    int i;
    unsigned char buff[17];
    unsigned char *pc = (unsigned char*)addr;

    // Output description if given.
    if (desc != NULL)
        dbglog(DBG_DEBUG, "%s:\n", desc);

    // Process every byte in the data.
    for (i = 0; i < len; i++) {
        // Multiple of 16 means new line (with line offset).

        if ((i % 16) == 0) {
            // Just don't print ASCII for the zeroth line.
            if (i != 0)
                dbglog(DBG_DEBUG, "  %s\n", buff);

            // Output the offset.
            dbglog(DBG_DEBUG, "  %04x ", i);
        }

        // Now the hex code for the specific character.
        dbglog(DBG_DEBUG, " %02x", pc[i]);

        // And store a printable ASCII character for later.
        if ((pc[i] < 0x20) || (pc[i] > 0x7e))
            buff[i % 16] = '.';
        else
            buff[i % 16] = pc[i];
        buff[(i % 16) + 1] = '\0';
    }

    // Pad out last line if not exactly 16 characters.
    while ((i % 16) != 0) {
        dbglog(DBG_DEBUG, "   ");
        i++;
    }

    // And print the final ASCII bit.
    dbglog(DBG_DEBUG, "  %s\n", buff);
}

char *fix_path_spaces(char *str) {
	if(!str) return NULL;

	int i, len = (int) strlen(str);

	for(i = 0; i < len; ++i) {
		if(str[i] == '\\') str[i] = ' ';
	}

	return str;
}
