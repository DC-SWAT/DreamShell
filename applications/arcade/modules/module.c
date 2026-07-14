/* DreamShell
   
   module.c - Arcade app module
   Copyright (C) 2026 SWAT
   
*/

#include <ds.h>
#include <sfx.h>
#include <isoldr.h>
#include <ffmpeg.h>
#include <naomi/coins.h>
#include <dc/maple/mie.h>
#include <tsunami/tsunami.h>

#include "app_module.h"
#include "app_internal.h"

DEFAULT_MODULE_EXPORTS(app_arcade);

#define ANALOG_THRESHOLD 64
#define ARCADE_TEST_LAUNCH_EVENT (SDL_USEREVENT + 12)

AppArcadeContext_t self;

static void UnloadPage(int page);
static void ShowLoadedPage(void);
static void SwitchPage(int dir);
static void UnfocusGame(int index);
static void FocusGame(int index);
static void SwitchSelection(int dir);
static void LaunchGame(int index, bool test_mode);
static void OpenIsoLoader(int index);
static bool StartMediaFadeOutIfPlaying(void);
static void StartLoaderThread(void);
static void *LoaderThread(void *param);

static void ScheduleTestLaunch(int idx) {
    SDL_Event ev;

    if (idx < 0 || idx >= self.game_count || self.launching) {
        return;
    }

    memset(&ev, 0, sizeof(ev));
    ev.type = ARCADE_TEST_LAUNCH_EVENT;
    ev.user.code = idx;
    SDL_PushEvent(&ev);
}

static void StartLoaderThread(void) {
    if (self.loader_active) {
        return;
    }

    self.loader_active = true;
    self.loader_thread = thd_create(0, LoaderThread, NULL);
}

static void *LoaderThread(void *param) {
    int count = (self.game_count < self.config.games_per_page) ? self.game_count : self.config.games_per_page;

    while(self.page_load_idx < count && !self.exiting) {
         if (self.abort_loading) break;

         if (self.game_count > 0) {
             int idx = (self.current_page * self.config.games_per_page + self.page_load_idx) % self.game_count;
             LoadGameVisual(idx, self.page_switch_dir);
         }
         self.page_load_idx++;
    }
    
    self.loader_active = false;
    return NULL;
}

static void UpdateCloudLogic();
static void CreateBackground();

static void LoadFogCircles(void) {
    self.fog_count = 0;

    if (!self.app || !self.app->elements) {
        return;
    }

    Item_t *item = listGetItemFirst(self.app->elements);

    while (item && self.fog_count < FOG_MAX) {
        if (item->type == LIST_ITEM_TSU_DRAWABLE && item->name &&
            !strncmp(item->name, "fog_circle_", 11)) {
            self.fog_circles[self.fog_count] = (Circle *)item->data;
            self.fog_count++;
        }
        item = listGetItemNext(item);
    }
}

static void FreeFogCircles(void) {
    for (int i = 0; i < self.fog_count; i++) {
        if (self.fog_movers[i]) {
            TSU_LogXYZMoverDestroy(&self.fog_movers[i]);
            self.fog_movers[i] = NULL;
        }
    }

    self.fog_count = 0;
}

void UpdateVideoLayout(void) {
    if (!self.video_border) {
        self.video_x = VIDEO_X;
        self.video_y = VIDEO_Y;
        self.video_w = VIDEO_W;
        self.video_h = VIDEO_H;
        return;
    }

    const Vector *pos = TSU_DrawableGetTranslate((Drawable *)self.video_border);
    float border_w = 0.0f;
    float border_h = 0.0f;

    TSU_DrawableGetSize((Drawable *)self.video_border, &border_w, &border_h);

    const Vector *scale = TSU_DrawableGetScale((Drawable *)self.video_border);
    if (scale) {
        border_w *= scale->x;
        border_h *= scale->y;
    }

    int inset = self.config.video_border_size;
    int border_x = (int)pos->x;
    int border_y = (int)(pos->y - border_h);

    self.video_x = border_x + inset;
    self.video_y = border_y + inset;
    self.video_w = (int)border_w - inset * 2;
    self.video_h = (int)border_h - inset * 2;

    if (self.video_w < 1) {
        self.video_w = VIDEO_W;
    }
    if (self.video_h < 1) {
        self.video_h = VIDEO_H;
    }
}

static void UpdatePageLabel(int page) {
    if (self.page_label) {
        char page_text[32];
        snprintf(page_text, sizeof(page_text), "Page %d/%d", page + 1, self.total_pages);
        TSU_LabelSetText(self.page_label, page_text);

        float w, h;
        TSU_LabelGetSize(self.page_label, &w, &h);
        Vector pos = {SCREEN_WIDTH - w - 20.0f, SCREEN_HEIGHT - h - 10.0f, 150.0f, 1.0f};
        TSU_DrawableSetTranslate((Drawable *)self.page_label, &pos);
    }
}

static void UpdateCoinLabel(void);
static void SetUIVisibility(bool visible);

static void ArcadeCoinCountCb(uint32_t count, void *user) {
    (void)user;
    self.coins = (int)count;
    self.coins_dirty = 1;
}

static void LaunchGame(int index, bool test_mode) {
    if (index < 0 || index >= self.game_count || self.launching) {
        return;
    }

    self.launching = true;
    self.manual_focus = true;
    SetUIVisibility(true);
    StopMedia();
    thd_sleep(50);

    ScreenFadeOutEx("Starting...", 0);
    RunGame(index, test_mode ? 1 : 0);

    self.launching = false;
    ScreenFadeIn();
}

