/* DreamShell

   module.c - Launch App lifecycle and input
   Copyright (C) 2026 SWAT

*/

#include <ds.h>
#include <sfx.h>
#include <tsunami/tsunami.h>
#include <math.h>

#include "app_module.h"
#include "app_internal.h"

DEFAULT_MODULE_EXPORTS(app_launch_app);

LaunchAppContext_t self;

void LaunchAppDeleteConfirm(Drawable *drawable) {
    (void)drawable;

    TSU_DialogHide(self.delete_dialog);

    if(DeletePendingItem()) {
        RebuildAppList();
    }

    ClearPendingDelete();
}

void LaunchAppDeleteCancel(Drawable *drawable) {
    (void)drawable;
    ClearPendingDelete();
    TSU_DialogHide(self.delete_dialog);
}

void UpdatePageSlide(uint64_t now) {
    float t;
    float ease;

    if(!self.slide_active) {
        self.scroll_x_display = (float)self.cur_x;
        return;
    }

    t = (float)(now - self.slide_start_ms) / (float)self.config.page_slide_ms;

    if(t >= 1.0f) {
        self.cur_x = self.slide_to_x;
        self.scroll_x_display = (float)self.cur_x;
        self.slide_active = 0;

        if(self.slide_focus_index >= 0) {
            SetFocusedIndex(self.slide_focus_index, 0, 1);
            self.slide_focus_index = -1;
        }
        else if(self.mouse_visible) {
            UpdateMouseFocus();
        }
        else {
            EnsureFocusVisible();
        }

        if(!self.mouse_visible && self.focused_index >= 0) {
            SyncMouseToFocus(self.focused_index);
        }
        return;
    }

    ease = t * (2.0f - t);
    self.scroll_x_display = (float)self.slide_from_x + (float)(self.slide_to_x - self.slide_from_x) * ease;
}

void BeginPageSlide(int to_x) {
    self.slide_from_x = self.cur_x;
    self.slide_to_x = to_x;
    self.slide_start_ms = timer_ms_gettime64();
    self.slide_active = 1;
}

static void SlidePage(int dir) {
    if(self.slide_active) {
        return;
    }

    if(dir < 0) {
        if(self.cur_x <= 0) {
            return;
        }

        ds_sfx_play(DS_SFX_SLIDE);
        self.pending_activate_index = -1;
        self.slide_focus_index = -1;
        BeginPageSlide(self.cur_x - self.panel_w);
    }
    else {
        if(self.cur_x + self.panel_w >= self.pages * self.panel_w) {
            return;
        }

        ds_sfx_play(DS_SFX_SLIDE);
        self.pending_activate_index = -1;
        self.slide_focus_index = -1;
        BeginPageSlide(self.cur_x + self.panel_w);
    }
}

static void SlideLeft(void) {
    SlidePage(-1);
}

static void SlideRight(void) {
    SlidePage(1);
}

static int GetCurrentPage(void) {
    if(self.panel_w <= 0) {
        return 0;
    }

    return self.cur_x / self.panel_w;
}

void SetFocusedIndex(int index, int play_sfx, int current_page_only) {
    if(index < -1 || index >= self.item_count) {
        return;
    }

    if(index >= 0) {
        if(current_page_only) {
            if(self.items[index].grid_page != GetCurrentPage()) {
                return;
            }
        }
        else if(!ItemVisibleOnPage(index)) {
            return;
        }
    }

    if(self.focused_index == index) {
        return;
    }

    if(self.focused_index >= 0) {
        self.items[self.focused_index].sway_start_t = (float)timer_ms_gettime64();
    }

    if(index >= 0) {
        self.items[index].sway_start_t = (float)timer_ms_gettime64();
    }

    self.focused_index = index;
    UpdateFocusLabels();

    if(play_sfx && index >= 0) {
        ds_sfx_play(DS_SFX_CLICK2);
    }
}

void SyncMouseToFocus(int index) {
    launch_item_t *item;
    float scroll_x;
    float cx;
    float cy;
    int mx;
    int my;

    if(index < 0 || index >= self.item_count || !ItemVisibleOnPage(index)) {
        return;
    }

    if(self.app == NULL || self.app->tsunami == NULL) {
        return;
    }

    item = &self.items[index];
    scroll_x = ItemScrollX(item);
    cx = scroll_x + item->cell_w * 0.5f;
    cy = item->base_y + item->cell_h * 0.5f;

    mx = (int)cx;
    my = (int)cy;

    if(mx < 0) {
        mx = 0;
    }
    else if(mx >= self.screen_w) {
        mx = self.screen_w - 1;
    }

    if(my < 0) {
        my = 0;
    }
    else if(my >= self.panel_h) {
        my = self.panel_h - 1;
    }

    SDL_WarpMouse((Uint16)mx, (Uint16)my);
    self.warp_motion_skip = 1;
    TSU_AppDoMouse(self.app->tsunami, mx, my);
}

