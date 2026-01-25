/*
   Tsunami for KallistiOS ##version##

   form.h

   Copyright (C) 2024-2026 Maniac Vera

*/

#ifndef __TSUNAMI_DRW_FORM_H
#define __TSUNAMI_DRW_FORM_H

#include "../sound.h"
#include "../drawable.h"
#include "../font.h"
#include "../color.h"
#include "../genmenu.h"
#include "../animation.h"
#include "../anims/logxymover.h"
#include "rectangle.h"
#include "box.h"
#include "triangle.h"
#include "scene.h"
#include "label.h"
#include "banner.h"
#include "itemmenu.h"
#include "optiongroup.h"
#include "combobox.h"
#include "checkbox.h"
#include "textbox.h"

typedef void (*SelectedEventPtr) (Drawable *drawable, uint bottom_index, uint column, uint row);
typedef void (*ViewIndexChangedEventPtr) (Drawable *form_ptr, int view_index);
typedef void (*GetObjectsCurrentViewEventPtr) (uint loop_index, int id, Drawable *drawable, uint type, uint row, uint column, int view_index);

#ifdef __cplusplus

#include <filesystem>

typedef struct ObjectStructure
{
	Drawable *object;
	uint type;
	uint column;
	uint row;
	int view_index;
} ObjectStruct;

typedef struct AttributeStructure
{
	uint width;
	uint height;
	uint span;
	int view_index;
} AttributeStruct;

enum SearchDirectionEnum
{
	SDE_LEFT = 1,
	SDE_RIGHT = 2,
	SDE_UP = 3,
	SDE_DOWN = 4
};

class Form : public Drawable
{
private:
	const float column_padding_left = 6;
	const float cursor_padding_left = 2;
	bool m_is_popup, m_enable, m_popup, m_visible_title, m_visible_bottom, m_cursor_animation_enable;
	uint m_width, m_height, m_zIndex, m_radius;
	uint m_body_height, m_title_height, m_bottom_height;
	uint m_columns_size, m_rows_size;
	int m_x, m_y;
	int m_current_view_index;
	int m_sfx_volume;
	
	ItemMenu *m_close_button;
	Sound *m_sfx_cursor, *m_sfx_click;
	Color m_body_color, m_title_background_color, m_bottom_background_color, m_border_color, m_background_color;
	Rectangle *m_background_rectangle, *m_title_rectangle, *m_body_rectangle, *m_bottom_rectangle, *m_cursor, *m_bottom_cursor;
	Box *m_main_box;
	Vector m_selector_translate, m_bottom_selector_translate;
	
	LogXYMover *m_cursor_animation, *m_bottom_cursor_animation;
	Font *m_title_font;
	Label *m_title_label;
	std::deque<ObjectStruct *> m_objects;
	std::deque<ObjectStruct *> m_bottom_objects;

	AttributeStruct *m_columns_attributes, *m_rows_attributes;
	uint m_number_columns, m_number_rows;
	uint m_current_column, m_current_row;
	Drawable *m_current_object_selected;
	SelectedEventPtr selected_event;
	
	ViewIndexChangedEventPtr view_index_changed_event;
	GetObjectsCurrentViewEventPtr get_objects_current_view_event;

	void createForm();
	Drawable* findNextNearestObject(int direction);
	Drawable* findNextObject(int direction, bool start);
	void freeObject(ObjectStruct *object_ptr);

public:
	Form(int x, int y, uint width, uint height, bool is_popup, int z_index, int radius, bool visible_title, bool visible_bottom, Font *title_font,
			const Color &border_color, const Color &title_background_color, const Color &body_color, const Color &bottom_background_color,
			ViewIndexChangedEventPtr view_index_changed_event);
	virtual ~Form();

	void inputEvent(int event_type, int key);
	void setCloseButton(const std::string &image, ClickEventFunctionPtr click_func_ptr, OnMouseOverEventFunctionPtr mouse_over_func_ptr);
	void setViewIndex(int view_index);
	void setAttributes(uint number_columns, uint number_rows, uint columns_size, uint rows_size);
	void setCursor(Drawable *drawable);
	void setCursorSize(float width, float height);
	void setTitle(const std::string &text);
	void setSize(float width, float height);
	void setPosition(float x, float y);
	void setColumnSize(uint column_number, float width);
	void setColumnSpan(uint column_number, uint column_span);
	void setRowSize(uint row_number, float height);
	void setRowSpan(uint row_number, uint row_span);
	void addBodyObject(Drawable *drawable_ptr, uint object_type, uint column_number, uint row_number); // SAVE OBJECT TYPE