static void OpenIsoLoader(int index) {
    GameEntry *g;
    char game_path[NAME_MAX];
    App_t *app;

    if (index < 0 || index >= self.game_count || self.launching) {
        return;
    }

    g = &self.games[index];

    if (!g->game_file[0]) {
        return;
    }

    snprintf(game_path, sizeof(game_path), "%s/%s", g->path, g->game_file);

    app = GetAppByName("ISO Loader");

    if (!app) {
        ds_printf("Arcade: ISO Loader app not found\n");
        return;
    }

    self.launching = true;
    self.manual_focus = true;
    SetUIVisibility(true);
    StopMedia();
    CloseApp(self.app, 0);
    OpenApp(app, game_path);
}

static void ArcadeTestCb(mie_jvs_input_t input, uint32_t mask) {
    int idx;

    (void)input;
    (void)mask;

    if (self.launching) {
        return;
    }

    idx = self.focused_game_idx;
    if (idx < 0) {
        idx = self.last_picked_idx;
    }

    if (idx >= 0) {
        ScheduleTestLaunch(idx);
    }
}

static void UpdateCoinLabel(void) {
    char coin_text[32];
    float w, h;

    if(!self.coin_label) {
        return;
    }

    snprintf(coin_text, sizeof(coin_text), "Coins %d", self.coins);
    TSU_LabelSetText(self.coin_label, coin_text);
    TSU_LabelGetSize(self.coin_label, &w, &h);
    Vector pos = {20.0f, SCREEN_HEIGHT - h - 10.0f, 150.0f, 1.0f};
    TSU_DrawableSetTranslate((Drawable *)self.coin_label, &pos);
    self.coins_dirty = 0;
}

static void RefreshCoinLabelIfNeeded(void) {
    if(!self.coin_label || self.loader_active || !self.coins_dirty) {
        return;
    }

    UpdateCoinLabel();
    self.last_action_time = timer_ms_gettime64();
    SetUIVisibility(true);
}

static void SetUIVisibility(bool visible) {
    float fade_to = visible ? 1.0f : 0.0f;
    float delta = visible ? 0.05f : -0.05f;

    if (self.page_label) {
        if (!self.page_fader) {
            self.page_fader = TSU_AlphaFaderCreate(fade_to, delta);
        }
        else {
            TSU_AnimationComplete((Animation *)self.page_fader, (Drawable *)self.page_label);
            TSU_AlphaFaderSetValues(self.page_fader, fade_to, delta);
        }
        TSU_DrawableAnimAdd((Drawable *)self.page_label, (Animation *)self.page_fader);
    }

    if (self.coin_label) {
        if (!self.coin_fader) {
            self.coin_fader = TSU_AlphaFaderCreate(fade_to, delta);
        }
        else {
            TSU_AnimationComplete((Animation *)self.coin_fader, (Drawable *)self.coin_label);
            TSU_AlphaFaderSetValues(self.coin_fader, fade_to, delta);
        }
        TSU_DrawableAnimAdd((Drawable *)self.coin_label, (Animation *)self.coin_fader);
    }
}

float GetRandomCloudZ() {
    float z;
    bool collision;
    int attempts = 0;

    do {
        z = 5.0f + (rand() % 75);
        collision = false;

        // Check Wave collision
        if (z >= self.wave_z - 5.0f && z <= self.wave_z + 5.0f) {
            collision = true;
        }

        // Check collision with other games to avoid Z-fighting
        if (!collision) {
             int start = self.current_page * self.config.games_per_page;
             int count = (self.game_count < self.config.games_per_page) ? self.game_count : self.config.games_per_page;
             for (int i = 0; i < count; i++) {
                 int idx = (start + i) % self.game_count;
                 if (self.games[idx].visual && self.games[idx].z > 0.1f) {
                     if (z >= self.games[idx].z - 0.1f && z <= self.games[idx].z + 0.1f) {
                         collision = true;
                         break;
                     }
                 }
             }
        }

        attempts++;
        if (attempts > 200) {
            break;
        }
    } while (collision);
    return z;
}

void SetupCloudAnimation(GameEntry *g, float z_depth, float target_x, float target_y, float factor) {
    float scale_val = 0.25f + (z_depth / 100.0f) * 0.35f;
    int obj_size = (int)(COVER_WIDTH * scale_val);

    if (!g->scale_mover) {
        g->scale_mover = TSU_LogScaleMoverCreate(scale_val, scale_val);
    } else {
        TSU_AnimationComplete((Animation *)g->scale_mover, (Drawable *)g->visual);
        TSU_LogScaleMoverSetTarget(g->scale_mover, scale_val, scale_val);
    }
    TSU_LogScaleMoverSetFactor(g->scale_mover, factor);
    TSU_DrawableAnimAdd((Drawable *)g->visual, (Animation *)g->scale_mover);

    if (!g->mover) {
        g->mover = TSU_LogXYZMoverCreate(target_x + obj_size / 2.0f, target_y, z_depth);
    } else {
        TSU_AnimationComplete((Animation *)g->mover, (Drawable *)g->visual);
        TSU_LogXYZMoverSetTarget(g->mover, target_x + obj_size / 2.0f, target_y, z_depth);
    }
    TSU_LogXYZMoverSetFactor(g->mover, factor);
    TSU_DrawableAnimAdd((Drawable *)g->visual, (Animation *)g->mover);

    if (g->label) {
        float w, h;
        TSU_LabelGetSize(g->label, &w, &h);
        float off_x = (obj_size - w) / 2.0f;
        float off_y = (float)obj_size / 2.0f + h + 5.0f;

        if (!g->label_mover) {
            g->label_mover = TSU_LogXYZMoverCreate(target_x + off_x, target_y + off_y, z_depth + 0.5f);
        }
        else {
            TSU_AnimationComplete((Animation *)g->label_mover, (Drawable *)g->label);
            TSU_LogXYZMoverSetTarget(g->label_mover, target_x + off_x, target_y + off_y, z_depth + 0.5f);
        }
        
        static Color white = {1.0f, 1.0f, 1.0f, 1.0f};
        TSU_LabelSetTint(g->label, &white);

        TSU_LogXYZMoverSetFactor(g->label_mover, factor);
        TSU_DrawableAnimAdd((Drawable *)g->label, (Animation *)g->label_mover);
    }
    g->z = z_depth;
}


