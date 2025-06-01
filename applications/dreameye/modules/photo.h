/* DreamShell ##version##

   photo.h - Dreameye app photo
   Copyright (C) 2024-2025 SWAT
*/

#include <ds.h>

int get_photo_count(maple_device_t *dev);

int export_photo(maple_device_t *dev, const char *dir, int num);

int erase_photo(maple_device_t *dev, int num);

GUI_Surface *create_photo_surface(uint8_t *jpeg_data, size_t jpeg_size,
   int width, int height, const char *name);
