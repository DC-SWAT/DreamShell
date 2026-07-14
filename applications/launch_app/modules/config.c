/* DreamShell

   config.c - Launch App config
   Copyright (C) 2026 SWAT

*/

#include <ds.h>
#include <lua.h>
#include "app_internal.h"

static void SetConfigColor(Color *dst, const char *hex) {
    unsigned int r = 255;
    unsigned int g = 190;
    unsigned int b = 11;
    unsigned int a = 255;
    int cnt;

    if(hex == NULL || *hex != '#') {
        dst->r = (float)r / 255.0f;
        dst->g = (float)g / 255.0f;
        dst->b = (float)b / 255.0f;
        dst->a = 1.0f;
        return;
    }

    cnt = sscanf(hex, "#%02x%02x%02x%02x", &r, &g, &b, &a);

    if(cnt >= 3) {
        dst->r = (float)r / 255.0f;
        dst->g = (float)g / 255.0f;
        dst->b = (float)b / 255.0f;
        dst->a = cnt == 4 ? (float)a / 255.0f : 1.0f;
    }
}

static void LoadConfigColor(const LuaConfig_t *lcfg, const char *key, Color *dst, const char *fallback) {
    char value[16];

    if(LuaConfigGetString(lcfg, key, value, sizeof(value))) {
        SetConfigColor(dst, value);
    }
    else {
        SetConfigColor(dst, fallback);
    }
}

void LaunchAppConfigSetDefaults(LaunchAppConfig_t *cfg) {
    memset(cfg, 0, sizeof(*cfg));

    cfg->grid_cols = GRID_COLS;
    cfg->grid_rows = GRID_ROWS;
    cfg->cell_pad_x = CELL_PAD_X;
    cfg->cell_pad_y = CELL_PAD_Y;
    cfg->grid_top = GRID_TOP;
    cfg->grid_bottom = GRID_BOTTOM;
    cfg->icon_label_size = ICON_LABEL_SIZE;
    cfg->icon_label_y_offset = ICON_LABEL_Y_OFFSET;
    cfg->icon_highlight_padding = ICON_HIGHLIGHT_PADDING;
    cfg->sway_amp_y = SWAY_AMP_Y;
    cfg->sway_amp_x = SWAY_AMP_X;
    cfg->sway_rot_deg = SWAY_ROT_DEG;
    cfg->focus_sway_amp_y = FOCUS_SWAY_AMP_Y;
    cfg->focus_sway_amp_x = FOCUS_SWAY_AMP_X;
    cfg->focus_sway_speed = FOCUS_SWAY_SPEED;
    cfg->focus_rot_deg = FOCUS_ROT_DEG;
    cfg->focus_rot_freq_mul = FOCUS_ROT_FREQ_MUL;
    cfg->page_slide_ms = PAGE_SLIDE_MS;
    cfg->col_match_eps = COL_MATCH_EPS;
    SetConfigColor(&cfg->focus_label_color, FOCUS_LABEL_COLOR);
    cfg->glow_alpha_min = GLOW_ALPHA_MIN;
    cfg->glow_alpha_max = GLOW_ALPHA_MAX;
    cfg->glow_radius_mul = GLOW_RADIUS_MUL;
    cfg->glow_radius_pulse = GLOW_RADIUS_PULSE;
}

