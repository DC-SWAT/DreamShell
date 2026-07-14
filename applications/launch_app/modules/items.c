/* DreamShell

   items.c - Launch App item list and layout
   Copyright (C) 2026 SWAT

*/

#include <ds.h>
#include <sfx.h>
#include <tsunami/tsunami.h>

#include "app_internal.h"

static void RunScriptItem(launch_item_t *item) {
    script_item_t *si;
    int c;

    if(item->type != LAUNCH_ITEM_SCRIPT || item->script == NULL) {
        return;
    }

    si = item->script;
    c = si->file[strlen(si->file) - 3];

    switch(c) {
        case 'l':
            LuaDo(LUA_DO_FILE, si->file, GetLuaState());
            break;
        case 'd':
        default:
            dsystem_script(si->file);
            break;
    }
}

void ActivateItem(int index) {
    launch_item_t *item;
    App_t *app;

    if(index < 0 || index >= self.item_count) {
        return;
    }

    item = &self.items[index];
    ds_sfx_play(DS_SFX_CLICK);

    if(item->type == LAUNCH_ITEM_APP) {
        app = GetAppById(item->app_id);

        if(app != NULL) {
            OpenApp(app, NULL);
        }
    }
    else {
        RunScriptItem(item);
    }
}

static int EnsureItemCapacity(int min_capacity) {
    launch_item_t *new_items;
    int new_capacity;

    if(min_capacity <= self.item_capacity) {
        return 1;
    }

    new_capacity = self.item_capacity;

    if(new_capacity == 0) {
        new_capacity = 16;
    }

    while(new_capacity < min_capacity) {
        new_capacity += 16;
    }

    new_items = (launch_item_t *)realloc(self.items, new_capacity * sizeof(launch_item_t));

    if(new_items == NULL) {
        return 0;
    }

    if(new_capacity > self.item_capacity) {
        memset(new_items + self.item_capacity, 0,
            (new_capacity - self.item_capacity) * sizeof(launch_item_t));
    }

    self.items = new_items;
    self.item_capacity = new_capacity;
    return 1;
}

static void DestroyItemVisual(launch_item_t *item) {
    if(item->banner != NULL) {
        Texture *texture = TSU_BannerGetTexture(item->banner);
        if(texture != NULL) {
            TSU_TextureDestroy(&texture);
        }
        TSU_AppSubRemoveBanner(self.app->tsunami, item->banner);
        TSU_BannerDestroy(&item->banner);
    }

    if(item->label != NULL) {
        TSU_AppSubRemoveLabel(self.app->tsunami, item->label);
        TSU_LabelDestroy(&item->label);
    }

    if(item->script != NULL) {
        free(item->script);
    }

    memset(item, 0, sizeof(*item));
}

void ClearAllItems(void) {
    int i;

    for(i = 0; i < self.item_count; i++) {
        DestroyItemVisual(&self.items[i]);
    }

    self.item_count = 0;
}

static Banner *CreateIconBanner(Texture *texture, float w, float h, float z, int item_index) {
    Banner *banner;
    Vector pos;
    Vector scale;

    if(texture == NULL) {
        return NULL;
    }

    banner = TSU_BannerCreate(PVR_LIST_TR_POLY, texture);

    if(banner == NULL) {
        return NULL;
    }

    TSU_BannerSetSize(banner, w, h);
    pos.x = 0.0f;
    pos.y = 0.0f;
    pos.z = z;
    pos.w = 1.0f;
    TSU_DrawableSetTranslate((Drawable *)banner, &pos);
    scale.x = 1.0f;
    scale.y = 1.0f;
    scale.z = 1.0f;
    scale.w = 0.0f;
    TSU_DrawableSetScale((Drawable *)banner, &scale);
    TSU_DrawableSetId((Drawable *)banner, item_index);

    return banner;
}

static void AdvanceLayoutSlot(void) {
    self.layout_slot++;

    if(self.layout_slot >= self.config.grid_cols * self.config.grid_rows) {
        self.layout_slot = 0;
        self.layout_page++;

        if(self.layout_page >= self.pages) {
            self.pages = self.layout_page + 1;
        }
    }
}

