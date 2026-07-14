/* DreamShell

   app_internal.h - Launch App internal header
   Copyright (C) 2026 SWAT

*/

#ifndef __LAUNCH_APP_INTERNAL_H
#define __LAUNCH_APP_INTERNAL_H

#include <ds.h>
#include <tsunami/tsunami.h>

#define GRID_COLS 4
#define GRID_ROWS 4
#define CELL_PAD_X 30
#define CELL_PAD_Y 18
#define GRID_TOP 14
#define GRID_BOTTOM 10
#define ICON_LABEL_SIZE 8
#define ICON_LABEL_Y_OFFSET 6.0f
#define ICON_HIGHLIGHT_PADDING 6
#define SWAY_AMP_Y 1.0f
#define SWAY_AMP_X 0.5f
#define SWAY_ROT_DEG 0.6f
#define FOCUS_SWAY_AMP_Y 4.2f
#define FOCUS_SWAY_AMP_X 2.1f
#define FOCUS_SWAY_SPEED 4.3f
#define FOCUS_ROT_DEG 12.0f
#define FOCUS_ROT_FREQ_MUL 0.82f
#define PAGE_SLIDE_MS 320
#define SWAY_EASE_IN_MS 1200.0f
#define COL_MATCH_EPS 4.0f
#define FOCUS_LABEL_COLOR "#FFBE0B"
#define FOG_MAX 8
#define GRID_LAYOUT_MAX 8
#define GLOW_ALPHA_MIN 0.14f
#define GLOW_ALPHA_MAX 0.42f
#define GLOW_RADIUS_MUL 0.50f
#define GLOW_RADIUS_PULSE 10.0f
#define ANALOG_THRESHOLD 64

typedef struct {
    int grid_cols;
    int grid_rows;
    int cell_pad_x;
    int cell_pad_y;
    int grid_top;
    int grid_bottom;
    int icon_label_size;
    float icon_label_y_offset;
    int icon_highlight_padding;
    float sway_amp_y;
    float sway_amp_x;
    float sway_rot_deg;
    float focus_sway_amp_y;
    float focus_sway_amp_x;
    float focus_sway_speed;
    float focus_rot_deg;
    float focus_rot_freq_mul;
    int page_slide_ms;
    float col_match_eps;
    Color focus_label_color;
    float glow_alpha_min;
    float glow_alpha_max;
    float glow_radius_mul;
    float glow_radius_pulse;
} LaunchAppConfig_t;

typedef struct script_item {
    char name[64];
    char file[NAME_MAX];
} script_item_t;

typedef enum {
    LAUNCH_ITEM_APP = 0,
    LAUNCH_ITEM_SCRIPT = 1
} launch_item_type_t;

typedef enum {
    LAUNCH_DELETE_NONE = 0,
    LAUNCH_DELETE_SCRIPT = 1,
    LAUNCH_DELETE_APP = 2
} launch_delete_type_t;

typedef struct launch_item {
    launch_item_type_t type;
    int app_id;
    script_item_t *script;
    char name[64];
    Banner *banner;
    Label *label;
    float base_x;
    float base_y;
    float icon_w;
    float icon_h;
    float cell_w;
    float cell_h;
    int grid_page;
    int grid_col;
    int grid_row;
    float z;
    float sway_phase;
    float sway_speed;
    float sway_start_t;
} launch_item_t;

typedef struct {
    App_t *app;
    char app_path[NAME_MAX];
    LaunchAppConfig_t config;
    Font *caption_font;
    launch_item_t *items;
    int item_count;
    int item_capacity;
    int layout_slot;
    int layout_page;
    int cur_x;
    float scroll_x_display;
    int slide_active;
    int slide_from_x;
    int slide_to_x;
    uint64_t slide_start_ms;
    int pages;
    int panel_w;
    int panel_h;
    int screen_w;
    int screen_h;
    int grid_x;
    int grid_y;
    int grid_w;
    int toolbar_top;
    int toolbar_height;
    float toolbar_z;
    struct tm datetime;
    Label *date_label;
    Label *time_label;
    Label *version_label;
    Banner *net_banner;
    Texture *net_on;
    Texture *net_off;
    int net_status;
    Circle *fog_circles[FOG_MAX];
    LogXYZMover *fog_movers[FOG_MAX];
    float fog_depths[FOG_MAX];
    int fog_count;
    Circle *focus_glow;
    Color glow_color;
    Event_t *input_event;
    Event_t *video_event;
    script_item_t pending_shortcut;
    launch_delete_type_t pending_delete_type;
    int pending_app_id;
    char pending_app_name[64];
    int focused_index;
    int mouse_visible;
    int warp_motion_skip;
    int pending_activate_index;
    int slide_focus_index;
    int analog_left_held;
    int analog_right_held;
    int analog_up_held;
    int analog_down_held;
    Dialog *delete_dialog;
} LaunchAppContext_t;

extern LaunchAppContext_t self;

void LaunchAppConfigSetDefaults(LaunchAppConfig_t *cfg);
int LoadLaunchAppConfig(const char *app_path, LaunchAppConfig_t *cfg);

void ActivateItem(int index);
void BuildAppList(void);
void ResetAppListState(void);
void ResetItemsSwayStart(void);
void RebuildAppList(void);
void ClearAllItems(void);
void ClearPendingDelete(void);
int DeletePendingItem(void);
int ItemCanDelete(const launch_item_t *item);
void RequestItemDelete(int mx, int my, int prefer_focus);
void ShowItemDelete(int item_index);

void LaunchAppDrawTransparentPolyEvent(void);
float ItemScrollX(const launch_item_t *item);
int ItemVisibleOnPage(int index);
int HitTestItem(int mx, int my);
void LoadLayoutFromDrawables(void);
void CreateBackground(void);
void CreateFocusEffects(void);
void DestroyFocusEffects(void);
void FreeFogCircles(void);
void RefreshToolbar(void);
void UpdateItemTransforms(uint64_t now);
void UpdateFocusLabels(void);
void UpdateFog(uint64_t now);
void *LaunchAppClockThread(void *arg);

void UpdatePageSlide(uint64_t now);
void BeginPageSlide(int to_x);
void SetFocusedIndex(int index, int play_sfx, int current_page_only);
void SyncMouseToFocus(int index);
void UpdateMouseFocus(void);
void EnsureFocusVisible(void);
void MoveFocusDir(int dx, int dy);
void ActivateFocused(void);

#endif
