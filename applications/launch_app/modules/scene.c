/* DreamShell

   scene.c - Launch App scene rendering and toolbar
   Copyright (C) 2026 SWAT

*/

#include <ds.h>
#include <tsunami/tsunami.h>
#include <sh4zam/shz_trig.h>
#include <math.h>

#include "app_internal.h"

static void GetItemSwayAmps(int index, float *amp_y, float *amp_x, float *rot_deg);
static void ShowVersion(void);
static void ShowNetStatus(int force);
static void ShowDateTime(int force);
static void HideFocusGlow(void);
static void UpdateFocusEffects(float t);

void LaunchAppDrawTransparentPolyEvent(void) {
    if(self.mouse_visible) {
        SDL_DS_Blit_Cursor();
    }
}

float ItemScrollX(const launch_item_t *item) {
    return item->base_x - self.scroll_x_display;
}

static void CalcItemLayoutPos(const launch_item_t *item, float scroll_x, float sway_x, float sway_y,
    float *icon_cx, float *icon_cy, float *label_x, float *label_y) {

    float label_w = 0.0f;
    float label_h = 0.0f;
    float icon_block_h;

    if(item->label != NULL) {
        TSU_LabelGetSize(item->label, &label_w, &label_h);
    }

    icon_block_h = item->icon_h + (float)self.config.icon_highlight_padding;

    *icon_cx = scroll_x + item->cell_w * 0.5f + sway_x;
    *icon_cy = item->base_y + icon_block_h * 0.5f + sway_y;
    *label_x = scroll_x + item->cell_w * 0.5f;

    if(item->label != NULL) {
        *label_y = item->base_y + icon_block_h + self.config.icon_label_y_offset + label_h * 0.5f + sway_y * 0.5f;
    }
    else {
        *label_y = *icon_cy;
    }
}

static int IsItemLabelOnScreen(float label_x, float label_y, float label_w, float label_h) {
    float left;
    float right;
    float top;
    float bottom;

    if(label_w <= 0.0f || label_h <= 0.0f) {
        return 0;
    }

    left = label_x - label_w * 0.5f;
    right = label_x + label_w * 0.5f;
    top = label_y - label_h * 0.5f;
    bottom = label_y + label_h * 0.5f;

    if(right <= (float)self.grid_x || left >= (float)(self.grid_x + self.panel_w)) {
        return 0;
    }

    if(bottom <= (float)self.grid_y || top >= (float)self.toolbar_top) {
        return 0;
    }

    return 1;
}

static void UpdateItemLabelTransform(launch_item_t *item, float label_x, float label_y, float z, float alpha) {
    Vector pos;
    Vector scale;

    if(item->label == NULL) {
        return;
    }

    pos.x = label_x;
    pos.y = label_y;
    pos.z = z;
    pos.w = 1.0f;
    scale.x = 1.0f;
    scale.y = 1.0f;
    scale.z = 1.0f;
    scale.w = 0.0f;
    TSU_DrawableSetTranslate((Drawable *)item->label, &pos);
    TSU_DrawableSetScale((Drawable *)item->label, &scale);
    TSU_DrawableSetAlpha((Drawable *)item->label, alpha);
}

static float GetItemSwayEase(const launch_item_t *item, float t) {
    float elapsed_ms = t * 1000.0f - item->sway_start_t;

    if(elapsed_ms <= 0.0f) {
        return 0.0f;
    }

    if(elapsed_ms >= SWAY_EASE_IN_MS) {
        return 1.0f;
    }

    return elapsed_ms / SWAY_EASE_IN_MS;
}

static void CalcItemSway(const launch_item_t *item, int index, float t, float *sway_x, float *sway_y, float *rot_deg) {
    float amp_y;
    float amp_x;
    float rot;
    float speed;
    float ease;

    GetItemSwayAmps(index, &amp_y, &amp_x, &rot);
    ease = GetItemSwayEase(item, t);

    if(index == self.focused_index) {
        speed = self.config.focus_sway_speed;
    }
    else {
        speed = item->sway_speed;
    }

    *sway_y = shz_sinf(t * speed + item->sway_phase) * amp_y * ease;
    *sway_x = shz_sinf(t * speed * 0.85f + item->sway_phase + 0.6f) * amp_x * ease;

    if(index == self.focused_index) {
        *rot_deg = shz_sinf(t * speed * self.config.focus_rot_freq_mul + item->sway_phase + 1.1f) * self.config.focus_rot_deg * ease;
    }
    else {
        *rot_deg = shz_sinf(t * speed * 0.6f + item->sway_phase) * rot * ease;
    }
}

