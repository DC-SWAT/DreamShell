/* DreamShell
   
   app_internal.h - Arcade app internal header
   Copyright (C) 2026 SWAT
   
*/

#ifndef __ARCADE_APP_INTERNAL_H
#define __ARCADE_APP_INTERNAL_H

#include <ds.h>
#include <ffmpeg.h>
#include <tsunami/tsunami.h>

#define GAMES_ALLOC_CHUNK 32
#define GAMES_PER_PAGE 8
#define SCREEN_WIDTH 640
#define SCREEN_HEIGHT 480

#define IDLE_TIME_MS 4000
#define FOCUS_TIME_MS 1000
#define PRE_VIDEO_TIME_MS 1500
#define NO_TRAILER_WAIT_MS 5000
#define FADE_TO_VIDEO_MS 1000.0f
#define VIDEO_PLAY_TIME_MS 20000
#define FADE_TO_COVER_MS 500.0f
#define UNFOCUS_TIME_MS 1500
#define VIDEO_FADE_IN_MS 500
#define MEDIA_FADE_OUT_MS 300
#define AUDIO_START_GRACE_MS 1000
#define PAGE_SWITCH_WAIT_MS 800
#define FADE_OVERLAY_MS 500.0f
#define DEFAULT_MEDIA_VOLUME 240
#define VIDEO_BORDER_SHOW_PROGRESS 0.7f
#define CDDA_PROBE_TRACK 5
#define CDDA_RANDOM_TRACK_COUNT 20
#define CDDA_RANDOM_TRACK_OFFSET 4
#define CDDA_MIN_TRACK_SIZE (5 << 10)

#define VIDEO_X 192
#define VIDEO_Y 42
#define VIDEO_W 256
#define VIDEO_H 256
#define VIDEO_BORDER_SIZE 2

#define COVER_WIDTH 256
#define COVER_HEIGHT 256

#define LABEL_SIZE 8
#define FOCUS_LABEL_SIZE 10

#define FOCUS_LABEL_R 1.0f
#define FOCUS_LABEL_G 0.816f
#define FOCUS_LABEL_B 0.0f

#define FOG_MAX 8
#define FOG_Z_RANGE 14.0f

#define ARCADE_MAX_GAME_PATHS 8
#define ARCADE_MAX_COVER_EXTS 8

#define ARCADE_SCAN_FALLBACK 0
#define ARCADE_SCAN_ALL 1

typedef struct {
	char games_paths[ARCADE_MAX_GAME_PATHS][NAME_MAX];
	int games_paths_count;
	int scan_mode;
	int games_per_page;
	char default_cover[NAME_MAX];
	char cover_exts[ARCADE_MAX_COVER_EXTS][8];
	int cover_exts_count;
	char trailer_filename[NAME_MAX];
	int media_volume;
	int idle_time_ms;
	int focus_time_ms;
	int pre_video_time_ms;
	int no_trailer_wait_ms;
	int video_play_time_ms;
	int unfocus_time_ms;
	int fade_to_video_ms;
	int fade_to_cover_ms;
	int video_fade_in_ms;
	int media_fade_out_ms;
	int audio_start_grace_ms;
	int page_switch_wait_ms;
	int fade_overlay_ms;
	float video_border_show_progress;
	int video_border_size;
	float fog_z_range;
	int label_size;
	int focus_label_size;
} ArcadeConfig_t;

enum {
	APP_DEVICE_CD = 0,
	APP_DEVICE_SD = 1,
	APP_DEVICE_IDE,
	APP_DEVICE_PC,
	APP_DEVICE_COUNT
};

typedef struct {
    char path[NAME_MAX];
    char game_file[NAME_MAX];
    char name[64];
    char cover_path[NAME_MAX];
    char trailer_path[NAME_MAX];

    Banner *visual;
    Texture *texture;
    LogXYZMover *mover;
    LogScaleMover *scale_mover;
    LogXYZMover *label_mover;
    Animation *anim;
    Label *label;

    float x, y, z;
    bool has_trailer;
    bool has_cover;
    bool assets_checked;
    uint8 md5[16];
    uint64_t last_unfocus_time;
} GameEntry;