static void AssignItemGridPos(launch_item_t *item, int page, int col, int row) {
    item->grid_page = page;
    item->grid_col = col;
    item->grid_row = row;
}

static int GetMaxRowInColumn(int page, int col) {
    int i;
    int max_row = -1;

    for(i = 0; i < self.item_count; i++) {
        launch_item_t *item = &self.items[i];

        if(item->grid_page == page && item->grid_col == col && item->grid_row > max_row) {
            max_row = item->grid_row;
        }
    }

    return max_row;
}

static int CountItemPages(void) {
    int max_page = 0;
    int i;

    for(i = 0; i < self.item_count; i++) {
        if(self.items[i].grid_page > max_page) {
            max_page = self.items[i].grid_page;
        }
    }

    return max_page + 1;
}

static void GetItemLabelSize(const launch_item_t *item, float *out_w, float *out_h) {
    float label_w = 0.0f;
    float label_h = 0.0f;

    if(item->label != NULL) {
        TSU_LabelGetSize(item->label, &label_w, &label_h);
    }

    if(label_h < 1.0f) {
        label_h = (float)self.config.icon_label_size;
    }

    *out_w = label_w;
    *out_h = label_h;
}

static float GetItemTotalHeight(const launch_item_t *item) {
    float label_w;
    float label_h;

    GetItemLabelSize(item, &label_w, &label_h);

    return item->icon_h + (float)self.config.icon_highlight_padding
        + self.config.icon_label_y_offset + label_h;
}

static float GetColumnUsedHeight(int page, int col, float gap_y) {
    int i;
    int count = 0;
    float used = 0.0f;

    for(i = 0; i < self.item_count; i++) {
        launch_item_t *item = &self.items[i];

        if(item->grid_page == page && item->grid_col == col) {
            used += GetItemTotalHeight(item);
            count++;
        }
    }

    if(count > 0) {
        used += gap_y * (float)(count - 1);
    }

    return used;
}

static int ColumnFitsHeight(int page, int col, float gap_y, float avail_h, float item_h) {
    float used_h = GetColumnUsedHeight(page, col, gap_y);

    if(used_h <= 0.0f) {
        return 1;
    }

    return used_h + gap_y + item_h <= avail_h;
}

static void LayoutScriptGridItems(int cols, int rows, float gap_y, float avail_h) {
    int i;
    int page;
    int col;
    int row;
    int best_col;
    int best_row;
    float item_h;

    for(i = 0; i < self.item_count; i++) {
        launch_item_t *item = &self.items[i];

        if(item->type != LAUNCH_ITEM_SCRIPT) {
            continue;
        }

        item_h = GetItemTotalHeight(item);
        page = 0;

        for(;;) {
            best_col = -1;
            best_row = 0;

            for(col = 0; col < cols; col++) {
                row = GetMaxRowInColumn(page, col) + 1;

                if(row >= rows) {
                    continue;
                }

                if(!ColumnFitsHeight(page, col, gap_y, avail_h, item_h)) {
                    continue;
                }

                best_col = col;
                best_row = row;
                break;
            }

            if(best_col >= 0) {
                AssignItemGridPos(item, page, best_col, best_row);
                break;
            }

            page++;
        }
    }
}

static void LayoutCenter512Icons(float avail_h) {
    int i;
    int page;
    int max_page = CountItemPages() - 1;
    float padding = (float)self.config.icon_highlight_padding;

    for(page = 0; page <= max_page; page++) {
        launch_item_t *only = NULL;
        float total_h;
        float shift_x;
        float shift_y;
        float block_w;

        for(i = 0; i < self.item_count; i++) {
            launch_item_t *item = &self.items[i];

            if(item->grid_page != page) {
                continue;
            }

            if(only != NULL) {
                only = NULL;
                break;
            }

            only = item;
        }

        if(only == NULL) {
            continue;
        }

        if(only->icon_w != 512.0f || only->icon_h != 512.0f) {
            continue;
        }

        block_w = only->icon_w + padding;
        total_h = GetItemTotalHeight(only);
        shift_x = (float)self.grid_x + ((float)self.panel_w - block_w) * 0.5f
            + (float)(page * self.panel_w) - only->base_x;
        only->base_x += shift_x;

        if(total_h > avail_h) {
            shift_y = (float)self.grid_y + ((float)self.panel_h - total_h) * 0.25f - only->base_y;
            only->base_y += shift_y;
        }
    }
}

