/*
   DC IMG Tool
   Copyright (C)2025 megavolt85
*/

#include "internal.h"
#include "isofs/isofs.h"
#include "isofs/cdi.h"

extern char *optarg;
static int basedir_len = 0;
static int verbose = 0;
static int descrmble = 0;
static uint32_t seed;

//#define DEBUG 1

#ifdef __WIN32
char * strndup(const char * src, size_t size) {
	size_t len = strnlen(src, size);
	len = len < size ? len : size;
	char * dst = malloc(len + 1);
	
	if (!dst) {
		return NULL;
	}
	memcpy(dst, src, len);
	dst[len] = '\0';
	
	return dst;
}
#endif

#define MAXCHUNK (2048*1024)

static void my_srand(size_t n) {
	seed = n & 0xffff;
}

static uint32_t my_rand() {
	seed = (seed * 2109 + 9273) & 0x7fff;
	return (seed + 0xc000) & 0xffff;
}

static void load(int fh, uint8_t *ptr, size_t sz)
{
	if(virt_iso_read(fh, ptr, sz) != sz) {
		printf("Read error!\n");
		exit(1);
	}
}

static void load_chunk(int fh, uint8_t *ptr, size_t sz) {
	static int idx[MAXCHUNK/32];
	int i;
	
	/* Convert chunk size to number of slices */
	sz /= 32;
	
	/* Initialize index table with unity,
		so that each slice gets loaded exactly once */
	for(i = 0; i < sz; i++) {
		idx[i] = i;
	}
	
	for(i = sz-1; i >= 0; --i) {
		/* Select a replacement index */
		int x = (my_rand() * i) >> 16;
		
		/* Swap */
		int tmp = idx[i];
		idx[i] = idx[x];
		idx[x] = tmp;
		
		/* Load resulting slice */
		load(fh, ptr+32*idx[i], 32);
	}
}

static void descramle_file(int fh, uint8_t *ptr, size_t filesz)
{
	size_t chunksz;
	
	my_srand(filesz);
	
	/* Descramble 2 meg blocks for as long as possible, then
		gradually reduce the window down to 32 bytes (1 slice) */
	for(chunksz = MAXCHUNK; chunksz >= 32; chunksz >>= 1) {
		while(filesz >= chunksz) {
			load_chunk(fh, ptr, chunksz);
			filesz -= chunksz;
			ptr += chunksz;
		}
	}
	
	/* Load final incomplete slice */
	if(filesz) {
		load(fh, ptr, filesz);
	}
}

void parse_dir(const char *name)
{
	dirent_t *vdir;
	int vfd = virt_iso_open(name, O_DIR);
	
	if (vfd < 0) {
		printf("ERROR: can't open dir (%s)\n", name);
	}
	
	while ((vdir = virt_iso_readdir(vfd)) != NULL) {
		uint32_t lba;
		char fname[256];
		sprintf(fname, "%s%s", name, vdir->name);
		int ffd = virt_iso_open(fname, vdir->size == -1 ? O_DIR : 0);
		
		if (ffd < 0) {
			printf("Can't open %s\n", fname);
			lba = 0;
		}
		else {
			virt_iso_ioctl(ffd, ISOFS_IOCTL_GET_FD_LBA, &lba);
			virt_iso_close(ffd);
		}
			
		printf("%s %d %d\n", fname, vdir->size, lba);
		
		if (vdir->size == -1) {
			char dirname[256];
			sprintf(dirname, "%s%s/", name, vdir->name);
			parse_dir(dirname);
		}
	}
		
	virt_iso_close(vfd);
}

