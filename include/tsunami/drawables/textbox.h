/*
   Tsunami for KallistiOS ##version##

   textbox.h

   Copyright (C) 2024-2026 Maniac Vera

*/

#ifndef __TSUNAMI_DRW_TEXTBOX_H
#define __TSUNAMI_DRW_TEXTBOX_H

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

enum CharDirectionEnum
{
	CDE_LEFT = 1,
	CDE_RIGHT = 2,
	CDE_UP = 3,
	CDE_DOWN = 4
};

#define LETTER_SIZE_ARRAY 26
#define LETTER_SIZE_ARRAY 26
#define NUMBER_SIZE_ARRAY 10
#define SYMBOL_SIZE_ARRAY 31

enum CharTypeEnum
{
	CT_LETTER = 1,
	CT_CAP_LETTER = 2,
	CT_NUMBER = 3,
	CT_SYMBOL = 4
};

class TextBox : public Drawable
{
private:
	const float m_border_width = 2;
	const float m_padding_width = 0;
	const float m_padding_height = 6;
	const char m_letter_array[LETTER_SIZE_ARRAY] = { 'a', 'b', 'c', 'd', 'e', 'f', 'g', 'h', 'i', 'j', 'k', 'l', 'm', 'n', 'o', 'p', 'q', 'r', 's', 't', 'u', 'v', 'w', 'x', 'y', 'z' };
	const char m_cap_letter_array[LETTER_SIZE_ARRAY] = { 'A', 'B', 'C', 'D', 'E', 'F', 'G', 'H', 'I', 'J', 'K', 'L', 'M', 'N', 'O', 'P', 'Q', 'R', 'S', 'T', 'U', 'V', 'W', 'X', 'Y', 'Z' };
	const char m_number_array[NUMBER_SIZE_ARRAY] = { '0', '1', '2', '3', '4', '5', '6', '7', '8', '9' };	
	const char m_symbol_array[SYMBOL_SIZE_ARRAY] = { '.', ',', ';', ':', '<', '>', '[', ']', '(', ')', '{', '}', '\"', '\'', '+', '-', '*', '=', '@', '\\', '|', '?', '/', '#', '~', '!', '`', '$', '&', '^', '%' };
	const char *m_chars_type_ptr;
	uint8 m_enable_chars_type;

	Font *m_display_font;
	int  m_char_index, m_char_type_index, m_chars_type_size, m_chars_type_displacement;
	int m_chars_type;
	std::string m_text;	

	uint8 checkCharType(char character);
	void setTextCursor();
	void moveToPreviousPlace();
	void moveToNextPlace();
	char previousCharType();
	char nextCharType();
	void moveString(int direction);
	char getChar(int index);
	void deleteChar(int index);
	void changeCharsType();
	void jumpCharType(int displacement);
	char getCharType(int index);
	int findCharIndex(char character);
	void fixPositionCharIndex();
	std::string putChar(char new_char);
	std::string changeChar(int index, char new_char);
	std::string insertChar(int index, char new_char);

	Label *m_display_label;
	Rectangle *m_control_rectangle, *m_text_cursor;
	Color m_body_color;

public:
	TextBox(Font *font_ptr, uint text_size, bool centered, float width, float height, const Color &body_color, bool enable_chars_type_letter, bool enable_chars_type_cap_letter, 
		bool enable_chars_type_number, bool enable_chars_type_symbol);

	virtual ~TextBox();

	void setFocus(bool focus);
	void inputEvent(int event_type, int key);
	void setCursor(Drawable *drawable);
	void setPosition(float x, float y);
	void setDisplayText(const std::string &text);
	const std::string getText();
	void clearText();
	void setIndex(int index);
	int getIndex();
	std::string trimText(const std::string &str);
	void setText(const std::string &text);
};

#else

typedef struct textbox TextBox;

#endif

#ifdef __cplusplus
extern "C"
{
#endif

	TextBox* TSU_TextBoxCreate(Font *display_font, uint text_size, bool centered, float width, float height, Color *body_color, bool enable_chars_type_letter, bool enable_chars_type_cap_letter, 
			bool enable_chars_type_number, bool enable_chars_type_symbol);
	void TSU_TextBoxDestroy(TextBox **textbox_ptr);
	void TSU_TextBoxSetStates(TextBox *textbox_ptr, int control_state, int previous_state);
	int TSU_TextBoxGetControlState(TextBox *textbox_ptr);
	void TSU_TextBoxSetWindowState(TextBox *textbox_ptr, int window_state);
	int TSU_TextBoxGetWindowState(TextBox *textbox_ptr);
	void TSU_TextBoxSetFocus(TextBox *textbox_ptr, bool focus);
	void TSU_TextBoxInputEvent(TextBox *textbox_ptr, int event_type, int key);
	const char* TSU_TextBoxGetText(TextBox *textbox_ptr);
	void TSU_TextBoxSetText(TextBox *textbox_ptr, const char *text);
	void TSU_TextBoxClear(TextBox *textbox_ptr);

#ifdef __cplusplus
};
#endif

#endif /* __TSUNAMI_DRW_TEXTBOX_H */
