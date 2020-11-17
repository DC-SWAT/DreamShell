/**
 * DreamShell ISO Loader
 * Utils
 * (c)2011-2016 SWAT <http://www.dc-swat.ru>
 */

#include <main.h>
#include <limits.h>
#include <dc/sq.h>

uint Load_BootBin() {
	
	int rv, bsec;
	uint32 bs = 0xfff000; /* FIXME: switch stack pointer for use all memory */
	uint32 bin_addr = IsoInfo->exec.addr & 0x0fffffff;
	bin_addr |= 0x80000000;
	const uint32 sec_size = 2048;
	uint8 *buff = (uint8*)(IsoInfo->exec.addr);
	
	if(IsoInfo->exec.size < bs) {
		bs = IsoInfo->exec.size;
	}
	
	bsec = (bs / sec_size) + ( bs % sec_size ? 1 : 0);
	
	if(loader_addr > bin_addr && loader_addr < (bin_addr + bs)) {

		int count = (loader_addr - bin_addr) / sec_size;
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
	
	uint32 lba = IsoInfo->track_lba[0];
	uint32 cnt = 16;
	uint8 *buff = (uint8*)IP_ADDR;
	uint8 pass = 0;
	
	if(IsoInfo->boot_mode == BOOT_MODE_IPBIN_TRUNC) {
		pass = 12;
	} else if(loader_addr <= 0x8c004000 && loader_addr > 0x8c000100) {
		pass = 7;
	}

	if(pass) {
		lba += pass;
		cnt -= pass;
		buff += (pass * 2048);
	}
	
	if(ReadSectors(buff, lba, cnt, NULL) == COMPLETED) {
		
		if(IsoInfo->boot_mode != BOOT_MODE_IPBIN_TRUNC) {
			*((uint16*)0xac0090d8) = 0x5113;
			*((uint16*)0xac00940a) = 0x000b;
			*((uint16*)0xac00940c) = 0x0009;
		}
		return 1;
	}
	
	return 0;
}


void Load_DS() {
	printf("Loading DreamShell core...\n");
	
	// FIXME: Get root path
	int fd = open("/DS/DS_CORE.BIN", O_RDONLY);

	if(fd > FILEHND_INVALID) {
		disable_syscalls(0);
		read(fd, (uint8*)(IsoInfo->exec.addr), total(fd));
		close(fd);
		launch(IsoInfo->exec.addr);
	}
}


void *search_memory(const uint8 *key, uint32 key_size) {
	
	uint32 start_loc = IsoInfo->exec.addr & 0x0fffffff;
	start_loc |= 0x80000000;
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
	
	uint32 end_loc = IsoInfo->exec.addr + IsoInfo->exec.size;
	uint8 *k = (uint8 *)&key;
	uint8 *v = (uint8 *)&val;
	int count = 0;
	
	for(uint8 *cur_loc = (uint8 *)IsoInfo->exec.addr; (uint32)cur_loc <= end_loc; cur_loc++) {

		if(*cur_loc == k[0]) {

			if(!memcmp((const uint8 *)cur_loc, k, 4)) {
				memcpy(cur_loc, v, 4);
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
	
#ifdef LOG
	char buff[128];
	va_list args;

	va_start(args, fmt);
	i = vsnprintf(buff, sizeof(buff), fmt, args);
	va_end(args);
	
	LOGF(buff);
#else
	char *buff = (char *)fmt;
#endif

	bfont_draw_str(vram + ((print_y * 24 + 4) * 640) + 12, 0xffff, 0x0000, buff);
	
	if(buff[strlen(buff)-1] == '\n')
		print_y++;

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

#if !_FS_READONLY && !defined(DEV_TYPE_GD)
static int log_fd;
#endif /* _FS_READONLY */

static char log_buff[128];

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


int OpenLog() {

	memset(log_buff, 0, sizeof(log_buff));

#if _FS_READONLY == 0 && !defined(DEV_TYPE_GD)
	log_fd = open("/isoldr.log", O_WRONLY);
	
	if(log_fd < 0) {
		return 0;
	}
	LOGF("----- Start log -----\n\n");
#else
#if !defined(DEV_TYPE_SD) && !defined(DEV_TYPE_DCL) && !defined(LOG_DCL)// && !defined(DEV_TYPE_IDE)
	scif_init();
#endif
#endif /* _FS_READONLY */
	return 1;
}

static int PutLog(char *buff) {
	
	int len = strlen(buff);
	
#if _FS_READONLY == 0 && !defined(DEV_TYPE_GD)
		if(write(log_fd, buff, len) != len) {
			return 0;
		}
#	else

#	if defined(DEV_TYPE_DCL) || defined(LOG_DCL)
		dcload_write_buffer((uint8 *)buff, len);
#	else
		scif_write_buffer((uint8 *)buff, len, 1);
#	endif

#endif /* _FS_READONLY */

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

void CloseLog() {
#if _FS_READONLY == 0 && !defined(DEV_TYPE_GD)
	LOGF("\n\n----- End log -----\n\n");
	close(log_fd);
#endif /* _FS_READONLY */
}

#endif /* LOG */


#if 0 // TODO
static void maple_raddr(uint8 addr, int * port, int * unit) {
	*port = (addr >> 6) & 3;

	if(addr & 0x20)
		*unit = 0;
	else if(addr & 0x10)
		*unit = 5;
	else if(addr & 0x08)
		*unit = 4;
	else if(addr & 0x04)
		*unit = 3;
	else if(addr & 0x02)
		*unit = 2;
	else if(addr & 0x01)
		*unit = 1;
	else {
		*port = -1;
		*unit = -1;
	}
}

static uint32 maple_dma_count;

void dump_maple_dma_buffer() {
	uint32 *data, addr, value, size = 8 * 1024;
	int port, unit;
	uint8 cmd, len;
	
	addr = *(vuint32 *)0xa05f6c04;
	data = (uint32 *)addr;
	LOGF("--- MAPLE FRAME: %ld at 0x%08lx ---\n", ++maple_dma_count, addr);
	
	while(size > 0) {
		/* First word: message length and destination port */
		value = *data++;
		len = value & 0xff;
		port = (value >> 16) & 0xff;
		
		if(!value) {
			break;
		}

		/* Second word: receive buffer physical address */
		addr = *data++;

		/* Third word: command, addressing, packet length */
		value = *data++;
		cmd = (value & 0xff);
	//	maple_addr = (value >> 8) & 0xff;
	//	port = (value >> 8) & 0xff;
	//	unit = (maple_addr & 0x20);
		maple_raddr((value >> 8) & 0xff, &port, &unit);
		value = *data++;
		LOGF("c=0x%02x p=0x%02x u=0x%02x a=0x%08lx d1=0x%08lx d2=0x%08lx", 
				cmd, port, unit, addr, value, *data);
		data += len;
		size -= len * 4;
	}

	LOGF("--- END MAPLE FRAME ---\n");
}
#endif


// TODO: use malloc instead of sector_buffer too
#ifdef HAVE_CDDA

// FIXME: Can be different
#define KATANA_MALLOC_INDEX  0x84
#define KATANA_FREE_INDEX    0x152
#define KATANA_REALLOC_INDEX 0x20a

static uint8 *malloc_root;
static const uint8 katana_malloc_key[] = {0xe6, 0x2f, 0xc6, 0x2f, 0xfc, 0x7f, 0x02, 0x00};

int malloc_init(void) {
	if (!malloc_root || memcmp(malloc_root, katana_malloc_key, sizeof(katana_malloc_key))) {
		malloc_root = search_memory(katana_malloc_key, sizeof(katana_malloc_key));
	}

	return malloc_root ? 1 : 0;
}

void malloc_stat(uint32 *free_size, uint32 *max_free_size) {
	if (malloc_root) {
		return (*(void(*)())malloc_root)(free_size, max_free_size);
	} else {
		*free_size = 0;
		*max_free_size = 0;
	}
}

void *malloc(uint32 size) {
	void *mem;

	if (malloc_root) {
		mem = (*(void*(*)())(malloc_root + KATANA_MALLOC_INDEX))(size);
		return mem;
	} else {
		return NULL;
	}
}

void free(void *data) {
	if (malloc_root) {
		return (*(void(*)()) (malloc_root + KATANA_FREE_INDEX))(data);
	}
}

void *realloc(void *data, uint32 size) {
	void *mem;

	if (data && malloc_root) {
		mem = (*(void*(*)())(malloc_root + KATANA_REALLOC_INDEX))(data, size);
		return mem;
	} else {
		return NULL;
	}
}
#endif

#if 0
typedef struct {
	int is_available;
	int size;
} MCB, *MCB_P;

static uint8 *mem_start_p;
static uint8 *heap_end;
static int max_mem;
static int allocated_mem; /* this is the memory in use. */
static int mcb_count;

enum { NEW_MCB = 0, NO_MCB, REUSE_MCB };
enum { FREE, IN_USE};

void malloc_init(uint8 *ptr, size_t size_in_bytes) {
	/* store the ptr and size_in_bytes in global variable */

	max_mem = size_in_bytes;
	mem_start_p = ptr;
	mcb_count = 0;
	allocated_mem = 0;
	heap_end = mem_start_p + size_in_bytes;
	memset(mem_start_p, 0, max_mem);
}

//void malloc_stat(uint32 *freesize, uint32 *max_freesize) {
//	*freesize = 0;
//	*max_freesize = 0;
//}

void *malloc(int elem_size) {
	/* check whether any chunk (allocated before) is free first */

	MCB_P p_mcb;
	int flag = NO_MCB;
	int sz;

	p_mcb = (MCB_P)mem_start_p;
	sz = sizeof(MCB);

	if((elem_size + sz) > (max_mem - (allocated_mem + mcb_count * sz))) {
		return NULL;
	}
	
	while(heap_end > ( (char *)p_mcb + elem_size + sz)) {

		if (p_mcb->is_available == 0) {
			if( p_mcb->size == 0) {
				flag = NEW_MCB;
				break;
			}
			if(p_mcb->size >= (elem_size + sz)) {
				flag = REUSE_MCB;
				break;
			}
		}
		p_mcb = (MCB_P) ( (char *)p_mcb + p_mcb->size);
	}

	if(flag != NO_MCB) {
		p_mcb->is_available = 1;

		if(flag == NEW_MCB) {
			p_mcb->size = elem_size + sizeof(MCB); 
		} else if(flag == REUSE_MCB) {
			elem_size = p_mcb->size - sizeof(MCB);
		}
		
		mcb_count++;
		allocated_mem += elem_size;
		return ((char *) p_mcb + sz);
	}

	return NULL;
	/* if size of the available chunk is equal to greater than required size, use that chunk */
}

void free(void *p) {
	/* Mark in MCB that this chunk is free */
	MCB_P ptr = (MCB_P)p;
	ptr--;

	if(ptr->is_available != FREE) {
		mcb_count--;
		ptr->is_available = FREE;
		allocated_mem -= (ptr->size - sizeof(MCB));
	}
}
#endif
