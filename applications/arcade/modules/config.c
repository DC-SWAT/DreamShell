/* DreamShell
   
   config.c - Arcade app config
   Copyright (C) 2026 SWAT
   
*/

#include <ds.h>
#include <lua.h>
#include "app_internal.h"

void ArcadeConfigSetDefaults(ArcadeConfig_t *cfg) {
    memset(cfg, 0, sizeof(*cfg));

    cfg->games_paths_count = 3;
    strncpy(cfg->games_paths[0], "/ide/games", NAME_MAX - 1);
    strncpy(cfg->games_paths[1], "/sd/games", NAME_MAX - 1);
    strncpy(cfg->games_paths[2], "/cd/games", NAME_MAX - 1);

    cfg->scan_mode = ARCADE_SCAN_FALLBACK;
    cfg->games_per_page = GAMES_PER_PAGE;
    strncpy(cfg->default_cover, "images/cover.png", NAME_MAX - 1);

    cfg->cover_exts_count = 2;
    strncpy(cfg->cover_exts[0], "png", sizeof(cfg->cover_exts[0]) - 1);
    strncpy(cfg->cover_exts[1], "jpg", sizeof(cfg->cover_exts[1]) - 1);

    strncpy(cfg->trailer_filename, "trailer.avi", NAME_MAX - 1);
    cfg->media_volume = -1;

    cfg->idle_time_ms = IDLE_TIME_MS;
    cfg->focus_time_ms = FOCUS_TIME_MS;
    cfg->pre_video_time_ms = PRE_VIDEO_TIME_MS;
    cfg->no_trailer_wait_ms = NO_TRAILER_WAIT_MS;
    cfg->video_play_time_ms = VIDEO_PLAY_TIME_MS;
    cfg->unfocus_time_ms = UNFOCUS_TIME_MS;
    cfg->fade_to_video_ms = (int)FADE_TO_VIDEO_MS;
    cfg->fade_to_cover_ms = (int)FADE_TO_COVER_MS;
    cfg->video_fade_in_ms = VIDEO_FADE_IN_MS;
    cfg->media_fade_out_ms = MEDIA_FADE_OUT_MS;
    cfg->audio_start_grace_ms = AUDIO_START_GRACE_MS;
    cfg->page_switch_wait_ms = PAGE_SWITCH_WAIT_MS;
    cfg->fade_overlay_ms = (int)FADE_OVERLAY_MS;
    cfg->video_border_show_progress = VIDEO_BORDER_SHOW_PROGRESS;
    cfg->video_border_size = VIDEO_BORDER_SIZE;
    cfg->fog_z_range = FOG_Z_RANGE;
    cfg->label_size = LABEL_SIZE;
    cfg->focus_label_size = FOCUS_LABEL_SIZE;
}

void ArcadeConfigResolvePath(const char *app_path, const char *path, char *result, size_t size) {
    if (!path || !path[0]) {
        result[0] = '\0';
        return;
    }

    if (path[0] == '/') {
        strncpy(result, path, size - 1);
        result[size - 1] = '\0';
    }
    else {
        snprintf(result, size, "%s/%s", app_path, path);
    }
}

static void ReadCoverExtensions(const LuaConfig_t *lcfg, ArcadeConfig_t *cfg) {
    int n = LuaConfigGetStringArray(lcfg, "cover_extensions",
        (char *)cfg->cover_exts, sizeof(cfg->cover_exts[0]), ARCADE_MAX_COVER_EXTS);
    if (n > 0) {
        cfg->cover_exts_count = n;
    }
}

static void ReadScanMode(const LuaConfig_t *lcfg, ArcadeConfig_t *cfg) {
    char mode[32];

    mode[0] = '\0';
    if (LuaConfigGetString(lcfg, "scan_mode", mode, sizeof(mode)) && !strcasecmp(mode, "all")) {
        cfg->scan_mode = ARCADE_SCAN_ALL;
    }
    else {
        cfg->scan_mode = ARCADE_SCAN_FALLBACK;
    }
}