void UpdateFocusLabels(void) {
    static Color normal_color = {1.0f, 1.0f, 1.0f, 1.0f};
    Color focus_color;
    int i;

    focus_color = self.config.focus_label_color;
    focus_color.a = 1.0f;

    for(i = 0; i < self.item_count; i++) {
        launch_item_t *item = &self.items[i];

        if(item->label == NULL) {
            continue;
        }

        if(i == self.focused_index && ItemVisibleOnPage(i)) {
            TSU_LabelSetTint(item->label, &focus_color);
        }
        else {
            TSU_LabelSetTint(item->label, &normal_color);
        }
    }
}

static void UpdateToolbarLayout(void) {
    float w;
    float h;
    Vector pos;
    static Color label_color = {1.0f, 1.0f, 1.0f, 1.0f};
    const float right_pad = 12.0f;
    const float icon_size = 16.0f;
    const float icon_gap = 8.0f;
    const float toolbar_center_y = (float)self.toolbar_top + (float)self.toolbar_height * 0.5f;
    const float toolbar_label_y = toolbar_center_y - 4.0f;

    if(self.date_label) {
        TSU_LabelSetCenter(self.date_label, false);
        TSU_LabelGetSize(self.date_label, &w, &h);
        TSU_LabelSetTint(self.date_label, &label_color);
        TSU_DrawableSetAlpha((Drawable *)self.date_label, 1.0f);
        pos.x = right_pad;
        pos.y = toolbar_label_y + h * 0.5f;
        pos.z = self.toolbar_z;
        pos.w = 1.0f;
        TSU_DrawableSetTranslate((Drawable *)self.date_label, &pos);
    }

    if(self.version_label) {
        TSU_LabelSetCenter(self.version_label, true);
        TSU_LabelGetSize(self.version_label, &w, &h);
        TSU_LabelSetTint(self.version_label, &label_color);
        TSU_DrawableSetAlpha((Drawable *)self.version_label, 1.0f);
        pos.x = (float)self.screen_w * 0.5f;
        pos.y = toolbar_label_y;
        pos.z = self.toolbar_z;
        pos.w = 1.0f;
        TSU_DrawableSetTranslate((Drawable *)self.version_label, &pos);
    }

    if(self.time_label) {
        TSU_LabelSetCenter(self.time_label, false);
        TSU_LabelGetSize(self.time_label, &w, &h);
        TSU_LabelSetTint(self.time_label, &label_color);
        TSU_DrawableSetAlpha((Drawable *)self.time_label, 1.0f);
        pos.x = (float)self.screen_w - right_pad - icon_size - icon_gap - w;
        pos.y = toolbar_label_y + h * 0.5f;
        pos.z = self.toolbar_z;
        pos.w = 1.0f;
        TSU_DrawableSetTranslate((Drawable *)self.time_label, &pos);
    }

    if(self.net_banner) {
        TSU_DrawableSetAlpha((Drawable *)self.net_banner, 1.0f);
        pos.x = (float)self.screen_w - right_pad - icon_size * 0.5f;
        pos.y = toolbar_center_y;
        pos.z = self.toolbar_z + 0.5f;
        pos.w = 1.0f;
        TSU_DrawableSetTranslate((Drawable *)self.net_banner, &pos);
    }
}

void RefreshToolbar(void) {
    if(self.app == NULL) {
        return;
    }

    ShowVersion();
    ShowDateTime(1);
    ShowNetStatus(1);
    UpdateToolbarLayout();
}