void insert_file(const char *fn, uint32_t size)
{
	int fd_in, fd_out;
	struct stat st;
	
	fd_in = open(fn, O_RDONLY
#ifdef __WIN32
							  | O_BINARY
#endif
							);
	
	if (fd_in < 0) {
		printf("ERROR: Can't open %s\n", fn);
		return;
	}
	
	fd_out = virt_iso_open(&fn[basedir_len], 0);
	
	if (fd_out < 0) {
		printf("ERROR: Can't open viso %s\n", &fn[basedir_len]);
		close(fd_in);
		return;
	}
	
	virt_iso_fstat(fd_out, &st);
	
	if (size != st.st_size) {
		printf("ERROR: in %d bytes vfs %ld bytes\t\t%s\n", size, (long int) st.st_size, &fn[basedir_len]);
	}
	else {
		uint32_t seccnt = size/2048;
		
		if (size%2048)
		{
			seccnt += 1;
		}
		
		void *buf = calloc(2048, seccnt);
		
		if (!buf) {
			printf("ERROR: can't allocate %d bytes\n", size);
		}
		else {
			int rd = read(fd_in, buf, size);
			int wr = virt_iso_write(fd_out, buf, size);
			
			if (verbose) {
				printf("rd = %d wr = %d size = %d %s\n", rd, wr, size, &fn[basedir_len]);
			}
			
			free(buf);
		}
	}
	
	virt_iso_close(fd_out);
	close(fd_in);
}

void replace_files(const char *dn)
{
	DIR *indir;
	struct dirent *de;
	
	if (!(indir = opendir(dn))) {
		printf("ERROR can't open directory %s\n",dn);
		exit(0);
	}
	
	while((de = readdir(indir))) {
		struct stat st;
		char name[2048];
		
		snprintf(name, 2047, "%s%s", dn, de->d_name);
		
		stat(name, &st);
		
		if (S_ISREG(st.st_mode)) {
			insert_file(name, st.st_size);
		}
		else {
			if (de->d_name[0] == '.') {
				continue;
			}
			strcat(name,"/");
			replace_files(name);
		}
	}
	closedir(indir);
}

static int dump_track(CDI_header_t *hdr, uint32_t lba, uint32_t count, uint32_t sec_sz, const char *dn, int cdi_fd)
{
	int fp;
	static uint8_t buf[2352];
	
	fp = open(dn, O_WRONLY | O_CREAT | O_TRUNC
#ifdef __WIN32
											   | O_BINARY
#endif
												, S_IRWXU);
	
	if (fp == -1) {
		printf("ERROR: can't create %s\n", dn);
		return -1;
	}
	
	for (uint32_t i = 0; i < count; i++) {
		if(cdi_read_sectors(hdr, cdi_fd, buf, lba+i, 1) < 0) {
			printf("ERROR: read sector\n");
			close(fp);
			return -1;
		}
		
		write(fp, buf, sec_sz);
	}
	
	close(fp);
	
	return 0;
}

