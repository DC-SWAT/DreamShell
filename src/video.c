/****************************
 * DreamShell ##version##   *
 * video.c                  *
 * DreamShell video         *
 * Created by SWAT          *
 ****************************/ 

#include "ds.h"
#include "console.h"
#include <kmg/kmg.h>
#include <zlib/zlib.h>

static int sdl_dc_no_ask_60hz = 0;
static int sdl_dc_default_60hz = 0;
#include "../lib/SDL/src/video/dc/60hz.h"

//#define DEBUG_VIDEO 1

static SDL_Surface *DScreen = NULL; 
static mutex_t video_mutex = MUTEX_INITIALIZER;
static kthread_t *video_thd;
static int video_inited = 0;
static int video_mode = -1;

static float screen_opacity = 1.0f;
static float plx_opacity = 1.0f;

static volatile int scr_fade_act = 0, first_fade = 1;
static volatile int screen_changed = 1, draw_screen = 1;

extern int sdl_dc_width;
extern int sdl_dc_height;
extern int sdl_dc_wtex;
extern int sdl_dc_htex;

static float native_width = 640.0f;
static float native_height = 480.0f;

static float sdl_dc_x = 0.0f;
static float sdl_dc_y = 0.0f;
static float sdl_dc_z = 1.0f;

static float sdl_dc_rot_x = 0;		/* X Rotation */
static float sdl_dc_rot_y = 0;		/* Y Rotation */
static float sdl_dc_rot_z = 0;		/* Y Rotation */

static float sdl_dc_trans_x = 0;
static float sdl_dc_trans_y = 0;
static float sdl_dc_trans_z = 0;

static pvr_ptr_t logo_txr;
static int logo_w = 1024;
static int logo_h = 512;

pvr_ptr_t sdl_dc_memtex;
unsigned short *sdl_dc_buftex;


static plx_font_t *plx_fnt;
static plx_fcxt_t *plx_cxt;
static plx_texture_t *plx_screen_texture;
static int screen_filter = PVR_FILTER_NEAREST;

static float sdl_dc_u1 = 0.3f;
static float sdl_dc_u2 = 0.3f;
static float sdl_dc_v1 = 0.9f;
static float sdl_dc_v2 = 0.6f;

static void *VideoThread(void *ptr);


SDL_Surface *GetScreen() {
    return DScreen;
}

int GetScreenWidth() {
	return DScreen->w;
}

int GetScreenHeight() {
	return DScreen->h;
}

pvr_ptr_t GetScreenTexture() {
     return sdl_dc_memtex;
}

int GetVideoMode() {
	return video_mode;
}

void SetVideoMode(int mode) {
	
	int pm;
	Settings_t *settings = GetSettings();
	
	switch(settings->video.bpp) {
		case 15:
			pm = PM_RGB555;
			break;
		case 16:
			pm = PM_RGB565;
			break;
		case 24:
		case 32:
			pm = PM_RGB888;
			break;
		default:
			pm = PM_RGB565;
			break;
	}
	
	if(mode < 0) {
		
		if(vid_check_cable() == CT_VGA) {
			
			video_mode = DM_640x480_VGA;
			
		} else {
			
			int region = flashrom_get_region_only();

			switch(region) {
				case FLASHROM_REGION_US:
				case FLASHROM_REGION_JAPAN:
					video_mode = DM_640x480_NTSC_IL;
					break;
				case FLASHROM_REGION_EUROPE:
					video_mode = DM_640x480_PAL_IL;
					break;
				case FLASHROM_REGION_UNKNOWN:
				default:
					if(!sdl_dc_ask_60hz()) {
						video_mode = DM_640x480_PAL_IL;
					} else {
						video_mode = DM_640x480_NTSC_IL;
					}
					break;
			}
		}

	} else {
		video_mode = mode;
	}
	
	vid_set_mode(video_mode, pm);
}

void SetScreenTexture(pvr_ptr_t *txr) {
	LockVideo(); 
	sdl_dc_memtex = txr;
	UnlockVideo();
}

void SetScreen(SDL_Surface *new_screen) {
	LockVideo(); 
	DScreen = new_screen;
	UnlockVideo();
}