static void RestoreFocusedGameVisual() {
    if (self.focused_game_idx != -1) {
        GameEntry *g = &self.games[self.focused_game_idx];
        if (g->visual) {
            if (TSU_DrawableGetParent((Drawable *)g->visual) == NULL) {
                TSU_AppSubAddBanner(self.app->tsunami, g->visual);
            }
            TSU_DrawableSetAlpha((Drawable *)g->visual, 1.0f);
        }
    }
}

static void UnfocusGame(int index) {
    GameEntry *g = &self.games[index];
    g->last_unfocus_time = timer_ms_gettime64();

    if (g->visual) {
        float z_depth = GetRandomCloudZ();
        float scale_val = 0.25f + (z_depth / 100.0f) * 0.35f;
        int obj_size = (int)(COVER_WIDTH * scale_val);

        float factor = 20.0f + (rand() % 20);
        float target_x = (rand() % SCREEN_WIDTH) - 50.0f;
        float target_y = (rand() % (SCREEN_HEIGHT - obj_size)) + obj_size * 0.75f;

        if (g->label) {
            TSU_LabelSetFontSize(g->label, self.config.label_size);
        }

        SetupCloudAnimation(g, z_depth, target_x, target_y, factor);
    }
}

static void FocusGame(int index) {
    GameEntry *g = &self.games[index];

    if (g->visual) {
        float target_x = 320.0f;
        float target_y = 170.0f;

        float z_depth = 99.9f;
        g->z = z_depth;

        if (g->mover) {
             TSU_AnimationComplete((Animation *)g->mover, (Drawable *)g->visual);
             TSU_LogXYZMoverSetTarget(g->mover, target_x, target_y, z_depth);
             TSU_LogXYZMoverSetFactor(g->mover, 10.0f);
             TSU_DrawableAnimAdd((Drawable *)g->visual, (Animation *)g->mover);
        }

        if (g->scale_mover) {
             TSU_AnimationComplete((Animation *)g->scale_mover, (Drawable *)g->visual);
             TSU_LogScaleMoverSetTarget(g->scale_mover, 1.0f, 1.0f);
             TSU_LogScaleMoverSetFactor(g->scale_mover, 10.0f);
             TSU_DrawableAnimAdd((Drawable *)g->visual, (Animation *)g->scale_mover);
        }

        if (g->label && g->label_mover) {
             TSU_AnimationComplete((Animation *)g->label_mover, (Drawable *)g->label);

             TSU_LabelSetFontSize(g->label, self.config.focus_label_size);

             static Color focus_color = {1.0f, FOCUS_LABEL_R, FOCUS_LABEL_G, FOCUS_LABEL_B};
             TSU_LabelSetTint(g->label, &focus_color);

             float w, h;
             TSU_LabelGetSize(g->label, &w, &h);

             float label_x = (SCREEN_WIDTH - w) / 2;
             float label_y = 33.0f;

             TSU_LogXYZMoverSetTarget(g->label_mover, label_x, label_y, z_depth + 10.0f);
             TSU_LogXYZMoverSetFactor(g->label_mover, 10.0f);
             TSU_DrawableAnimAdd((Drawable *)g->label, (Animation *)g->label_mover);
        }
    }
}

static void UnloadPage(int page) {
    if (self.game_count == 0) return;

    int start = page * self.config.games_per_page;
    int count = (self.game_count < self.config.games_per_page) ? self.game_count : self.config.games_per_page;

    for (int i = 0; i < count; i++) {
        int idx = (start + i) % self.game_count;
        UnloadGameVisual(idx);
    }
}

static void ShowLoadedPage(void) {
    if (self.game_count == 0) return;

    int start = self.current_page * self.config.games_per_page;
    int count = (self.game_count < self.config.games_per_page) ? self.game_count : self.config.games_per_page;

    for (int i = 0; i < count; i++) {
        int idx = (start + i) % self.game_count;
        GameEntry *g = &self.games[idx];
        if (!g->visual) continue;

        if (TSU_DrawableGetParent((Drawable *)g->visual) == NULL) {
            TSU_AppSubAddBanner(self.app->tsunami, g->visual);
        }
        if (g->label && TSU_DrawableGetParent((Drawable *)g->label) == NULL) {
            TSU_AppSubAddLabel(self.app->tsunami, g->label);
        }

        float z_depth = GetRandomCloudZ();
        float scale_val = 0.25f + (z_depth / 100.0f) * 0.35f;
        int obj_size = (int)(COVER_WIDTH * scale_val);

        float target_x = (rand() % SCREEN_WIDTH) - 50.0f;
        float target_y = (rand() % (SCREEN_HEIGHT - obj_size)) + obj_size * 0.75f;

        float factor = 20.0f + (rand() % 20);

        SetupCloudAnimation(g, z_depth, target_x, target_y, factor);
    }
}

