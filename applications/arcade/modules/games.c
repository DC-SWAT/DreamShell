/* DreamShell
   
   games.c - Arcade app games
   Copyright (C) 2026 SWAT
   
*/

#include <kos/md5.h>
#include <naomi/cart.h>
#include <isoldr.h>
#include <stdlib.h>

#include "app_internal.h"

static void FormatGameLabelText(const GameEntry *g, char *buf, size_t size) {
    int i;

    for (i = 0; g->name[i] && i < (int)size - 1; i++) {
        buf[i] = (g->name[i] == '_') ? ' ' : g->name[i];
    }
    buf[i] = '\0';
}

static int EnsureGamesCapacity(int min_capacity) {
    if (min_capacity <= self.games_capacity) {
        return 1;
    }

    int new_capacity = self.games_capacity;

    if (new_capacity == 0) {
        new_capacity = GAMES_ALLOC_CHUNK;
    }

    while (new_capacity < min_capacity) {
        new_capacity += GAMES_ALLOC_CHUNK;
    }

    GameEntry *new_games = (GameEntry *)realloc(self.games, new_capacity * sizeof(GameEntry));

    if (!new_games) {
        ds_printf("Arcade: Failed to allocate memory for %d games\n", new_capacity);
        return 0;
    }

    if (new_capacity > self.games_capacity) {
        memset(new_games + self.games_capacity, 0,
            (new_capacity - self.games_capacity) * sizeof(GameEntry));
    }

    self.games = new_games;
    self.games_capacity = new_capacity;
    return 1;
}

static int AddGameEntry(const char *game_dir, const char *dir_name, const char *game_file) {
    if (!EnsureGamesCapacity(self.game_count + 1)) {
        ds_printf("Arcade: Out of memory, stopping scan at %d games\n", self.game_count);
        return 0;
    }

    GameEntry *g = &self.games[self.game_count];
    snprintf(g->path, sizeof(g->path), "%s", game_dir);
    snprintf(g->game_file, sizeof(g->game_file), "%s", game_file);
    snprintf(g->name, sizeof(g->name), "%s", dir_name);

    g->has_cover = false;
    g->has_trailer = false;
    g->assets_checked = false;

    self.game_count++;
    return 1;
}

void FreeGamesList(void) {
    free(self.games);
    self.games = NULL;
    self.games_capacity = 0;
    self.game_count = 0;
}

static int GetGameFileFromDir(const char *dir, char *result_name, size_t result_size) {
    file_t d = fs_open(dir, O_DIR | O_RDONLY);
    if (d == FILEHND_INVALID) return 0;

    const dirent_t *de;
    int found = 0;

    while ((de = fs_readdir(d))) {
        if (IsGameExtension(de->name)) {
            if (result_name) snprintf(result_name, result_size, "%s", de->name);
            found = 1;
            break;
        }
    }
    fs_close(d);
    return found;
}

static void ProcessGameEntry(int index) {
    GameEntry *g = &self.games[index];

    if (g->assets_checked) return;

    g->cover_path[0] = '\0';
    g->trailer_path[0] = '\0';
    g->has_cover = false;
    g->has_trailer = false;

    for (int i = 0; i < self.config.cover_exts_count; i++) {
        snprintf(g->cover_path, sizeof(g->cover_path), "%s/cover.%s", g->path, self.config.cover_exts[i]);
        if (FileExists(g->cover_path)) {
            g->has_cover = true;
            break;
        }
    }
    if (!g->has_cover) {
        g->cover_path[0] = '\0';
    }

    snprintf(g->trailer_path, sizeof(g->trailer_path), "%s/%s", g->path, self.config.trailer_filename);
    g->has_trailer = FileExists(g->trailer_path);
    if (!g->has_trailer) {
        g->trailer_path[0] = '\0';
    }
    g->assets_checked = true;
}

static bool ScanGamesInDir(const char *dir) {
    file_t d = fs_open(dir, O_DIR | O_RDONLY);
    if (d == FILEHND_INVALID) return false;

    const dirent_t *de;
    while ((de = fs_readdir(d))) {
        if (de->name[0] == '.') continue;
        if (de->attr & O_DIR) {

            char dir_name[NAME_MAX];
            strncpy(dir_name, de->name, sizeof(dir_name) - 1);
            dir_name[sizeof(dir_name) - 1] = '\0';

            char game_dir[NAME_MAX];
            snprintf(game_dir, sizeof(game_dir), "%s/%s", dir, dir_name);

            char game_file[NAME_MAX];
            if (GetGameFileFromDir(game_dir, game_file, sizeof(game_file))) {
                if (!AddGameEntry(game_dir, dir_name, game_file)) {
                    fs_close(d);
                    return false;
                }
            }
            else {
                if (!ScanGamesInDir(game_dir)) {
                    fs_close(d);
                    return false;
                }
            }
        }
    }
    fs_close(d);
    return true;
}