static void ComputeAdaptiveColWidths(int page, int cols, float *col_w) {
    int i;
    int c;

    for(c = 0; c < cols; c++) {
        col_w[c] = 0.0f;
    }

    for(i = 0; i < self.item_count; i++) {
        launch_item_t *item = &self.items[i];
        float item_w;
        float label_w;
        float label_h;

        if(item->grid_page != page) {
            continue;
        }

        if(item->grid_col < 0 || item->grid_col >= cols) {
            continue;
        }

        item_w = item->icon_w + (float)self.config.icon_highlight_padding;
        GetItemLabelSize(item, &label_w, &label_h);

        if(label_w > item_w) {
            item_w = label_w;
        }

        if(item_w > col_w[item->grid_col]) {
            col_w[item->grid_col] = item_w;
        }
    }

    for(c = 0; c < cols; c++) {
        if(col_w[c] < 1.0f) {
            col_w[c] = 1.0f;
        }
    }
}

static void FitPageColWidths(int cols, float pad_x, float *col_w) {
    float content_w = 0.0f;
    float avail_content_w;
    float scale;
    int c;

    for(c = 0; c < cols; c++) {
        content_w += col_w[c];
    }

    if(content_w < 1.0f) {
        return;
    }

    avail_content_w = (float)self.grid_w - pad_x * (float)(cols + 1);

    if(avail_content_w < 1.0f) {
        avail_content_w = 1.0f;
    }

    scale = avail_content_w / content_w;

    for(c = 0; c < cols; c++) {
        col_w[c] *= scale;
    }
}

static void ApplyAdaptivePageGrid(int page, int cols, int rows, float pad_x, float gap_y,
    const float *col_w) {

    int col;
    int row;
    int i;
    float col_x[GRID_LAYOUT_MAX];
    float col_y[GRID_LAYOUT_MAX];
    float x = (float)self.grid_x + pad_x + (float)(page * self.panel_w);
    float top_y = (float)self.grid_y + (float)self.config.grid_top;

    for(col = 0; col < cols; col++) {
        col_x[col] = x;
        col_y[col] = top_y;
        x += col_w[col] + pad_x;
    }

    for(row = 0; row < rows; row++) {
        for(col = 0; col < cols; col++) {
            launch_item_t *item = NULL;

            for(i = 0; i < self.item_count; i++) {
                if(self.items[i].grid_page == page && self.items[i].grid_col == col &&
                    self.items[i].grid_row == row) {

                    item = &self.items[i];
                    break;
                }
            }

            if(item == NULL) {
                continue;
            }

            item->base_x = col_x[col];
            item->base_y = col_y[col];
            item->cell_w = col_w[col];
            item->cell_h = GetItemTotalHeight(item);
            col_y[col] += item->cell_h + gap_y;
        }
    }
}

static void LayoutAdaptiveGrid(int cols, int rows, float pad_x, float gap_y) {
    float col_w[GRID_LAYOUT_MAX];
    int page;
    int max_page = CountItemPages() - 1;

    for(page = 0; page <= max_page; page++) {
        ComputeAdaptiveColWidths(page, cols, col_w);
        FitPageColWidths(cols, pad_x, col_w);
        ApplyAdaptivePageGrid(page, cols, rows, pad_x, gap_y, col_w);
    }
}

static void LayoutAppItems(void) {
    int cols;
    int rows;
    float pad_x;
    float avail_h;

    cols = self.config.grid_cols;
    rows = self.config.grid_rows;
    pad_x = (float)self.config.cell_pad_x;
    avail_h = (float)self.panel_h - (float)self.config.grid_top - (float)self.config.grid_bottom;

    if(cols < 1) {
        cols = 1;
    }

    if(cols > GRID_LAYOUT_MAX) {
        cols = GRID_LAYOUT_MAX;
    }

    if(rows < 1) {
        rows = 1;
    }

    if(rows > GRID_LAYOUT_MAX) {
        rows = GRID_LAYOUT_MAX;
    }

    LayoutScriptGridItems(cols, rows, (float)self.config.cell_pad_y, avail_h);
    LayoutAdaptiveGrid(cols, rows, pad_x, (float)self.config.cell_pad_y);
    LayoutCenter512Icons(avail_h);

    self.pages = CountItemPages();

    if(self.pages < 1) {
        self.pages = 1;
    }
}

