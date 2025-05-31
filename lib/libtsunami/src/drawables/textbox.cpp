/*
   Tsunami for KallistiOS ##version##

   textbox.cpp

   Copyright (C) 2024-2025 Maniac Vera

*/

#include "drawables/textbox.h"
#include "genmenu.h"
#include "tsudefinition.h"
#include <algorithm>
#include <cstring>
#include <string>

TextBox::TextBox(Font *display_font, uint text_size, bool centered, float width, float height, const Color &body_color, bool enable_chars_type_letter, bool enable_chars_type_cap_letter, 
		bool enable_chars_type_number, bool enable_chars_type_symbol) {

	setObjectType(ObjectTypeEnum::TEXTBOX_TYPE);
	m_display_font = display_font;
	m_width = width + m_padding_width + m_border_width + 2;
	m_height = height + m_padding_height + m_border_width;
	m_char_index = -1;
	m_display_label = nullptr;
	m_change_state = nullptr;
	m_chars_type = 0;
	m_chars_type_ptr = nullptr;
	m_enable_chars_type = 0;
	m_char_type_index = -1;
	m_text_cursor = nullptr;
	m_control_rectangle = nullptr;
	m_body_color = body_color.a ? body_color : Color(DEFAULT_BODY_COLOR);

	if (!enable_chars_type_letter && !enable_chars_type_cap_letter && !enable_chars_type_number && !enable_chars_type_symbol) {
		m_enable_chars_type = (1<<CharTypeEnum::CT_LETTER) | (1<<CharTypeEnum::CT_CAP_LETTER);
	}
	else {
		if (enable_chars_type_letter) {
			m_enable_chars_type |= (1<<CharTypeEnum::CT_LETTER);
		}

		if (enable_chars_type_cap_letter) {
			m_enable_chars_type |= (1<<CharTypeEnum::CT_CAP_LETTER);
		}

		if (enable_chars_type_number) {
			m_enable_chars_type |= (1<<CharTypeEnum::CT_NUMBER);
		}

		if (enable_chars_type_symbol) {
			m_enable_chars_type |= (1<<CharTypeEnum::CT_SYMBOL);
		}
	}

	if (text_size == 0) {
		text_size = 20;
	}

	if (display_font) {
		Color border_color = {1, 1.0f, 1.0f, 1.0f};
		float z_index = 0;
		float radius = 0;

		Vector position = this->getTranslate();
		m_control_rectangle = new Rectangle (PVR_LIST_OP_POLY, position.x - width/2 - m_padding_width/2 + 1, position.y + height/2 + m_padding_height, width + m_padding_width, height + m_padding_height, m_body_color, z_index, m_border_width, border_color, radius);
		this->subAdd(m_control_rectangle);

		m_display_label = new Label(display_font, "", text_size, centered, false);
		this->subAdd(m_display_label);
		
		Vector control_rectangle_vector = m_control_rectangle->getTranslate();
		Vector display_label_control = m_display_label->getTranslate();

		control_rectangle_vector.x += (width/2 + m_border_width);
		control_rectangle_vector.y -= m_border_width*2;
		m_control_rectangle->setTranslate(control_rectangle_vector);

		if (centered) {
			display_label_control.x += (m_padding_width + width/2 + m_border_width);
			display_label_control.y -= 3;			
		}
		else {
			display_label_control.x += (m_padding_width + m_border_width + 2);
			display_label_control.y += height/2;
			display_label_control.y -= 3;
		}
		display_label_control.z = z_index + 1;
		
		m_display_label->setTranslate(display_label_control);
		changeCharsType();
	}
}

TextBox::~TextBox() {
	clearText();
	
	if (m_display_label != nullptr) {
		m_display_label->setFinished();
	}

	if (m_control_rectangle != nullptr) {
		m_control_rectangle->setFinished();
	}

	if (m_text_cursor != nullptr) {
		m_text_cursor->setFinished();
	}

	this->subRemoveFinished();

	if (m_display_label != nullptr) {
		delete m_display_label;
		m_display_label = nullptr;
	}

	if (m_control_rectangle != nullptr) {
		delete m_control_rectangle;
		m_control_rectangle = nullptr;
	}

	if (m_text_cursor != nullptr) {
		delete m_text_cursor;
		m_text_cursor = nullptr;
	}

	m_display_font = nullptr;
	m_change_state = nullptr;
}

void TextBox::setStates(int control_state, int previous_state, int *change_state) {
	m_previous_state = previous_state;
	m_change_state = change_state;
	m_control_state = control_state;
}

int TextBox::getControlState() {
	return m_control_state;
}