static void SwitchPage(int dir) {
    if (self.total_pages <= 1 || self.launching) return;

    if (!StartMediaFadeOutIfPlaying()) {
        if (self.state == STATE_FADE_TO_VIDEO || self.state == STATE_FADE_TO_COVER) {
            StopMedia();
            RestoreFocusedGameVisual();
        }
    }

    if (self.focused_game_idx != -1) {
        UnfocusGame(self.focused_game_idx);
        self.focused_game_idx = -1;
    }

    int page_dir = (dir > 0) ? 1 : -1;
    int next_page = self.target_page + page_dir;

    if (next_page < 0) next_page = self.total_pages - 1;
    if (next_page >= self.total_pages) next_page = 0;
    
    self.target_page = next_page;
    self.page_switch_dir = dir;
    self.abort_loading = true;

    ds_sfx_play(DS_SFX_SLIDE);

    UpdatePageLabel(self.target_page);

    self.state = STATE_PAGE_SWITCH_OUT;
    self.last_action_time = timer_ms_gettime64();
    self.manual_focus = true;
    SetUIVisibility(true);

    if (self.game_count > 0) {
        int start = self.current_page * self.config.games_per_page;
        int count = (self.game_count < self.config.games_per_page) ? self.game_count : self.config.games_per_page;

        for(int i = 0; i < count; i++) {
            int idx = (start + i) % self.game_count;
            GameEntry *g = &self.games[idx];
            float target_x = 0;
            float target_y = g->y;

            if(g->mover && g->visual) {
                TSU_AnimationComplete((Animation *)g->mover, (Drawable *)g->visual);
                
                if (dir == 2 || dir == -2) {
                    target_x = g->x;
                    target_y = (dir > 0) ? -COVER_HEIGHT * 2 : SCREEN_HEIGHT + COVER_HEIGHT * 2;
                } else {
                    target_x = (dir > 0) ? -COVER_WIDTH * 2 : SCREEN_WIDTH + COVER_WIDTH * 2;
                }

                TSU_LogXYZMoverSetTarget(g->mover, target_x, target_y, g->z);
                TSU_LogXYZMoverSetFactor(g->mover, 20.0f);
                TSU_DrawableAnimAdd((Drawable *)g->visual, (Animation *)g->mover);
            }

            if (g->label && g->label_mover) {
                TSU_AnimationComplete((Animation *)g->label_mover, (Drawable *)g->label);

                float w, h;
                TSU_LabelGetSize(g->label, &w, &h);
                float off_x = (COVER_WIDTH - w) / 2.0f;
                TSU_LogXYZMoverSetTarget(g->label_mover, target_x + off_x, target_y + COVER_HEIGHT + 5.0f, g->z + 0.5f);
                TSU_LogXYZMoverSetFactor(g->label_mover, 20.0f);
                TSU_DrawableAnimAdd((Drawable *)g->label, (Animation *)g->label_mover);
            }
        }
    }
}

static void SwitchSelection(int dir) {
    if (self.launching) {
        return;
    }

    if (self.state == STATE_PAGE_SWITCH_OUT ||
        self.state == STATE_PAGE_SWITCH_IN ||
        self.state == STATE_UNFOCUSING) return;

    if (self.game_count > 0) {
        if (!StartMediaFadeOutIfPlaying()) {
            if (self.state == STATE_FADE_TO_VIDEO || self.state == STATE_FADE_TO_COVER) {
                StopMedia();
                RestoreFocusedGameVisual();
            }
        }

        if (self.focused_game_idx != -1) {
            UnfocusGame(self.focused_game_idx);
        }
        
        int start = self.current_page * self.config.games_per_page;
        int count = (self.game_count < self.config.games_per_page) ? self.game_count : self.config.games_per_page;
        
        int rel_idx;
        if (self.focused_game_idx == -1) {
            rel_idx = (dir > 0) ? -1 : count; 
        } else {
            rel_idx = (self.focused_game_idx - start);
            while(rel_idx < 0) rel_idx += self.game_count;
            rel_idx = rel_idx % self.game_count;
        }
        
        rel_idx += dir;
        if (rel_idx < 0) rel_idx = count - 1;
        if (rel_idx >= count) rel_idx = 0;
        
        int next_idx = (start + rel_idx) % self.game_count;

        FocusGame(next_idx);
        self.focused_game_idx = next_idx;
        self.last_picked_idx = next_idx;
        self.state = STATE_FOCUSING;
        self.last_action_time = timer_ms_gettime64();
        self.manual_focus = true;
        SetUIVisibility(true);
        ds_sfx_play(DS_SFX_CLICK2);
    }
}

static void OnLeft() {
    SwitchSelection(-1);
}

static void OnRight() {
    SwitchSelection(1);
}

static void OnSelect() {
    if (self.launching) {
        return;
    }

    self.manual_focus = true;
    SetUIVisibility(true);

    if (self.state == STATE_PLAYING_VIDEO || self.state == STATE_FADE_TO_VIDEO || self.state == STATE_FADE_TO_COVER || self.state == STATE_PLAYING_AUDIO) {
        StopMedia();
    }

    if (self.focused_game_idx != -1) {
        LaunchGame(self.focused_game_idx, false);
    }
}

static void OnIsoLoader(void) {
    if (self.launching) {
        return;
    }

    if (self.focused_game_idx != -1) {
        OpenIsoLoader(self.focused_game_idx);
    }
}

static void OnBack() {
    if (self.launching) {
        return;
    }

    self.manual_focus = true;
    SetUIVisibility(true);

    if (self.state == STATE_PLAYING_VIDEO || self.state == STATE_FADE_TO_VIDEO || self.state == STATE_FADE_TO_COVER) {
        StopMedia();
        RestoreFocusedGameVisual();
        if (self.focused_game_idx != -1) {
            self.state = STATE_WAIT_PRE_VIDEO;
            self.last_action_time = timer_ms_gettime64();
        }
    }
    else {
        OpenMainApp();
    }
}