void SetScreenOpacity(float opacity) {
	LockVideo();
	screen_opacity = opacity;
	UnlockVideo();
}

float GetScreenOpacity() {
    return screen_opacity;
}

void SetScreenFilter(int filter) {
	LockVideo();
	
	if(filter < 0) {
		
		Settings_t *settings = GetSettings();
	
		if(settings->video.virt_width > 640 || settings->video.virt_height > 480) {
			screen_filter = PVR_FILTER_BILINEAR; //PVR_FILTER_TRILINEAR1
		} else {
			screen_filter = PVR_FILTER_NEAREST;
		}
		
	} else {
		screen_filter = filter;
	}

	plx_txr_setfilter(plx_screen_texture, screen_filter);
	UnlockVideo();
}

void DisableScreen() {
	LockVideo();
	draw_screen = 0;
	UnlockVideo();
}

void EnableScreen() {
	LockVideo();
	draw_screen = 1;
	UnlockVideo();
}

int ScreenIsEnabled() {
	return draw_screen;
}

void ScreenChanged() {
	
#if 0	
	if(VideoMustLock()) {
//		ds_printf("%s outside\n", __func__);
		LockVideo();
		screen_changed = 1;
		UnlockVideo();
	} else {
		//ds_printf("%s inside\n", __func__);
		screen_changed = 1;
	}
	
#else
#ifdef DEBUG_VIDEO
	//ds_printf("%s\n", __func__);
#endif
	screen_changed = 1;
#endif
}

int ScreenUpdated() {
	return screen_changed != 1;
}

void ScreenWaitUpdate() {
	if(!video_inited) return;
	while(screen_changed) thd_pass();
}

void SetScreenVertex(float u1, float v1, float u2, float v2) {
	LockVideo(); 
	sdl_dc_u1 = u1;
	sdl_dc_v1 = v1;
	sdl_dc_u2 = u2;
	sdl_dc_v2 = v2;
	UnlockVideo();
}


void ScreenFadeIn() {
	LockVideo();
	scr_fade_act = 1;
	UnlockVideo();
}

void ScreenFadeOut() {
	LockVideo();
	scr_fade_act = 2;
	UnlockVideo();
}

void ScreenFadeStop() {
	LockVideo();
	scr_fade_act = 0;
	UnlockVideo();
}

static inline void ScreenFadeStep() {
	switch(scr_fade_act) {
		case 1:
			if(screen_opacity >= 1.0f) {
				scr_fade_act = 0;
				if(first_fade) {
					first_fade = 0;
//					pvr_set_bg_color(0.85f, 0.85f, 0.85f);
				}
			} else {
				screen_opacity += 0.05f;
				plx_opacity -= 0.05f;
			}
			break;
		case 2:
			if(screen_opacity <= 0.0f) {
				scr_fade_act = 0;
			} else {
				screen_opacity -= 0.05f;
				plx_opacity += 0.05f;
			}
			break;
		default:
			break;
	}
}


void ScreenRotate(float x, float y, float z) {
	LockVideo();
	sdl_dc_rot_x = x;
	sdl_dc_rot_y = y;
	sdl_dc_rot_z = z;
	UnlockVideo();
}

void ScreenTranslate(float x, float y, float z) {
	LockVideo();
	sdl_dc_trans_x = x;
	sdl_dc_trans_y = y;
	sdl_dc_trans_z = z;
	UnlockVideo();
}


void InitVideoThread() {
	video_inited = 1;
	video_thd = thd_create(0, VideoThread, NULL);
	strncpy(video_thd->label, "[video]\0", 8);
	thd_pass();
}

void ShutdownVideoThread() {

	// LockVideo();
	video_inited = 0;
	// UnlockVideo();
	
	if(video_thd) {
		thd_join(video_thd, NULL); 
	}

	// if(VideoIsLocked()) UnlockVideo();
}


int VideoIsLocked() {
	//if(!video_inited) return 0;
	return mutex_is_locked(&video_mutex);
}

int VideoMustLock() {
#ifdef DEBUG_VIDEO
	kthread_t *ct = thd_get_current();
	printf("%s: %p != %p\n", __func__, video_thd, ct);
#endif
	return (video_inited && video_thd != thd_get_current());
}

