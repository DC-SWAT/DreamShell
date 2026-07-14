-- Arcade application configuration.
-- Must return a table. Missing fields fall back to built-in defaults.
--
-- This is regular Lua: declare local variables before return,
-- compute values, combine them.
--
-- Video position and outer frame size are defined by the video_border
-- rectangle in app.xml. video_border_size here is the inner inset used
-- to compute the actual trailer playback area.
--
-- Background fog layers are defined in app.xml as circle elements named
-- fog_circle_* (up to 8 layers).

return {
    -- Directories to scan for games (in order)
    games_paths = {
        "/ide/games",
        "/sd/games",
        "/cd/games",
    },

    -- Scan mode: "fallback" - next path only if the page is not full;
    -- "all" - scan every path and merge results
    scan_mode = "fallback",

    -- Number of game covers shown on one page
    games_per_page = 8,

    -- Default cover image (relative to app directory or absolute path)
    default_cover = "images/cover.png",

    -- Cover file extensions inside a game folder (cover.png, cover.jpg, ...)
    cover_extensions = { "png", "jpg" },

    -- Trailer filename inside a game folder
    trailer_filename = "trailer.avi",

    -- Trailer and CDDA volume: 0-255, -1 = use DreamShell audio settings
    media_volume = -1,

    -- Inner inset from video_border rect in app.xml; defines trailer area
    video_border_size = 2,

    -- fade_to_video progress (0.0-1.0) when video border becomes visible
    video_border_show_progress = 0.7,

    -- Fog circle Z-axis drift range
    fog_z_range = 14,

    -- Game label font sizes
    label_size = 8,
    focus_label_size = 10,

    -- Idle delay before auto-selecting the next game, ms
    idle_time_ms = 4000,

    -- Focus animation duration, ms
    focus_time_ms = 1000,

    -- Delay before starting trailer/audio after focus, ms
    pre_video_time_ms = 1500,

    -- Wait time without trailer/audio before unfocusing, ms
    no_trailer_wait_ms = 5000,

    -- Max trailer/audio playback time in auto mode, ms
    video_play_time_ms = 20000,

    -- Unfocus animation duration, ms
    unfocus_time_ms = 1500,

    -- Cover fade-out duration before trailer starts, ms
    fade_to_video_ms = 1000,

    -- Cover fade-in duration after trailer/audio ends, ms
    fade_to_cover_ms = 500,

    -- Trailer fade-in duration on start, ms
    video_fade_in_ms = 500,

    -- Media fade-out duration when switching selection, ms
    media_fade_out_ms = 300,

    -- Delay before checking whether CDDA track has finished, ms
    audio_start_grace_ms = 1000,

    -- Pause when switching pages before loading new covers, ms
    page_switch_wait_ms = 800,

    -- UI overlay fade duration (reserved), ms
    fade_overlay_ms = 500,
}