// Input Handler
static void InputHandler(void *ds_event, void *param, int action) {
    SDL_Event *event = (SDL_Event *)param;

    if (event->type == ARCADE_TEST_LAUNCH_EVENT) {
        LaunchGame(event->user.code, true);
        return;
    }

    switch(event->type) {
        case SDL_JOYBUTTONDOWN:
            switch(event->jbutton.button) {
                case SDL_DC_B: OnBack(); break;
                case SDL_DC_A: 
                case SDL_DC_START: OnSelect(); break;
                case SDL_DC_Y: OnIsoLoader(); break;
                default: break;
            }
            break;

        case SDL_JOYAXISMOTION:
            if (event->jaxis.axis == 0) { // Analog X
                int ax = event->jaxis.value;
                if (ax < -ANALOG_THRESHOLD) {
                    if (!self.analog_left_held) {
                        OnLeft();
                        self.analog_left_held = true;
                    }
                }
                else {
                    self.analog_left_held = false;
                }
                if (ax > ANALOG_THRESHOLD) {
                    if (!self.analog_right_held) {
                        OnRight();
                        self.analog_right_held = true;
                    }
                }
                else {
                    self.analog_right_held = false;
                }

            }
            else if (event->jaxis.axis == 1) { // Analog Y
                int ay = event->jaxis.value;
                if (ay < -ANALOG_THRESHOLD) {
                    if (!self.analog_up_held) {
                        SwitchPage(-2);
                        self.analog_up_held = true;
                    }
                }
                else {
                    self.analog_up_held = false;
                }
                if (ay > ANALOG_THRESHOLD) {
                    if (!self.analog_down_held) {
                        SwitchPage(2);
                        self.analog_down_held = true;
                    }
                }
                else {
                    self.analog_down_held = false;
                }
            }
            else if (event->jaxis.axis == 2) { // R Trigger
                if (event->jaxis.value > ANALOG_THRESHOLD) {
                    if (!self.r_trig_held) {
                        SwitchPage(1);
                        self.r_trig_held = true;
                    }
                }
                else {
                    self.r_trig_held = false;
                }
            }
            else if (event->jaxis.axis == 3) { // L Trigger
                if (event->jaxis.value > ANALOG_THRESHOLD) {
                    if (!self.l_trig_held) {
                        SwitchPage(-1);
                        self.l_trig_held = true;
                    }
                }
                else {
                    self.l_trig_held = false;
                }
            }
            break;

        case SDL_JOYHATMOTION:
             switch(event->jhat.value) {
                case SDL_HAT_LEFT: OnLeft(); break;
                case SDL_HAT_RIGHT: OnRight(); break;
                case SDL_HAT_UP: SwitchPage(-2); break;
                case SDL_HAT_DOWN: SwitchPage(2); break;
                default: break;
            }
            break;

        case SDL_KEYDOWN:
            switch(event->key.keysym.sym) {
                case SDLK_LEFT: OnLeft(); break;
                case SDLK_RIGHT: OnRight(); break;
                case SDLK_UP: SwitchPage(-2); break;
                case SDLK_DOWN: SwitchPage(2); break;
                case SDLK_RETURN:
                case SDLK_KP_ENTER: OnSelect(); break;
                case SDLK_SLASH: OnIsoLoader(); break;
                case SDLK_BACKSPACE:
                case SDLK_ESCAPE: OnBack(); break;
                default: break;
            }
            break;
    }
}

static void VideoHandler(void *ds_event, void *param, int action) {
    if (action == EVENT_ACTION_RENDER) {
        if (self.app && self.app->tsunami) {
             UpdateCloudLogic();
             RefreshCoinLabelIfNeeded();
        }
    }
}


static void CreateBackground() {
    Wave *wave = (Wave *)APP_GET_TSU_DRAWABLE("wave");
    self.video_border = (Rectangle *)APP_GET_TSU_DRAWABLE("video_border");
    self.page_label = (Label *)APP_GET_TSU_DRAWABLE("page_label");
    self.coin_label = (Label *)APP_GET_TSU_DRAWABLE("coin_label");

    if (wave) {
        const Vector *cur_pos = TSU_DrawableGetTranslate((Drawable *)wave);
        if (cur_pos) {
            self.wave_z = cur_pos->z;
        }
    }

    if (self.video_border) {
        UpdateVideoLayout();
        TSU_AppSubRemoveRectangle(self.app->tsunami, self.video_border);
    }

    LoadFogCircles();

    for (int i = 0; i < self.fog_count; i++) {
        if (self.fog_circles[i]) {
            const Vector *cur_pos = TSU_DrawableGetTranslate((Drawable *)self.fog_circles[i]);
            if (cur_pos) {
                self.fog_depths[i] = cur_pos->z;
            }

            float start_x = (rand() % SCREEN_WIDTH) - 256;
            float start_y = (rand() % SCREEN_HEIGHT) - 256;
            float start_z = self.fog_depths[i];

            Vector pos = {start_x, start_y, start_z, 1.0f};
            TSU_DrawableSetTranslate((Drawable *)self.fog_circles[i], &pos);

            float target_x = (rand() % SCREEN_WIDTH) - 100;
            float target_y = (rand() % SCREEN_HEIGHT) - 100;
            float target_z = self.fog_depths[i] + ((rand() % (int)(self.config.fog_z_range * 20)) / 10.0f) - self.config.fog_z_range;

            self.fog_movers[i] = TSU_LogXYZMoverCreate(target_x, target_y, target_z);
            TSU_LogXYZMoverSetFactor(self.fog_movers[i], 150.0f + (rand() % 50));
            TSU_DrawableAnimAdd((Drawable *)self.fog_circles[i], (Animation *)self.fog_movers[i]);
        }
    }

    UpdatePageLabel(self.current_page);
}