static void ScanGamesFallback(void) {
    int path_count = self.config.games_paths_count;

    if (path_count <= 0) {
        return;
    }

    ScanGamesInDir(self.config.games_paths[0]);

    for (int i = 1; i < path_count; i++) {
        if (self.game_count >= self.config.games_per_page) {
            break;
        }
        if (i >= 2 && self.game_count > 0) {
            break;
        }
        ScanGamesInDir(self.config.games_paths[i]);
    }
}

void ScanGames() {
    self.game_count = 0;

    if (self.config.scan_mode == ARCADE_SCAN_ALL) {
        for (int i = 0; i < self.config.games_paths_count; i++) {
            if (!ScanGamesInDir(self.config.games_paths[i])) {
                break;
            }
        }
    }
    else {
        ScanGamesFallback();
    }

    ds_printf("Arcade: Found %d games\n", self.game_count);

    self.total_pages = (self.game_count + self.config.games_per_page - 1) / self.config.games_per_page;
    if (self.total_pages == 0) self.total_pages = 1;
}

void UnloadGameVisual(int index) {
    if (index < 0 || index >= self.game_count) return;
    GameEntry *g = &self.games[index];

    if (g->visual) {
        TSU_AppSubRemoveBanner(self.app->tsunami, g->visual);
        TSU_BannerDestroy(&g->visual);
        g->visual = NULL;
    }

    if (g->label) {
        TSU_AppSubRemoveLabel(self.app->tsunami, g->label);
        TSU_LabelDestroy(&g->label);
        g->label = NULL;
    }

    if (g->texture != self.def_cover_texture) {
        TSU_TextureDestroy(&g->texture);
    }
    g->texture = NULL;

    if (g->mover) {
        TSU_LogXYZMoverDestroy(&g->mover);
        g->mover = NULL;
    }
    if (g->scale_mover) {
        TSU_LogScaleMoverDestroy(&g->scale_mover);
        g->scale_mover = NULL;
    }
    if (g->label_mover) {
        TSU_LogXYZMoverDestroy(&g->label_mover);
        g->label_mover = NULL;
    }
}

void PreloadDefaultCover(void) {
    if (self.def_cover_texture) {
        return;
    }

    char def_cover_path[NAME_MAX];
    ArcadeConfigResolvePath(self.app_path, self.config.default_cover,
        def_cover_path, sizeof(def_cover_path));

    self.def_cover_texture = TSU_TextureCreateEmpty();
    if (!TSU_TextureLoadFromFile(self.def_cover_texture, def_cover_path, true, false, 0)) {
        TSU_TextureDestroy(&self.def_cover_texture);
        self.def_cover_texture = NULL;
    }
}

void EnsurePageVisualsLoaded(void) {
    if (self.game_count <= 0) {
        return;
    }

    int start = self.current_page * self.config.games_per_page;
    int count = (self.game_count < self.config.games_per_page) ? self.game_count : self.config.games_per_page;

    for (int i = 0; i < count; i++) {
        int idx = (start + i) % self.game_count;
        if (!self.games[idx].visual) {
            LoadGameVisual(idx, self.page_switch_dir);
        }
    }
}