static void ShowVersion(void) {
    char vers[64];
    const char *os;
    const char *ver;

    if(self.version_label == NULL) {
        return;
    }

    os = getenv("OS");
    ver = getenv("VERSION");

    if(os == NULL) {
        os = "DreamShell";
    }

    if(ver == NULL || !ver[0]) {
        ver = getenv("VERSION_SHORT");
    }

    if(ver == NULL || !ver[0]) {
        ver = "";
    }

    snprintf(vers, sizeof(vers), "%s %s", os, ver);
    TSU_LabelSetText(self.version_label, vers);
}

static void ShowNetStatus(int force) {
    int old_status = self.net_status;
    const char *ipv4 = getenv("NET_IPV4");

    self.net_status = (ipv4 != NULL) ? strncmp(ipv4, "0.0.0.0", 7) : 0;

    if(self.net_banner && (force || self.net_status != old_status)) {
        if(self.net_status && self.net_on != NULL) {
            TSU_BannerSetTexture(self.net_banner, self.net_on);
        }
        else if(self.net_off != NULL) {
            TSU_BannerSetTexture(self.net_banner, self.net_off);
        }
    }
}

static void ShowDateTime(int force) {
    char str[32];
    time_t unix_time;
    struct tm *datetime;

    unix_time = rtc_unix_secs();
    datetime = localtime(&unix_time);

    if(datetime == NULL) {
        return;
    }

    if(self.date_label && (force || datetime->tm_mday != self.datetime.tm_mday)) {
        switch(flashrom_get_region_only()) {
            case FLASHROM_REGION_JAPAN:
                snprintf(str, sizeof(str), "%04d-%02d-%02d", datetime->tm_year + 1900, datetime->tm_mon + 1, datetime->tm_mday);
                break;
            case FLASHROM_REGION_US:
                snprintf(str, sizeof(str), "%02d/%02d/%04d", datetime->tm_mon + 1, datetime->tm_mday, datetime->tm_year + 1900);
                break;
            case FLASHROM_REGION_EUROPE:
            default:
                snprintf(str, sizeof(str), "%02d.%02d.%04d", datetime->tm_mday, datetime->tm_mon + 1, datetime->tm_year + 1900);
                break;
        }

        TSU_LabelSetText(self.date_label, str);
    }

    if(self.time_label && (force || datetime->tm_min != self.datetime.tm_min)) {
        snprintf(str, sizeof(str), "%02d:%02d", datetime->tm_hour, datetime->tm_min);
        TSU_LabelSetText(self.time_label, str);
    }

    memcpy(&self.datetime, datetime, sizeof(*datetime));
}

void *LaunchAppClockThread(void *arg) {
    (void)arg;

    while(self.app->state & APP_STATE_OPENED) {
        ShowDateTime(0);
        ShowNetStatus(0);
        thd_sleep(250);
    }

    return NULL;
}

static void GetItemSwayAmps(int index, float *amp_y, float *amp_x, float *rot_deg) {
    if(index == self.focused_index && self.focused_index >= 0) {
        *amp_y = self.config.focus_sway_amp_y;
        *amp_x = self.config.focus_sway_amp_x;
        *rot_deg = 0.0f;
    }
    else {
        *amp_y = self.config.sway_amp_y;
        *amp_x = self.config.sway_amp_x;
        *rot_deg = self.config.sway_rot_deg;
    }
}

