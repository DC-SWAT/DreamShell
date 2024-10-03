/****************************
 * DreamShell ##version##   *
 * video.c                  *
 * DreamShell video         *
 * (c)2004-2024 SWAT        *
 * http://www.dc-swat.ru    *
 ****************************/

#include "ds.h"
#include "console.h"
#include "img/load.h"
#include <kmg/kmg.h>
#include <zlib/zlib.h>

static int sdl_dc_no_ask_60hz = 0;
static int sdl_dc_default_60hz = 0;
#include "../lib/SDL/src/video/dc/60hz.h"

#define SCREEN_FADE_STEP 0.075f

static SDL_Surface *DScreen = NULL; 
static mutex_t video_mutex = MUTEX_INITIALIZER;
static kthread_t *video_thd;
static int video_inited = 0;
static int video_mode = -1;

static float screen_opacity = 1.0f;
static float scr_fade_text_opacity = 1.0f;
static char *scr_fade_text = NULL;
static volatile uint32 scr_fade_act = 0;
static volatile uint32 screen_changed = 1, draw_screen = 1;

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
static plx_texture_t *plx_screen_txr;
static plx_texture_t *plx_cursor_txr;
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

	plx_txr_setfilter(plx_screen_txr, screen_filter);
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
	LockVideo();
	screen_changed = 1;
	UnlockVideo();
}

int ScreenUpdated() {
	return screen_changed == 0;
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
	scr_fade_text = "Loading...";
	scr_fade_act = 2;
	UnlockVideo();
}

void ScreenFadeOutEx(const char *text, int wait) {
	LockVideo();
	scr_fade_text = (char *)text;
	scr_fade_act = 2;
	UnlockVideo();

	if(wait) {
		do {
			thd_pass();
		} while(scr_fade_act != 0);
	}
}

void ScreenFadeStop() {
	LockVideo();
	screen_opacity = scr_fade_act == 1 ? 1.0f : 0.0f;
	scr_fade_act = 0;
	UnlockVideo();
}

int ScreenIsHidden() {
	return scr_fade_act != 0 || screen_opacity == 0.0f;
}

