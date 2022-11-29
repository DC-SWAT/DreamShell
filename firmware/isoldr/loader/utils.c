/**
 * DreamShell ISO Loader
 * Utils
 * (c)2011-2022 SWAT <http://www.dc-swat.ru>
 */

#include <main.h>
#include <limits.h>
#include <dc/sq.h>
#include <dcload.h>
#include <asic.h>


void setup_machine_state() {

	/* Clear IRQ stuff */
	*ASIC_IRQ9_MASK  = 0;
	*ASIC_IRQ11_MASK = 0;
	*ASIC_IRQ13_MASK = 0;
	ASIC_IRQ_STATUS[ASIC_MASK_NRM_INT] = 0x04038;
	(void) *((volatile uint8 *)0xA05F709C);
}

uint Load_BootBin() {
	
	int rv, bsec;
	uint32 bs = 0xfff000; /* FIXME: switch stack pointer for use all memory */
	uint32 exec_addr = CACHED_ADDR(IsoInfo->exec.addr);
	const uint32 sec_size = 2048;
	uint8 *buff = (uint8*)(UNCACHED_ADDR(IsoInfo->exec.addr));

	if(IsoInfo->exec.size < bs) {
		bs = IsoInfo->exec.size;
	}
	
	bsec = (bs / sec_size) + ( bs % sec_size ? 1 : 0);
	
	if(loader_addr > exec_addr && loader_addr < (exec_addr + bs)) {

		int count = (loader_addr - exec_addr) / sec_size;
		int part = (ISOLDR_MAX_MEM_USAGE / sec_size) + count;
		
		rv = ReadSectors(buff, IsoInfo->exec.lba, count, NULL);
		
		if(rv == COMPLETED) {
			rv = ReadSectors(buff + (part * sec_size), IsoInfo->exec.lba + part, bsec - part, NULL);
		}
		
	} else {
		rv = ReadSectors(buff, IsoInfo->exec.lba, bsec, NULL);
	}

	return rv == COMPLETED ? 1 : 0;
}


uint Load_IPBin() {
	
	uint32 ipbin_addr = UNCACHED_ADDR(IPBIN_ADDR);
	uint32 lba = IsoInfo->track_lba[0];
	uint32 cnt = 16;
	uint8 *buff = (uint8*)ipbin_addr;
	uint8 pass = 0;
	
	if(IsoInfo->boot_mode == BOOT_MODE_IPBIN_TRUNC) {
		pass = 12;
	} else if(loader_addr < APP_ADDR && loader_addr > ISOLDR_DEFAULT_ADDR_MIN) {
		pass = 7;
	}

	if(pass) {
		lba += pass;
		cnt -= pass;
		buff += (pass * 2048);
	}
	
	if(ReadSectors(buff, lba, cnt, NULL) == COMPLETED) {
		
		if(IsoInfo->boot_mode != BOOT_MODE_IPBIN_TRUNC) {

			*((uint16*)ipbin_addr + 0x10d8) = 0x5113;
			*((uint16*)ipbin_addr + 0x140a) = 0x000b;
			*((uint16*)ipbin_addr + 0x140c) = 0x0009;
		}
		return 1;
	}
	
	return 0;
}


void Load_DS() {
	printf("Loading DreamShell...\n");

	if (iso_fd > FILEHND_INVALID) {
		close(iso_fd);
	}

	char *fn = "/DS/DS_CORE.BIN";
	int fd = open(fn, O_RDONLY);

	if (fd < 0) {
		fd = open(fn + 3, O_RDONLY);
	}
	if (fd < 0) {
		printf("FAILED\n");
		return;
	}

	int all_sc = loader_addr < ISOLDR_DEFAULT_ADDR_LOW ||
		(IsoInfo->heap >= HEAP_MODE_SPECIFY && IsoInfo->heap < ISOLDR_DEFAULT_ADDR_LOW);
	disable_syscalls(all_sc);

	if (read(fd, (uint8 *)UNCACHED_ADDR(APP_ADDR), total(fd)) > 0) {
		launch(APP_ADDR);
	}
	close(fd);
}