void TextBox::setFocus(bool focus) {
	if (focus) {
		*m_change_state = m_control_state;
		Color text_color = {1, 1.0f, 1.0f, 0.1f};
		m_display_label->setTint(text_color);
		
		setIndex(m_text.length());
		fixPositionCharIndex();
		setTextCursor();
	}
	else {		
		*m_change_state = m_previous_state;

		if (m_text_cursor != nullptr) {
			this->subRemove(m_text_cursor);
			delete m_text_cursor;
			m_text_cursor = nullptr;
		}

		m_text = trimText(m_text);

		bool whiteSpacesOnly = std::all_of(m_text.begin(),m_text.end(),isspace);
		if (whiteSpacesOnly) {
			m_text = "";			
		}

		setDisplayText(m_text);

		Color text_color = {1, 1.0f, 1.0f, 1.0f};
		m_display_label->setTint(text_color);
	}
}

std::string TextBox::trimText(const std::string &str) {
    size_t first = str.find_first_not_of(' ');
    if (std::string::npos == first)
    {
        return str;
    }
    size_t last = str.find_last_not_of(' ');
    return str.substr(first, (last - first + 1));
}

void TextBox::setTextCursor() {
	float w = 0, h = 0, tw = 0, th = 0, lw = 0, lh = 0;

	std::string dump_text_before = m_text.substr(0, m_char_index);
	std::string dump_text = m_text.substr(0, m_char_index+1);

	m_display_font->getTextSize(dump_text_before, &w, &h);

	if (dump_text.length() > 0) {
		m_display_font->getTextSize(dump_text.substr(dump_text.length() - 1, 1), &lw, &lh);
	}
	
	m_display_font->getTextSize(m_text, &tw, &th);

	float font_x = 0, font_y = 0;
	if (m_text_cursor != nullptr) {
		this->subRemove(m_text_cursor);
		delete m_text_cursor;
		m_text_cursor = nullptr;
	}

	Vector position = m_display_label->getTranslate();
	Vector control_position = m_control_rectangle->getTranslate();

	if (m_display_label->isCentered()) {
		font_x = (position.x - tw/2) + w;
	}
	else {
		font_x = (position.x) + w;
	}

	font_y = control_position.y - m_border_width/2;

	Color background_color = {0.4f, 0.90f, 0.90f, 0.90f};
	Color border_color = {1.0f, 1.0f, 1.0f, 1.0f};
	float border_width = 1.0f;
	int z_index = position.z;

	m_text_cursor = new Rectangle (PVR_LIST_TR_POLY, font_x - .5, font_y - 1, lw, m_height - (m_border_width*2 + 2), background_color, z_index, border_width, border_color, 0);
	this->subAdd(m_text_cursor);
}

void TextBox::changeCharsType() {
	
	bool changed = false;
	TRY_CHARS_TYPE_CHANGED:
	{
		if (m_chars_type> CharTypeEnum::CT_SYMBOL) {
			m_chars_type = 0;
		}

		m_chars_type++;
		if (m_chars_type == CharTypeEnum::CT_LETTER && m_enable_chars_type & (1<<CharTypeEnum::CT_LETTER))
		{
			m_chars_type_ptr = &m_letter_array[0];
			m_chars_type_size = LETTER_SIZE_ARRAY;
			m_chars_type_displacement = 4;
			changed = true;
		}
		else if (m_chars_type == CharTypeEnum::CT_CAP_LETTER && m_enable_chars_type & (1<<CharTypeEnum::CT_CAP_LETTER))
		{
			m_chars_type_ptr = &m_cap_letter_array[0];
			m_chars_type_size = LETTER_SIZE_ARRAY;
			m_chars_type_displacement = 4;
			changed = true;
		}
		else if (m_chars_type == CharTypeEnum::CT_NUMBER && m_enable_chars_type & (1<<CharTypeEnum::CT_NUMBER))
		{
			m_chars_type_ptr = &m_number_array[0];
			m_chars_type_size = NUMBER_SIZE_ARRAY;
			m_chars_type_displacement = 2;
			changed = true;
		}
		else if (m_chars_type == CharTypeEnum::CT_SYMBOL && m_enable_chars_type & (1<<CharTypeEnum::CT_SYMBOL))
		{
			m_chars_type_ptr = &m_symbol_array[0];
			m_chars_type_size = SYMBOL_SIZE_ARRAY;
			m_chars_type_displacement = 4;
			changed = true;
		}

		if (!changed) {
			goto TRY_CHARS_TYPE_CHANGED;
		}
	}
}

void TextBox::jumpCharType(int displacement) {
	if (m_char_type_index + displacement <= m_chars_type_size) {
		m_char_type_index += displacement;
	}
	else {
		m_char_type_index = 0;
	}
}