void build_custom_gdi(const char *dn)
{
	char out_file[1024];
	uint32_t type;
	CDROM_TOC toc;
	FILE *fp;
	int cdi_fd;
	CDI_header_t *hdr;
	
	int vfd = virt_iso_open("/", O_DIR);
	
	if (vfd == -1) {
		printf("ERROR: can't open vfs\n");
		return;
	}
	
	mkdir(dn
#ifndef __WIN32
	, S_IRWXU
#endif
	);
	
	virt_iso_ioctl(vfd, ISOFS_IOCTL_GET_IMAGE_TYPE, &type);
	
	if (type != ISOFS_IMAGE_TYPE_CDI) {
		printf("ERROR: is not CDI\n");
		return;
	}
	
	virt_iso_ioctl(vfd, ISOFS_IOCTL_GET_IMAGE_FD, &cdi_fd);
	virt_iso_ioctl(vfd, ISOFS_IOCTL_GET_TOC_DATA, &toc);
	virt_iso_ioctl(vfd, ISOFS_IOCTL_GET_IMAGE_HEADER_PTR, &hdr);
	
	sprintf(out_file, "%sdisc.gdi", dn);
	
	fp = fopen(out_file, "wt");
	
	if (!fp) {
		printf("ERROR: can't create %s\n", out_file);
		return;
	}
	
	fprintf(fp, "%d\n", TOC_TRACK(toc.last));
	
	if (verbose) {
		printf("TOC content:\n");
	}
	
	for (int i = 0; i < TOC_TRACK(toc.last); i++) {
		uint32_t lba = TOC_LBA(toc.entry[i]) - 150;
		
		if (i && !TOC_CTRL(toc.entry[i]) && !TOC_CTRL(toc.entry[i-1])) {
			lba -= 150;
		}
		
		fprintf(fp, "%d %d %d %d track%02d.%s 0\n", i+1, lba, TOC_CTRL(toc.entry[i]), 
													!TOC_CTRL(toc.entry[i]) ? 2352 : 2048, 
													i+1, !TOC_CTRL(toc.entry[i]) ? "raw" : "iso");
		
		sprintf(out_file, "%strack%02d.%s", dn, i+1, !TOC_CTRL(toc.entry[i]) ? "raw" : "iso");
		
		uint32_t cnt = TOC_LBA(toc.entry[i]);
		
		virt_iso_ioctl(vfd, ISOFS_IOCTL_GET_TRACK_SECTOR_COUNT, &cnt);
		
		if (dump_track(hdr, TOC_LBA(toc.entry[i]) - 150, cnt, !(TOC_CTRL(toc.entry[i])) ? 2352 : 2048, out_file, cdi_fd) < 0) {
			printf("ERROR: can't dump track %d\n", i + 1);
			fclose(fp);
			virt_iso_close(vfd);
			return;
		}
		
		if (verbose) {
			printf("track%02d %d\t%s %d sectors\n", i+1, lba, TOC_CTRL(toc.entry[i]) == 4 ? "DATA" : "CDDA", cnt);
		}
	}
	
	fclose(fp);
	
	virt_iso_close(vfd);
	
	if (!descrmble) {
		return;
	}
	
	fs_iso_shutdown();
	
	fs_iso_init(out_file);
	
	vfd = virt_iso_open("/", O_DIR);
	
	if (vfd == -1) {
		printf("ERROR: can't open vfs\n");
		return;
	}
	
	virt_iso_ioctl(vfd, ISOFS_IOCTL_GET_IMAGE_FD, &cdi_fd);
	
	for (int i = 0x3800; i < 0x8000; i += 0x800) {
		uint32_t sec_buf[512];
		int found = 0;
		
		lseek(cdi_fd, i, SEEK_SET);
		
		if (read(cdi_fd, sec_buf, 2048) != 2048) {
			printf("ERROR: can't read IP.BIN\n");
			virt_iso_close(vfd);
			return;
		}
		
		for (int n = 0; n < 512; n++) {
			if (sec_buf[n] == 0xA05F74E4) {
				sec_buf[n] = 0xA05F74EC;
				found = 1;
			}
		}
		
		if (found) {
			lseek(cdi_fd, i, SEEK_SET);
			write(cdi_fd, sec_buf, 2048);
		}
	}
	
	ipbin_meta_t meta;
	char boot_fn[17];
	lseek(cdi_fd, 0, SEEK_SET);
	read(cdi_fd, &meta, sizeof(meta));
	
	memcpy(boot_fn, meta.boot_file, 16);
	boot_fn[16] = '\0';
	
	for (int i = 15; i > 0; i--) {
		if (boot_fn[i] == ' ') {
			boot_fn[i]  = '\0';
		}
		else {
			break;
		}
	}
	
	
	int fd_boot = virt_iso_open(boot_fn, O_RDWR);
	
	if (fd_boot == -1) {
		printf("ERROR: can't open boot file (%s)\n", boot_fn);
		virt_iso_close(vfd);
		return;
	}
	
	size_t sz_boot = virt_iso_total(fd_boot);
	
	uint32_t *buf_boot = (uint32_t *) calloc((sz_boot>>2)+1, sizeof(uint32_t));
	
	if (!buf_boot) {
		printf("ERROR: can't allocate %ld bytes\n", ((sz_boot>>2)+1)*sizeof(uint32_t));
		virt_iso_close(fd_boot) ;
		virt_iso_close(vfd);
		return;
	}
	
	/* descramle */
	descramle_file(fd_boot, (uint8_t *) buf_boot, sz_boot);
	
	for (size_t i = 0; i < (sz_boot >> 2); i++) {
		if (buf_boot[i] == 0xA05F74E4) {
			buf_boot[i] = 0xA05F74EC;
		}
	}
	
	virt_iso_seek(fd_boot, 0, SEEK_SET);
	printf("Write descrambled %s\n", boot_fn);
	virt_iso_write(fd_boot, buf_boot, sz_boot);
	virt_iso_close(fd_boot);
	virt_iso_close(vfd);
	free(buf_boot);
}

