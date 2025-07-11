/* DreamShell ##version##

   gallery.c - Dreameye app gallery
   Copyright (C) 2025 SWAT
*/

#include <ds.h>
#include <drivers/dreameye.h>
#include "gallery.h"
#include "photo.h"

typedef struct {
    GUI_Surface *surface;
    GUI_Surface *hl_surface;
    int photo_index;
    bool loaded;
} gallery_thumb_t;

typedef struct {
    int photo_index;
    uint32_t width;
    uint32_t height;
    gallery_photo_callback_t callback;
} photo_load_params_t;

typedef struct {
    int page;
    gallery_thumb_callback_t callback;
} page_load_params_t;

static struct {
    gallery_state_t state;
    gallery_thumb_t thumbs[GALLERY_MAX_THUMBS];
    maple_device_t *device;
} gallery;

gallery_state_t *gallery_get_state(void) {
    return &gallery.state;
}

int gallery_init(maple_device_t *device) {
    memset(&gallery.state, 0, sizeof(gallery_state_t));
    gallery.state.current_page = 0;
    gallery.state.current_photo = -1;
    gallery.state.total_photos = 0;
    gallery.state.total_pages = (gallery.state.total_photos + GALLERY_THUMBS_PER_PAGE - 1) / GALLERY_THUMBS_PER_PAGE;
    gallery.device = device;
    return 0;
}

void gallery_shutdown(void) {
    gallery_clear_cache();
}

void gallery_clear_cache(void) {
    gallery_load_abort();

    for (int i = 0; i < GALLERY_MAX_THUMBS; i++) {
        if (gallery.thumbs[i].surface) {
            GUI_ObjectDecRef((GUI_Object*)gallery.thumbs[i].surface);
            gallery.thumbs[i].surface = NULL;
        }
        if (gallery.thumbs[i].hl_surface) {
            GUI_ObjectDecRef((GUI_Object*)gallery.thumbs[i].hl_surface);
            gallery.thumbs[i].hl_surface = NULL;
        }
        gallery.thumbs[i].loaded = 0;
        gallery.thumbs[i].photo_index = -1;
    }
}

static GUI_Surface *create_alpha_surface(GUI_Surface *orig_surface, const char *name) {
    if (!orig_surface) {
        return NULL;
    }

    SDL_Surface *orig_sdl = GUI_SurfaceGet(orig_surface);
    GUI_Surface *hl_surface = GUI_SurfaceCreate(name,
        orig_sdl->flags,
        orig_sdl->w,
        orig_sdl->h,
        orig_sdl->format->BitsPerPixel,
        orig_sdl->format->Rmask,
        orig_sdl->format->Gmask,
        orig_sdl->format->Bmask,
        orig_sdl->format->Amask);

    if (!hl_surface) {
        return NULL;
    }

    GUI_SurfaceBlit(orig_surface, NULL, hl_surface, NULL);

    SDL_Surface *hl_sdl = GUI_SurfaceGet(hl_surface);
    SDL_SetAlpha(hl_sdl, SDL_SRCALPHA, 150);

    return hl_surface;
}