static Texture *LoadIconFromPath(const char *path, int warn_on_fail) {
    Texture *texture;

    if(path == NULL || !FileExists(path)) {
        return NULL;
    }

    texture = TSU_TextureCreateFromFile(path, true, false, 0);

    if(texture == NULL || TSU_TextureGetW(texture) < 1 || TSU_TextureGetH(texture) < 1) {
        if(texture != NULL) {
            TSU_TextureDestroy(&texture);
            texture = NULL;
        }

        if(warn_on_fail) {
            ds_printf("DS_WARNING: Launch app: can't load icon '%s'\n", path);
        }

        return NULL;
    }

    return texture;
}

static Texture *LoadScriptIconTexture(const char *basename, int is_lua) {
    static const char *icon_exts[] = {
        "png",
        "pvr",
        NULL
    };
    char path[NAME_MAX];
    Texture *texture;
    int i;

    for(i = 0; icon_exts[i] != NULL; i++) {
        snprintf(path, NAME_MAX, "%s/images/%s.%s",
            self.app_path, basename, icon_exts[i]);
        texture = LoadIconFromPath(path, 1);

        if(texture != NULL) {
            return texture;
        }
    }

    snprintf(path, NAME_MAX, "%s/gui/icons/normal/%s.png",
        getenv("PATH"), is_lua ? "lua" : "script");

    return LoadIconFromPath(path, 0);
}

static void AddItemFromTexture(const char *name, Texture *texture,
                        launch_item_type_t type,
                        int app_id,
                        script_item_t *script) {

    launch_item_t *item;
    float icon_w;
    float icon_h;
    float z;
    static Color label_color = {1.0f, 1.0f, 1.0f, 1.0f};

    if(!EnsureItemCapacity(self.item_count + 1)) {
        if(texture != NULL) {
            TSU_TextureDestroy(&texture);
        }
        if(script != NULL) {
            free(script);
        }
        return;
    }

    item = &self.items[self.item_count];

    if(texture == NULL) {
        if(script != NULL) {
            free(script);
        }
        memset(item, 0, sizeof(*item));
        return;
    }

    icon_w = (float)TSU_TextureGetW(texture);
    icon_h = (float)TSU_TextureGetH(texture);

    if(type == LAUNCH_ITEM_APP) {
        item->grid_page = self.layout_page;
        item->grid_col = self.layout_slot % self.config.grid_cols;
        item->grid_row = self.layout_slot / self.config.grid_cols;
    }
    else {
        item->grid_page = 0;
        item->grid_col = 0;
        item->grid_row = 0;
    }

    item->type = type;
    item->app_id = app_id;
    item->script = script;
    item->icon_w = icon_w;
    item->icon_h = icon_h;
    item->base_x = 0.0f;
    item->base_y = 0.0f;
    item->sway_phase = (float)(self.item_count + 1) * 1.37f;
    item->sway_speed = 1.4f + (float)(self.item_count % 5) * 0.15f;
    item->sway_start_t = (float)timer_ms_gettime64();
    z = 72.0f + (float)(self.item_count % 12) * 1.5f;
    item->z = z;

    if(name) {
        strncpy(item->name, name, sizeof(item->name) - 1);
        item->name[sizeof(item->name) - 1] = '\0';
    }

    item->banner = CreateIconBanner(texture,
        icon_w + (float)self.config.icon_highlight_padding,
        icon_h + (float)self.config.icon_highlight_padding,
        z,
        self.item_count);

    if(item->banner == NULL) {
        if(texture != NULL) {
            TSU_TextureDestroy(&texture);
        }
        if(script != NULL) {
            free(script);
        }
        memset(item, 0, sizeof(*item));
        return;
    }

    TSU_AppSubAddBanner(self.app->tsunami, item->banner);

    if(name) {
        item->label = TSU_LabelCreate(self.caption_font, name, self.config.icon_label_size, false, false, false);

        if(item->label != NULL) {
            TSU_LabelSetCenter(item->label, true);
            TSU_LabelSetTint(item->label, &label_color);
            TSU_DrawableSetId((Drawable *)item->label, self.item_count);
            TSU_AppSubAddLabel(self.app->tsunami, item->label);
        }
    }

    self.item_count++;

    if(type == LAUNCH_ITEM_APP) {
        AdvanceLayoutSlot();
    }
}