void UpdateMouseFocus(void) {
    int mx;
    int my;
    int idx;

    if(!self.mouse_visible || self.slide_active || TSU_DialogIsVisible(self.delete_dialog)) {
        return;
    }

    SDL_GetMouseState(&mx, &my);
    idx = HitTestItem(mx, my);

    if(idx >= 0) {
        SetFocusedIndex(idx, 1, 0);
    }
    else {
        SetFocusedIndex(-1, 0, 0);
    }
}

void EnsureFocusVisible(void) {
    int i;

    if(self.focused_index >= 0 && ItemVisibleOnPage(self.focused_index)) {
        return;
    }

    for(i = 0; i < self.item_count; i++) {
        if(ItemVisibleOnPage(i)) {
            SetFocusedIndex(i, 0, 0);
            return;
        }
    }

    SetFocusedIndex(-1, 0, 0);
}

static int FindItemAtGrid(int page, int col, int row) {
    int i;

    for(i = 0; i < self.item_count; i++) {
        launch_item_t *item = &self.items[i];

        if(item->grid_page == page && item->grid_col == col && item->grid_row == row) {
            return i;
        }
    }

    return -1;
}

static int FindVerticalOnPage(int page, int col, int row, int dy) {
    int r;
    int c;
    int next;

    if(dy > 0) {
        for(r = row + 1; r < self.config.grid_rows; r++) {
            next = FindItemAtGrid(page, col, r);

            if(next >= 0) {
                return next;
            }
        }

        for(c = col + 1; c < self.config.grid_cols; c++) {
            next = FindItemAtGrid(page, c, 0);

            if(next >= 0) {
                return next;
            }
        }
    }
    else if(dy < 0) {
        for(r = row - 1; r >= 0; r--) {
            next = FindItemAtGrid(page, col, r);

            if(next >= 0) {
                return next;
            }
        }

        for(c = col - 1; c >= 0; c--) {
            for(r = self.config.grid_rows - 1; r >= 0; r--) {
                next = FindItemAtGrid(page, c, r);

                if(next >= 0) {
                    return next;
                }
            }
        }
    }

    return -1;
}

static int PageHasVerticalDown(int page, int col, int row) {
    int r;
    int c;

    for(r = row + 1; r < self.config.grid_rows; r++) {
        if(FindItemAtGrid(page, col, r) >= 0) {
            return 1;
        }
    }

    for(c = col + 1; c < self.config.grid_cols; c++) {
        if(FindItemAtGrid(page, c, 0) >= 0) {
            return 1;
        }
    }

    return 0;
}

static int PageHasVerticalUp(int page, int col, int row) {
    int r;
    int c;

    for(r = row - 1; r >= 0; r--) {
        if(FindItemAtGrid(page, col, r) >= 0) {
            return 1;
        }
    }

    for(c = col - 1; c >= 0; c--) {
        for(r = self.config.grid_rows - 1; r >= 0; r--) {
            if(FindItemAtGrid(page, c, r) >= 0) {
                return 1;
            }
        }
    }

    return 0;
}

static int FindPageEntryVertical(int page, int dy) {
    int r;
    int c;
    int next;

    if(dy > 0) {
        for(c = 0; c < self.config.grid_cols; c++) {
            next = FindItemAtGrid(page, c, 0);

            if(next >= 0) {
                return next;
            }
        }
    }
    else {
        for(c = self.config.grid_cols - 1; c >= 0; c--) {
            for(r = self.config.grid_rows - 1; r >= 0; r--) {
                next = FindItemAtGrid(page, c, r);

                if(next >= 0) {
                    return next;
                }
            }
        }
    }

    return -1;
}