void UpdateItemTransforms(uint64_t now) {
    float t = (float)now / 1000.0f;
    int i;
    Vector pos;
    Vector rot;
    float scroll_x;
    float sway_y;
    float sway_x;
    float rot_deg;

    for(i = 0; i < self.item_count; i++) {
        launch_item_t *item = &self.items[i];
        float cx;
        float cy;
        float label_x;
        float label_y;
        float label_w = 0.0f;
        float label_h = 0.0f;

        if(item->banner == NULL) {
            continue;
        }

        scroll_x = ItemScrollX(item);

        if(scroll_x + item->cell_w + (float)self.config.icon_highlight_padding < -32.0f ||
            scroll_x > (float)self.panel_w + 32.0f) {
            TSU_DrawableSetAlpha((Drawable *)item->banner, 0.0f);

            if(item->label != NULL) {
                CalcItemLayoutPos(item, scroll_x, 0.0f, 0.0f, &cx, &cy, &label_x, &label_y);
                UpdateItemLabelTransform(item, label_x, label_y, item->z + 0.2f, 0.0f);
            }
            continue;
        }

        TSU_DrawableSetAlpha((Drawable *)item->banner, 1.0f);

        CalcItemSway(item, i, t, &sway_x, &sway_y, &rot_deg);

        CalcItemLayoutPos(item, scroll_x, sway_x, sway_y, &cx, &cy, &label_x, &label_y);

        pos.x = cx;
        pos.y = cy;
        pos.z = item->z;
        pos.w = 1.0f;
        TSU_DrawableSetTranslate((Drawable *)item->banner, &pos);

        rot.x = 0.0f;
        rot.y = 0.0f;
        rot.z = 1.0f;
        rot.w = rot_deg;
        TSU_DrawableSetRotate((Drawable *)item->banner, &rot);

        if(item->label != NULL) {
            float alpha;

            TSU_LabelGetSize(item->label, &label_w, &label_h);
            alpha = IsItemLabelOnScreen(label_x, label_y, label_w, label_h) ? 1.0f : 0.0f;
            UpdateItemLabelTransform(item, label_x, label_y, item->z + 0.2f, alpha);
        }
    }

    UpdateFocusEffects(t);
    UpdateFocusLabels();
}

static void HideFocusGlow(void) {
    if(self.focus_glow == NULL) {
        return;
    }

    TSU_DrawableSetAlpha((Drawable *)self.focus_glow, 0.0f);
}

static void UpdateFocusEffects(float t) {
    launch_item_t *item;
    float scroll_x;
    float sway_x;
    float sway_y;
    float rot_unused;
    float cx;
    float cy;
    float label_x;
    float label_y;
    float icon_block_w;
    float icon_block_h;
    float pulse;
    float glow_alpha;
    float glow_radius;
    Vector pos;
    Color glow_center;
    Color glow_edge;

    if(self.focus_glow == NULL) {
        return;
    }

    if(self.focused_index < 0 ||
        !ItemVisibleOnPage(self.focused_index) ||
        (self.delete_dialog != NULL && TSU_DialogIsVisible(self.delete_dialog))) {

        HideFocusGlow();
        return;
    }

    item = &self.items[self.focused_index];

    if(item->banner == NULL) {
        HideFocusGlow();
        return;
    }

    scroll_x = ItemScrollX(item);

    if(scroll_x + item->cell_w + (float)self.config.icon_highlight_padding < 0.0f ||
        scroll_x > (float)self.panel_w) {

        HideFocusGlow();
        return;
    }

    CalcItemSway(item, self.focused_index, t, &sway_x, &sway_y, &rot_unused);
    CalcItemLayoutPos(item, scroll_x, sway_x, sway_y, &cx, &cy, &label_x, &label_y);

    icon_block_w = item->icon_w + (float)self.config.icon_highlight_padding;
    icon_block_h = item->icon_h + (float)self.config.icon_highlight_padding;

    if(self.config.focus_rot_deg > 0.0f) {
        float rot_pulse = shz_sinf(t * self.config.focus_sway_speed * self.config.focus_rot_freq_mul
            + item->sway_phase + 1.1f);

        pulse = fabsf(rot_pulse);
    }
    else {
        pulse = 0.0f;
    }

    glow_alpha = self.config.glow_alpha_min + pulse * (self.config.glow_alpha_max - self.config.glow_alpha_min);
    glow_radius = (icon_block_w > icon_block_h ? icon_block_w : icon_block_h) * self.config.glow_radius_mul
        + pulse * self.config.glow_radius_pulse;

    glow_center = self.glow_color;
    glow_center.a = glow_alpha * 0.35f;
    glow_edge = self.glow_color;
    glow_edge.a = glow_alpha * 0.65f;

    TSU_CircleSetRadius(self.focus_glow, glow_radius);
    TSU_CircleSetColors(self.focus_glow, &glow_center, &glow_edge);
    TSU_DrawableSetAlpha((Drawable *)self.focus_glow, 1.0f);
    pos.x = cx;
    pos.y = cy;
    pos.z = item->z - 0.6f;
    pos.w = 1.0f;
    TSU_DrawableSetTranslate((Drawable *)self.focus_glow, &pos);
}