void BuildAppList(void) {
    file_t fd;
    const dirent_t *ent;
    char path[NAME_MAX];
    int elen;
    int type;
    App_t *app;
    Item_list_t *applist = GetAppList();
    if(applist != NULL) {
        Item_t *item = listGetItemFirst(applist);

        while(item != NULL) {
            app = (App_t *)item->data;

            if(self.app == NULL || app->id != self.app->id) {
                AddItemFromTexture(app->name, LoadIconFromPath(app->icon, 0),
                    LAUNCH_ITEM_APP, app->id, NULL);
            }
            item = listGetItemNext(item);
        }
    }

    snprintf(path, NAME_MAX, "%s/scripts", self.app_path);
    fd = fs_open(path, O_RDONLY | O_DIR);

    if(fd == FILEHND_INVALID) {
        LayoutAppItems();
        return;
    }

    while((ent = fs_readdir(fd)) != NULL) {
        if(ent->name[0] == '.') {
            continue;
        }

        elen = strlen(ent->name);
        type = elen > 3 ? ent->name[elen - 3] : 'd';

        if(!ent->attr && (type == 'l' || type == 'd')) {
            script_item_t *si = (script_item_t *)calloc(1, sizeof(script_item_t));

            if(si == NULL) {
                break;
            }

            snprintf(si->file, NAME_MAX, "%s/scripts/%s", self.app_path, ent->name);

            elen -= 4;

            if(elen >= (int)sizeof(si->name)) {
                elen = sizeof(si->name) - 1;
            }

            strncpy(si->name, ent->name, elen);
            si->name[elen] = '\0';

            AddItemFromTexture((si->name[0] != '_' ? si->name : NULL),
                LoadScriptIconTexture(si->name, type == 'l'),
                LAUNCH_ITEM_SCRIPT, 0, si);
        }
    }

    fs_close(fd);
    LayoutAppItems();
}

void ResetAppListState(void) {
    self.layout_slot = 0;
    self.layout_page = 0;
    self.cur_x = 0;
    self.scroll_x_display = 0.0f;
    self.slide_active = 0;
    self.pages = 1;
    self.slide_focus_index = -1;
}

void ResetItemsSwayStart(void) {
    int i;
    float now = (float)timer_ms_gettime64();

    for(i = 0; i < self.item_count; i++) {
        self.items[i].sway_start_t = now;
    }
}

void RebuildAppList(void) {
    int old_x = self.cur_x;
    int max_x;

    ClearAllItems();
    ResetAppListState();
    BuildAppList();

    max_x = (self.pages - 1) * self.panel_w;

    if(max_x < 0) {
        max_x = 0;
    }

    if(old_x > max_x) {
        old_x = max_x;
    }

    if(old_x < 0) {
        old_x = 0;
    }

    self.cur_x = old_x;
    self.scroll_x_display = (float)old_x;
    self.slide_active = 0;
}

static int DeleteShortcutFiles(const char *file, const char *name) {
    static const char *icon_exts[] = {
        "png",
        "pvr",
        NULL
    };
    char icon_path[NAME_MAX];
    int i;

    if(!file || !name || !name[0]) {
        return 0;
    }

    if(fs_unlink(file) < 0) {
        ds_printf("DS_ERROR: Can't delete shortcut script: %s\n", file);
        return 0;
    }

    for(i = 0; icon_exts[i]; i++) {
        snprintf(icon_path, sizeof(icon_path), "%s/images/%s.%s", self.app_path, name, icon_exts[i]);
        fs_unlink(icon_path);
    }

    return 1;
}

void ClearPendingDelete(void) {
    self.pending_delete_type = LAUNCH_DELETE_NONE;
    self.pending_app_id = 0;
    self.pending_app_name[0] = '\0';
    memset(&self.pending_shortcut, 0, sizeof(self.pending_shortcut));
}