void TextBox::fixPositionCharIndex() {
	uint8 char_type = checkCharType(getChar(getIndex()));
	m_chars_type = char_type - 1;
	changeCharsType();

	int index = findCharIndex(getChar(getIndex()));
	if (index >= 0) {
		m_char_type_index = index;
	}
}

int TextBox::findCharIndex(char character) {
	int index = -1;

	for (int i = 0; i < m_chars_type_size; i++) {
		if ((m_chars_type_ptr)[i] == character) {
			index = i;
			break;
		}
	}

	return index;
}

char TextBox::getCharType(int index) {
	if (m_chars_type_ptr != nullptr) {
		return (m_chars_type_ptr)[index];
	}
	else {
		return 0;
	}
}

char TextBox::getChar(int index) {
	if (m_text.length() > 0) {
		return m_text[index];
	}
	else {
		return 0;
	}
}

std::string TextBox::putChar(char new_char) {
	m_text += new_char;
	setIndex(getIndex()+1);
	setDisplayText(m_text);

	return m_text;
}

std::string TextBox::insertChar(int index, char new_char) {
	if (index <= (int)m_text.length() - 1) {
		m_text.insert(index, 1, new_char);
	}
	setDisplayText(m_text);
	return m_text;
}

std::string TextBox::changeChar(int index, char new_char) {

	if (index == (int)m_text.length()) {
		putChar(new_char);
	}
	else if (index <= (int)m_text.length()) {
		m_text[index] = new_char;
		setDisplayText(m_text);
	}

	return m_text;
}

uint8 TextBox::checkCharType(char character) {
	uint char_type = 0;
	
	if (char_type == 0) {
		for (int i = 0; i < LETTER_SIZE_ARRAY; i++) {
			if (m_letter_array[i] == character) {
				char_type = CharTypeEnum::CT_LETTER;
				break;
			}
		}
	}
	
	if (char_type == 0) {
		for (int i = 0; i < LETTER_SIZE_ARRAY; i++) {
			if (m_cap_letter_array[i] == character) {
				char_type = CharTypeEnum::CT_CAP_LETTER;
				break;
			}
		}
	}

	if (char_type == 0) {
		for (int i = 0; i < NUMBER_SIZE_ARRAY; i++) {
			if (m_number_array[i] == character) {
				char_type = CharTypeEnum::CT_NUMBER;
				break;
			}
		}
	}

	if (char_type == 0) {
		for (int i = 0; i < SYMBOL_SIZE_ARRAY; i++) {
			if (m_symbol_array[i] == character) {
				char_type = CharTypeEnum::CT_SYMBOL;
				break;
			}
		}
	}

	return char_type;
}

char TextBox::previousCharType() {
	m_char_type_index--;
	if (m_char_type_index < 0) {
		m_char_type_index = m_chars_type_size - 1;
	}
	else if (m_char_type_index >= m_chars_type_size) {
		m_char_type_index = 0;
	}
	
	return getCharType(m_char_type_index);
}

char TextBox::nextCharType() {
	
	m_char_type_index++;
	if (m_char_type_index < 0) {
		m_char_type_index = m_chars_type_size - 1;
	}
	else if (m_char_type_index >= m_chars_type_size) {
		m_char_type_index = 0;
	}

	return getCharType(m_char_type_index);
}

void TextBox::deleteChar(int index) {	
	m_text.erase(index, 1);
	setDisplayText(m_text);
	setIndex(index-1);
}

void TextBox::moveString(int direction) {	
	switch (direction)
	{
		case CharDirectionEnum::CDE_LEFT:
			deleteChar(getIndex());
			break;
	
		case CharDirectionEnum::CDE_RIGHT:
			{
				std::string previous_text = m_text;
				if (insertChar(getIndex() + 1,' ') == previous_text)
				{
					putChar(' ');
				}
				else
				{
					setIndex(getIndex() + 1);
				}
			}
			break;
	}
}

void TextBox::moveToPreviousPlace() {
	setIndex(getIndex()-1);
}

void TextBox::moveToNextPlace() {
	setIndex(getIndex()+1);
}