static void LoadFogCircles(void) {
    Item_t *list_item;

    self.fog_count = 0;

    if(!self.app || !self.app->elements) {
        return;
    }

    list_item = listGetItemFirst(self.app->elements);

    while(list_item && self.fog_count < FOG_MAX) {
        if(list_item->type == LIST_ITEM_TSU_DRAWABLE && list_item->name &&
            !strncmp(list_item->name, "fog_circle_", 11)) {
            self.fog_circles[self.fog_count] = (Circle *)list_item->data;
            self.fog_count++;
        }
        list_item = listGetItemNext(list_item);
    }
}

void FreeFogCircles(void) {
    int i;

    for(i = 0; i < self.fog_count; i++) {
        if(self.fog_circles[i]) {
            TSU_DrawableAnimRemoveAll((Drawable *)self.fog_circles[i]);
        }

        if(self.fog_movers[i]) {
            TSU_LogXYZMoverDestroy(&self.fog_movers[i]);
            self.fog_movers[i] = NULL;
        }
    }

    self.fog_count = 0;
}

void LoadLayoutFromDrawables(void) {
    Rectangle *toolbar;
    Rectangle *icon_grid;
    Label *date_label;
    const Vector *pos;
    float w;
    float h;

    self.screen_w = GetScreenWidth();
    self.screen_h = GetScreenHeight();
    self.panel_w = self.screen_w;
    self.grid_x = 0;
    self.grid_y = 0;
    self.grid_w = self.screen_w;
    self.toolbar_top = self.screen_h - 32;
    self.toolbar_height = 32;
    self.toolbar_z = 121.0f;
    self.panel_h = self.toolbar_top;

    toolbar = (Rectangle *)APP_GET_TSU_DRAWABLE("toolbar_bg");

    if(toolbar != NULL) {
        TSU_DrawableGetSize((Drawable *)toolbar, &w, &h);
        pos = TSU_DrawableGetTranslate((Drawable *)toolbar);

        if(pos != NULL) {
            self.toolbar_height = (int)h;

            if(self.toolbar_height < 1) {
                self.toolbar_height = 32;
            }

            self.toolbar_top = (int)(pos->y - h);

            if(self.toolbar_top < 0) {
                self.toolbar_top = 0;
            }

            if(w >= 1.0f) {
                self.screen_w = (int)w;
                self.panel_w = self.screen_w;
            }
        }
    }

    icon_grid = (Rectangle *)APP_GET_TSU_DRAWABLE("icon_grid");

    if(icon_grid != NULL) {
        TSU_DrawableGetSize((Drawable *)icon_grid, &w, &h);
        pos = TSU_DrawableGetTranslate((Drawable *)icon_grid);

        if(pos != NULL) {
            self.grid_x = (int)pos->x;
            self.grid_y = (int)(pos->y - h);
            self.grid_w = (int)w;

            if(self.grid_w < 1) {
                self.grid_w = self.screen_w;
            }
        }
    }

    self.panel_h = self.toolbar_top - self.grid_y;

    if(self.panel_h < 1) {
        self.panel_h = self.toolbar_top;
    }

    date_label = (Label *)APP_GET_TSU_DRAWABLE("date");

    if(date_label != NULL) {
        pos = TSU_DrawableGetTranslate((Drawable *)date_label);

        if(pos != NULL) {
            self.toolbar_z = pos->z;
        }
    }
}