static void *load_page_thread(void *param) {
    page_load_params_t *params = (page_load_params_t*)param;
    int page = params->page;
    gallery_thumb_callback_t callback = params->callback;

    uint8_t *jpeg_data;
    int jpeg_size;

    int start_photo = page * GALLERY_THUMBS_PER_PAGE;
    int end_photo = start_photo + GALLERY_THUMBS_PER_PAGE;

    if (end_photo > gallery.state.total_photos) {
        end_photo = gallery.state.total_photos;
    }

    for (int i = start_photo; i < end_photo; i++) {
        if (gallery.state.loading_abort) break;

        int thumb_idx = i % GALLERY_MAX_THUMBS;

        if (gallery.thumbs[thumb_idx].loaded && gallery.thumbs[thumb_idx].photo_index == i) {
            gallery.state.loading_progress = ((i - start_photo + 1) * 100) / (end_photo - start_photo);
            if (callback) {
                callback(i - start_photo, gallery.thumbs[thumb_idx].surface,
                    gallery.thumbs[thumb_idx].hl_surface);
            }
            continue;
        }

        if (gallery.thumbs[thumb_idx].surface) {
            GUI_ObjectDecRef((GUI_Object*)gallery.thumbs[thumb_idx].surface);
            gallery.thumbs[thumb_idx].surface = NULL;
        }
        if (gallery.thumbs[thumb_idx].hl_surface) {
            GUI_ObjectDecRef((GUI_Object*)gallery.thumbs[thumb_idx].hl_surface);
            gallery.thumbs[thumb_idx].hl_surface = NULL;
        }

        if (dreameye_get_image(gallery.device, i + 2, &jpeg_data, &jpeg_size) == MAPLE_EOK) {

            char photo_name[32];
            snprintf(photo_name, sizeof(photo_name), "Photo %d", i + 1);

            gallery.thumbs[thumb_idx].surface = create_photo_surface(jpeg_data, jpeg_size,
                GALLERY_THUMB_WIDTH, GALLERY_THUMB_HEIGHT, photo_name);

            if (gallery.thumbs[thumb_idx].surface) {
                gallery.thumbs[thumb_idx].hl_surface = 
                    create_alpha_surface(gallery.thumbs[thumb_idx].surface, photo_name);
            }

            free(jpeg_data);
        }

        gallery.thumbs[thumb_idx].photo_index = i;
        gallery.thumbs[thumb_idx].loaded = 1;

        gallery.state.loading_progress = ((i - start_photo + 1) * 100) / (end_photo - start_photo);

        if (callback) {
            callback(i - start_photo, gallery.thumbs[thumb_idx].surface,
                gallery.thumbs[thumb_idx].hl_surface);
        }
    }

    gallery.state.loading = 0;
    free(params);
    return NULL;
}

void gallery_load_page(int page, gallery_thumb_callback_t callback) {
    if (page < 0 || page >= gallery.state.total_pages) {
        return;
    }

    while (gallery.state.loading) {
        thd_sleep(50);
    }

    page_load_params_t *params = malloc(sizeof(page_load_params_t));
    if (!params) {
        return;
    }

    gallery.state.loading_abort = 0;
    gallery.state.loading = 1;
    gallery.state.current_page = page;
    gallery.state.loading_progress = 0;

    params->page = page;
    params->callback = callback;

    thd_create(1, load_page_thread, params);
}

static void *load_photo_thread(void *param) {
    photo_load_params_t *params = (photo_load_params_t*)param;
    GUI_Surface *surface = NULL;
    GUI_Surface *hl_surface = NULL;
    uint8_t *jpeg_data;
    int jpeg_size;
    char photo_name[32];

    if (dreameye_get_image(gallery.device, params->photo_index + 2, &jpeg_data, &jpeg_size) == MAPLE_EOK) {            
        snprintf(photo_name, sizeof(photo_name), "Photo %d", params->photo_index + 1);
        
        surface = create_photo_surface(jpeg_data, jpeg_size, params->width, params->height, photo_name);
        if (surface && params->width > 0 && params->height > 0) {
            hl_surface = create_alpha_surface(surface, photo_name);
        }
        else {
            hl_surface = surface;
        }
        free(jpeg_data);
    }

    gallery.state.loading = 0;

    if (params->callback) {
        params->callback(surface, hl_surface);
    }

    free(params);
    return NULL;
}

void gallery_load_photo(int photo_index, int width, int height, gallery_photo_callback_t callback) {
    if (photo_index < 0 || photo_index >= gallery.state.total_photos) {
        return;
    }

    photo_load_params_t *params = malloc(sizeof(photo_load_params_t));

    if (!params) {
        return;
    }

    while (gallery.state.loading) {
        thd_pass();
    }

    gallery.state.loading_abort = 0;
    gallery.state.loading = 1;

    params->photo_index = photo_index;
    params->width = width;
    params->height = height;
    params->callback = callback;

    thd_create(1, load_photo_thread, params);
}

void gallery_load_abort(void) {
    if (!gallery.state.loading || gallery.state.loading_abort) {
        return;
    }

    gallery.state.loading_abort = 1;
    do {
        thd_pass();
    } while (gallery.state.loading);

    gallery.state.loading_abort = 0;
}