int ItemCanDelete(const launch_item_t *item) {
    if(item == NULL) {
        return 0;
    }

    if(item->type == LAUNCH_ITEM_SCRIPT) {
        return item->script != NULL;
    }

    if(item->type == LAUNCH_ITEM_APP) {
        if(self.app != NULL && item->app_id == self.app->id) {
            return 0;
        }

        return item->app_id > 0;
    }

    return 0;
}

static int DeleteAppItem(App_t *app) {
    char app_dir[NAME_MAX];
    char *slash;

    if(app == NULL || !app->fn[0]) {
        return 0;
    }

    strncpy(app_dir, app->fn, sizeof(app_dir) - 1);
    app_dir[sizeof(app_dir) - 1] = '\0';

    slash = strrchr(app_dir, '/');

    if(slash == NULL) {
        return 0;
    }

    *slash = '\0';

    if(self.app != NULL && app->id == self.app->id) {
        return 0;
    }

    if(!RemoveApp(app)) {
        return 0;
    }

    if(!DirExists(app_dir)) {
        return 1;
    }

    if(!RemoveDirectory(app_dir, 0)) {
        ds_printf("DS_ERROR: Launch app: can't remove app directory '%s'\n", app_dir);
        return 0;
    }

    return 1;
}

int DeletePendingItem(void) {
    App_t *app;

    if(self.pending_delete_type == LAUNCH_DELETE_SCRIPT) {
        if(!self.pending_shortcut.file[0]) {
            return 0;
        }

        return DeleteShortcutFiles(self.pending_shortcut.file, self.pending_shortcut.name);
    }

    if(self.pending_delete_type == LAUNCH_DELETE_APP) {
        app = GetAppById(self.pending_app_id);

        if(app == NULL) {
            return 0;
        }

        return DeleteAppItem(app);
    }

    return 0;
}

void ShowItemDelete(int item_index) {
    launch_item_t *item;
    char body[256];

    if(self.delete_dialog == NULL) {
        return;
    }

    if(item_index < 0 || item_index >= self.item_count) {
        return;
    }

    item = &self.items[item_index];

    if(!ItemCanDelete(item)) {
        return;
    }

    if(item->type == LAUNCH_ITEM_SCRIPT) {
        self.pending_delete_type = LAUNCH_DELETE_SCRIPT;
        self.pending_app_id = 0;
        self.pending_app_name[0] = '\0';
        memcpy(&self.pending_shortcut, item->script, sizeof(self.pending_shortcut));
        snprintf(body, sizeof(body), "Delete shortcut %s?", self.pending_shortcut.name);
    }
    else {
        self.pending_delete_type = LAUNCH_DELETE_APP;
        self.pending_app_id = item->app_id;
        memset(&self.pending_shortcut, 0, sizeof(self.pending_shortcut));
        strncpy(self.pending_app_name, item->name, sizeof(self.pending_app_name) - 1);
        self.pending_app_name[sizeof(self.pending_app_name) - 1] = '\0';
        snprintf(body, sizeof(body), "Delete app %s?", self.pending_app_name);
    }

    self.pending_activate_index = -1;
    TSU_DialogShow(self.delete_dialog, body);
    TSU_DialogSetFocus(self.delete_dialog, 1);
}

static int GetItemDeleteIndexAt(int mx, int my, int prefer_focus) {
    int idx;

    if(prefer_focus &&
        self.focused_index >= 0 &&
        ItemCanDelete(&self.items[self.focused_index]) &&
        ItemVisibleOnPage(self.focused_index)) {
        return self.focused_index;
    }

    idx = HitTestItem(mx, my);

    if(idx >= 0 && ItemCanDelete(&self.items[idx])) {
        return idx;
    }

    if(!prefer_focus &&
        self.focused_index >= 0 &&
        ItemCanDelete(&self.items[self.focused_index]) &&
        ItemVisibleOnPage(self.focused_index)) {
        return self.focused_index;
    }

    return -1;
}

void RequestItemDelete(int mx, int my, int prefer_focus) {
    int idx;

    if(self.delete_dialog == NULL) {
        return;
    }

    idx = GetItemDeleteIndexAt(mx, my, prefer_focus);

    if(idx >= 0) {
        ShowItemDelete(idx);
    }
}
