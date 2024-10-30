/*
   Tsunami for KallistiOS ##version##

   form.h

   Copyright (C) 2024 Maniac Vera

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
	float m_z_index = 0;
	float m_border_width = 2;
	float m_padding_width = 32;
	float m_padding_height = 6;
	Font *m_display_font;

	int m_option_selected;
	Label *m_display_label;
	Rectangle *m_control_rectangle, *m_rectangle;

public:
	CheckBox(Font *font_ptr, uint text_size, float width, float height);
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

	CheckBox* TSU_CheckBoxCreate(Font *display_font, uint text_size, float width, float height);
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