	void addBodyScene(Scene *scene_ptr, uint column_number, uint row_number);
	void addBodyForm(Form *form_ptr, uint column_number, uint row_number);
	void addBodyBanner(Banner *banner_ptr, uint column_number, uint row_number);
	void addBodyRectangle(Rectangle *banner_ptr, uint column_number, uint row_number);
	void addBodyLabel(Label *label_ptr, uint column_number, uint row_number);
	void addBodyItemMenu(ItemMenu *itemmenu_ptr, uint column_number, uint row_number);
	void addBodyOptionGroup(OptionGroup *optiongroup_ptr, uint column_number, uint row_number);
	void addBodyTextBox(TextBox *textbox_ptr, uint column_number, uint row_number);
	void addBodyComboBox(ComboBox *combobox_ptr, uint column_number, uint row_number);
	void addBodyCheckBox(CheckBox *checkbox_ptr, uint column_number, uint row_number);
	Drawable* getBodyObject(uint column_number, uint row_number);
	Vector getPositionXY(uint column_number, uint row_number);
	Vector getBottomPositionXY(uint index);

	void removeBodyObject(Drawable *object_ptr);
	void removeBodyScene(Scene *scene_ptr);
	void removeBodyForm(Form *form_ptr);
	void removeBodyBanner(Banner *banner_ptr);
	void removeBodyRectangle(Rectangle *rectangle_ptr);
	void removeBodyLabel(Label *label_ptr);
	void removeBodyItemMenu(ItemMenu *itemmenu_ptr);
	void removeBodyOptionGroup(OptionGroup *optiongroup_ptr);
	void removeBodyTextBox(TextBox *textbox_ptr);
	void removeBodyComboBox(ComboBox *combobox_ptr);
	void removeBodyCheckBox(CheckBox *checkbox_ptr);

	void clearObjects();
	void clearBottomObjects();

	void selectedEvent(SelectedEventPtr func_ptr);
	void getObjectsCurrentViewEvent(GetObjectsCurrentViewEventPtr func_ptr);

	void addBottomObject(Drawable *drawable_ptr, uint object_type);
	Drawable* getBottomObject(uint index);
	void removeBottomObject(Drawable *object_ptr);

	void addBottomBanner(Banner *banner_ptr);
	void addBottomLabel(Label* label_ptr);
	void removeBottomBanner(Banner *banner_ptr);
	void removeBottomLabel(Label *label_ptr);
	Banner* getBottomBanner(uint index);
	Label* getBottomLabel(uint index);
	void show();
	void hide();
	uint getState();		  // SHOW|HIDEN, ENABLE|DISABLE
	void enable(bool enable); // ANOTHER VERSION
	bool isEnable();
	Font* getTitleFont();

	void onViewIndexChangedEvent();
	void onGetObjectsCurrentViewEvent();
	void setSfxClick(const std::filesystem::path &fn);
	void setSfxCursor(const std::filesystem::path &fn);
	void setSfxVolume(int volume);
};

#else

typedef struct form Form;

#endif