static int FindFocusNeighbor(int from_idx, int dx, int dy) {
    launch_item_t *from = &self.items[from_idx];
    int best = -1;
    float best_dist = 1000000000.0f;
    int i;

    for(i = 0; i < self.item_count; i++) {
        launch_item_t *item = &self.items[i];
        float dist_x;
        float dist_y;
        float dist;

        if(i == from_idx || !ItemVisibleOnPage(i)) {
            continue;
        }

        if(item->grid_page != GetCurrentPage()) {
            continue;
        }

        dist_x = item->base_x - from->base_x;
        dist_y = item->base_y - from->base_y;

        if(dx > 0 && dist_x <= self.config.col_match_eps) {
            continue;
        }

        if(dx < 0 && dist_x >= -self.config.col_match_eps) {
            continue;
        }

        if(dy > 0 && dist_y <= self.config.col_match_eps) {
            continue;
        }

        if(dy < 0 && dist_y >= -self.config.col_match_eps) {
            continue;
        }

        if(dx != 0 && dy != 0) {
            continue;
        }

        if(dy != 0 && item->grid_col != from->grid_col) {
            continue;
        }

        if(dx != 0 && fabsf(dist_y) > (from->cell_h + (float)self.config.cell_pad_y)) {
            continue;
        }

        dist = dist_x * dist_x + dist_y * dist_y;

        if(dist < best_dist) {
            best_dist = dist;
            best = i;
        }
    }

    return best;
}

static void MoveFocusNav(int index, int play_sfx) {
    SetFocusedIndex(index, play_sfx, 1);

    if(index >= 0 && self.mouse_visible) {
        SyncMouseToFocus(index);
    }
}

void MoveFocusDir(int dx, int dy) {
    int next;
    int i;
    launch_item_t *from;
    int page;

    if(self.item_count <= 0 || self.slide_active) {
        return;
    }

    self.pending_activate_index = -1;
    self.mouse_visible = 0;

    EnsureFocusVisible();

    if(self.focused_index < 0) {
        for(i = 0; i < self.item_count; i++) {
            if(self.items[i].grid_page == GetCurrentPage()) {
                MoveFocusNav(i, 1);
                return;
            }
        }
        return;
    }

    if(dy != 0 && dx == 0) {
        from = &self.items[self.focused_index];
        page = GetCurrentPage();

        next = FindVerticalOnPage(page, from->grid_col, from->grid_row, dy);

        if(next >= 0) {
            MoveFocusNav(next, 1);
            return;
        }

        if(dy > 0) {
            if(!PageHasVerticalDown(page, from->grid_col, from->grid_row) && page + 1 < self.pages) {
                next = FindPageEntryVertical(page + 1, dy);

                if(next >= 0) {
                    ds_sfx_play(DS_SFX_SLIDE);
                    self.slide_focus_index = next;
                    BeginPageSlide((page + 1) * self.panel_w);
                }
            }
        }
        else {
            if(!PageHasVerticalUp(page, from->grid_col, from->grid_row) && page > 0) {
                next = FindPageEntryVertical(page - 1, dy);

                if(next >= 0) {
                    ds_sfx_play(DS_SFX_SLIDE);
                    self.slide_focus_index = next;
                    BeginPageSlide((page - 1) * self.panel_w);
                }
            }
        }

        return;
    }

    next = FindFocusNeighbor(self.focused_index, dx, dy);

    if(next >= 0) {
        MoveFocusNav(next, 1);
        return;
    }

    if(dx < 0 && self.cur_x > 0) {
        SlideLeft();
        return;
    }

    if(dx > 0 && self.cur_x + self.panel_w < self.pages * self.panel_w) {
        SlideRight();
    }
}

void ActivateFocused(void) {
    self.pending_activate_index = -1;

    if(self.focused_index >= 0) {
        ActivateItem(self.focused_index);
    }
}

static void HandleDialogInput(SDL_Event *event) {
    if(event->type == SDL_MOUSEBUTTONDOWN) {
        if(event->button.button == SDL_BUTTON_LEFT) {
            TSU_DialogHandleClick(self.delete_dialog, event->button.x, event->button.y);
        }
        else if(event->button.button == SDL_BUTTON_RIGHT) {
            LaunchAppDeleteCancel(NULL);
        }
        return;
    }

    if(event->type == SDL_JOYBUTTONDOWN) {
        switch(event->jbutton.button) {
            case SDL_DC_B:
                LaunchAppDeleteCancel(NULL);
                break;
            case SDL_DC_A:
            case SDL_DC_START:
                TSU_DialogActivateFocused(self.delete_dialog);
                break;
            default:
                break;
        }
        return;
    }

    if(event->type == SDL_JOYHATMOTION) {
        if(event->jhat.hat) {
            return;
        }

        switch(event->jhat.value) {
            case SDL_HAT_LEFT:
                TSU_DialogMoveFocus(self.delete_dialog, -1);
                break;
            case SDL_HAT_RIGHT:
                TSU_DialogMoveFocus(self.delete_dialog, 1);
                break;
            default:
                break;
        }
        return;
    }

    if(event->type == SDL_KEYDOWN) {
        switch(event->key.keysym.sym) {
            case SDLK_LEFT:
                TSU_DialogMoveFocus(self.delete_dialog, -1);
                break;
            case SDLK_RIGHT:
                TSU_DialogMoveFocus(self.delete_dialog, 1);
                break;
            case SDLK_RETURN:
            case SDLK_KP_ENTER:
                TSU_DialogActivateFocused(self.delete_dialog);
                break;
            case SDLK_ESCAPE:
            case SDLK_BACKSPACE:
                LaunchAppDeleteCancel(NULL);
                break;
            default:
                break;
        }
    }
}

