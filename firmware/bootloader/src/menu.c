/**
 * DreamShell boot loader
 * Menu
 * (c)2011-2016 SWAT <http://www.dc-swat.ru>
 */


#include "main.h"

static pvr_ptr_t txr_font;
static float alpha, curs_alpha;
static int frame, curs_alpha_dir;
static float progress_w, old_per;
static mutex_t video_mutex = MUTEX_INITIALIZER;

static volatile int load_process = 0;
static int load_in_thread = 0;
static volatile int binary_ready = 0;
static uint32 binary_size = 0;
static uint8 *binary_buff = NULL; //(uint8 *)0x8ce00000

typedef struct menu_item {
	char name[MAX_FN_LEN];
	char path[MAX_FN_LEN];
	int mode;
	int is_gz;
	int is_scrambled;
	struct menu_item *next;
} menu_item_t;

static menu_item_t *items;
static int items_cnt = 0;
static int selected = 0;
static char message[MAX_FN_LEN];

//static char cur_path[MAX_FN_LEN];
//static int use_browsing = 0;
//static void clear_menu();

static int must_lock_video() {
	kthread_t *ct = thd_get_current();
	return ct->tid != 1;
}


#define lock_video() \
	do { \
		if(must_lock_video()) \
			mutex_lock(&video_mutex); \
	} while(0)


#define unlock_video() \
	do { \
		if(must_lock_video()) \
			mutex_unlock(&video_mutex); \
	} while(0)


int show_message(const char *fmt, ...) {
	va_list args;
	int i;
	
	if(!start_pressed)
		return 0;
	
	va_start(args, fmt);
	i = vsnprintf(message, MAX_FN_LEN, fmt, args);
	va_end(args);
	
//	if(!start_pressed)
//		dbglog(DBG_INFO, "%s\n", message);
		
	return i;
}


void init_menu_txr() {
	uint16	*vram;
	int	x, y;

	txr_font = pvr_mem_malloc(256*256*2);
	vram = (uint16*)txr_font;

	for (y = 0; y < 8; y++) {
		for (x = 0; x < 16; x++) {
			bfont_draw(vram, 256, 0, y * 16 + x);
			vram += 16;
		}
		vram += 23 * 256;
	}
}

/* The following funcs blatently ripped from libconio =) */

/* Draw one font character (6x12) */
static void draw_char(float x1, float y1, float z1, float a, float r,
	float g, float b, int c) {
	pvr_vertex_t	vert;
	int ix, iy;
	float u1, v1, u2, v2;

	if (c == ' ')
		return;

	//assert( c > ' ' && c < 127 );
	
	if(c > ' ' && c < 127) {
	
		ix = (c % 16) * 16;
		iy = (c / 16) * 24;
		u1 = ix * 1.0f / 256.0f;
		v1 = iy * 1.0f / 256.0f;
		u2 = (ix+12) * 1.0f / 256.0f;
		v2 = (iy+24) * 1.0f / 256.0f;

		vert.flags = PVR_CMD_VERTEX;
		vert.x = x1;
		vert.y = y1 + 24;
		vert.z = z1;
		vert.u = u1;
		vert.v = v2;
		vert.argb = PVR_PACK_COLOR(a, r, g, b);
		vert.oargb = 0;
		pvr_prim(&vert, sizeof(vert));
		
		vert.x = x1;
		vert.y = y1;
		vert.u = u1;
		vert.v = v1;
		pvr_prim(&vert, sizeof(vert));
		
		vert.x = x1 + 12;
		vert.y = y1 + 24;
		vert.u = u2;
		vert.v = v2;
		pvr_prim(&vert, sizeof(vert));

		vert.flags = PVR_CMD_VERTEX_EOL;
		vert.x = x1 + 12;
		vert.y = y1;
		vert.u = u2;
		vert.v = v1;
		pvr_prim(&vert, sizeof(vert));
	}
}