static void copy_vfs_to_file(int src_fd, const char *out_dir, const char *fname)
{
	size_t cnt, size, cur = 0, buf_size;
	int dest_fd;
	uint8_t *buff;
	char dest_fn[2048];
	
	sprintf(dest_fn, "%s%s", out_dir, fname+1);

	dest_fd = open(dest_fn, O_WRONLY | O_CREAT | O_TRUNC
#ifdef __WIN32
														| O_BINARY
#endif
														, S_IRWXU);

	if (dest_fd == FILEHND_INVALID) {
		printf("ERROR: Can't open %s for write\n", dest_fn);
		return;
	}

	size = virt_iso_total(src_fd) / 1024;

	if(size >= 256) {
		buf_size = 32 * 1024;
	} else if(size < 32 && size > 8) {
		buf_size = 1024;
	} else if(size <= 8) {
		buf_size = 512;
	} else {
		buf_size = 16 * 1024;
	}

	buff = (uint8_t *) malloc(buf_size);

	if(buff == NULL) {
		printf("ERROR: No memory: %ld bytes\n", buf_size); 
		close(dest_fd);
		return;
	}

	while ((cnt = virt_iso_read(src_fd, buff, buf_size)) > 0) {
		cur += cnt;
		
		if(write(dest_fd, buff, cnt) < 0) {
			break;
		}
	}

	close(dest_fd);
	free(buff);
}

void extract_image(const char *name, const char *dn, int tree)
{
	dirent_t *vdir;
	int vfd = virt_iso_open(name, O_DIR);
	char out_dir[2048];
	
	sprintf(out_dir, "%s%s", dn, name+1);
	mkdir(out_dir
#ifndef __WIN32
	, S_IRWXU
#endif
	);
	
	while ((vdir = virt_iso_readdir(vfd)) != NULL) {
		char fname[256];
		sprintf(fname, "%s%s", name, vdir->name);
		
		int ffd = virt_iso_open(fname, vdir->size == -1 ? O_DIR : O_RDONLY);
		
		if (ffd < 0) {
			printf("ERROR: Can't open %s\n", fname);
			continue;
		}
		
		if (verbose) {
			printf("Extract %s %s\n", vdir->size == -1 ? "directory" : "file", fname);
		}
		
		if (vdir->size == -1) {
			virt_iso_close(ffd);
			char dirname[256];
			sprintf(dirname, "%s%s/", name, vdir->name);
			extract_image(dirname, dn, tree+1);
		}
		else {
			copy_vfs_to_file(ffd, dn, fname);
			virt_iso_close(ffd);
		}
	}
	
	if (!tree) {
		if (verbose) {
			printf("Extract IP.BIN\n");
		}
		
		uint8_t *ip_buf = malloc(16*2048);
		
		if (!ip_buf) {
			printf("ERROR: Can't allocate memory\n");
			virt_iso_close(vfd);
			return;
		}
		
		virt_iso_ioctl(vfd, ISOFS_IOCTL_GET_BOOT_SECTOR_DATA, ip_buf);
		
		char ip_path[2100];
		sprintf(ip_path, "%sIP.BIN", out_dir);
		
		int fd = open(ip_path, O_WRONLY | O_CREAT | O_TRUNC
#ifdef __WIN32
															| O_BINARY
#endif
															, S_IRWXU);
		
		if (fd == FILEHND_INVALID) {
			printf("ERROR: Can't open %s for write\n", ip_path);
			free(ip_buf);
			return;
		}
		
		write(fd, ip_buf, 16*2048);
		close(fd);
		free(ip_buf);
	}
		
	virt_iso_close(vfd);
}

int usage(const char *s) {
	printf("DC IMG Tool\n");
	printf("-f Input image file (ISO, CDI, GDI)\n");
	printf("-r Folder for replace files in image\n");
	printf("-e Extract image to folder\n");
	printf("-b Build custom GDI from CDI\n");
	printf("-d Descramble main binary used with -b\n");
	printf("-l List files in image\n");
	printf("-v Verbose\n");
	printf("-h This help\n");
	printf("\nExample for rebuild CDI to GDI\n%s -v -d -f game.cdi -b game_gdi\n", s);
	
	return 0;
}

