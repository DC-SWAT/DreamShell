/* DreamShell ##version##

   gallery.h - Dreameye app gallery
   Copyright (C) 2025 SWAT
*/

#include <ds.h>
#include <dc/maple.h>
#include <stdbool.h>

#define GALLERY_CACHE_PAGES 2
#define GALLERY_THUMB_WIDTH 140
#define GALLERY_THUMB_HEIGHT 110

#define GALLERY_THUMBS_PER_PAGE 6
#define GALLERY_MAX_THUMBS (GALLERY_THUMBS_PER_PAGE * GALLERY_CACHE_PAGES)

typedef struct {
    int total_photos;
    int total_pages;
    int current_page;
    int current_photo;
    volatile bool loading;
    volatile int loading_progress;
} gallery_state_t;

typedef void (*gallery_thumb_callback_t)(int thumb_index, GUI_Surface *surface);
typedef void (*gallery_photo_callback_t)(GUI_Surface *surface);

int gallery_init(maple_device_t *device);
void gallery_shutdown(void);
void gallery_clear_cache(void);
gallery_state_t *gallery_get_state(void);

void gallery_load_page(int page, gallery_thumb_callback_t callback);
void gallery_load_photo(int photo_index, int width, int height,
    gallery_photo_callback_t callback);
void gallery_load_abort(void);