static void InputHandler(void *ds_event, void *param, int action) {
    SDL_Event *event = (SDL_Event *)param;
    int idx;

    (void)ds_event;

    if(action != EVENT_ACTION_UPDATE) {
        return;
    }

    if(TSU_DialogIsVisible(self.delete_dialog)) {
        HandleDialogInput(event);
        return;
    }

    if(self.slide_active) {
        return;
    }

    switch(event->type) {
        case SDL_MOUSEMOTION:
            if(event->motion.z < 0) {
                SlideLeft();
            }
            else if(event->motion.z > 0) {
                SlideRight();
            }

            if(self.warp_motion_skip > 0) {
                self.warp_motion_skip--;
                break;
            }

            self.mouse_visible = 1;

            if(self.app != NULL && self.app->tsunami != NULL) {
                TSU_AppDoMouse(self.app->tsunami, event->motion.x, event->motion.y);
            }
            break;

        case SDL_MOUSEBUTTONDOWN:
            if(self.warp_motion_skip == 0) {
                self.mouse_visible = 1;
            }

            if(event->button.button == SDL_BUTTON_LEFT) {
                idx = HitTestItem(event->button.x, event->button.y);

                if(idx >= 0) {
                    SetFocusedIndex(idx, 0, 0);
                    self.pending_activate_index = idx;
                }
                else {
                    self.pending_activate_index = -1;
                }
            }
            else if(event->button.button == SDL_BUTTON_RIGHT) {
                RequestItemDelete(event->button.x, event->button.y, 0);
            }
            break;

        case SDL_MOUSEBUTTONUP:
            if(event->button.button == SDL_BUTTON_LEFT) {
                if(self.pending_activate_index >= 0) {
                    ActivateItem(self.pending_activate_index);
                }

                self.pending_activate_index = -1;
            }
            break;

        case SDL_JOYBUTTONDOWN:
            switch(event->jbutton.button) {
                case SDL_DC_B:
                    RequestItemDelete(0, 0, 1);
                    break;
                case SDL_DC_A:
                case SDL_DC_START:
                    ActivateFocused();
                    break;
                case SDL_DC_L:
                    SlideLeft();
                    break;
                case SDL_DC_R:
                    SlideRight();
                    break;
                default:
                    break;
            }
            break;

        case SDL_JOYHATMOTION:
            if(event->jhat.hat) {
                break;
            }

            switch(event->jhat.value) {
                case SDL_HAT_UP:
                    MoveFocusDir(0, -1);
                    break;
                case SDL_HAT_DOWN:
                    MoveFocusDir(0, 1);
                    break;
                case SDL_HAT_LEFT:
                    MoveFocusDir(-1, 0);
                    break;
                case SDL_HAT_RIGHT:
                    MoveFocusDir(1, 0);
                    break;
                default:
                    break;
            }
            break;

        case SDL_JOYAXISMOTION:
            if(event->jaxis.axis == 0) {
                int ax = event->jaxis.value;

                if(ax < -ANALOG_THRESHOLD) {
                    if(!self.analog_left_held) {
                        MoveFocusDir(-1, 0);
                        self.analog_left_held = 1;
                    }
                }
                else {
                    self.analog_left_held = 0;
                }

                if(ax > ANALOG_THRESHOLD) {
                    if(!self.analog_right_held) {
                        MoveFocusDir(1, 0);
                        self.analog_right_held = 1;
                    }
                }
                else {
                    self.analog_right_held = 0;
                }
            }
            else if(event->jaxis.axis == 1) {
                int ay = event->jaxis.value;

                if(ay < -ANALOG_THRESHOLD) {
                    if(!self.analog_up_held) {
                        MoveFocusDir(0, -1);
                        self.analog_up_held = 1;
                    }
                }
                else {
                    self.analog_up_held = 0;
                }

                if(ay > ANALOG_THRESHOLD) {
                    if(!self.analog_down_held) {
                        MoveFocusDir(0, 1);
                        self.analog_down_held = 1;
                    }
                }
                else {
                    self.analog_down_held = 0;
                }
            }
            break;

        case SDL_KEYDOWN:
            switch(event->key.keysym.sym) {
                case SDLK_COMMA:
                    SlideLeft();
                    break;
                case SDLK_PERIOD:
                    SlideRight();
                    break;
                case SDLK_UP:
                    MoveFocusDir(0, -1);
                    break;
                case SDLK_DOWN:
                    MoveFocusDir(0, 1);
                    break;
                case SDLK_LEFT:
                    MoveFocusDir(-1, 0);
                    break;
                case SDLK_RIGHT:
                    MoveFocusDir(1, 0);
                    break;
                case SDLK_RETURN:
                    ActivateFocused();
                    break;
                case SDLK_DELETE:
                    RequestItemDelete(0, 0, 1);
                    break;
                default:
                    break;
            }
            break;

        default:
            break;
    }
}

