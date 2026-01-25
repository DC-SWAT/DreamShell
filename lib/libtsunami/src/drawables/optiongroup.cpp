/*
   Tsunami for KallistiOS ##version##

   optiongroup.cpp

   Copyright (C) 2024-2025 Maniac Vera

*/

#include "drawables/optiongroup.h"
#include "genmenu.h"
#include "tsudefinition.h"
#include <algorithm>
#include <cstring>
#include <string>

extern "C" {
	uint8 SDL_GetMouseState (int *x, int *y);
}

OptionGroup::OptionGroup(Font *display_font, uint text_size, float width, float height, const Color &body_color) {
	setObjectType(ObjectTypeEnum::OPTIONGROUP_TYPE);

	m_display_font = display_font;
	m_width = width + m_padding_width + border_width;
	m_height = height + m_padding_height + border_width;
	m_previous_index = m_index_selected = -1;
	m_option_selected = nullptr;
	m_display_label = nullptr;
	m_text_animation = nullptr;
	m_left_triangle = nullptr;
	m_right_triangle = nullptr;
	m_body_color = body_color.a ? body_color : Color(DEFAULT_BODY_COLOR);

	if (text_size == 0) {
		text_size = 20;
	}

	if (display_font) {
		Color background_color = {1, 0.22f, 0.06f, 0.25f};
		Color border_color = {1, 1.0f, 1.0f, 1.0f};		
		float z_index = 0;
		float radius = 0;		

		Vector position = this->getTranslate();
		m_control_rectangle = new Rectangle (PVR_LIST_OP_POLY, position.x - width/2 - m_padding_width/2 + 1, position.y + height/2 + m_padding_height, width + m_padding_width, height + m_padding_height, m_body_color, z_index, border_width, border_color, radius);

		background_color.r = 1.0f;
		background_color.g = 1.0f;
		background_color.b = 1.0f;
		m_left_rectangle = new Rectangle (PVR_LIST_OP_POLY, (position.x - width/2) - 10, position.y + height/2 + 2, 10, height -2, background_color, z_index, border_width, border_color, radius);
		m_right_rectangle = new Rectangle (PVR_LIST_OP_POLY, (position.x + width/2) + (6 - border_width * 2), position.y + height/2 + 2, 10, height - 2, background_color, z_index, border_width, border_color, radius);

		this->subAdd(m_control_rectangle);
		this->subAdd(m_left_rectangle);
		this->subAdd(m_right_rectangle);

		m_display_label = new Label(display_font, "", text_size, true, false, false);
		this->subAdd(m_display_label);

		Vector left_rectangle_vector = m_left_rectangle->getTranslate();
		Vector right_rectangle_vector = m_right_rectangle->getTranslate();
		Vector control_rectangle_vector = m_control_rectangle->getTranslate();
		Vector display_label_control = m_display_label->getTranslate();

		left_rectangle_vector.x += (12 + width/2 + border_width*2);
		left_rectangle_vector.y -= border_width*2;
		left_rectangle_vector.z = z_index + 1;
		m_left_rectangle->setTranslate(left_rectangle_vector);

		right_rectangle_vector.x += (12 + width/2 + border_width*2);
		right_rectangle_vector.y -= border_width*2;
		right_rectangle_vector.z = z_index + 1;
		m_right_rectangle->setTranslate(right_rectangle_vector);

		control_rectangle_vector.x += (12 + width/2 + border_width*2);
		control_rectangle_vector.y -= border_width*2;
		m_control_rectangle->setTranslate(control_rectangle_vector);

		display_label_control.x += (13 + width/2 + border_width*2);
		display_label_control.y -= 3;
		display_label_control.z = z_index + 1;
		m_display_label->setTranslate(display_label_control);

		Color triangle_color = m_body_color;

		m_left_triangle = new Triangle(PVR_LIST_OP_POLY, 
				10, 0,
				0, (height/2),
				10, (height),
				triangle_color, z_index + 2, 0, triangle_color, 0);

		this->subAdd(m_left_triangle);

		Vector left_triangle_vector = m_left_triangle->getPosition();
		left_triangle_vector.x -= 5;
		left_triangle_vector.y -= (height/2 + border_width/2);
		m_left_triangle->setTranslate(left_triangle_vector);

		m_right_triangle = new Triangle(PVR_LIST_OP_POLY, 
				0, 0,
				10, (height/2),
				0, (height),
				triangle_color, z_index + 2, 0, triangle_color, 0);

		Vector right_triangle_vector = m_right_triangle->getPosition();
		right_triangle_vector.x += m_width - (3 + border_width + 10);
		right_triangle_vector.y -= (height/2 + border_width/2);
		m_right_triangle->setTranslate(right_triangle_vector);

		this->subAdd(m_right_triangle);
	}
}

