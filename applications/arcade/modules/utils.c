/* DreamShell
   
   utils.c - Arcade app utils
   Copyright (C) 2026 SWAT
   
*/

#include <ds.h>
#include <isoldr.h>
#include <naomi/cart.h>
#include "app_internal.h"

int getDeviceType(const char *dir) {
    if(!strncasecmp(dir, "/cd", 3)) {
        return APP_DEVICE_CD;
    }
    else if(!strncasecmp(dir, "/sd",   3)) {
        return APP_DEVICE_SD;
    }
    else if(!strncasecmp(dir, "/ide",  4)) {
        return APP_DEVICE_IDE;
    }
    else if(!strncasecmp(dir, "/pc",   3)) {
        return APP_DEVICE_PC;
    }
    else {
        return -1;
    }
}

void makeGameRelativePath(char *dst, size_t dst_size, const char *base_path, const char *game_path, const char *filename) {
    char game_dir[NAME_MAX];
    getFirstPathComponent(game_path, game_dir);
    snprintf(dst, dst_size, "%s/%s/%s", base_path, game_dir, filename);
}

void getFirstPathComponent(const char *path, char *result) {
    const char *p = strchr(path, '/');
    memset(result, 0, NAME_MAX);

    if (p) {
        size_t len = p - path;
        strncpy(result, path, len);
    }
}

int IsNaomiRom(const char *path) {
    file_t f = fs_open(path, O_RDONLY);
    if (f == FILEHND_INVALID) return 0;

    naomi_cart_header_t hdr;
    if (fs_read(f, &hdr, sizeof(hdr)) != sizeof(hdr)) {
        fs_close(f);
        return 0;
    }
    fs_close(f);

    return (strncmp(hdr.system_name, "NAOMI", 5) == 0);
}

int IsGameExtension(const char *filename) {
    int len = strlen(filename);
    const char *ext;

    if (len <= 4) {
        return 0;
    }
    if (!strncasecmp(filename, "track", 5)) {
        return 0;
    }

    ext = filename + len - 4;

    if (!strcasecmp(ext, ".gdi")) {
        return 4;
    }
    if (!strcasecmp(ext, ".dni")) {
        return 3;
    }
    if (!strcasecmp(ext, ".cdi") || !strcasecmp(ext, ".cso") ||
        !strcasecmp(ext, ".iso")) {
        return 2;
    }
    if (!strcasecmp(ext, ".bin")) {
        return 1;
    }
    return 0;
}