int LoadLaunchAppConfig(const char *app_path, LaunchAppConfig_t *cfg) {
    char config_path[NAME_MAX];
    LuaConfig_t lcfg;

    LaunchAppConfigSetDefaults(cfg);
    snprintf(config_path, sizeof(config_path), "%s/config.lua", app_path);

    if(LuaOpenConfigFile(config_path, &lcfg) != 0) {
        ds_printf("Launch app: Failed to load %s, using default config\n", config_path);
        return -1;
    }

    cfg->grid_cols = LuaConfigGetInt(&lcfg, "grid_cols", cfg->grid_cols);
    cfg->grid_rows = LuaConfigGetInt(&lcfg, "grid_rows", cfg->grid_rows);
    cfg->cell_pad_x = LuaConfigGetInt(&lcfg, "cell_pad_x", cfg->cell_pad_x);
    cfg->cell_pad_y = LuaConfigGetInt(&lcfg, "cell_pad_y", cfg->cell_pad_y);
    cfg->grid_top = LuaConfigGetInt(&lcfg, "grid_top", cfg->grid_top);
    cfg->grid_bottom = LuaConfigGetInt(&lcfg, "grid_bottom", cfg->grid_bottom);
    cfg->icon_label_size = LuaConfigGetInt(&lcfg, "icon_label_size", cfg->icon_label_size);
    cfg->icon_highlight_padding = LuaConfigGetInt(&lcfg, "icon_highlight_padding", cfg->icon_highlight_padding);
    cfg->page_slide_ms = LuaConfigGetInt(&lcfg, "page_slide_ms", cfg->page_slide_ms);

    cfg->icon_label_y_offset = LuaConfigGetFloat(&lcfg, "icon_label_y_offset", cfg->icon_label_y_offset);
    cfg->sway_amp_y = LuaConfigGetFloat(&lcfg, "sway_amp_y", cfg->sway_amp_y);
    cfg->sway_amp_x = LuaConfigGetFloat(&lcfg, "sway_amp_x", cfg->sway_amp_x);
    cfg->sway_rot_deg = LuaConfigGetFloat(&lcfg, "sway_rot_deg", cfg->sway_rot_deg);
    cfg->focus_sway_amp_y = LuaConfigGetFloat(&lcfg, "focus_sway_amp_y", cfg->focus_sway_amp_y);
    cfg->focus_sway_amp_x = LuaConfigGetFloat(&lcfg, "focus_sway_amp_x", cfg->focus_sway_amp_x);
    cfg->focus_sway_speed = LuaConfigGetFloat(&lcfg, "focus_sway_speed", cfg->focus_sway_speed);
    cfg->focus_rot_deg = LuaConfigGetFloat(&lcfg, "focus_rot_deg", cfg->focus_rot_deg);
    cfg->focus_rot_freq_mul = LuaConfigGetFloat(&lcfg, "focus_rot_freq_mul", cfg->focus_rot_freq_mul);
    cfg->col_match_eps = LuaConfigGetFloat(&lcfg, "col_match_eps", cfg->col_match_eps);
    LoadConfigColor(&lcfg, "focus_label_color", &cfg->focus_label_color, FOCUS_LABEL_COLOR);
    cfg->glow_alpha_min = LuaConfigGetFloat(&lcfg, "glow_alpha_min", cfg->glow_alpha_min);
    cfg->glow_alpha_max = LuaConfigGetFloat(&lcfg, "glow_alpha_max", cfg->glow_alpha_max);
    cfg->glow_radius_mul = LuaConfigGetFloat(&lcfg, "glow_radius_mul", cfg->glow_radius_mul);
    cfg->glow_radius_pulse = LuaConfigGetFloat(&lcfg, "glow_radius_pulse", cfg->glow_radius_pulse);

    if(cfg->grid_cols < 1) {
        cfg->grid_cols = GRID_COLS;
    }

    if(cfg->grid_rows < 1) {
        cfg->grid_rows = GRID_ROWS;
    }

    if(cfg->page_slide_ms < 1) {
        cfg->page_slide_ms = PAGE_SLIDE_MS;
    }

    if(cfg->glow_radius_mul <= 0.0f) {
        cfg->glow_radius_mul = GLOW_RADIUS_MUL;
    }

    if(cfg->glow_radius_pulse < 0.0f) {
        cfg->glow_radius_pulse = GLOW_RADIUS_PULSE;
    }

    LuaCloseConfigFile(&lcfg);
    return 0;
}
