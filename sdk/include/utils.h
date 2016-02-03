/** 
 * \file    utils.h
 * \brief   DreamShell utils
 * \date    2004-2016
 * \author  SWAT www.dc-swat.ru
 */


#ifndef _DS_UTILS_H
#define _DS_UTILS_H


/* Macros v2 for accessing/creating DreamShell version codes */
	
#define DS_VER_MAJOR(v) ((v >> 24) & 0xff)
#define DS_VER_MINOR(v) ((v >> 16) & 0xff)
#define DS_VER_MICRO(v) ((v >> 8) & 0xff)
#define DS_VER_BUILD(v) ((v >> 0) & 0xff)

/* Divide build to type and number (if build counter is not used) */
#define DS_VER_BUILD_TYPE(b)     ((b & 0xf0) / 0xf)
#define DS_VER_BUILD_TYPE_STR(b) (GetVersionBuildTypeString(DS_VER_BUILD_TYPE(b)))
#define DS_VER_BUILD_NUM(b)      (b & 0x0f)

enum {
	DS_VER_BUILD_TYPE_APLHA = 0,
	DS_VER_BUILD_TYPE_BETA,
	DS_VER_BUILD_TYPE_RC,
	DS_VER_BUILD_TYPE_RELEASE
};

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
int is_hacked_bios();
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

/* Optimized memory utils */
void *memcpy_sh4(void *dest, const void *src, size_t count);
void *memmove_sh4(void *dest, const void *src, size_t count);
void *memset_sh4(void *dest, uint32 val, size_t count);

#define memcpy  memcpy_sh4
#define memmove memmove_sh4
#define memset  memset_sh4

#ifndef strcasestr
char *strcasestr(const char *str1, const char *str2);
#endif

void vmu_draw_string(const char *str);
void vmu_draw_string_xy(const char *str, int x, int y);

#endif /*_DS_UTILS_H */
