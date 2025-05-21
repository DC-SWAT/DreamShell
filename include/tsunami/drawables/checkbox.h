/*
   Tsunami for KallistiOS ##version##

   form.h

   Copyright (C) 2024-2025 Maniac Vera

*/

#ifndef __TSUNAMI_DRW_CHECKBOX_H
#define __TSUNAMI_DRW_CHECKBOX_H

#include "../drawable.h"
#include "../font.h"
#include "../color.h"
#include "../animation.h"
#include "../anims/logxymover.h"
#include "label.h"
#include "rectangle.h"

#ifdef __cplusplus

#include <string>
#include <vector>


class CheckBox : public Drawable
{
private:
	float m_z_index;
	float m_border_width;
	float m_padding_width;
	float m_padding_height;
	Font *m_display_font;

	int m_option_selected;
	Label *m_display_label;
	Rectangle *m_control_rectangle, *m_rectangle;
	Color m_body_color;
	std::string m_on_text, m_off_text;

public:
	CheckBox(Font *display_font, uint text_size, float width, float height, const Color &body_color);
	CheckBox(Font *display_font, uint text_size, float width, float height, const Color &body_color, const char *on_text, const char *off_text);
	virtual ~CheckBox();

	void inputEvent(int event_type, int key);
	void setCursor(Drawable *drawable);
	void setSize(float width, float height);
	void setPosition(float x, float y);
	const std::string getText();
	int getValue();
	void setOn();
	void setOff();
};

#else

typedef struct checkbox CheckBox;

#endif

#ifdef __cplusplus
extern "C"
{
#endif

	CheckBox* TSU_CheckBoxCreate(Font *display_font, uint text_size, float width, float height, Color *body_color);
	CheckBox* TSU_CheckBoxCreateWithCustomText(Font *display_font, uint text_size, float width, float height, Color *body_color, const char *on_text, const char *off_text);
	void TSU_CheckBoxDestroy(CheckBox **checkbox_ptr);
	void TSU_CheckBoxInputEvent(CheckBox *checkbox_ptr, int event_type, int key);
	const char* TSU_CheckBoxGetText(CheckBox *checkbox_ptr);
	int TSU_CheckBoxGetValue(CheckBox *checkbox_ptr);
	void TSU_CheckBoxSetOn(CheckBox *checkbox_ptr);
	void TSU_CheckBoxSetOff(CheckBox *checkbox_ptr);

#ifdef __cplusplus
};
#endif

#endif /* __TSUNAMI_DRW_CHECKBOX_H */
