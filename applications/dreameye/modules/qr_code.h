/* DreamShell ##version##

   qr_code.h - DreamEye app QR Code scanner
   Copyright (C) 2024 SWAT
*/

#include <ds.h>

int qr_scan_init();
void qr_scan_shutdown();

int qr_scan_resize(int w, int h);
int qr_scan_frame(int bpp, uint8_t *frame, size_t len, char *qr_data);