void LockVideo() { 
	
//	if(!video_inited) return;

#ifdef DEBUG_VIDEO
	printf("%s\n", __func__);
#endif
	mutex_lock(&video_mutex);
//	mutex_lock_timed(&video_mutex, 60000);
}


void UnlockVideo() {

//	if(!video_inited) return;

#ifdef DEBUG_VIDEO
	printf("%s\n", __func__);
#endif
	mutex_unlock(&video_mutex);
}


/* Utility function to fill out the initial poly contexts */
static void plx_fill_contexts(plx_texture_t * txr) {
	pvr_poly_cxt_txr(&txr->cxt_opaque, PVR_LIST_OP_POLY, txr->fmt, txr->w, txr->h,
		txr->ptr, screen_filter);
	pvr_poly_cxt_txr(&txr->cxt_trans, PVR_LIST_TR_POLY, txr->fmt, txr->w, txr->h,
		txr->ptr, screen_filter);
	pvr_poly_cxt_txr(&txr->cxt_pt, PVR_LIST_PT_POLY, txr->fmt, txr->w, txr->h,
		txr->ptr, screen_filter);

	plx_txr_flush_hdrs(txr);
}


void InitVideoHardware() {

	Settings_t *settings = GetSettings();
	
	if(settings->video.mode.width > 0 && settings->video.mode.height > 0) {
		
		vid_set_mode_ex(&settings->video.mode);
		native_width = (float)settings->video.mode.width;
		native_height = (float)settings->video.mode.height;
		
	} else {
		SetVideoMode(-1);
	}

//	dbglog(DBG_INFO, "PVR hardware ID: %08lx Revision: %08lx\n", PVR_GET(PVR_ID), PVR_GET(PVR_REVISION));
	pvr_init_defaults();
//	pvr_dma_init();
}


int InitVideo(int w, int h, int bpp) {
    
	int sdl_init_flags, sdl_video_flags, tex_mode;
	char fn[MAX_FN_LEN];
	Settings_t *settings = GetSettings();
	
	SDL_DC_ShowAskHz(SDL_TRUE);
	SDL_DC_Default60Hz(SDL_FALSE);
	SDL_DC_VerticalWait(SDL_TRUE);
	SDL_DC_EmulateKeyboard(SDL_FALSE);
	SDL_DC_EmulateMouse(SDL_TRUE);

	sdl_init_flags = SDL_INIT_VIDEO|SDL_INIT_JOYSTICK;
	sdl_video_flags = SDL_HWSURFACE|SDL_DOUBLEBUF;
	SDL_DC_SetVideoDriver(SDL_DC_TEXTURED_VIDEO);

	if (SDL_Init(sdl_init_flags) < 0) {
		ds_printf("SDL init failed: %s\n", SDL_GetError());
		return 0;
	}

    if(!(DScreen = SDL_SetVideoMode(w, h, bpp, sdl_video_flags))) {
		dbglog(DBG_ERROR, "Change SDL video mode failed: %s\n", SDL_GetError());
		return 0;
    }
	
	//SDL_DC_EmulateMouse(SDL_TRUE);
	//SDL_DC_EmulateKeyboard(SDL_TRUE);
	SDL_JoystickOpen(0);
	SDL_JoystickEventState(SDL_ENABLE);
	SDL_EnableUNICODE(SDL_ENABLE);
	SDL_ShowCursor(SDL_DISABLE);

	/* Init matrices */
	plx_mat3d_init();                    /* Clear internal to an identity matrix */
	plx_mat3d_mode(PLX_MAT_PROJECTION);  /** Projection (frustum, screenview) matrix */
	plx_mat3d_identity();                /** Load an identity matrix */
	plx_mat3d_perspective(45.0f, native_width / native_height, 0.1f, 100.0f);
	plx_mat3d_mode(PLX_MAT_MODELVIEW);   /** Modelview (rotate, scale) matrix */
	plx_mat_identity();
	plx_mat3d_apply_all();
	
	if(settings->video.tex_filter >= PVR_FILTER_NEAREST) {
		screen_filter = settings->video.tex_filter;
	}

	plx_screen_texture = (plx_texture_t*) malloc(sizeof(plx_texture_t));
	
	switch(settings->video.bpp) {
		case 15:
			tex_mode = PVR_TXRFMT_ARGB1555;
			break;
		case 16:
		case 24:
		case 32:
		default:
			tex_mode = PVR_TXRFMT_RGB565;
			break;
	}
	
	if(plx_screen_texture != NULL) {
		plx_screen_texture->ptr = sdl_dc_memtex;
		plx_screen_texture->w = w;
		plx_screen_texture->h = h;
		plx_screen_texture->fmt = tex_mode|PVR_TXRFMT_NONTWIDDLED;//|PVR_TXRFMT_STRIDE;
		plx_fill_contexts(plx_screen_texture);
		plx_txr_setfilter(plx_screen_texture, screen_filter);
	}
	
	plx_cxt_init();
	sprintf(fn, "%s/fonts/txf/axaxax.txf", getenv("PATH"));
	plx_fnt = plx_font_load(fn);
	
	if(plx_fnt) {
		plx_cxt = plx_fcxt_create(plx_fnt, PVR_LIST_TR_POLY);
		plx_fcxt_setcolor4f(plx_cxt, plx_opacity, 0.9f, 0.9f, 0.9f);
	} else {
		ds_printf("DS_ERROR: Can't load %s\n", fn);
	}
	
	mutex_init((mutex_t *)&video_mutex, MUTEX_TYPE_NORMAL);
	return 1; 
}