static void UpdateFog(uint64_t now) {
    static uint64_t last_cloud_update = 0;

    if (now - last_cloud_update > 1000) {
        last_cloud_update = now;
        for (int i = 0; i < self.fog_count; i++) {
            if (self.fog_circles[i] && self.fog_movers[i]) {
                if (rand() % 10 < 1) { // 10% chance per update
                    TSU_AnimationComplete((Animation *)self.fog_movers[i], (Drawable *)self.fog_circles[i]);

                    float tx = (rand() % (SCREEN_WIDTH + COVER_WIDTH / 2)) - (COVER_WIDTH / 4);
                    float ty = (rand() % (SCREEN_HEIGHT + COVER_WIDTH / 2)) - (COVER_WIDTH / 4);
                    float tz = self.fog_depths[i] + ((rand() % (int)(self.config.fog_z_range * 20)) / 10.0f) - self.config.fog_z_range;

                    TSU_LogXYZMoverSetTarget(self.fog_movers[i], tx, ty, tz);
                    TSU_LogXYZMoverSetFactor(self.fog_movers[i], 150.0f + (rand() % 50));
                    TSU_DrawableAnimAdd((Drawable *)self.fog_circles[i], (Animation *)self.fog_movers[i]);
                }
            }
        }
    }
}

static void RestoreFadingGameVisual(void) {
    if (self.fading_game_idx == -1) return;

    GameEntry *g = &self.games[self.fading_game_idx];
    if (!g->visual) return;

    if (TSU_DrawableGetParent((Drawable *)g->visual) == NULL) {
        TSU_AppSubAddBanner(self.app->tsunami, g->visual);
    }
    TSU_DrawableSetAlpha((Drawable *)g->visual, 1.0f);
}

static bool StartMediaFadeOutIfPlaying(void) {
    if (self.state != STATE_PLAYING_VIDEO && self.state != STATE_PLAYING_AUDIO) {
        return false;
    }

    self.media_fading_out = true;
    self.media_fade_start = timer_ms_gettime64();
    self.fading_game_idx = self.focused_game_idx;
    return true;
}

static void UpdateFadingMedia(uint64_t now) {
    if (!self.media_fading_out) return;

    uint64_t diff = now - self.media_fade_start;
    if (diff > self.config.media_fade_out_ms) {
        StopMedia();
        self.media_fading_out = false;
        RestoreFadingGameVisual();
        return;
    }

    float p = (float)diff / self.config.media_fade_out_ms;
    int vol = GetMediaVolume();
    int new_vol = (int)(vol * (1.0f - p));

    if (IsCDDATrackPlaying()) SetCDDAVolume(new_vol);
    if (ffplay_is_playing()) ffplay_set_volume(new_vol);
}

static void UpdateIdleState(uint64_t now) {
    if (self.game_count == 0 || now - self.last_action_time <= self.config.idle_time_ms) return;

    int start = self.current_page * self.config.games_per_page;
    int count = (self.game_count < self.config.games_per_page) ? self.game_count : self.config.games_per_page;
    int idx = start;

    int rel_idx = -1;
    if (self.last_picked_idx != -1) {
        for (int i = 0; i < count; i++) {
            if (((start + i) % self.game_count) == self.last_picked_idx) {
                rel_idx = i;
                break;
            }
        }
    }

    int next_rel_idx = rel_idx + 1;

    if (next_rel_idx >= count) {
        if (self.total_pages > 1) {
            SwitchPage(1);
            self.manual_focus = false;
            self.last_picked_idx = -1;
            return;
        }
        else {
            next_rel_idx = 0;
        }
    }

    idx = (start + next_rel_idx) % self.game_count;

    self.last_picked_idx = idx;
    self.focused_game_idx = idx;
    FocusGame(idx);
    self.state = STATE_FOCUSING;
    self.last_action_time = now;
    self.manual_focus = false;
    SetUIVisibility(false);
}

static void UpdateFocusingState(uint64_t now) {
    if (now - self.last_action_time > self.config.focus_time_ms) {
        self.state = STATE_WAIT_PRE_VIDEO;
        self.last_action_time = now;
    }
}

static bool TryPlayGameAudio(uint64_t now) {
    char filepath[NAME_MAX];
    char filename[NAME_MAX];
    size_t track_size = 0;

    const char *full_path = self.games[self.focused_game_idx].path;
    strcpy(filename, strrchr(full_path, '/') + 1);

    track_size = GetCDDATrackFilename(CDDA_PROBE_TRACK, full_path, filename, filepath);
    if (!track_size) return false;

    do {
        track_size = GetCDDATrackFilename((random() % CDDA_RANDOM_TRACK_COUNT) + CDDA_RANDOM_TRACK_OFFSET,
                                          full_path,
                                          filename,
                                          filepath);
    } while(track_size < CDDA_MIN_TRACK_SIZE);

    PlayCDDATrack(filepath, 0);
    self.state = STATE_PLAYING_AUDIO;
    self.last_action_time = now;
    return true;
}

static void UpdateWaitPreVideoState(uint64_t now) {
    if (now - self.last_action_time <= self.config.pre_video_time_ms) return;

    if (self.games[self.focused_game_idx].has_trailer) {
        PlayTrailer(self.focused_game_idx);
        self.state = STATE_FADE_TO_VIDEO;
        self.last_action_time = now;
        return;
    }

    if (TryPlayGameAudio(now)) return;

    if (now - self.last_action_time > self.config.no_trailer_wait_ms) {
        self.state = STATE_UNFOCUSING;
        UnfocusGame(self.focused_game_idx);
        self.last_action_time = now;
    }
}

