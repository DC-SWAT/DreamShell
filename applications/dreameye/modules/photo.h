/* DreamShell ##version##

   photo.h - DreamEye app photo
   Copyright (C) 2024 SWAT
*/

#include <ds.h>

int get_photo_count(maple_device_t *dev);

int export_photo(maple_device_t *dev, const char *dir, int num);

int erase_photo(maple_device_t *dev, int num);