void ShutdownVideo() { 
	
	mutex_destroy(&video_mutex);
	
	if(plx_screen_texture) {
		free(plx_screen_texture);
		plx_screen_texture = NULL;
	}
	 
	if (plx_cxt) {
		plx_fcxt_destroy(plx_cxt);
		plx_cxt = NULL;
	}
	if (plx_fnt) {
		plx_font_destroy(plx_fnt);
		plx_fnt = NULL;
	}
		
	SDL_Quit();
}



void SDL_DS_SetWindow(int width, int height) {
	
	LockVideo();

	sdl_dc_width = width;
	sdl_dc_height = height;

	sdl_dc_u1 = 0.3f*(1.0f/((float)sdl_dc_wtex));
	sdl_dc_v1 = 0.3f*(1.0f/((float)sdl_dc_wtex));
	sdl_dc_u2 = (((float)sdl_dc_width)+0.5f)*(1.0f/((float)sdl_dc_wtex));
	sdl_dc_v2 = (((float)sdl_dc_height)+0.5f)*(1.0f/((float)sdl_dc_htex));
	
	//plx_mat3d_perspective(45.0f, (float)sdl_dc_width / (float)sdl_dc_height, 0.1f, 100.0f);
	
	/*
	dbg_printf(" sdl_dc_width=%d\n sdl_dc_height=%d\n sdl_dc_wtex=%d\n sdl_dc_htex=%d\n sdl_dc_u1=%f\n sdl_dc_v1=%f\n sdl_dc_u2=%f\n sdl_dc_v2=%f\n", 
	sdl_dc_width, sdl_dc_height,
	sdl_dc_wtex, sdl_dc_htex, sdl_dc_u1, sdl_dc_v1, sdl_dc_u2, sdl_dc_v2);
	*/
	UnlockVideo();
}


static inline void plx_vert_ifpm3(int flags, float x, float y, float z, uint32 color, float u, float v) {       
	plx_mat_tfip_3d(x, y, z);
	plx_vert_ifp(flags, x, y, z, color, u, v);
	//printf("%f %f %f\n",x,y,z);
}


