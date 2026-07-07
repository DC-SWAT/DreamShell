/*
   Tsunami for KallistiOS ##version##

   dialog.h

   Copyright (C) 2026 SWAT

*/
#ifndef __TSUNAMI_DRW_DIALOG_H
#define __TSUNAMI_DRW_DIALOG_H

#include "../drawable.h"
#include "../font.h"
#include "../color.h"
#include "label.h"
#include "rectangle.h"

#ifdef __cplusplus

#include <string>

class Dialog : public Drawable
{
private:
	float m_screen_w;
	float m_screen_h;
	float m_box_w;
	float m_box_h;
	float m_panel_x;
	float m_panel_y;
	float m_bg_z;
	float m_msg_z;
	uint m_text_size;
	bool m_visible;
	int m_focus_btn;
	std::string m_confirm_text;
	std::string m_cancel_text;
	Color m_text_color;
	Color m_btn_color;
	Color m_btn_dim_color;
	Color m_btn_focus_color;
	Rectangle *m_overlay;
	Rectangle *m_panel;
	Rectangle *m_confirm_bg;
	Rectangle *m_cancel_bg;
	Label *m_msg;
	Label *m_confirm;
	Label *m_cancel;

	void updateLayout();
	void updateButtonFocus();
	void setVisible(bool visible);

public:
	Dialog(Font *display_font, float screen_w, float screen_h, const Color &overlay_color, const Color &text_color, const Color &btn_color);
	Dialog(Font *display_font, float screen_w, float screen_h, float box_w, float box_h, const Color &overlay_color, const Color &panel_color, const Color &text_color, const Color &btn_color, const char *confirm_text, const char *cancel_text, uint text_size);
	virtual ~Dialog();

	void show(const char *text);
	void hide();
	bool isVisible() const;
	void setConfirmText(const char *text);
	void setCancelText(const char *text);
	void setConfirmCallback(ClickEventFunctionPtr callback, int callback_id = 0);
	void setCancelCallback(ClickEventFunctionPtr callback, int callback_id = 0);
	void setFocus(int button);
	void moveFocus(int dir);
	void activateFocused();
	void handleClick(int mx, int my);
	virtual void draw(pvr_list_type_t list);
};

#else

typedef struct dialog Dialog;

#endif

#ifdef __cplusplus
extern "C"
{
#endif

	Dialog* TSU_DialogCreate(Font *display_font, float screen_w, float screen_h, Color *bg_color, Color *text_color, Color *btn_color);
	Dialog* TSU_DialogCreateEx(Font *display_font, float screen_w, float screen_h, float box_w, float box_h, Color *overlay_color, Color *panel_color, Color *text_color, Color *btn_color, const char *confirm_text, const char *cancel_text, uint text_size);
	void TSU_DialogDestroy(Dialog **dialog_ptr);
	void TSU_DialogSetConfirmText(Dialog *dialog_ptr, const char *text);
	void TSU_DialogSetCancelText(Dialog *dialog_ptr, const char *text);
	void TSU_DialogSetConfirmCallback(Dialog *dialog_ptr, ClickEventFunctionPtr callback, int callback_id);
	void TSU_DialogSetCancelCallback(Dialog *dialog_ptr, ClickEventFunctionPtr callback, int callback_id);
	void TSU_DialogShow(Dialog *dialog_ptr, const char *text);
	void TSU_DialogHide(Dialog *dialog_ptr);
	int TSU_DialogIsVisible(Dialog *dialog_ptr);
	void TSU_DialogSetFocus(Dialog *dialog_ptr, int button);
	void TSU_DialogMoveFocus(Dialog *dialog_ptr, int dir);
	void TSU_DialogActivateFocused(Dialog *dialog_ptr);
	void TSU_DialogHandleClick(Dialog *dialog_ptr, int mx, int my);

#ifdef __cplusplus
};
#endif

#endif
