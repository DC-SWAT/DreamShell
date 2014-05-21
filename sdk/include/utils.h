/** 
 * \file    utils.h
 * \brief   DreamShell utils
 * \date    2004-2014
 * \author  SWAT www.dc-swat.ru
 */


#ifndef _DS_UTILS_H
#define _DS_UTILS_H


/* Macros v2 for accessing/creating DreamShell version codes */
	
#define DS_VER_MAJOR(v) ((v >> 24) & 0xff)
#define DS_VER_MINOR(v) ((v >> 16) & 0xff)
#define DS_VER_MICRO(v) ((v >> 8) & 0xff)
#define DS_VER_BUILD(v) ((v >> 0) & 0xff)

/* Divide build to type and number (if not used build counter) */
#define DS_VER_BUILD_TYPE(b) 		(b & 0xf0) / 0xf)
#define DS_VER_BUILD_TYPE_STR(b) 	(GetVersionBuildTypeString(DS_VER_BUILD_TYPE(b))
#define DS_VER_BUILD_NUM(b) 		(b & 0x0f)

#define DS_MAKE_VER(MAJOR, MINOR, MICRO, BUILD) \
					( (((MAJOR) & 0xff) << 24) 	\
					| (((MINOR) & 0xff) << 16) 	\
					| (((MICRO) & 0xff) << 8) 	\
					| (((BUILD) & 0xff) << 0) )


/* Return 0 if error */
int FileSize(const char *fn);
int FileExists(const char *fn);
int DirExists(const char *dir);
int PeriphExists(const char *name);
int CopyFile(const char *src_fn, const char *dest_fn, int verbose);
int CopyDirectory(const char *src_path, const char *dest_path, int verbose);

void arch_shutdown();

/* Use thd_sleep, SDL_Delay or timer_spin_sleep instead it */
void ds_sleep(int ms) __attribute__((deprecated)); 

int flashrom_get_region_only();
int is_custom_bios();
int is_no_syscalls();

uint32 gzip_get_file_size(const char *filename);

void dbgio_set_dev_ds();
void dbgio_set_dev_scif();
void dbgio_set_dev_fb();
void dbgio_set_dev_sd();


char *realpath(const char *path, char resolved[PATH_MAX]);
int makeabspath_wd(char *buff, char *path, char *dir, size_t size);
const char *relativeFilePath(const char *rel, const char *file);
int relativeFilePath_wb(char *buff, const char *rel, const char *file);
char *getFilePath(const char *file);
int mkpath(const char *path);


void readstr(FILE *f, char *string);
int splitline(char **argv, int max, char *line);
int hex_to_int(char c);
char *strtolower(char *str);
char *strsep(char **stringp, const char *delim);
char* substring(const char* str, size_t begin, size_t len);

#if defined(__STRICT_ANSI__)
char *strndup(const char *, size_t);
#endif

#endif