void *search_memory(const uint8 *key, uint32 key_size) {
	
	uint32 start_loc = CACHED_ADDR(IsoInfo->exec.addr);
	uint32 end_loc = start_loc + IsoInfo->exec.size;
	
	for(uint8 *cur_loc = (uint8 *)start_loc; (uint32)cur_loc <= end_loc; cur_loc++) {

		if(*cur_loc == key[0]) {

			if(!memcmp((const uint8 *)cur_loc, key, key_size))
				return (void *)cur_loc;
		}
	}
	
	return NULL;
}


int patch_memory(const uint32 key, const uint32 val) {

	uint32 exec_addr = UNCACHED_ADDR(IsoInfo->exec.addr);
	uint32 end_loc = exec_addr + IsoInfo->exec.size;
	uint8 *k = (uint8 *)&key;
	uint8 *v = (uint8 *)&val;
	int count = 0;

	for(uint8 *cur_loc = (uint8 *)exec_addr; (uint32)cur_loc <= end_loc; cur_loc++) {

		if(*cur_loc == k[0]) {

			if(!memcmp((const uint8 *)cur_loc, k, sizeof(val))) {
				memcpy(cur_loc, v, sizeof(val));
				count++;
			}
		}
	}

	return count;
}


void apply_patch_list() {

	if(!IsoInfo->patch_addr[0]) {
		return;
	}

	for(uint i = 0; i < sizeof(IsoInfo->patch_addr) >> 2; ++i) {

		if(*(uint32 *)IsoInfo->patch_addr[i] != IsoInfo->patch_value[i]) {
			*(uint32 *)IsoInfo->patch_addr[i] = IsoInfo->patch_value[i];
		}
	}
}


/* copies n bytes from src to dest, dest must be 32-byte aligned */
void *sq_cpy(void *dest, const void *src, int n) {
	unsigned int *d = (unsigned int *)(void *)
					  (0xe0000000 | (((unsigned long)dest) & 0x03ffffe0));
	const unsigned int *s = src;

	/* Set store queue memory area as desired */
	QACR0 = ((((unsigned int)dest) >> 26) << 2) & 0x1c;
	QACR1 = ((((unsigned int)dest) >> 26) << 2) & 0x1c;

	/* fill/write queues as many times necessary */
	n >>= 5;

	while(n--) {
		__asm__("pref @%0" : : "r"(s + 8));  /* prefetch 32 bytes for next loop */
		d[0] = *(s++);
		d[1] = *(s++);
		d[2] = *(s++);
		d[3] = *(s++);
		d[4] = *(s++);
		d[5] = *(s++);
		d[6] = *(s++);
		d[7] = *(s++);
		__asm__("pref @%0" : : "r"(d));
		d += 8;
	}

	/* Wait for both store queues to complete */
	d = (unsigned int *)0xe0000000;
	d[0] = d[8] = 0;

	return dest;
}


int printf(const char *fmt, ...) {

	if(IsoInfo != NULL && IsoInfo->fast_boot) {
		return 0;
	}

	static int print_y = 1;
	int i = 0;
	uint16 *vram = (uint16*)(VIDEO_VRAM_START);

	if(fmt == NULL) {
		print_y = 1;
		return 0;
	}

#if defined(LOG)
	char buff[64];
	va_list args;

	va_start(args, fmt);
	i = vsnprintf(buff, sizeof(buff), fmt, args);
	va_end(args);

# ifndef LOG_SCREEN
	LOGF(buff);
# endif
#else
	char *buff = (char *)fmt;
#endif

	bfont_draw_str(vram + ((print_y * 24 + 4) * 640) + 12, 0xffff, 0x0000, buff);

	if(buff[strlen(buff)-1] == '\n') {
		if (print_y++ > 15) {
			print_y = 0;
			memset((uint32 *)vram, 0, 2 * 1024 * 1024);
		}
	}

	return i;
}

void vid_waitvbl() {
	vuint32 *vbl = ((vuint32 *)0xa05f8000) + 0x010c / 4;

	while(!(*vbl & 0x01ff))
		;

	while(*vbl & 0x01ff)
		;
}

