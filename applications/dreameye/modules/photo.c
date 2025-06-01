/* DreamShell ##version##

   photo.c - Dreameye app photos
   Copyright (C) 2024 SWAT 
*/

#include <ds.h>
#include <drivers/dreameye.h>
#include "photo.h"

static int save_image(const char *fn, uint8 *buf, int size) {
    file_t fd;

    fd = fs_open(fn, O_CREAT | O_TRUNC | O_WRONLY);

    if(fd == FILEHND_INVALID) {
        ds_printf("DS_ERROR: Can't open file for write: %s\n", fn);
        return -1;
    }

    fs_write(fd, buf, size);
    fs_close(fd);
    return 0;
}

int get_photo_count(maple_device_t *dev) {

    dreameye_state_ext_t *state;

    if(!dev) {
        return 0;
    }

    dreameye_get_image_count(dev, 1);
    state = (dreameye_state_ext_t *)maple_dev_status(dev);

    if(state->image_count_valid && state->image_count > 0) {
        return state->image_count;
    }

    return 0;
}

int export_photo(maple_device_t *dev, const char *dir, int num) {
    char fn[NAME_MAX];
    uint8 *buf;
    int size, err;

    if(!dev) {
        return -1;
    }

    err = dreameye_get_image(dev, num + 2, &buf, &size);

    if(err != MAPLE_EOK) {
        ds_printf("DS_ERROR: No image data: %d\n", num);
        return -1;
    }

    sprintf(fn, "%s/photo_%d.jpg", dir, num);

    if(save_image(fn, buf, size) < 0) {
        ds_printf("DS_ERROR: Couldn't write to %s\n", fn);
    }
    free(buf);
    return 0;
}

int erase_photo(maple_device_t *dev, int num) {
    int err;

    if(!dev) {
        return -1;
    }

    err = dreameye_erase_image(dev, num + 2, 1);

    if(err != MAPLE_EOK) {
        ds_printf("DS_ERROR: Couldn't erase image at index: %d\n", num);
        return -1;
    }
    return 0;
}

GUI_Surface *create_photo_surface(uint8_t *jpeg_data, size_t jpeg_size,
    int width, int height, const char *name) {

    if (!jpeg_data || jpeg_size == 0) {
        return NULL;
    }

    SDL_RWops *rw = SDL_RWFromConstMem(jpeg_data, jpeg_size);
    if (!rw) {
        return NULL;
    }

    SDL_Surface *original = IMG_Load_RW(rw, 0);
    SDL_RWclose(rw);

    if (!original) {
        return NULL;
    }

    if (width == 0 && height == 0) {
        return GUI_SurfaceFrom(name, original);
    }

    double zoom_x = (double)width / (double)original->w;
    double zoom_y = (double)height / (double)original->h;
    double zoom = (zoom_x < zoom_y) ? zoom_x : zoom_y;
    
    SDL_Surface *scaled = zoomSurface(original, zoom, zoom, 1);
    SDL_FreeSurface(original);

    if (!scaled) {
        return NULL;
    }

    if (scaled->w == width && scaled->h == height) {
        return GUI_SurfaceFrom(name, scaled);
    }

    SDL_Surface *final = SDL_CreateRGBSurface(SDL_SWSURFACE, width, height, 16, 0xF800, 0x07E0, 0x001F, 0);
    if (!final) {
        SDL_FreeSurface(scaled);
        return NULL;
    }

    SDL_FillRect(final, NULL, 0);

    SDL_Rect dst = {
        (width - scaled->w) / 2,
        (height - scaled->h) / 2,
        scaled->w,
        scaled->h
    };

    SDL_BlitSurface(scaled, NULL, final, &dst);
    SDL_FreeSurface(scaled);

    return GUI_SurfaceFrom(name, final);
}