static void VideoHandler(void *ds_event, void *param, int action) {
    uint64_t now;

    (void)ds_event;
    (void)param;

    if(action != EVENT_ACTION_RENDER || self.app == NULL || self.app->tsunami == NULL) {
        return;
    }

    now = timer_ms_gettime64();
    UpdatePageSlide(now);
    UpdateItemTransforms(now);
    UpdateMouseFocus();
    UpdateFog(now);
}

void LaunchApp_Init(App_t *app) {
    memset(&self, 0, sizeof(self));
    self.app = app;

    if(app == NULL || app->tsunami == NULL) {
        ds_printf("DS_ERROR: Launch app requires tsunami body\n");
        return;
    }

    GetAppPath(self.app_path, sizeof(self.app_path), app->fn);

    LoadLaunchAppConfig(self.app_path, &self.config);
    LoadLayoutFromDrawables();

    self.caption_font = APP_GET_TSU_FONT("caption_font");

    if(self.caption_font == NULL) {
        ds_printf("DS_ERROR: Launch app can't find fonts\n");
        return;
    }

    TSU_AppSetDrawTransparentPolyEvent(app->tsunami, LaunchAppDrawTransparentPolyEvent);

    self.date_label = (Label *)APP_GET_TSU_DRAWABLE("date");
    self.time_label = (Label *)APP_GET_TSU_DRAWABLE("time");
    self.version_label = (Label *)APP_GET_TSU_DRAWABLE("version");
    self.net_banner = (Banner *)APP_GET_TSU_DRAWABLE("net_icon");
    self.net_on = APP_GET_TSU_IMAGE("net-on");
    self.net_off = APP_GET_TSU_IMAGE("net-off");
    self.delete_dialog = (Dialog *)APP_GET_TSU_DRAWABLE("delete_dialog");
    self.focus_glow = (Circle *)APP_GET_TSU_DRAWABLE("focus_glow");

    if(self.delete_dialog != NULL) {
        TSU_DialogHide(self.delete_dialog);
    }

    CreateBackground();
    CreateFocusEffects();
    ResetAppListState();
    BuildAppList();
    RefreshToolbar();
    UpdateFocusLabels();
}

static void StopLaunchAppEvents(void) {
    if(self.input_event) {
        RemoveEvent(self.input_event);
        self.input_event = NULL;
    }

    if(self.video_event) {
        RemoveEvent(self.video_event);
        self.video_event = NULL;
    }

    SDL_DC_EmulateMouse(SDL_TRUE);
}

void LaunchApp_Open(App_t *app) {
    (void)app;

    if(self.app == NULL || self.app->tsunami == NULL) {
        return;
    }

    SDL_DC_EmulateMouse(SDL_FALSE);

    self.mouse_visible = 0;
    self.warp_motion_skip = 0;

    EnsureFocusVisible();
    if(self.focused_index >= 0) {
        SyncMouseToFocus(self.focused_index);
    }

    ResetItemsSwayStart();
    UpdateItemTransforms(timer_ms_gettime64());

    RefreshToolbar();

    self.input_event = AddEvent("LaunchAppInput", EVENT_TYPE_INPUT, EVENT_PRIO_DEFAULT, InputHandler, NULL);
    self.video_event = AddEvent("LaunchAppVideo", EVENT_TYPE_VIDEO, EVENT_PRIO_DEFAULT, VideoHandler, NULL);
    self.app->thd = thd_create(0, LaunchAppClockThread, NULL);
}

void LaunchApp_Close(App_t *app) {
    (void)app;

    TSU_DialogHide(self.delete_dialog);
    self.pending_activate_index = -1;
    StopLaunchAppEvents();
}

void LaunchApp_Shutdown(App_t *app) {
    (void)app;

    StopLaunchAppEvents();

    ClearAllItems();
    free(self.items);
    self.items = NULL;
    self.item_capacity = 0;

	DestroyFocusEffects();
	FreeFogCircles();
	self.app = NULL;
}
