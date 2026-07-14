/* DreamShell
   
   utils.c - Arcade app utils
   Copyright (C) 2026 SWAT
   
*/

#include <ds.h>
#include <kos/md5.h>
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

void GetMD5(const char *path, uint8 *md5) {
    file_t fd = fs_open(path, O_RDONLY);
    if (fd != FILEHND_INVALID) {
        uint8_t boot_sector[2048];
        if (fs_read(fd, boot_sector, sizeof(boot_sector)) == sizeof(boot_sector)) {
            kos_md5(boot_sector, sizeof(boot_sector), md5);
        }
        else {
            memset(md5, 0, 16);
        }
        fs_close(fd);
    }
    else {
        memset(md5, 0, 16);
    }
}

int IsGameExtension(const char *filename) {
    int len = strlen(filename);
    if (len > 4) {
        const char *ext = filename + len - 4;
        if (!strcasecmp(ext, ".gdi") || !strcasecmp(ext, ".cdi") || 
            !strcasecmp(ext, ".iso") || !strcasecmp(ext, ".cso") ||
            !strcasecmp(ext, ".dni") || !strcasecmp(ext, ".bin")) {
            return 1;
        }
    }
    return 0;
}