void SDL_DS_Blit_Textured() {
	
	if(!first_fade && screen_opacity < 0.9f && plx_fnt) {
		
		point_t w;
		w.x = 255.0f;
		w.y = 230.0f;
		w.z = 12.0f;

		pvr_list_begin(PVR_LIST_TR_POLY);
		plx_fcxt_begin(plx_cxt);
		plx_fcxt_setpos_pnt(plx_cxt, &w);
//		plx_fcxt_setcolor4f(plx_cxt, plx_opacity, 0.2f, 0.2f, 0.2f);
		plx_fcxt_setcolor4f(plx_cxt, plx_opacity, 0.9f, 0.9f, 0.9f);
		plx_fcxt_draw(plx_cxt, "Loading...");
		plx_fcxt_end(plx_cxt);
	}
	
	//printf("Draw frame: %d\n", ++frame);
	
	if (screen_changed) {
		
#ifdef DEBUG_VIDEO
		ds_printf("%s: buftex=%p memtex=%p size=%d\n", __func__, (unsigned)sdl_dc_buftex, sdl_dc_memtex, sdl_dc_wtex * sdl_dc_htex * 2);
#endif
		
		//if(wait_dma_ready() < 0) {
			//pvr_txr_load(sdl_dc_buftex, sdl_dc_memtex, sdl_dc_wtex * sdl_dc_htex * 2);
			sq_cpy(sdl_dc_memtex, sdl_dc_buftex, sdl_dc_wtex * sdl_dc_htex * 2);
		//} else {
//			dcache_flush_range((unsigned)sdl_dc_buftex, sdl_dc_wtex * sdl_dc_htex * 2);
//			pvr_txr_load_dma(sdl_dc_buftex, sdl_dc_memtex, sdl_dc_wtex * sdl_dc_htex * 2, -1, NULL, 0);
		//}
		
		screen_changed = 0;
	}
	
	pvr_list_begin(PVR_LIST_TR_POLY);

	plx_mat3d_identity();
	plx_mat3d_translate(sdl_dc_trans_x, sdl_dc_trans_y, sdl_dc_trans_z);
	//plx_mat3d_translate((sdl_dc_wtex/2)+512.0f*sdl_dc_trans_x/native_width,(sdl_dc_htex/2)+512.0f*sdl_dc_trans_y/native_height,sdl_dc_trans_z - 3);
	
	/* Clear internal to an identity matrix */
	plx_mat_identity();
	
	/* "Applying" all matrixs: multiply a matrix onto the "internal" one */
	plx_mat3d_apply_all();

	plx_mat3d_rotate(sdl_dc_rot_x, 1.0f, 0.0f, 0.0f);
	plx_mat3d_rotate(sdl_dc_rot_y, 0.0f, 1.0f, 0.0f);
	plx_mat3d_rotate(sdl_dc_rot_z, 0.0f, 0.0f, 1.0f);
	plx_mat3d_translate(0, 0, 0);
	
	// FIXME: I don't know how it's works
//	if(screen_filter > PVR_FILTER_BILINEAR) {
//		if(screen_filter == PVR_FILTER_TRILINEAR1) {
//			screen_filter = PVR_FILTER_TRILINEAR2;
//			plx_txr_setfilter(plx_screen_texture, screen_filter);
//		} else {
//			screen_filter = PVR_FILTER_TRILINEAR1;
//			plx_txr_setfilter(plx_screen_texture, screen_filter);
//		}
//	}
	
	plx_cxt_texture(plx_screen_texture);
	//plx_cxt_culling(PLX_CULL_NONE);
	plx_cxt_send(PLX_LIST_TR_POLY);
	
	uint32 color = PVR_PACK_COLOR(screen_opacity, 1.0f, 1.0f, 1.0f);
	
	plx_vert_ifpm3(PLX_VERT, sdl_dc_x, sdl_dc_y, sdl_dc_z, color, sdl_dc_u1, sdl_dc_v1);
	plx_vert_ifpm3(PLX_VERT, sdl_dc_x + native_width, sdl_dc_y, sdl_dc_z, color, sdl_dc_u2, sdl_dc_v1);
	plx_vert_ifpm3(PLX_VERT, sdl_dc_x, sdl_dc_y + native_height, sdl_dc_z, color, sdl_dc_u1, sdl_dc_v2);
	plx_vert_ifpm3(PLX_VERT_EOS, sdl_dc_x + native_width, sdl_dc_y + native_height, sdl_dc_z, color, sdl_dc_u2, sdl_dc_v2);
}


static void *VideoThread(void *ptr) {

	while(video_inited) {

		LockVideo();

		/*
		plx_mat3d_identity();
		plx_mat_identity();
		plx_mat3d_apply_all();
		*/
		
		pvr_wait_ready();
		pvr_scene_begin();

		if(draw_screen) {

			ScreenFadeStep();
			ProcessVideoEventsRender();
			SDL_DS_Blit_Textured();

		} else {
			ProcessVideoEventsRender();
		}
		
		pvr_scene_finish();
		UnlockVideo();
	}

	//dbglog(DBG_DEBUG, "Exiting from video thread\n");
	return NULL;
}