typedef enum {
    STATE_IDLE,
    STATE_FOCUSING,
    STATE_WAIT_PRE_VIDEO,
    STATE_FADE_TO_VIDEO,
    STATE_PLAYING_VIDEO,
    STATE_PLAYING_AUDIO,
    STATE_FADE_TO_COVER,
    STATE_UNFOCUSING,
    STATE_PAGE_SWITCH_OUT,
    STATE_PAGE_SWITCH_IN
} ArcadeState;

typedef struct AppArcadeContext {
    App_t *app;
    char app_path[NAME_MAX];
    ArcadeConfig_t config;

    GameEntry *games;
    int games_capacity;
    int game_count;
    int current_page;
    int total_pages;
    int page_switch_dir;
    int page_load_idx;

    int focused_game_idx;
    int last_picked_idx;
    ArcadeState state;
    uint64_t last_action_time;

    Event_t *input_event;
    Event_t *video_event;

    Rectangle *video_border;
    int video_x;
    int video_y;
    int video_w;
    int video_h;
    Wave *wave;
    float wave_z;

    Circle *fog_circles[FOG_MAX];
    LogXYZMover *fog_movers[FOG_MAX];
    float fog_depths[FOG_MAX];
    int fog_count;

    Texture *def_cover_texture;
    bool exiting;
    bool manual_focus;

    Label *page_label;
    Label *coin_label;
    Font *font;
    AlphaFader *page_fader;
    AlphaFader *coin_fader;
    int coins;
    volatile uint8_t coins_dirty;

    struct {
        char path[NAME_MAX];
        ffplay_params_t params;
        kthread_t *thread;
        mutex_t mutex;
        bool active;
        int64_t duration;
    } trailer;

    int64_t cdda_duration;

    kthread_t *loader_thread;
    bool loader_active;
    bool abort_loading;
    int target_page;
    bool l_trig_held;
    bool r_trig_held;

    bool media_fading_out;
    uint64_t media_fade_start;
    int fading_game_idx;

    bool analog_left_held;
    bool analog_right_held;
    bool analog_up_held;
    bool analog_down_held;

    bool launching;
} AppArcadeContext_t;

extern AppArcadeContext_t self;

void ArcadeConfigSetDefaults(ArcadeConfig_t *cfg);
int LoadArcadeConfig(const char *app_path, ArcadeConfig_t *cfg);
void ArcadeConfigResolvePath(const char *app_path, const char *path, char *result, size_t size);
void UpdateVideoLayout(void);

int getDeviceType(const char *dir);
void getFirstPathComponent(const char *path, char *result);
void makeGameRelativePath(char *dst, size_t dst_size, const char *base_path, const char *game_path, const char *filename);
int IsGameExtension(const char *filename);
int IsNaomiRom(const char *path);
void GetMD5(const char *path, uint8 *md5);

void ScanGames(void);
void FreeGamesList(void);
void UnloadGameVisual(int index);
void PreloadDefaultCover(void);
void LoadGameVisual(int index, int fly_in_dir);
void EnsurePageVisualsLoaded(void);
void RunGame(int index, int test_mode);
void PlayTrailer(int index);
void StopTrailer(void);
void StopMedia(void);
int GetMediaVolume(void);
size_t GetCDDATrackFilename(int num, const char *fpath, const char *filename, char *result);
void PlayCDDATrack(const char *file, int loop);
void StopCDDATrack(void);
void PauseCDDATrack(void);
void ResumeCDDATrack(void);
int IsCDDATrackPlaying(void);
void SetCDDAVolume(int vol);

float GetRandomCloudZ(void);
void SetupCloudAnimation(GameEntry *g, float z_depth, float target_x, float target_y, float factor);

#endif