int LoadArcadeConfig(const char *app_path, ArcadeConfig_t *cfg) {
    char config_path[NAME_MAX];

    ArcadeConfigSetDefaults(cfg);
    snprintf(config_path, sizeof(config_path), "%s/config.lua", app_path);

    LuaConfig_t lcfg;

    if (LuaOpenConfigFile(config_path, &lcfg) != 0) {
        ds_printf("Arcade: Failed to load %s, using default config\n", config_path);
        return -1;
    }

    cfg->games_paths_count = LuaConfigGetStringArray(&lcfg, "games_paths",
        (char *)cfg->games_paths, NAME_MAX, ARCADE_MAX_GAME_PATHS);
    ReadScanMode(&lcfg, cfg);
    ReadCoverExtensions(&lcfg, cfg);

    cfg->games_per_page = LuaConfigGetInt(&lcfg, "games_per_page", cfg->games_per_page);
    cfg->media_volume = LuaConfigGetInt(&lcfg, "media_volume", cfg->media_volume);

    cfg->idle_time_ms = LuaConfigGetInt(&lcfg, "idle_time_ms", cfg->idle_time_ms);
    cfg->focus_time_ms = LuaConfigGetInt(&lcfg, "focus_time_ms", cfg->focus_time_ms);
    cfg->pre_video_time_ms = LuaConfigGetInt(&lcfg, "pre_video_time_ms", cfg->pre_video_time_ms);
    cfg->no_trailer_wait_ms = LuaConfigGetInt(&lcfg, "no_trailer_wait_ms", cfg->no_trailer_wait_ms);
    cfg->video_play_time_ms = LuaConfigGetInt(&lcfg, "video_play_time_ms", cfg->video_play_time_ms);
    cfg->unfocus_time_ms = LuaConfigGetInt(&lcfg, "unfocus_time_ms", cfg->unfocus_time_ms);
    cfg->fade_to_video_ms = LuaConfigGetInt(&lcfg, "fade_to_video_ms", cfg->fade_to_video_ms);
    cfg->fade_to_cover_ms = LuaConfigGetInt(&lcfg, "fade_to_cover_ms", cfg->fade_to_cover_ms);
    cfg->video_fade_in_ms = LuaConfigGetInt(&lcfg, "video_fade_in_ms", cfg->video_fade_in_ms);
    cfg->media_fade_out_ms = LuaConfigGetInt(&lcfg, "media_fade_out_ms", cfg->media_fade_out_ms);
    cfg->audio_start_grace_ms = LuaConfigGetInt(&lcfg, "audio_start_grace_ms", cfg->audio_start_grace_ms);
    cfg->page_switch_wait_ms = LuaConfigGetInt(&lcfg, "page_switch_wait_ms", cfg->page_switch_wait_ms);
    cfg->fade_overlay_ms = LuaConfigGetInt(&lcfg, "fade_overlay_ms", cfg->fade_overlay_ms);
    cfg->video_border_show_progress = LuaConfigGetFloat(&lcfg, "video_border_show_progress", cfg->video_border_show_progress);
    cfg->video_border_size = LuaConfigGetInt(&lcfg, "video_border_size", cfg->video_border_size);
    cfg->fog_z_range = LuaConfigGetFloat(&lcfg, "fog_z_range", cfg->fog_z_range);
    cfg->label_size = LuaConfigGetInt(&lcfg, "label_size", cfg->label_size);
    cfg->focus_label_size = LuaConfigGetInt(&lcfg, "focus_label_size", cfg->focus_label_size);

    LuaConfigGetString(&lcfg, "default_cover", cfg->default_cover, sizeof(cfg->default_cover));
    LuaConfigGetString(&lcfg, "trailer_filename", cfg->trailer_filename, sizeof(cfg->trailer_filename));

    if (cfg->games_per_page < 1) {
        cfg->games_per_page = GAMES_PER_PAGE;
    }
    if (cfg->video_border_size < 0) {
        cfg->video_border_size = VIDEO_BORDER_SIZE;
    }
    if (cfg->fog_z_range <= 0.0f) {
        cfg->fog_z_range = FOG_Z_RANGE;
    }
    if (cfg->label_size < 1) {
        cfg->label_size = LABEL_SIZE;
    }
    if (cfg->focus_label_size < 1) {
        cfg->focus_label_size = FOCUS_LABEL_SIZE;
    }
    if (cfg->games_paths_count <= 0) {
        cfg->games_paths_count = 2;
        strncpy(cfg->games_paths[0], "/ide/games", NAME_MAX - 1);
        strncpy(cfg->games_paths[1], "/sd/games", NAME_MAX - 1);
        // strncpy(cfg->games_paths[2], "/cd/games", NAME_MAX - 1);
    }

    LuaCloseConfigFile(&lcfg);
    return 0;
}