void TextBox::inputEvent(int event_type, int key) {
	if (event_type != GenericMenu::Event::EvtKeypress)
		return;

	char new_char = 0;

	switch (key)
	{
		// LEFT TRIGGER
		case GenericMenu::Event::KeyPgup:
		{
			moveString(CharDirectionEnum::CDE_LEFT);
			setTextCursor();
		}
		break;

		// RIGHT TRIGGER
		case GenericMenu::Event::KeyPgdn:
		{
			moveString(CharDirectionEnum::CDE_RIGHT);
			setTextCursor();
		}
		break;

		case GenericMenu::Event::KeyCancel:
		{
			setFocus(false);
		}
		break;

		case GenericMenu::Event::KeyMiscX:
		{
			changeCharsType();
			changeChar(getIndex(), getCharType(m_char_type_index));
			setTextCursor();
		}
		break;

		case GenericMenu::Event::KeyMiscY:
		{
			jumpCharType(m_chars_type_displacement);
			changeChar(getIndex(), getCharType(m_char_type_index));
			setTextCursor();
		}
		break;

		case GenericMenu::Event::KeyDown:
		{
			new_char = previousCharType();
			changeChar(getIndex(), new_char);
			setTextCursor();
		}
		break;

		case GenericMenu::Event::KeyUp:
		{
			new_char = nextCharType();
			changeChar(getIndex(), new_char);
			setTextCursor();
		}
		break;

		case GenericMenu::Event::KeyLeft:
		{
			moveToPreviousPlace();
			fixPositionCharIndex();			
			setTextCursor();
		}
		break;

		case GenericMenu::Event::KeyRight:
		{
			moveToNextPlace();
			fixPositionCharIndex();
			setTextCursor();
		}
		break;

		case GenericMenu::Event::KeySelect:
		{
			insertChar(getIndex(), getCharType(m_char_type_index));
			setIndex(getIndex() + 1);
			setTextCursor();
		}
		break;
	}
}

void TextBox::setCursor(Drawable *drawable) {

}

void TextBox::setPosition(float x, float y) {

}

void TextBox::clearText() {
	m_text = "";
	setDisplayText(m_text);
}

void TextBox::setText(const std::string &text) {
	m_text = text;
}

void TextBox::setDisplayText(const std::string &text) {
	if (m_display_label != nullptr) {		
		m_display_label->setText(text);
	}
}

const std::string TextBox::getText() {
	m_text = m_display_label->getText();
	return m_text;
}

void TextBox::setIndex(int index) {
	if (index < 0) {
		index = 0;
	}
	else if (index > (int)m_text.length() - 1) {
		index = (int)m_text.length() - 1;
	}

	m_char_index = index;
}

int TextBox::getIndex() {
	if (m_char_index < 0) {
		m_char_index = 0;
	}

	return m_char_index;
}

extern "C"
{
	TextBox* TSU_TextBoxCreate(Font *display_font, uint text_size, bool centered, float width, float height, Color *body_color, bool enable_chars_type_letter, bool enable_chars_type_cap_letter, 
		bool enable_chars_type_number, bool enable_chars_type_symbol)
	{
		return new TextBox(display_font, text_size, centered, width, height, *body_color, enable_chars_type_letter, enable_chars_type_cap_letter, 
				enable_chars_type_number, enable_chars_type_symbol);
	}

	void TSU_TextBoxDestroy(TextBox **textbox_ptr)
	{
		if (*textbox_ptr != NULL)
		{
			delete *textbox_ptr;
			*textbox_ptr = NULL;
		}
	}

	void TSU_TextBoxSetStates(TextBox *textbox_ptr, int control_state, int previous_state, int *change_state)
	{
		if (textbox_ptr != NULL)
		{
			textbox_ptr->setStates(control_state, previous_state, change_state);
		}	
	}

	int TSU_TextBoxGetControlState(TextBox *textbox_ptr)
	{
		if (textbox_ptr != NULL)
		{
			return textbox_ptr->getControlState();
		}
		else
		{
			return -1;
		}
	}

	void TSU_TextBoxSetFocus(TextBox *textbox_ptr, bool focus)
	{
		if (textbox_ptr != NULL)
		{
			textbox_ptr->setFocus(focus);
		}		
	}

	void TSU_TextBoxInputEvent(TextBox *textbox_ptr, int event_type, int key)
	{
		if (textbox_ptr != NULL)
		{
			textbox_ptr->inputEvent(event_type, key);
		}
	}

	const char* TSU_TextBoxGetText(TextBox *textbox_ptr)
	{
		if (textbox_ptr != NULL) {
			static char text[255] = {0};

			std::string str = textbox_ptr->getText();
			strncpy(text, str.c_str(), sizeof(text) - 1);
			text[sizeof(text) - 1] = '\0';

			return text;
		}
		else {
			return NULL;
		}
	}

	void TSU_TextBoxSetText(TextBox *textbox_ptr, const char *text) {
		if (textbox_ptr != NULL) {
			textbox_ptr->setText(text);
			textbox_ptr->setDisplayText(text);
		}
	}
	
	void TSU_TextBoxClear(TextBox *textbox_ptr) {
		if (textbox_ptr != NULL) {
			return textbox_ptr->clearText();
		}
	}
}