/* draw len chars at string */
static void draw_string(float x, float y, float z, float a, float r, float g,
		float b, char *str, int len) {
	int i;
	pvr_poly_cxt_t cxt;
	pvr_poly_hdr_t poly;

	pvr_poly_cxt_txr(&cxt, PVR_LIST_TR_POLY, PVR_TXRFMT_ARGB1555 | PVR_TXRFMT_NONTWIDDLED,
		256, 256, txr_font, PVR_FILTER_NONE);
	pvr_poly_compile(&poly, &cxt);
	pvr_prim(&poly, sizeof(poly));

	for (i = 0; i < len; i++) {
		draw_char(x, y, z, a, r, g, b, str[i]);
		x += 12;
	}
}

/* draw a box (used by cursor and border, etc) (at 1.0f z coord) */
static void draw_box(float x, float y, float w, float h, float z, float a, float r, float g, float b) {
	pvr_poly_cxt_t	cxt;
	pvr_poly_hdr_t	poly;
	pvr_vertex_t	vert;

	pvr_poly_cxt_col(&cxt, PVR_LIST_TR_POLY);
	pvr_poly_compile(&poly, &cxt);
	pvr_prim(&poly, sizeof(poly));

	vert.flags = PVR_CMD_VERTEX;
	vert.x = x;
	vert.y = y + h;
	vert.z = z;
	vert.u = vert.v = 0.0f;
	vert.argb = PVR_PACK_COLOR(a, r, g, b);
	vert.oargb = 0;
	pvr_prim(&vert, sizeof(vert));

	vert.y -= h;
	pvr_prim(&vert, sizeof(vert));

	vert.y += h;
	vert.x += w;
	pvr_prim(&vert, sizeof(vert));

	vert.flags = PVR_CMD_VERTEX_EOL;
	vert.y -= h;
	pvr_prim(&vert, sizeof(vert));
}



static int search_root_check(char *device, char *path, char *file) {
	
	char check[MAX_FN_LEN];
	
	if(file == NULL) {
		sprintf(check, "/%s%s", device, path);
	} else {
		sprintf(check, "/%s%s/%s", device, path, file);
	}
	
	if((file == NULL && DirExists(check)) || (file != NULL && FileExists(check))) {
		return 0;
	}
	
	return -1;
}


static int search_root() {
	
	dirent_t *ent;
	file_t hnd;
	menu_item_t *item;
	char name[MAX_FN_LEN];
	int i;
	
	hnd = fs_open("/", O_RDONLY | O_DIR);
	
	if(hnd == FILEHND_INVALID) {
		dbglog(DBG_ERROR, "Can't open root directory!\n");
		return -1;
	}
	
	while ((ent = fs_readdir(hnd)) != NULL) {
		
		if(ent->name[0] == 0 || !strncasecmp(ent->name, "pty", 3) || !strncasecmp(ent->name, "ram", 3) ||
			!strncasecmp(ent->name, "sock", 4) || !strncasecmp(ent->name, "vmu", 3)) {
			continue;
		}
		
		item = calloc(1, sizeof(menu_item_t));
		
		if(!item) {
			break;
		}
			
		item->next = items;
		items = item;
		
		for(i = 0; i < strlen(ent->name); i++) {
			name[i] = toupper((int)ent->name[i]);
		}
		
		name[i] = '\0';
		snprintf(item->name, MAX_FN_LEN, "Boot from %s", name);
		item->mode = 0;
		items_cnt++;
		
		dbglog(DBG_INFO, "Checking for root directory on /%s\n", ent->name);
		
		if(!search_root_check(ent->name, "/DS", "/DS_CORE.BIN")) {
				
			snprintf(item->path, MAX_FN_LEN, "/%s/DS/DS_CORE.BIN", ent->name);
			
		} else if(!search_root_check(ent->name, "", "/DS_CORE.BIN")) {
						
			snprintf(item->path, MAX_FN_LEN, "/%s/DS_CORE.BIN", ent->name);
			
		} else if(!search_root_check(ent->name, "", "/1DS_CORE.BIN")) {
						
			snprintf(item->path, MAX_FN_LEN, "/%s/1DS_CORE.BIN", ent->name);
			item->is_scrambled = 1;
			
		} else if(!search_root_check(ent->name, "/DS", "/ZDS_CORE.BIN")) {
				
			snprintf(item->path, MAX_FN_LEN, "/%s/DS/ZDS_CORE.BIN", ent->name);
			item->is_gz = 1;
			
		} else if(!search_root_check(ent->name, "", "/ZDS_CORE.BIN")) {
						
			snprintf(item->path, MAX_FN_LEN, "/%s/ZDS_CORE.BIN", ent->name);
			item->is_gz = 1;
			
		} else {
			item->path[0] = 0;
		}
	}
	
	fs_close(hnd);
	return 0;
}