int main(int argc, char *argv[]) {
	int someopt;
	char filename[512];
	char dirname[512];
	int mode = 0;
	
	if (argc < 2) {
		return usage(argv[0]);
	}
	
	memset(filename, 0, 512);
	memset(dirname, 0, 512);
	
	someopt = getopt(argc, argv, "f:e:b:dlhr:v");
	
	while (someopt > 0) {
		switch (someopt) {
			case 'f':
				for (int i = 0, j = 0; i < 512; i++) {
					if (optarg[i] == '\0') {
						filename[j] = '\0';
						break;
					}
					else if (optarg[i] == '"') {
						if (i) {
							filename[j] = '\0';
							break;
						}
						else {
							continue;
						}
					}
					else {
						filename[j] = optarg[i];
					}
					j++;
				}
				break;
			
			case 'r':
				if(mode) {
					printf("You can only specify one of -b, -e, -r or -l\n");
					exit(0);
				}
				
				for (int i = 0, j = 0; i < 512; i++) {
					if (optarg[i] == '\0') {
						dirname[j] = '\0';
						break;
					}
					else if (optarg[i] == '"') {
						if (i) {
							dirname[j] = '\0';
							break;
						}
						else {
							continue;
						}
					}
					else {
						dirname[j] = optarg[i];
					}
					
					j++;
				}
				mode = someopt;
				basedir_len = strlen(dirname) - 1;
				break;
			
			case 'l':
				if(mode) {
					printf("You can only specify one of -b, -e, -r or -l\n");
					exit(0);
				}
				mode = someopt;
				break;
			
			case 'b':
				if(mode) {
					printf("You can only specify one of -b, -e, -r or -l\n");
					exit(0);
				}
				
				for (int i = 0, j = 0; i < 512; i++) {
					if (optarg[i] == '\0') {
						dirname[j] = '\0';
						break;
					}
					else if (optarg[i] == '"') {
						if (i) {
							dirname[j] = '\0';
							break;
						}
						else {
							continue;
						}
					}
					else {
						dirname[j] = optarg[i];
					}
					
					j++;
				}
				mode = someopt;
				basedir_len = strlen(dirname) - 1;
				break;
			
			case 'e':
				if(mode) {
					printf("You can only specify one of -b, -e, -r or -l\n");
					exit(0);
				}
				
				for (int i = 0, j = 0; i < 512; i++) {
					if (optarg[i] == '\0') {
						dirname[j] = '\0';
						break;
					}
					else if (optarg[i] == '"') {
						if (i) {
							dirname[j] = '\0';
							break;
						}
						else {
							continue;
						}
					}
					else {
						dirname[j] = optarg[i];
					}
					j++;
				}
				mode = someopt;
				basedir_len = strlen(dirname) - 1;
				break;
			
			case 'v':
				verbose = 1;
				break;
			
			case 'h':
				return usage(argv[0]);
				break;
			
			case 'd':
				descrmble = 1;
				break;
			
			default:
				break;
		}
		someopt = getopt(argc, argv, "f:e:b:dlhr:v");
	}
	
	if (!filename[0] || !mode) {
		return usage(argv[0]);
	}
	
	if (dirname[0]) {
		int len = strlen(dirname);
		if (dirname[len-1] != '/') {
			strcat(dirname, "/");
		}
	}
	
	if (fs_iso_init(filename)) {
		printf("Can't mount %s\n", filename);
		return 0;
	}
	
	if (mode == 'l') {
		parse_dir("/");
	}
	else if (mode == 'r') {
		replace_files(dirname);
	}
	else if (mode == 'b') {
		build_custom_gdi(dirname);
	}
	else if (mode == 'e') {
		extract_image("/", dirname, 0);
	}
	
	fs_iso_shutdown();
	
	return 0;
}

uint32_t FileSize(const char *fn) {
	struct stat st;
	stat(fn, &st);
	
	return st.st_size;
}

static char *substring(const char* str, size_t begin, size_t len)  { 
	if (str == 0 || strlen(str) == 0 || strlen(str) < begin 
				 || strlen(str) < (begin+len)) {
		return 0;
	}

	return strndup(str + begin, len); 
}

char *getFilePath(const char *file) {
	char *rslash;
	
	if((rslash = strrchr(file, '/')) != NULL) {
		return substring(file, 0, strlen(file) - strlen(rslash));
	}
	
	return NULL;
}