void SetScreenMode(int w, int h, float x, float y, float z) {

	Settings_t *settings = GetSettings();
	
	if(settings->video.tex_filter < PVR_FILTER_NEAREST) {
		if(w > 640 || h > 480) {
			SetScreenFilter(PVR_FILTER_BILINEAR);
//			SetScreenFilter(PVR_FILTER_TRILINEAR1);
		} else {
			SetScreenFilter(PVR_FILTER_NEAREST);
		}
	}

#if 0	
	if(h > 512) {

		sdl_dc_htex = 1024;
		DScreen->h = h;
		SDL_DS_FreeScreenTexture(0);
		SDL_DS_AllocScreenTexture(DScreen);

		if(plx_screen_texture != NULL) {
			plx_screen_texture->h = sdl_dc_htex;
			plx_screen_texture->ptr = sdl_dc_memtex;
			plx_fill_contexts(plx_screen_texture);
		}
		
	} else {
		
		if(DScreen->h > 512) {
			
			DScreen->h = h;
			sdl_dc_htex = 512;
			SDL_DS_FreeScreenTexture(0);
			SDL_DS_AllocScreenTexture(DScreen);

			if(plx_screen_texture != NULL) {
				plx_screen_texture->h = sdl_dc_htex;
				plx_screen_texture->ptr = sdl_dc_memtex;
				plx_fill_contexts(plx_screen_texture);
			}
		}
	}
#else
	DScreen->h = h;
#endif
	DScreen->w = w;
	sdl_dc_x = x;
	sdl_dc_y = y;
	sdl_dc_z = z;
	
	SDL_DS_SetWindow(w, h);

	/* FIXME: resize console
	ConsoleInformation *console = GetConsole();
	
	if(console != NULL && (console->ConsoleSurface->w != w || console->ConsoleSurface->h != h)) {
		SDL_Rect rect;
		rect.w = w;
		rect.h = h;
		rect.x = 0;
		rect.y = 0;
		CON_Resize(console, rect);
	}
	*/
}


Uint32 SDL_GetPixel(SDL_Surface *surface, int x, int y) {
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to retrieve */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

    switch(bpp) {
    case 1:
        return *p;
        break;

    case 2:
        return *(Uint16 *)p;
        break;

    case 3:
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN)
            return p[0] << 16 | p[1] << 8 | p[2];
        else
            return p[0] | p[1] << 8 | p[2] << 16;
        break;

    case 4:
        return *(Uint32 *)p;
        break;

    default:
        return 0;       /* shouldn't happen, but avoids warnings */
    }
}



void SDL_PutPixel(SDL_Surface *surface, int x, int y, Uint32 pixel) {
    int bpp = surface->format->BytesPerPixel;
    /* Here p is the address to the pixel we want to set */
    Uint8 *p = (Uint8 *)surface->pixels + y * surface->pitch + x * bpp;

    switch(bpp) {
    case 1:
        *p = pixel;
        break;

    case 2:
        *(Uint16 *)p = pixel;
        break;

    case 3:
        if(SDL_BYTEORDER == SDL_BIG_ENDIAN) {
            p[0] = (pixel >> 16) & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = pixel & 0xff;
        } else {
            p[0] = pixel & 0xff;
            p[1] = (pixel >> 8) & 0xff;
            p[2] = (pixel >> 16) & 0xff;
        }
        break;

    case 4:
        *(Uint32 *)p = pixel;
        break;
    }
}