static inline void ScreenFadeStep() {
	switch(scr_fade_act) {
		case 1:
			screen_opacity += SCREEN_FADE_STEP;
			scr_fade_text_opacity -= SCREEN_FADE_STEP;

			if(screen_opacity >= 1.0f) {
				screen_opacity = 1.0f;
				scr_fade_text_opacity = 0.0f;
				scr_fade_act = 0;
			}
			break;
		case 2:
			screen_opacity -= SCREEN_FADE_STEP;
			scr_fade_text_opacity += SCREEN_FADE_STEP;

			if(screen_opacity <= 0.0f) {
				screen_opacity = 0.0f;
				scr_fade_text_opacity = 1.0f;
				scr_fade_act = 0;
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

	video_inited = 0;

	if(video_thd) {
		thd_join(video_thd, NULL); 
	}

	if(VideoIsLocked()) UnlockVideo();
}

int VideoIsLocked() {
	return mutex_is_locked(&video_mutex);
}

int VideoMustLock() {
	return (video_inited && video_thd != thd_get_current());
}

void LockVideo() { 
	if(!VideoMustLock()) {
		return;
	}
	mutex_lock(&video_mutex);
}

void UnlockVideo() {
	if(!VideoMustLock()) {
		return;
	}
	mutex_unlock(&video_mutex);
}


void plx_fill_contexts(plx_texture_t * txr) {
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
	pvr_dma_init();
}


int InitVideo(int w, int h, int bpp) {
    
	int sdl_init_flags, sdl_video_flags, tex_mode;
	char fn[NAME_MAX];
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
		dbglog(DBG_ERROR, "SDL init failed: %s\n", SDL_GetError());
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

	plx_cxt_init();

	if(settings->video.tex_filter >= PVR_FILTER_NEAREST) {
		screen_filter = settings->video.tex_filter;
	}

	plx_screen_txr = (plx_texture_t*) malloc(sizeof(plx_texture_t));

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

	if(plx_screen_txr != NULL) {
		plx_screen_txr->ptr = sdl_dc_memtex;
		plx_screen_txr->w = w;
		plx_screen_txr->h = h;
		plx_screen_txr->fmt = tex_mode | PVR_TXRFMT_NONTWIDDLED;
		plx_fill_contexts(plx_screen_txr);
		plx_txr_setfilter(plx_screen_txr, screen_filter);
	}

	sprintf(fn, "%s/gui/cursors/default.png", getenv("PATH"));
	plx_cursor_txr = plx_txr_load(fn, 1, PVR_TXRLOAD_16BPP | PVR_TXRLOAD_SQ);

	sprintf(fn, "%s/fonts/txf/axaxax.txf", getenv("PATH"));
	plx_fnt = plx_font_load(fn);

	if(plx_fnt) {
		plx_cxt = plx_fcxt_create(plx_fnt, PVR_LIST_TR_POLY);
		plx_fcxt_setcolor4f(plx_cxt, scr_fade_text_opacity, 0.9f, 0.9f, 0.9f);
	} else {
		ds_printf("DS_ERROR: Can't load %s\n", fn);
	}

	mutex_init((mutex_t *)&video_mutex, MUTEX_TYPE_NORMAL);
	return 1; 
}


void ShutdownVideo() { 

	mutex_destroy(&video_mutex);

	if(plx_screen_txr) {
		free(plx_screen_txr);
		plx_screen_txr = NULL;
	}

	if(plx_cursor_txr) {
		plx_txr_destroy(plx_cursor_txr);
		plx_cursor_txr = NULL;
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
	UnlockVideo();
}


void SDL_DS_Blit_Textured() {

	if(screen_opacity < 0.9f && plx_fnt && scr_fade_text) {

		point_t w;
		w.x = 255.0f;
		w.y = 230.0f;
		w.z = 12.0f;

		plx_fcxt_begin(plx_cxt);
		plx_fcxt_setpos_pnt(plx_cxt, &w);
		plx_fcxt_setcolor4f(plx_cxt, scr_fade_text_opacity, 0.9f, 0.9f, 0.9f);
		plx_fcxt_draw(plx_cxt, scr_fade_text);
		plx_fcxt_end(plx_cxt);
	}

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

	plx_cxt_texture(plx_screen_txr);
	plx_cxt_culling(PLX_CULL_NONE);
	plx_cxt_send(PLX_LIST_TR_POLY);

	uint32 color = PVR_PACK_COLOR(screen_opacity, 1.0f, 1.0f, 1.0f);

	plx_vert_ifpm3(PLX_VERT, sdl_dc_x, sdl_dc_y, sdl_dc_z, color, sdl_dc_u1, sdl_dc_v1);
	plx_vert_ifpm3(PLX_VERT, sdl_dc_x + native_width, sdl_dc_y, sdl_dc_z, color, sdl_dc_u2, sdl_dc_v1);
	plx_vert_ifpm3(PLX_VERT, sdl_dc_x, sdl_dc_y + native_height, sdl_dc_z, color, sdl_dc_u1, sdl_dc_v2);
	plx_vert_ifpm3(PLX_VERT_EOS, sdl_dc_x + native_width, sdl_dc_y + native_height, sdl_dc_z, color, sdl_dc_u2, sdl_dc_v2);

	if (!ConsoleIsVisible()) {
		int mouse_cursor_x = 0;
		int mouse_cursor_y = 0;

		SDL_GetMouseState(&mouse_cursor_x, &mouse_cursor_y);

		plx_cxt_texture(plx_cursor_txr);
		plx_cxt_culling(PLX_CULL_NONE);
		plx_cxt_send(PLX_LIST_TR_POLY);

		plx_vert_ifpm3(PLX_VERT, mouse_cursor_x, mouse_cursor_y, 1.0f, color, 0.0f, 0.0f);
		plx_vert_ifpm3(PLX_VERT, mouse_cursor_x + plx_cursor_txr->w, mouse_cursor_y, 1.0f, color, 1.0f, 0.0f);
		plx_vert_ifpm3(PLX_VERT, mouse_cursor_x, mouse_cursor_y + plx_cursor_txr->w, 1.0f, color, 0.0f, 1.0f);
		plx_vert_ifpm3(PLX_VERT_EOS, mouse_cursor_x + plx_cursor_txr->w, mouse_cursor_y + plx_cursor_txr->w, 1.0f, color, 1.0f, 1.0f);
	}
}


static void *VideoThread(void *ptr) {

	while(video_inited) {

		if(pvr_wait_ready() < 0) {
			// dbglog(DBG_ERROR, "VideoThread: pvr_wait_ready() failed\n");
			thd_pass();
			continue;
		}

		LockVideo();

		if(screen_changed && draw_screen) {
			// pvr_txr_load(sdl_dc_buftex, sdl_dc_memtex, sdl_dc_wtex * sdl_dc_htex * 2);
			dcache_flush_range((uintptr_t)sdl_dc_buftex, sdl_dc_wtex * sdl_dc_htex * 2);
			do {
				int rs = pvr_txr_load_dma(sdl_dc_buftex, sdl_dc_memtex,
					sdl_dc_wtex * sdl_dc_htex * 2, 1, NULL, 0);
				if(rs < 0 && errno == EINPROGRESS) {
					thd_pass();
					continue;
				}
				break;
			} while(1);
			screen_changed = 0;
		}

		pvr_scene_begin();

		if(draw_screen) {
			pvr_list_begin(PVR_LIST_TR_POLY);
			ScreenFadeStep();

			ProcessVideoEventsRender();
			SDL_DS_Blit_Textured();
			ProcessVideoEventsRenderPost();

			pvr_list_finish();
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

		if(plx_screen_txr != NULL) {
			plx_screen_txr->h = sdl_dc_htex;
			plx_screen_txr->ptr = sdl_dc_memtex;
			plx_fill_contexts(plx_screen_txr);
		}
		
	} else {
		
		if(DScreen->h > 512) {
			
			DScreen->h = h;
			sdl_dc_htex = 512;
			SDL_DS_FreeScreenTexture(0);
			SDL_DS_AllocScreenTexture(DScreen);

			if(plx_screen_txr != NULL) {
				plx_screen_txr->h = sdl_dc_htex;
				plx_screen_txr->ptr = sdl_dc_memtex;
				plx_fill_contexts(plx_screen_txr);
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

void ShowLogo() {

	kos_img_t img;
	float opacity = 0.0f;
	float w = native_width;
	float h = native_height;
	float u1, v1, u2, v2;

	pvr_poly_cxt_t cxt;
	pvr_poly_hdr_t hdr;
	pvr_vertex_t vert;

	screen_opacity = 0.0f;

	if(gzip_kmg_to_img("/rd/logo.kmg.gz", &img)) {
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
	float w = native_width;
	float h = native_height;
	float u1 = 0.3f * (1.0f / ((float)logo_w));
	float v1 = 0.3f * (1.0f / ((float)logo_w));
	float u2 = (w + 0.5f) * (1.0f / ((float)logo_w));
	float v2 = (h + 0.5f) * (1.0f / ((float)logo_h));

	pvr_poly_cxt_t cxt;
	pvr_poly_hdr_t hdr;
	pvr_vertex_t vert;

	if(!logo_txr) {
		scr_fade_act = 1;
		if(!video_inited) {
			InitVideoThread();
		}
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