OptionGroup::~OptionGroup() {
	clearOptions();
	
	if (m_display_label != nullptr) {
		m_display_label->setFinished();
	}

	if (m_control_rectangle != nullptr) {
		m_control_rectangle->setFinished();
	}

	if (m_left_triangle != nullptr) {
		m_left_triangle->setFinished();
	}

	if (m_right_triangle != nullptr) {
		m_right_triangle->setFinished();
	}

	if (m_left_rectangle != nullptr) {
		m_left_rectangle->setFinished();
	}

	if (m_right_rectangle != nullptr) {
		m_right_rectangle->setFinished();
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

	if (m_left_triangle != nullptr) {
		delete m_left_triangle;
		m_left_triangle = nullptr;
	}

	if (m_right_triangle != nullptr) {
		delete m_right_triangle;
		m_right_triangle = nullptr;
	}

	if (m_left_rectangle != nullptr) {
		delete m_left_rectangle;
		m_left_rectangle = nullptr;
	}

	if (m_right_rectangle != nullptr) {
		delete m_right_rectangle;
		m_right_rectangle = nullptr;
	}

	if (m_text_animation != nullptr) {
		delete m_text_animation;
		m_text_animation = nullptr;
	}

	m_option_selected = nullptr;
	m_display_font = nullptr;
}

void OptionGroup::setFocus(bool focus) {
	if (focus) {
		m_previous_index = m_index_selected;
		ToggleToControlState();
		Color text_color = {1, 1.0f, 1.0f, 0.1f};
		Color rectangle_color = {1, 1.0f, 0.67f, 0.10f};
		m_display_label->setTint(text_color);
		m_left_rectangle->setTint(rectangle_color);
		m_right_rectangle->setTint(rectangle_color);
	}
	else {		
		ToggleToPreviousState();
		Color text_color = {1, 1.0f, 1.0f, 1.0f};
		Color rectangle_color = {1, 1.0f, 1.0f, 1.0f};
		m_display_label->setTint(text_color);
		m_left_rectangle->setTint(rectangle_color);
		m_right_rectangle->setTint(rectangle_color);
	}
}

OptionValueStruct* OptionGroup::previousOption() {
	if (m_index_selected > 0 ) {
		m_index_selected--;
		setDisplayText(m_options[m_index_selected]->text);
	}
	
	return m_options[m_index_selected];
}

OptionValueStruct* OptionGroup::nextOption() {
	if (m_index_selected < (int)m_options.size() - 1) {
		m_index_selected++;
		setDisplayText(m_options[m_index_selected]->text);
	}

	return m_options[m_index_selected];
}

void OptionGroup::inputEvent(int event_type, int key) {
	if (event_type != GenericMenu::Event::EvtKeypress)
		return;

	OptionValueStruct *option = nullptr;

	switch (key)
	{
		case GenericMenu::Event::KeyCancel:
		{
			m_index_selected = m_previous_index;
			setDisplayText(m_options[m_index_selected]->text);
			setFocus(false);
		}
		break;

		case GenericMenu::Event::KeyLeft:
		{
			option = previousOption();
		}
		break;

		case GenericMenu::Event::KeyRight:
		{
			option = nextOption();
		}
		break;

		case GenericMenu::Event::KeySelect:
		{
			Vector lpos = m_left_rectangle->getPosition();
			float lwidth = 0;
			float lheight = 0;
			m_left_rectangle->getSize(&lwidth, &lheight);

			Vector rpos = m_right_rectangle->getPosition();
			float rwidth = 0;
			float rheight = 0;
			m_right_rectangle->getSize(&rwidth, &rheight);

			int x, y;
			SDL_GetMouseState(&x, &y);
			
			if ((x >= lpos.x) && (x < lpos.x + lwidth) && (y <= lpos.y) && (y > lpos.y - lheight)) {
				option = previousOption();
			}
			else if ((x >= rpos.x) && (x < rpos.x + rwidth) && (y <= rpos.y) && (y > rpos.y - rheight)) {
				option = nextOption();
			}
			else {
				setFocus(false);
			}
		}
		break;
	}

	bool changed = (option && m_option_selected != option ? true : false);

	if (changed) {
		m_option_selected = option;
	}
}

void OptionGroup::setCursor(Drawable *drawable) {

}

void OptionGroup::setSize(float width, float height){

}

void OptionGroup::setPosition(float x, float y) {

}

void OptionGroup::addOption(int32 key, const std::string &text) {
	m_options.push_back(new OptionValueStruct{ key, text });

	if (m_options.size() == 1) {
		m_index_selected = 0;
		m_option_selected = m_options[m_index_selected];

		setDisplayText(m_options[m_index_selected]->text);
	}
}

void OptionGroup::deleteOption(const std::string &text) {
}

void OptionGroup::deleteOptionByKey(int32 key) {

}

void OptionGroup::deleteOptionByIndex(int index) {

}

void OptionGroup::clearOptions() {
	for (uint i = 0; i < m_options.size(); i++) {
        delete m_options[i];
    }
    m_options.clear();
}

void OptionGroup::setDisplayText(const std::string &text) {
	if (m_display_label != nullptr) {
		m_display_label->setText(text);
	}
}

const std::string OptionGroup::getTextSelected() {
	if (m_option_selected != nullptr) {
		return m_option_selected->text;
	}
	else {
		return "";
	}
}

int32 OptionGroup::getKeySelected() {
	return m_options[m_index_selected]->key;
}

void OptionGroup::selectOptionByIndex(int index) {
	if (m_options.size() > 0 && index <= (int)m_options.size()) {
		m_index_selected = index;
		m_option_selected = m_options[m_index_selected];
		setDisplayText(m_options[m_index_selected]->text);
	}
}

void OptionGroup::selectOptionByKey(int32 key) {
	auto is_ptr = [=](OptionValueStruct *sp) { return sp->key == key ;};
	auto it = std::find_if(m_options.begin(), m_options.end(), is_ptr);

	size_t index = std::distance(m_options.begin(), it);
	if(index != m_options.size()) {
		m_index_selected = (int)index;
		m_option_selected = m_options[m_index_selected];
		setDisplayText(m_options[m_index_selected]->text);	
	}
}

void OptionGroup::selectOptionByText(const char *text) {
	auto is_ptr = [=](OptionValueStruct *sp) { return sp->text == text ;};
	auto it = std::find_if(m_options.begin(), m_options.end(), is_ptr);

	size_t index = std::distance(m_options.begin(), it);
	if(index != m_options.size()) {
		m_index_selected = (int)index;
		m_option_selected = m_options[m_index_selected];
		setDisplayText(m_options[m_index_selected]->text);	
	}
}

int OptionGroup::getIndexSelected() {
	return m_index_selected;
}

void OptionGroup::setOptionByText(const std::string &current_text, const std::string &new_text) {
	auto is_ptr = [=](OptionValueStruct *sp) { return sp->text == current_text ;};

	auto it = std::find_if(m_options.begin(), m_options.end(), is_ptr);

	if (it != m_options.end()) {
		(*it)->text = new_text;
	}
}

void OptionGroup::setOptionByKey(int32 key, const std::string &text) {
	auto is_ptr = [=](OptionValueStruct *sp) { return sp->key == key ;};

	auto it = std::find_if(m_options.begin(), m_options.end(), is_ptr);

	if (it != m_options.end()) {
		(*it)->text = text;
	}
}

void OptionGroup::setOptionByIndex(int index, const std::string &text) {
	if (index >= 0) {
		m_options[index]->text = text;
	}
}

OptionValueStruct* OptionGroup::getOptionByText(const std::string &text) {
	OptionValueStruct *result = nullptr;
	auto is_ptr = [=](OptionValueStruct *sp) { return sp->text == text ;};
	auto it = std::find_if(m_options.begin(), m_options.end(), is_ptr);

	if (it != m_options.end()) {
		result = (*it);
	}

	return result;
}

OptionValueStruct* OptionGroup::getOptionByKey(int32 key) {
	OptionValueStruct *result = nullptr;
	auto is_ptr = [=](OptionValueStruct *sp) { return sp->key == key ;};
	auto it = std::find_if(m_options.begin(), m_options.end(), is_ptr);

	if (it != m_options.end()) {
		result = (*it);
	}

	return result;
}

OptionValueStruct* OptionGroup::getOptionByIndex(int index) {
	return m_options[index];
}

extern "C"
{
	OptionGroup* TSU_OptionGroupCreate(Font *display_font, uint text_size, float width, float height, Color *body_color)
	{
		return new OptionGroup(display_font, text_size, width, height, *body_color);
	}

	void TSU_OptionGroupDestroy(OptionGroup **optiongroup_ptr)
	{
		if (*optiongroup_ptr != NULL)
		{
			delete *optiongroup_ptr;
			*optiongroup_ptr = NULL;
		}
	}

	void TSU_OptionGroupSetStates(OptionGroup *optiongroup_ptr, int control_state, int previous_state)
	{
		if (optiongroup_ptr != NULL)
		{
			optiongroup_ptr->setStates(control_state, previous_state);
		}	
	}

	int TSU_OptionGroupGetControlState(OptionGroup *optiongroup_ptr)
	{
		if (optiongroup_ptr != NULL)
		{
			return optiongroup_ptr->getControlState();
		}
		else
		{
			return -1;
		}
	}

	void TSU_OptionGroupSetWindowState(OptionGroup *optiongroup_ptr, int window_state)
	{
		if (optiongroup_ptr != NULL)
		{
			optiongroup_ptr->setWindowState(window_state);
		}
	}

	int TSU_OptionGroupGetWindowState(OptionGroup *optiongroup_ptr)
	{
		if (optiongroup_ptr != NULL)
		{
			return optiongroup_ptr->getWindowState();
		}

		return 0;
	}

	void TSU_OptionGroupSetFocus(OptionGroup *optiongroup_ptr, bool focus)
	{
		if (optiongroup_ptr != NULL)
		{
			optiongroup_ptr->setFocus(focus);
		}		
	}

	void TSU_OptionGroupInputEvent(OptionGroup *optiongroup_ptr, int event_type, int key)
	{
		if (optiongroup_ptr != NULL)
		{
			optiongroup_ptr->inputEvent(event_type, key);
		}
	}

	void TSU_OptionGroupAdd(OptionGroup *optiongroup_ptr, int32 key, const char *text)
	{
		if (optiongroup_ptr != NULL)
		{
			optiongroup_ptr->addOption(key, text);
		}
	}

	void TSU_OptionGroupDelete(OptionGroup *optiongroup_ptr, const char *text)
	{
		if (optiongroup_ptr != NULL)
		{
			optiongroup_ptr->deleteOption(text);
		}
	}

	void TSU_OptionGroupDeleteByIndex(OptionGroup *optiongroup_ptr, int index)
	{
		if (optiongroup_ptr != NULL)
		{
			optiongroup_ptr->deleteOptionByIndex(index);
		}
	}

	const char* TSU_OptionGroupGetTextSelected(OptionGroup *optiongroup_ptr)
	{
		if (optiongroup_ptr != NULL) {
			static char text[255] = {0};
			std::string selected_text = optiongroup_ptr->getTextSelected();

			strncpy(text, selected_text.c_str(), sizeof(text) - 1);
			text[sizeof(text) - 1] = '\0';

			return text;
		}
		else {
			return NULL;
		}
	}

	void TSU_OptionGroupSetDisplayText(OptionGroup *optiongroup_ptr, const char *text) {
		if (optiongroup_ptr != NULL) {
			optiongroup_ptr->setDisplayText(text);
		}
	}
	
	void TSU_OptionGroupClear(OptionGroup *optiongroup_ptr) {
		if (optiongroup_ptr != NULL) {
			return optiongroup_ptr->clearOptions();
		}
	}

	void TSU_OptionGroupSelectOptionByIndex(OptionGroup *optiongroup_ptr, int index) {
		if (optiongroup_ptr != NULL) {
			return optiongroup_ptr->selectOptionByIndex(index);
		}
	}

	void TSU_OptionGroupSelectOptionByKey(OptionGroup *optiongroup_ptr, int32 key){
		if (optiongroup_ptr != NULL) {
			return optiongroup_ptr->selectOptionByKey(key);
		}
	}

	void TSU_OptionGroupSelectOptionByText(OptionGroup *optiongroup_ptr, const char *text) {
		if (optiongroup_ptr != NULL) {
			return optiongroup_ptr->selectOptionByText(text);
		}
	}

	int32 TSU_OptionGroupGetKeySelected(OptionGroup *optiongroup_ptr) {
		if (optiongroup_ptr != NULL) {
			return optiongroup_ptr->getKeySelected();
		}
		else {
			return -1;
		}
	}

	int TSU_OptionGroupGetIndexSelected(OptionGroup *optiongroup_ptr) {
		if (optiongroup_ptr != NULL) {
			return optiongroup_ptr->getIndexSelected();
		}
		else {
			return -1;
		}
	}

	void TSU_OptionGroupSetOptionByText(OptionGroup *optiongroup_ptr, const char *curent_text, const char *new_text) {
		if (optiongroup_ptr != NULL) {
			return optiongroup_ptr->setOptionByText(curent_text, new_text);
		}
	}

	void TSU_OptionGroupSetOptionByKey(OptionGroup *optiongroup_ptr, int32 key, const char *text) {
		if (optiongroup_ptr != NULL) {
			return optiongroup_ptr->setOptionByKey(key, text);
		}
	}

	void TSU_OptionGroupSetOptionByIndex(OptionGroup *optiongroup_ptr, int index, const char *text) {
		if (optiongroup_ptr != NULL) {
			return optiongroup_ptr->setOptionByIndex(index, text);
		}
	}

	OptionValueStructure* TSU_OptionGroupGetOptionByText(OptionGroup *optiongroup_ptr, const char *text) {
		if (optiongroup_ptr != NULL) {
			return optiongroup_ptr->getOptionByText(text);
		}
		else {
			return NULL;
		}
	}

	OptionValueStructure* TSU_OptionGroupGetOptionByKey(OptionGroup *optiongroup_ptr, int32 key) {
		if (optiongroup_ptr != NULL) {
			return optiongroup_ptr->getOptionByKey(key);
		}
		else {
			return NULL;
		}
	}

	OptionValueStructure* TSU_OptionGroupGetOptionByIndex(OptionGroup *optiongroup_ptr, int index) {
		if (optiongroup_ptr != NULL) {
			return optiongroup_ptr->getOptionByIndex(index);
		}
		else {
			return NULL;
		}
	}

	int32 TSU_OptionGroupGetKey(OptionValueStructure *option_ptr) {
		return -1;
	}

	const char* TSU_OptionGroupGetText(OptionValueStructure *option_ptr) {
		return NULL;
	}
}