void CreateBackground(void) {
    int i;

    LoadFogCircles();
    srand((unsigned int)timer_ms_gettime64());

    for(i = 0; i < self.fog_count; i++) {
        if(self.fog_circles[i]) {
            const Vector *cur_pos = TSU_DrawableGetTranslate((Drawable *)self.fog_circles[i]);
            float start_x;
            float start_y;
            float start_z;
            float target_x;
            float target_y;
            float target_z;
            Vector pos;

            if(cur_pos) {
                self.fog_depths[i] = cur_pos->z;
            }

            start_x = (float)(rand() % self.screen_w) - 256.0f;
            start_y = (float)(rand() % self.panel_h) - 256.0f;
            start_z = self.fog_depths[i];

            pos.x = start_x;
            pos.y = start_y;
            pos.z = start_z;
            pos.w = 1.0f;
            TSU_DrawableSetTranslate((Drawable *)self.fog_circles[i], &pos);

            target_x = (float)(rand() % self.screen_w) - 100.0f;
            target_y = (float)(rand() % self.panel_h) - 100.0f;
            target_z = self.fog_depths[i];

            self.fog_movers[i] = TSU_LogXYZMoverCreate(target_x, target_y, target_z);
            TSU_LogXYZMoverSetFactor(self.fog_movers[i], 150.0f + (float)(rand() % 50));
            TSU_DrawableAnimAdd((Drawable *)self.fog_circles[i], (Animation *)self.fog_movers[i]);
        }
    }
}

void CreateFocusEffects(void) {
    Color center;

    if(self.focus_glow == NULL) {
        return;
    }

    TSU_CircleGetColors(self.focus_glow, &center, NULL);
    self.glow_color = center;
    self.glow_color.a = 1.0f;
    TSU_DrawableSetAlpha((Drawable *)self.focus_glow, 0.0f);
}

void DestroyFocusEffects(void) {
    HideFocusGlow();
    self.focus_glow = NULL;
}

void UpdateFog(uint64_t now) {
    static uint64_t last_cloud_update = 0;
    int i;

    (void)now;

    if(now - last_cloud_update <= 1000) {
        return;
    }

    last_cloud_update = now;

    for(i = 0; i < self.fog_count; i++) {
        if(self.fog_circles[i] && self.fog_movers[i]) {
            if(rand() % 10 < 1) {
                float tx;
                float ty;

                TSU_AnimationComplete((Animation *)self.fog_movers[i], (Drawable *)self.fog_circles[i]);

                tx = (float)(rand() % (self.screen_w + 128)) - 64.0f;
                ty = (float)(rand() % (self.panel_h + 128)) - 64.0f;

                TSU_LogXYZMoverSetTarget(self.fog_movers[i], tx, ty, self.fog_depths[i]);
                TSU_LogXYZMoverSetFactor(self.fog_movers[i], 150.0f + (float)(rand() % 50));
                TSU_DrawableAnimAdd((Drawable *)self.fog_circles[i], (Animation *)self.fog_movers[i]);
            }
        }
    }
}

int ItemVisibleOnPage(int index) {
    launch_item_t *item;
    float scroll_x;

    if(index < 0 || index >= self.item_count) {
        return 0;
    }

    item = &self.items[index];

    if(item->banner == NULL || TSU_DrawableGetAlpha((Drawable *)item->banner) <= 0.0f) {
        return 0;
    }

    scroll_x = ItemScrollX(item);

    if(scroll_x + item->cell_w + (float)self.config.icon_highlight_padding < 0.0f ||
        scroll_x > (float)self.panel_w) {
        return 0;
    }

    return 1;
}

static int PointInItemCell(int mx, int my, launch_item_t *item, float scroll_x) {
    float left;
    float top;
    float right;
    float bottom;

    left = scroll_x;
    right = scroll_x + item->cell_w;
    top = item->base_y;
    bottom = item->base_y + item->cell_h;

    if((float)mx < left || (float)mx >= right) {
        return 0;
    }

    if((float)my < top || (float)my >= bottom) {
        return 0;
    }

    return 1;
}

int HitTestItem(int mx, int my) {
    int i;

    for(i = self.item_count - 1; i >= 0; i--) {
        launch_item_t *item = &self.items[i];
        float scroll_x;

        if(!ItemVisibleOnPage(i)) {
            continue;
        }

        scroll_x = ItemScrollX(item);

        if(PointInItemCell(mx, my, item, scroll_x)) {
            return i;
        }
    }

    return -1;
}