void LoadGameVisual(int index, int fly_in_dir) {
    if (index < 0 || index >= self.game_count) return;

    if (self.games[index].visual) {
        UnloadGameVisual(index);
    }

    GameEntry *g = &self.games[index];
    ProcessGameEntry(index);

    bool is_transparent = false;

    if (g->has_cover) {
        g->texture = TSU_TextureCreateEmpty();
        if (!TSU_TextureLoadFromFile(g->texture, g->cover_path, true, false, 0)) {
            TSU_TextureDestroy(&g->texture);
            g->texture = NULL;
            g->has_cover = false;
        }
    }
    else if (g->game_file[0]) {
        char full_game_path[NAME_MAX];
        snprintf(full_game_path, sizeof(full_game_path), "%s/%s", g->path, g->game_file);

        int fn_len = strlen(g->game_file);
        bool is_naomi = (fn_len > 4 && !strcasecmp(g->game_file + fn_len - 4, ".dni"));

        if (!is_naomi && fs_iso_mount("/isocover", full_game_path) == 0) {
            const char *cover_path = "/isocover/0GDTEX.PVR";

            if (FileExists(cover_path)) {
                g->texture = TSU_TextureCreateEmpty();
                if (TSU_TextureLoadFromFile(g->texture, cover_path, true, false, 0)) {
                    is_transparent = true;
                }
                else {
                    TSU_TextureDestroy(&g->texture);
                    g->texture = NULL;
                }
            }
            fs_iso_unmount("/isocover");
        }
    }

    if (!g->texture) {
        PreloadDefaultCover();
        g->texture = self.def_cover_texture;
        is_transparent = true;
    }

    if (!g->texture) return;

    g->visual = TSU_BannerCreate(is_transparent ? PVR_LIST_TR_POLY : PVR_LIST_OP_POLY, g->texture);
    if (!g->visual) {
        if (g->texture != self.def_cover_texture) {
            TSU_TextureDestroy(&g->texture);
        }
        g->texture = NULL;
        return;
    }

    TSU_BannerSetSize(g->visual, COVER_WIDTH, COVER_HEIGHT);

    float z_depth = GetRandomCloudZ();
    float scale_val = 0.25f + (z_depth / 100.0f) * 0.35f;

    g->z = z_depth;

    Vector scale = {scale_val, scale_val, 1.0f, 0.0f};
    TSU_DrawableSetScale((Drawable *)g->visual, &scale);

    int obj_size = (int)(COVER_WIDTH * scale_val);
    float target_y = (rand() % (SCREEN_HEIGHT - obj_size)) + obj_size * 0.75f;

    float start_x, start_y;

    if (fly_in_dir == 2 || fly_in_dir == -2) {
        start_x = (rand() % SCREEN_WIDTH) - 50.0f;

        if (fly_in_dir > 0) start_y = SCREEN_HEIGHT + 100.0f;
        else start_y = -COVER_HEIGHT - 100.0f;
    }
    else {
        start_y = target_y;

        if (fly_in_dir != 0) {
            if (fly_in_dir > 0) start_x = SCREEN_WIDTH + 100.0f;
            else start_x = -COVER_WIDTH - 100.0f;
        } else {
            start_x = (rand() % SCREEN_WIDTH) - 50.0f;
        }
    }

    g->x = start_x;
    g->y = start_y;

    Vector pos = {g->x + obj_size / 2.0f, g->y, g->z, 1.0f};
    TSU_DrawableTranslate((Drawable *)g->visual, &pos);

    if (self.font) {
        char label_text[sizeof(g->name)];

        FormatGameLabelText(g, label_text, sizeof(label_text));

        g->label = TSU_LabelCreate(self.font, label_text, self.config.label_size, false, false, false);
        static Color white = {1.0f, 1.0f, 1.0f, 1.0f};
        TSU_LabelSetTint(g->label, &white);

        float w, h;
        TSU_LabelGetSize(g->label, &w, &h);

        float off_x = (obj_size - w) / 2.0f;
        float off_y = (float)obj_size / 2.0f + h + 5.0f;

        Vector l_abs_pos = {g->x + off_x, g->y + off_y, g->z + 0.5f, 1.0f};
        TSU_DrawableSetTranslate((Drawable *)g->label, &l_abs_pos);
    }
}

static int FindGameFile(GameEntry *g, char *full_path, size_t size) {
    if (!g->game_file[0]) return 0;

    snprintf(full_path, size, "%s/%s", g->path, g->game_file);

    int len = strlen(g->game_file);
    if (len <= 4) return 0;

    const char *ext = g->game_file + len - 4;

    if (!strcasecmp(ext, ".dni")) {
        if (IsNaomiRom(full_path)) {
            naomi_cart_header_t hdr;
            file_t f = fs_open(full_path, O_RDONLY);
            if (f != FILEHND_INVALID) {
                fs_read(f, &hdr, sizeof(hdr));
                fs_close(f);
                kos_md5((uint8 *)&hdr, sizeof(hdr), g->md5);
                return 1;
            }
        }
    }
    else {
        GetMD5(full_path, g->md5);
        return 1;
    }
    return 0;
}

void RunGame(int index, int test_mode) {
    GameEntry *g = &self.games[index];
    char game_path[NAME_MAX];

    if (!FindGameFile(g, game_path, sizeof(game_path))) {
        ds_printf("Arcade: Failed to find game file for %s\n", g->path);
        return;
    }

    isoldr_info_t *isoldr = isoldr_get_info(game_path, test_mode);

    if (!isoldr) {
        ds_printf("Arcade: Failed to get isoldr info for %s\n", game_path);
        return;
    }

    char *preset_file = isoldr_find_preset(game_path, g->md5, 0);
    uintptr_t exec_addr = isoldr_apply_preset(isoldr, preset_file);

    if (exec_addr == (uintptr_t)-1) {
        free(isoldr);
        return;
    }

    isoldr_exec(isoldr, exec_addr);
    free(isoldr);
}
