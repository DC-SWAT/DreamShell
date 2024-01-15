/* DreamShell ##version##

   photo.c - DreamEye app photos
   Copyright (C) 2024 SWAT 
*/

#include <ds.h>
#include <drivers/dreameye.h>

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