#ifdef __cplusplus
extern "C"
{
#endif

	Form *TSU_FormCreate(int x, int y, uint width, uint height, bool is_popup, int z_index, int radius, 
						bool visible_title, bool visible_bottom, Font *title_font,
						Color *border_color, Color *title_background_color, Color *body_color, Color *bottom_background_color,
						ViewIndexChangedEventPtr view_index_changed_event_ptr);

	void TSU_FormSetCloseButton(Form *form_ptr, const char *image, ClickEventFunctionPtr click_func_ptr, OnMouseOverEventFunctionPtr mouse_over_func_ptr);
	void TSU_FormSetViewIndex(Form *form_ptr, int view_index);
	void TSU_FormSetAttributes(Form *form_ptr, uint number_columns, uint number_rows, uint columns_size, uint rows_size);	
	void TSU_FormSetSfxClick(Form *form_ptr, const char *file);
	void TSU_FormSetSfxCursor(Form *form_ptr, const char *file);
	void TSU_FormSetSfxVolume(Form *form_ptr, int volume);
	void TSU_FormRemove(Form *form_ptr);
	void TSU_FormDestroy(Form **form_ptr);
	void TSU_FormInputEvent(Form *form_ptr, int event_type, int key);
	void TSU_FormSelectedEvent(Form *form_ptr, SelectedEventPtr func_ptr);
	void TSU_FormGetObjectsCurrentViewEvent(Form *form_ptr, GetObjectsCurrentViewEventPtr func_ptr);
	void TSU_FormOnGetObjectsCurrentView(Form *form_ptr);
	Font* TSU_FormGetTitleFont(Form *form_ptr);

	void TSU_FormSetTitle(Form *form_ptr, const char *text);
	void TSU_FormSetSize(Form *form_ptr, float width, float height);
	void TSU_FormSetPosition(Form *form_ptr, float x, float y);
	void TSU_FormSetColumnSize(Form *form_ptr, uint column_number, float width);
	void TSU_FormSetColumnSpan(Form *form_ptr, uint column_number, uint column_span);
	void TSU_FormSetRowSize(Form *form_ptr, uint row_number, float height);
	void TSU_FormSetRowSpan(Form *form_ptr, uint row_number, uint row_span);

	void TSU_FormAddBodyScene(Form *form_ptr, Scene *scene_ptr, uint column_number, uint row_number);
	void TSU_FormAddBodyForm(Form *form_ptr, Form *new_form_ptr, uint column_number, uint row_number);
	void TSU_FormAddBodyBanner(Form *form_ptr, Banner *banner_ptr, uint column_number, uint row_number);
	void TSU_FormAddBodyRectangle(Form *form_ptr, Rectangle *rectangle_ptr, uint column_number, uint row_number);
	void TSU_FormAddBodyLabel(Form *form_ptr, Label *label_ptr, uint column_number, uint row_number);
	void TSU_FormAddBodyItemMenu(Form *form_ptr, ItemMenu *itemmenu_ptr, uint column_number, uint row_number);
	void TSU_FormAddBodyOptionGroup(Form *form_ptr, OptionGroup *optiongroup, uint column_number, uint row_number);
	void TSU_FormAddBodyTextBox(Form *form_ptr, TextBox *textbox_ptr, uint column_number, uint row_number);
	void TSU_FormAddBodyComboBox(Form *form_ptr, ComboBox *combobox_ptr, uint column_number, uint row_number);
	void TSU_FormAddBodyCheckBox(Form *form_ptr, CheckBox *checkbox_ptr, uint column_number, uint row_number);
	Drawable *TSU_FormGetBodyObject(Form *form_ptr, uint column_number, uint row_number);

	void TSU_FormRemoveBodyScene(Form *form_ptr, Scene *scene_ptr);
	void TSU_FormRemoveBodyForm(Form *form_ptr, Form *remove_form_ptr);
	void TSU_FormRemoveBodyBanner(Form *form_ptr, Banner *banner_ptr);
	void TSU_FormRemoveBodyRectangle(Form *form_ptr, Rectangle *rectangle_ptr);
	void TSU_FormRemoveBodyLabel(Form *form_ptr, Label *label_ptr);
	void TSU_FormRemoveBodyItemMenu(Form *form_ptr, ItemMenu *itemmenu_ptr);
	void TSU_FormRemoveBodyOptionGroup(Form *form_ptr, OptionGroup *optiongroup_ptr);
	void TSU_FormRemoveBodyTextBox(Form *form_ptr, TextBox *textbox_ptr);
	void TSU_FormRemoveBodyComboBox(Form *form_ptr, ComboBox *combobox_ptr);
	void TSU_FormRemoveBodyCheckBox(Form *form_ptr, CheckBox *checkbox_ptr);

	void TSU_FormAddBottomBanner(Form *form_ptr, Banner *banner_ptr);
	Banner *TSU_FormGetBottomBanner(Form *form_ptr, uint index);
	void TSU_FormAddBottomLabel(Form *form_ptr, Label *label_ptr);
	Label *TSU_FormGetBottomLabel(Form *form_ptr, uint index);

	void TSU_FormRemoveBottomBanner(Form *form_ptr, Banner *banner_ptr);
	void TSU_FormRemoveBottomLabel(Form *form_ptr, Label *label_ptr);

	void TSU_FormShow(Form *form_ptr);
	void TSU_FormHide(Form *form_ptr);
	uint TSU_FormGetState(Form *form_ptr);			  // SHOW|HIDEN, ENABLE|DISABLE
	void TSU_FormEnable(Form *form_ptr, bool enable); // ANOTHER VERSION
	void TSU_FormIsEnable(Form *form_ptr);
	void TSU_FormSetCursor(Form *form_ptr, Drawable *drawable_ptr);
	void TSU_FormSetCursorSize(Form *form_ptr, float width, float height);
	void TSU_FormClearBodyObjects(Form *form_ptr);
	int TSU_FormGetWindowState(Form *form_ptr);
	void TSU_FormSetWindowState(Form *form_ptr, int window_state);

#ifdef __cplusplus
};
#endif

#endif /* __TSUNAMI_DRW_FORM_H */