void draw_gdtex(uint8 *src) {
	
	int xPos = 640 - 280;
	int yPos = 480 - 280;
	int r, g, b, a, pos;
	uint16 *vram = (uint16*)(VIDEO_VRAM_START);

	for (uint x = 0; x < 256; ++x) {
		for (uint y = 0; y < 256; ++y) {

			pos = (yPos + x) * 640 + y + xPos;
			r = src[0];
			g = src[1];
			b = src[2];
			a = src[3];
		   
			if(a) {
				
				r >>= 3;
				g >>= 2;
				b >>= 3;
				
				if(a < 255) {
					int p = vram[pos];	
					r = ((r * a) + (((p & 0xf800) >> 11) * (255 - a))) >> 8;
					g = ((g * a) + (((p & 0x07e0) >> 5) * (255 - a))) >> 8;
					b = ((b * a) + (( p & 0x001f) * (255 - a))) >> 8;	
				}
				
				vram[pos] = (r << 11) | (g << 5) | b;
			}
			
		   src += 4;
		}
	}
}

#if !_FS_READONLY
void video_screen_shot() {
	
	file_t fd;
	uint16 pixel;
	uint16 cur = 0;
	uint16 *vram = (uint16*)(VIDEO_VRAM_START);
	uint8 buffer[258];
	
	fd = open("screenshot.ppm", O_WRONLY);
	
	if(fd < 0) 
		return;
	
	write(fd, "P6\n#DreamShell ISO Loader\n640 480\n255\n", 38);

	for(int i = 0; i < 0x4B000; i++) {
		pixel = vram[i];
		buffer[cur++ * 3] = (((pixel >> 11) & 0x1f) << 3);
		buffer[cur++ * 3] = (((pixel >>  5) & 0x3f) << 2);
		buffer[cur++ * 3] = (((pixel >>  0) & 0x1f) << 3);
		
		if(cur * 3 >= sizeof(buffer)) {
			write(fd, buffer, sizeof(buffer));
			cur = 0;
		}
	}

	close(fd);
}
#endif


#ifdef LOG

size_t strnlen(const char *s, size_t maxlen) {
	const char *e;
	size_t n;

	for (e = s, n = 0; *e && n < maxlen; e++, n++)
		;
	return n;
}

int check_digit (char c) {
	if((c>='0') && (c<='9')) {
		return 1;
	}
	return 0;
}

/*
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
}*/

static char log_buff[128];

#if _FS_READONLY == 0 && defined(LOG_FILE)
static int log_fd = FS_ERR_SYSERR;
static int open_log_file() {
	int fd = open(LOG_FILE, O_WRONLY | O_APPEND);
	if (fd > FS_ERR_SYSERR) {
		write(fd, "--- Start log ---\n", 18);
	}
	return fd;
}
#endif /* _FS_READONLY */

int OpenLog() {

	memset(log_buff, 0, sizeof(log_buff));

#if _FS_READONLY == 0 && defined(LOG_FILE)
	log_fd = open_log_file();
#endif

#if defined(DEV_TYPE_DCL) || defined(LOG_DCL)
	dcload_init();
#elif !defined(LOG_SCREEN)
	scif_init();
#endif
	return 1;
}

static int PutLog(char *buff) {

	int len = strlen(buff);

#if _FS_READONLY == 0 && defined(LOG_FILE)
	if (log_fd == FS_ERR_SYSERR) {
		log_fd = open_log_file();
	}
	if(log_fd > FS_ERR_SYSERR) {
		write(log_fd, buff, len);
	}
#endif

#if defined(LOG_SCREEN)
	printf(buff);
#elif defined(DEV_TYPE_DCL) || defined(LOG_DCL)
	dcload_write_buffer((uint8 *)buff, len);
#else
	scif_write_buffer((uint8 *)buff, len, 1);
#endif
	return len;
}

int WriteLog(const char *fmt, ...) {
	va_list args;
	int i;

	va_start(args, fmt);
	i = vsnprintf(log_buff, sizeof(log_buff), fmt, args);
	va_end(args);

	PutLog(log_buff);
	return i;
}

int WriteLogFunc(const char *func, const char *fmt, ...) {

	PutLog((char *)func);

	if(fmt == NULL) {
		PutLog("\n");
		return 0;
	}

	PutLog(": ");

	va_list args;
	int i;

	va_start(args, fmt);
	i = vsnprintf(log_buff, sizeof(log_buff), fmt, args);
	va_end(args);

	PutLog(log_buff);
	return i;
}

#endif /* LOG */