static void UpdatePlayingAudioState(uint64_t now) {
    bool time_to_stop = false;
    uint64_t elapsed = now - self.last_action_time;

    if (elapsed > self.config.audio_start_grace_ms) {
        if (!IsCDDATrackPlaying()) {
            time_to_stop = true;
        }
    }

    if (!self.manual_focus) {
        if (elapsed > self.config.video_play_time_ms - self.config.fade_to_cover_ms) {
            float p = (float)(elapsed - (self.config.video_play_time_ms - self.config.fade_to_cover_ms)) / self.config.fade_to_cover_ms;
            if (p > 1.0f) p = 1.0f;
            int vol = GetMediaVolume();
            int new_vol = (int)(vol * (1.0f - p));
            SetCDDAVolume(new_vol);
        }

        if (elapsed > self.config.video_play_time_ms) {
            time_to_stop = true;
        }
    }

    if (time_to_stop) {
        StopMedia();
        self.state = STATE_UNFOCUSING;
        UnfocusGame(self.focused_game_idx);
        self.last_action_time = now;
    }
}

static void UpdateFadeToVideoState(uint64_t now) {
    float p = (now - self.last_action_time) / (float)self.config.fade_to_video_ms;
    bool finished = false;

    if (p >= 1.0f) {
        p = 1.0f;
        finished = true;
    }

    GameEntry *g = &self.games[self.focused_game_idx];
    if (g->visual) TSU_DrawableSetAlpha((Drawable *)g->visual, 1.0f - p);

    if (p >= self.config.video_border_show_progress && self.video_border && TSU_DrawableGetParent((Drawable *)self.video_border) == NULL) {
        TSU_AppSubAddRectangle(self.app->tsunami, self.video_border);
    }

    if (!finished) return;

    self.state = STATE_PLAYING_VIDEO;
    self.last_action_time = now;

    if (self.video_border && TSU_DrawableGetParent((Drawable *)self.video_border) == NULL) {
        TSU_AppSubAddRectangle(self.app->tsunami, self.video_border);
    }

    if (g->visual) {
        TSU_AppSubRemoveBanner(self.app->tsunami, g->visual);
    }

    ffplay_info_t info;
    if (ffplay_info(&info) == 0 && info.duration > 0) {
        self.trailer.duration = info.duration;
    }
    else {
        self.trailer.duration = 0;
    }
}

static void UpdatePlayingVideoState(uint64_t now) {
    bool time_to_fade = false;

    if (self.trailer.duration > 0) {
        int64_t pos = ffplay_get_pos();
        if (pos >= 0) {
            if (pos >= (self.trailer.duration - self.config.fade_to_cover_ms)) {
                time_to_fade = true;
            }
        }
    }
    else {
        if (!ffplay_is_playing()) {
            time_to_fade = true;
        }
    }

    if (!self.manual_focus) {
        if (now - self.last_action_time > self.config.video_play_time_ms) {
            time_to_fade = true;
        }
    }

    if (!time_to_fade) return;

    GameEntry *g = &self.games[self.focused_game_idx];
    if (g->visual) {
        TSU_AppSubAddBanner(self.app->tsunami, g->visual);
        TSU_DrawableSetAlpha((Drawable *)g->visual, 0.0f);
    }
    self.state = STATE_FADE_TO_COVER;
    self.last_action_time = now;
}

static void UpdateFadeToCoverState(uint64_t now) {
    float p = (now - self.last_action_time) / (float)self.config.fade_to_cover_ms;
    if (p >= 1.0f) {
        p = 1.0f;
        StopTrailer();
        self.state = STATE_UNFOCUSING;
        UnfocusGame(self.focused_game_idx);
        self.last_action_time = now;
    }

    int vol = GetMediaVolume();
    int new_vol = (int)(vol * (1.0f - p));
    ffplay_set_volume(new_vol);

    GameEntry *g = &self.games[self.focused_game_idx];
    if (g->visual) TSU_DrawableSetAlpha((Drawable *)g->visual, p);
}

static void UpdateUnfocusingState(uint64_t now) {
    if (now - self.last_action_time > self.config.unfocus_time_ms) {
        self.state = STATE_IDLE;
        self.focused_game_idx = -1;
        self.last_action_time = now;
    }
}

static void UpdatePageSwitchOutState(uint64_t now) {
    if (now - self.last_action_time <= self.config.page_switch_wait_ms) return;
    if (self.loader_active) return;

    UnloadPage(self.current_page);
    self.current_page = self.target_page;
    UpdatePageLabel(self.current_page);

    self.state = STATE_PAGE_SWITCH_IN;
    self.page_load_idx = 0;
    self.last_action_time = now;
    self.abort_loading = false;

    StartLoaderThread();
}

static void UpdatePageSwitchInState(uint64_t now) {
    if (self.loader_active) return;
    if (self.abort_loading) return;

    EnsurePageVisualsLoaded();
    ShowLoadedPage();
    self.state = STATE_IDLE;
    self.last_action_time = now;
    RefreshCoinLabelIfNeeded();
}

static void UpdateStateMachine(uint64_t now) {
    if (self.launching) {
        return;
    }

    switch(self.state) {
        case STATE_IDLE:
            UpdateIdleState(now);
            break;

        case STATE_FOCUSING:
            UpdateFocusingState(now);
            break;

        case STATE_WAIT_PRE_VIDEO:
            UpdateWaitPreVideoState(now);
            break;

        case STATE_PLAYING_AUDIO:
            UpdatePlayingAudioState(now);
            break;

        case STATE_FADE_TO_VIDEO:
            UpdateFadeToVideoState(now);
            break;

        case STATE_PLAYING_VIDEO:
            UpdatePlayingVideoState(now);
            break;

        case STATE_FADE_TO_COVER:
            UpdateFadeToCoverState(now);
            break;

        case STATE_UNFOCUSING:
            UpdateUnfocusingState(now);
            break;

        case STATE_PAGE_SWITCH_OUT:
            UpdatePageSwitchOutState(now);
            break;

        case STATE_PAGE_SWITCH_IN:
            UpdatePageSwitchInState(now);
            break;
    }
}