int gzip_kmg_to_img(const char * fn, kos_img_t * rv) {
	gzFile f;
	kmg_header_t	hdr;

	/* Open the file */
	f = gzopen(fn, "r");
	
	if (f == NULL) {
		dbglog(DBG_ERROR, "%s: can't open file '%s'\n", __func__, fn);
		return -1;
	}

	/* Read the header */
	if (gzread(f, &hdr, sizeof(hdr)) != sizeof(hdr)) {
		gzclose(f);
		dbglog(DBG_ERROR, "%s: can't read header from file '%s'\n", __func__, fn);
		return -2;
	}

	/* Verify a few things */
	if (hdr.magic != KMG_MAGIC || hdr.version != KMG_VERSION ||
		hdr.platform != KMG_PLAT_DC)
	{
		gzclose(f);
		dbglog(DBG_ERROR, "%s: file '%s' is incompatible:\n"
			"   magic %08lx version %d platform %d\n",
			__func__, fn, hdr.magic, (int)hdr.version, (int)hdr.platform);
		return -3;
	}


	/* Setup the kimg struct */
	rv->w = hdr.width;
	rv->h = hdr.height;
	rv->byte_count = hdr.byte_count;
	rv->data = malloc(hdr.byte_count);
	
	if (!rv->data) {
		dbglog(DBG_ERROR, "%s: can't malloc(%d) while loading '%s'\n",
			__func__, (int)hdr.byte_count, fn);
		gzclose(f);
		return -4;
	}
	
	
	int dep = 0;
	if (hdr.format & KMG_DCFMT_VQ)
		dep |= PVR_TXRLOAD_FMT_VQ;
	if (hdr.format & KMG_DCFMT_TWIDDLED)
		dep |= PVR_TXRLOAD_FMT_TWIDDLED;

	switch (hdr.format & KMG_DCFMT_MASK) {
	case KMG_DCFMT_RGB565:
		rv->fmt = KOS_IMG_FMT(KOS_IMG_FMT_RGB565, dep);
		break;

	case KMG_DCFMT_ARGB4444:
		rv->fmt = KOS_IMG_FMT(KOS_IMG_FMT_ARGB4444, dep);
		break;

	case KMG_DCFMT_ARGB1555:
		rv->fmt = KOS_IMG_FMT(KOS_IMG_FMT_ARGB1555, dep);
		break;

	case KMG_DCFMT_YUV422:
		rv->fmt = KOS_IMG_FMT(KOS_IMG_FMT_YUV422, dep);
		break;

	case KMG_DCFMT_BUMP:
		/* XXX */
		rv->fmt = KOS_IMG_FMT(KOS_IMG_FMT_RGB565, dep);
		break;

	case KMG_DCFMT_4BPP_PAL:
	case KMG_DCFMT_8BPP_PAL:
	default:
		dbglog(DBG_ERROR, "%s: currently-unsupported KMG pixel format", __func__);
		gzclose(f);
		free(rv->data);
		return -5;
	}
	
	if (gzread(f, rv->data, rv->byte_count) != rv->byte_count) {
		dbglog(DBG_ERROR, "%s: can't read %d bytes while loading '%s'\n",
			__func__, (int)hdr.byte_count, fn);
		gzclose(f);
		free(rv->data);
		return -6;
	}

	/* Ok, all done */
	gzclose(f);

	/* If the byte count is not a multiple of 32, bump it up as well.
	   This is for DMA/SQ usage. */
	rv->byte_count = (rv->byte_count + 31) & ~31;
	
	return 0;
}