/*
static int read_directory() {
	
	dirent_t *ent;
	file_t hnd;
	menu_item_t *item;
	int i;
	
	hnd = fs_open(cur_path, O_RDONLY | O_DIR);
	
	if(hnd == FILEHND_INVALID) {
		dbglog(DBG_ERROR, "Can't open root directory!\n");
		return -1;
	}
	
	while ((ent = fs_readdir(hnd)) != NULL) {
		
		if(!strcasecmp(ent->name, ".")) {
			continue;
		}
		
		item = calloc(1, sizeof(menu_item_t));
		
		if(item) {

			item->next = items;
			items = item;
			
			if(ent->attr == O_DIR) {
				strncpy(item->name, ent->name, MAX_FN_LEN);
				item->mode = 0;
			} else {
				snprintf(item->name, MAX_FN_LEN, "%s %dKB", ent->name, ent->size);
				item->mode = 1;
			}
			
			item->name[strlen(item->name)] = '\0';
			snprintf(item->path, MAX_FN_LEN, "%s/%s", cur_path, ent->name);
			item->path[strlen(item->path)] = '\0';
			items_cnt++;
			
		} else {
			break;
		}
	}
	
	fs_close(hnd);
	return 0;
}
*/

static void update_progress(float per) {

	lock_video();
	progress_w = (460.0f / 100.0f) * per;
	
	if(old_per != per) {
		old_per = per;
		show_message("Loading %02d%c", (int)per, '%');
	}
	
	unlock_video();
}