static void UpdateBackgroundCovers(uint64_t now) {
    if (self.state == STATE_PAGE_SWITCH_OUT || self.state == STATE_PAGE_SWITCH_IN) return;

    int start = self.current_page * self.config.games_per_page;
    int count = (self.game_count < self.config.games_per_page) ? self.game_count : self.config.games_per_page;

    for (int i = 0; i < count; i++) {
        int idx = (start + i) % self.game_count;
        if (idx == self.focused_game_idx) continue;

        GameEntry *g = &self.games[idx];

        if (now - g->last_unfocus_time < 3000) continue;

        if (g->mover && g->visual) {
            if (rand() % 200 < 1) {

                float z_depth = GetRandomCloudZ();
                float scale_val = 0.25f + (z_depth / 100.0f) * 0.35f;
                int obj_size = (int)(256.0f * scale_val);

                float target_x = (rand() % SCREEN_WIDTH) - 50.0f;
                float target_y = (rand() % (SCREEN_HEIGHT - obj_size)) + obj_size * 0.75f;
                float factor = 20.0f + (rand() % 20);

                SetupCloudAnimation(g, z_depth, target_x, target_y, factor);
            }
        }
    }
}

static void UpdateCloudLogic() {
    uint64_t now = timer_ms_gettime64();
    UpdateFadingMedia(now);
    UpdateStateMachine(now);
    if (self.state != STATE_PLAYING_VIDEO) {
        UpdateFog(now);
    }
    UpdateBackgroundCovers(now);
}

void ArcadeApp_Init(App_t *app) {
    memset(&self, 0, sizeof(self));
    self.app = app;
    self.focused_game_idx = -1;

    GetAppPath(self.app_path, sizeof(self.app_path), self.app ? self.app->fn : NULL);

    LoadArcadeConfig(self.app_path, &self.config);

    self.font = APP_GET_TSU_FONT("label_font");

    if (self.app->tsunami != NULL) {

        srand(timer_ms_gettime64());

        ScanGames();
        CreateBackground();
        PreloadDefaultCover();
        
        self.state = STATE_PAGE_SWITCH_IN;
        self.page_load_idx = 0;
        self.page_switch_dir = 1;
        self.last_action_time = timer_ms_gettime64();

        self.current_page = 0;
        self.target_page = 0;
        self.abort_loading = false;
        self.last_picked_idx = -1;
        self.media_fading_out = false;
        self.fading_game_idx = -1;
        self.launching = false;

        mutex_init(&self.trailer.mutex, MUTEX_TYPE_NORMAL);
        self.trailer.active = false;

        StartLoaderThread();

        if(self.loader_thread != NULL) {
            thd_join(self.loader_thread, NULL);
            self.loader_thread = NULL;
        }
        self.loader_active = false;
    }
}

void ArcadeApp_Open(App_t *app) {
    if (self.app->tsunami) {
        self.launching = false;
        SDL_DC_EmulateMouse(SDL_FALSE);

        naomi_coin_set_callback(ArcadeCoinCountCb, NULL);
        mie_jvs_callback(MIE_JVS_IN_PANEL_PSW, MIE_JVS_PANEL_TEST_BIT, ArcadeTestCb);
        mie_jvs_callback(MIE_JVS_IN_SYSTEM, MIE_JVS_SYS_TEST_BIT, ArcadeTestCb);

        self.coins = (int)naomi_coins_count();
        UpdateCoinLabel();

        self.input_event = AddEvent("ArcadeInput", EVENT_TYPE_INPUT, EVENT_PRIO_DEFAULT, InputHandler, NULL);
        self.video_event = AddEvent("ArcadeVideo", EVENT_TYPE_VIDEO, EVENT_PRIO_DEFAULT, VideoHandler, NULL);
    }
}

void ArcadeApp_Close(App_t *app) {
    (void)app;

    StopMedia();

    naomi_coin_set_callback(NULL, NULL);
    mie_jvs_callback(MIE_JVS_IN_PANEL_PSW, MIE_JVS_PANEL_TEST_BIT, NULL);
    mie_jvs_callback(MIE_JVS_IN_SYSTEM, MIE_JVS_SYS_TEST_BIT, NULL);

    if (self.input_event) {
        RemoveEvent(self.input_event);
        self.input_event = NULL;
    }
    if (self.video_event) {
        RemoveEvent(self.video_event);
        self.video_event = NULL;
    }

    SDL_DC_EmulateMouse(SDL_TRUE);
}

void ArcadeApp_Shutdown(App_t *app) {
    self.exiting = true;
    self.abort_loading = true;

    if(self.loader_thread != NULL) {
        thd_join(self.loader_thread, NULL);
        self.loader_thread = NULL;
    }
    self.loader_active = false;

    StopMedia();

    naomi_coin_set_callback(NULL, NULL);
    mie_jvs_callback(MIE_JVS_IN_PANEL_PSW, MIE_JVS_PANEL_TEST_BIT, NULL);
    mie_jvs_callback(MIE_JVS_IN_SYSTEM, MIE_JVS_SYS_TEST_BIT, NULL);

    if (self.input_event) RemoveEvent(self.input_event);
    if (self.video_event) RemoveEvent(self.video_event);
    self.input_event = NULL;
    self.video_event = NULL;

    if (self.page_fader) {
        TSU_AlphaFaderDestroy(&self.page_fader);
    }

    if (self.coin_fader) {
        TSU_AlphaFaderDestroy(&self.coin_fader);
    }

    for (int i = 0; i < self.game_count; i++) {
        UnloadGameVisual(i);
    }

    FreeGamesList();

    FreeFogCircles();

    if (self.def_cover_texture) {
        TSU_TextureDestroy(&self.def_cover_texture);
    }

    SDL_DC_EmulateMouse(SDL_TRUE);
    mutex_destroy(&self.trailer.mutex);
}