void ShowLogo() {

	kos_img_t img;
	float opacity = 0.0f;
	float w = native_width; //(float)DScreen->w;
	float h = native_height; //(float)DScreen->h;
	float u1, v1, u2, v2;

	pvr_poly_cxt_t cxt;
	pvr_poly_hdr_t hdr;
	pvr_vertex_t vert;

	screen_opacity = 0.0f;

	if(gzip_kmg_to_img("/rd/logo.kmg.gz", &img)) {
	//if(png_to_img(fn, &img)) {
		dbglog(DBG_ERROR, "%s: error in gzip_kmg_to_img\n", __func__);
		return;
	}

	logo_txr = pvr_mem_malloc(img.byte_count);
	
	if(!logo_txr) {
		kos_img_free(&img, 0);
		return;
	}
	
	if(img.w)
		logo_w = img.w;
		
	if(img.h)
		logo_h = img.h;

	u1 = 0.3f * (1.0f / ((float)logo_w));
	v1 = 0.3f * (1.0f / ((float)logo_w));
	u2 = (w + 0.5f) * (1.0f / ((float)logo_w));
	v2 = (h + 0.5f) * (1.0f / ((float)logo_h));

	pvr_txr_load_kimg(&img, logo_txr, 0);
	kos_img_free(&img, 0);
	
	if(video_inited) {
		ShutdownVideoThread();
	}
	
	while(opacity < 1.0f) {
		pvr_wait_ready();
		pvr_scene_begin();
		pvr_list_begin(PVR_LIST_TR_POLY);

		pvr_poly_cxt_txr(&cxt, PVR_LIST_TR_POLY, PVR_TXRFMT_RGB565, logo_w, logo_h, logo_txr, PVR_FILTER_BILINEAR);
		pvr_poly_compile(&hdr, &cxt);
		pvr_prim(&hdr, sizeof(hdr));
		
		vert.argb = PVR_PACK_COLOR(opacity, 1.0f, 1.0f, 1.0f);
		vert.oargb = 0;
		vert.flags = PVR_CMD_VERTEX;

		vert.x = 1.0f;
		vert.y = 1.0f;
		vert.z = 1.0f;
		vert.u = u1;
		vert.v = v1;
		pvr_prim(&vert, sizeof(vert));

		vert.x = native_width;
		vert.y = 1.0f;
		vert.z = 1.0f;
		vert.u = u2;
		vert.v = v1;
		pvr_prim(&vert, sizeof(vert));

		vert.x = 1.0f;
		vert.y = native_height;
		vert.z = 1.0f;
		vert.u = u1;
		vert.v = v2;
		pvr_prim(&vert, sizeof(vert));

		vert.x = native_width;
		vert.y = native_height;
		vert.z = 1.0f;
		vert.u = u2;
		vert.v = v2;
		vert.flags = PVR_CMD_VERTEX_EOL;
		pvr_prim(&vert, sizeof(vert));
		
		pvr_list_finish();
		pvr_scene_finish();
		opacity += 0.04f;
	}
}

void HideLogo() {

	float opacity = 1.0f;
	float w = native_width;//(float)DScreen->w;
	float h = native_height;//(float)DScreen->h;
	float u1 = 0.3f * (1.0f / ((float)logo_w));
	float v1 = 0.3f * (1.0f / ((float)logo_w));
	float u2 = (w + 0.5f) * (1.0f / ((float)logo_w));
	float v2 = (h + 0.5f) * (1.0f / ((float)logo_h));

	pvr_poly_cxt_t cxt;
	pvr_poly_hdr_t hdr;
	pvr_vertex_t vert;
	
	if(!logo_txr) {
		scr_fade_act = 1;
		return;
	}
	
	while(opacity > 0.0f) {
		pvr_wait_ready();
		pvr_scene_begin();
		pvr_list_begin(PVR_LIST_TR_POLY);

		pvr_poly_cxt_txr(&cxt, PVR_LIST_TR_POLY, PVR_TXRFMT_RGB565, logo_w, logo_h, logo_txr, PVR_FILTER_BILINEAR);
		pvr_poly_compile(&hdr, &cxt);
		pvr_prim(&hdr, sizeof(hdr));
		
		vert.argb = PVR_PACK_COLOR(opacity, 1.0f, 1.0f, 1.0f);
		vert.oargb = 0;
		vert.flags = PVR_CMD_VERTEX;

		vert.x = 1.0f;
		vert.y = 1.0f;
		vert.z = 1.0f;
		vert.u = u1;
		vert.v = v1;
		pvr_prim(&vert, sizeof(vert));

		vert.x = native_width;
		vert.y = 1.0f;
		vert.z = 1.0f;
		vert.u = u2;
		vert.v = v1;
		pvr_prim(&vert, sizeof(vert));

		vert.x = 1.0f;
		vert.y = native_height;
		vert.z = 1.0f;
		vert.u = u1;
		vert.v = v2;
		pvr_prim(&vert, sizeof(vert));

		vert.x = native_width;
		vert.y = native_height;
		vert.z = 1.0f;
		vert.u = u2;
		vert.v = v2;
		vert.flags = PVR_CMD_VERTEX_EOL;
		pvr_prim(&vert, sizeof(vert));
		
		pvr_list_finish();
		pvr_scene_finish();
		opacity -= 0.05f;
	}
	
	pvr_mem_free(logo_txr);
	scr_fade_act = 1;
	
	if(!video_inited) {
		InitVideoThread();
	}
}