static uint32 gzip_get_file_size(char *filename) {
	
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



void *loading_thd(void *param) {
	
	menu_item_t *item = (menu_item_t *)param;
	file_t fd = FILEHND_INVALID;
	gzFile fdz = NULL;
	uint8 *pbuff;
	uint32 count = 0;
	int i = 0;
	
	if(item->is_gz) {
		
		dbglog(DBG_INFO, "Loading compressed binary %s ...\n", item->path);
		
		binary_size = gzip_get_file_size(item->path);
		fdz = gzopen(item->path, "r");
	
		if(fdz == NULL) {
			lock_video();
			show_message("Can't open file");
			unlock_video();
			load_process = 0;
			binary_size = 0;
			return NULL;
		}
		
		
	} else {
		
		dbglog(DBG_INFO, "Loading binary %s ...\n", item->path);
	
		fd = fs_open(item->path, O_RDONLY);
		
		if(fd == FILEHND_INVALID) {
			lock_video();
			show_message("Can't open file");
			unlock_video();
			load_process = 0;
			return NULL;
		}
		
		binary_size = fs_total(fd);
	}
	
	if(!binary_size) {
		lock_video();
		show_message("File is empty");
		unlock_video();
		
		if(fd != FILEHND_INVALID)
			fs_close(fd);
			
		if(fdz)
			gzclose(fdz);
		
		load_process = 0;
		return NULL;
	}
	
	update_progress(0);
	binary_buff = (uint8 *)memalign(32, binary_size);
	
	if(binary_buff != NULL) {
		
		memset(binary_buff, 0, binary_size);
		pbuff = binary_buff;
		
		if(item->is_gz) {
			
			if(!load_in_thread) {
				
				i = gzread(fdz, pbuff, binary_size);
				
				if(i < 0) {
					lock_video();
					show_message("Loading error");
					load_process = 0;
					binary_size = 0;
					free(binary_buff);
					gzclose(fdz);
					unlock_video();
					return NULL;
				}
				
			} else {
			
				while((i = gzread(fdz, pbuff, 16384)) > 0) {
					update_progress((float)count / ((float)binary_size / 100.0f));
//					dbglog(DBG_INFO, "Loaded and uncompressed %d to %p\n", i, pbuff);
//					thd_sleep(10);
					pbuff += i;
					count += i;
				}
			}
			
			gzclose(fdz);
			
		} else {
			
			if(!load_in_thread) {
				
				i = fs_read(fd, pbuff, binary_size);
				
				if(i < 0) {
					lock_video();
					show_message("Loading error");
					load_process = 0;
					binary_size = 0;
					free(binary_buff);
					fs_close(fd);
					unlock_video();
					return NULL;
				}

			} else {
		
				while((i = fs_read(fd, pbuff, 32768)) > 0) {
					
					update_progress((float)count / ((float)binary_size / 100.0f));
//					dbglog(DBG_INFO, "Loaded %d to %p\n", i, pbuff);
//					thd_sleep(10);
					pbuff += i;
					count += i;
				}
			}
			
			fs_close(fd);
		}
		
		if(item->is_scrambled) {

			lock_video();
			show_message("Descrambling...");
			unlock_video();

			uint8 *tmp_buf = (uint8 *)malloc(binary_size);
			descramble(binary_buff, tmp_buf, binary_size);
			free(binary_buff);
			binary_buff = tmp_buf;
		}
		
		lock_video();
		show_message("Executing...");
		unlock_video();
		
		if(load_in_thread) {
			thd_sleep(100);
			binary_ready = 1;
		} else {
			dbglog(DBG_INFO, "Executing...\n");
			arch_exec(binary_buff, binary_size);
		}
		
	} else {
		
		lock_video();
		show_message("Not enough memory: %d Kb", binary_size / 1024);
		binary_size = 0;
		unlock_video();
	}
	
	load_process = 0;
	return NULL;
}


static menu_item_t *get_selected() {
	
	menu_item_t *item = NULL;
	int i = 0;
	
	for (item = items, i = 0; i < selected; i++, item = item->next)
		;
		
	return item;
}
/*
static void clear_menu() {
	
	menu_item_t *item = NULL;
	int i = 0;
	
	for (item = items, i = 0; i < items_cnt; i++, item = item->next) {
		if(item)
			free(item);
	}
}
*/

void loading_core(int no_thd) {
	
	menu_item_t *item = get_selected();
	
	if(item->path[0] != 0 && !load_process) {
		
		file_t	f;
		f = fs_open(item->path, O_RDONLY);
		
		if(f == FILEHND_INVALID) {
			show_message("Can't open %s", item->path);
			return;
		}
		
		fs_close(f);
		show_message("Loading...");
		load_process = 1;
		
		if(no_thd) {
			load_in_thread = 0;
			loading_thd((void*)item);
		} else {
			load_in_thread = 1;
			thd_create(0, loading_thd, (void*)item);
		}
	}
}


static uint32 last_btns = 0;
static uint32 frames = 0;

static void check_input() {
	maple_device_t *cont = NULL;
	cont_state_t *state = NULL;
	
	if(binary_ready) {
		arch_exec(binary_buff, binary_size);
	}

	cont = maple_enum_type(0, MAPLE_FUNC_CONTROLLER);
	frames++;

	if(cont) {
		state = (cont_state_t *)maple_dev_status(cont);
		
		if(!state) return;

		if (last_btns != state->buttons) {
			if (state->buttons & CONT_DPAD_UP) {
				selected--;
				if (selected < 0)
					selected += items_cnt;
			}
			if (state->buttons & CONT_DPAD_DOWN) {
				selected++;
				if (selected >= items_cnt)
					selected -= items_cnt;
			}
			if ((state->buttons & CONT_DPAD_LEFT) || (state->buttons & CONT_DPAD_RIGHT)) {
				menu_item_t *item = get_selected();
				item->mode = item->mode ? 0 : 1;
			}
			if (state->buttons & CONT_A) {
				loading_core(0);
			}
			if (state->buttons & CONT_B) {
				loading_core(0);
			}
			if(state->buttons & CONT_Y) {
				loading_core(1);//arch_exit();
			}
			if(state->buttons & CONT_X) {
				loading_core(1);//arch_menu();
			}
			frames = 0;
			last_btns = state->buttons;
		}
	}
	
	if(frames > 300) {
		loading_core(0);
		frames = 0;
	}
}

int menu_init() {
	
	alpha = 0.0f;
	frame = 0;
	selected = 0;
	progress_w = 0.0f;
	curs_alpha = 0.5f;
	curs_alpha_dir = 0;
	
	if(start_pressed)
		init_menu_txr();
		
	search_root();
	
	while(selected < items_cnt) {
		menu_item_t *item = get_selected();

		if(item->path[0] != 0) {
			return 0;
		}
		selected++;
	}
	
	selected = 0;
	return 0;
}


void menu_frame() {
	menu_item_t *item;
	float y;

	/* Delay */
	frame++;
	if (frame < 120)
		return;

	/* Adjust alpha */
	if (alpha < 1.0f)
		alpha += 1/30.0f;
	else
		alpha = 1.0f;
	
	if(!curs_alpha_dir) {
		curs_alpha -= 1/120.0f;
		if(curs_alpha <= 0.30f) {
			curs_alpha_dir = 1;
		}
	} else {
		curs_alpha += 1/120.0f;
		if(curs_alpha >= 0.70f) {
			curs_alpha_dir = 0;
		}
	}

	/* Draw title */
	draw_box(90, 90, 640-180, 26, 100.0f, alpha * 0.6f, 0.0f, 0.0f, 0.0f);
	draw_string(320.0f - (12*sizeof(title))/2, 92.0f, 101.0f, alpha, 1, 1, 1, (char*)title, sizeof(title));
	
	/* Draw background plane */
	draw_box(90, 90+26, 640-180, 480-(180+26), 100.0f, alpha * 0.3f, 0.0f, 0.0f, 0.0f);
	//draw_box(88, 88+30, 640-176, 480-(180+22), 100.0f, alpha * 0.6f, 0.0f, 0.0f, 0.0f);

	/* Draw menu items */
	for (y = 90+37, item = items; item; item = item->next, y += 25.0f) {
		
		if(item->path[0] != 0) {
			draw_string(100.0f, y, 101.0f, alpha, 1, 1, 1, 
						(item->mode ? item->path : item->name), 
						strlen(item->mode ? item->path : item->name));
			
		} else {
			draw_string(100.0f, y, 101.0f, alpha * 0.6f, 1, 1, 1, item->name, strlen(item->name));
		}
	}

	/* Cursor */
	draw_box(90, 90 + 36 + selected * 25 - 1,
		640-180, 26, 100.5f, 
		alpha * curs_alpha, 0.3f, 0.3f, 0.3f);
		
		
	mutex_lock(&video_mutex);
	draw_box(90, 480-(90+26), 640-180, 26, 100.0f, alpha * 0.6f, 0.0f, 0.0f, 0.0f);
	draw_box(90, 480-(90+26), progress_w, 26, 101.0f, alpha * 0.9f, 0.85f, 0.15f, 0.15f);
	draw_string(320.0f - (12*strlen(message))/2, 480-(90+24), 102.0f, alpha, 1, 1, 1, message, strlen(message));
	mutex_unlock(&video_mutex);
	
	/* Check for key input */
	check_input();
}
