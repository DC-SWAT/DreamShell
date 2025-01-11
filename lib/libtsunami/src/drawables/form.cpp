#include "drawables/form.h"
#include "tsudefinition.h"
#include <algorithm>

Form::Form(int x, int y, uint width, uint height, bool is_popup, int z_index, bool visible_title, bool visible_bottom, Font *title_font,
	ViewIndexChangedEventPtr view_index_changed_event)  {

	setObjectType(ObjectTypeEnum::FORM_TYPE);

	m_cursor_animation_enable = false;
	m_current_view_index = 0;
	m_current_column = m_current_row = 0;
	m_is_popup = is_popup;
	m_body_rectangle = nullptr;
	m_background_rectangle = nullptr;
	m_title_rectangle = nullptr;
	m_bottom_rectangle = nullptr;
	m_columns_attributes = nullptr;
	m_rows_attributes = nullptr;
	m_current_object_selected = nullptr;
	m_title_font = nullptr;
	m_cursor = nullptr;
	m_cursor_animation = nullptr;
	m_bottom_cursor = nullptr;
	m_bottom_cursor_animation = nullptr;
	m_selector_translate = {0, 0, ML_CURSOR, 1};
	m_bottom_selector_translate = {0, 0, ML_CURSOR, 1};	
	this->view_index_changed_event = view_index_changed_event;
	selected_event = nullptr;
	get_objects_current_view_event = nullptr;

	m_zIndex = z_index;
	if (m_is_popup) {
		m_zIndex += ML_POPUP;
	}

	m_x = x;
	m_y = y;
	m_width = width;
	m_height = height;
	m_visible_title = visible_title;
	m_visible_bottom = visible_bottom;

	if (title_font) {
		m_title_font = title_font;
		m_title_label = new Label(title_font, "", 20, true, false);
	}

	m_title_background_color = {1, 0.22f, 0.06f, 0.25f};
	m_bottom_background_color = {1, 0.22f, 0.06f, 0.25f};
	createForm();
}

Form::~Form() {	

	if (m_columns_attributes != nullptr) {
		free(m_columns_attributes);
		m_columns_attributes = nullptr;
	}

	if (m_rows_attributes != nullptr) {
		free(m_rows_attributes);
		m_rows_attributes = nullptr;
	}

	if (m_cursor_animation != nullptr) {
		m_cursor_animation->complete(m_cursor);
		delete m_cursor_animation;
		m_cursor_animation = nullptr;
	}

	if (m_bottom_cursor_animation != nullptr) {
		m_bottom_cursor_animation->complete(m_bottom_cursor);
		delete m_bottom_cursor_animation;
		m_bottom_cursor_animation = nullptr;
	}

	if (m_cursor != nullptr) {
		m_cursor->setFinished();
	}

	if (m_bottom_cursor != nullptr) {
		m_bottom_cursor->setFinished();
	}	

	if (m_background_rectangle != nullptr) {
		m_background_rectangle->setFinished();
	}
	
	if (m_title_rectangle != nullptr) {
		m_title_rectangle->setFinished();
	}
	
	if (m_body_rectangle != nullptr) {
		m_body_rectangle->setFinished();
	}
	
	if (m_bottom_rectangle != nullptr) {
		m_bottom_rectangle->setFinished();
	}
	
	if (m_title_label != nullptr) {
		m_title_label->setFinished();
	}

	this->setFinished();
	this->getParent()->subRemoveFinished();

	clearObjects();
	clearBottomObjects();

	if (m_cursor != nullptr) {
		delete m_cursor;
		m_cursor = nullptr;
	}

	if (m_bottom_cursor != nullptr) {
		delete m_bottom_cursor;
		m_bottom_cursor = nullptr;
	}	

	if (m_background_rectangle != nullptr) {
		delete m_background_rectangle;
		m_background_rectangle = nullptr;
	}
	
	if (m_title_rectangle != nullptr) {
		delete m_title_rectangle;
		m_title_rectangle = nullptr;
	}
	
	if (m_body_rectangle != nullptr) {
		delete m_body_rectangle;
		m_body_rectangle = nullptr;
	}
	
	if (m_bottom_rectangle != nullptr) {
		delete m_bottom_rectangle;
		m_bottom_rectangle = nullptr;
	}
	
	if (m_title_label != nullptr) {
		delete m_title_label;
		m_title_label = nullptr;
	}

	if (m_title_font != nullptr) {
		delete m_title_font;
		m_title_font = nullptr;
	}
}

void Form::clearBottomObjects() {
	for (auto it = m_bottom_objects.begin(); it != m_bottom_objects.end();) {
		if ((*it)->object) {
			(*it)->object->setFinished();
		}
		it++;
	}

	this->subRemoveFinished();

	for (auto it = m_bottom_objects.begin(); it != m_bottom_objects.end();) {
		freeObject(*it);		
		(*it)->object = nullptr;

		delete *it;
		*it = nullptr;
		it++;
	}

	m_bottom_objects.clear();
}

void Form::freeObject(ObjectStruct *object_ptr) {
	if (object_ptr) {

		switch (object_ptr->type)
		{
			case LABEL_TYPE:
				delete ((Label *)object_ptr->object);
				break;

			case TEXTBOX_TYPE:
				delete ((TextBox *)object_ptr->object);
				break;

			case OPTIONGROUP_TYPE:
				delete ((OptionGroup *)object_ptr->object);
				break;

			case CHECKBOX_TYPE:
				delete ((CheckBox *)object_ptr->object);
				break;

			case RECTANGLE_TYPE:
				delete ((Rectangle *)object_ptr->object);
				break;

			case ITEMMENU_TYPE:
				delete ((ItemMenu *)object_ptr->object);
				break;

			case BANNER_TYPE:
				delete ((Banner *)object_ptr->object);
				break;

			case BOX_TYPE:
				delete ((Box *)object_ptr->object);
				break;

			case TRIANGLE_TYPE:
				delete ((Triangle *)object_ptr->object);
				break;
		
			default:
				delete object_ptr->object;
				break;
		}

		object_ptr->object = nullptr;
	}
}

void Form::clearObjects() {
	for (auto it = m_objects.begin(); it != m_objects.end();) {
		if ((*it)->object) {
			(*it)->object->setFinished();
		}
		it++;
	}

	this->subRemoveFinished();

	for (auto it = m_objects.begin(); it != m_objects.end();) {
		freeObject(*it);		
		(*it)->object = nullptr;

		delete *it;
		*it = nullptr;
		it++;
	}

	m_objects.clear();
}

void Form::setAttributes(uint number_columns, uint number_rows, uint columns_size, uint rows_size) {
	if (m_columns_attributes) {
		free(m_columns_attributes);
		m_columns_attributes = nullptr;
	}

	if (m_rows_attributes) {
		free(m_rows_attributes);
		m_rows_attributes = nullptr;
	}

	if (columns_size > 0) {
		m_columns_size = columns_size;
	}
	else {
		m_columns_size = 100;
	}

	if (rows_size > 0) {
		m_rows_size = rows_size;
	}
	else {
		m_rows_size = 100;
	}

	if (number_columns > 0) {
		m_number_columns = number_columns;
	}
	else {
		m_number_columns = 1;
	}
	m_columns_attributes = (AttributeStruct *)calloc(m_number_columns, sizeof(AttributeStruct));

	for (uint i = 0; i < m_number_columns; i++) {
		m_columns_attributes[i].width = m_columns_size;
	}

	if (number_rows > 0) {
		m_number_rows = number_rows;
	}
	else {
		m_number_rows = 1;
	}
	m_rows_attributes = (AttributeStruct *)calloc(m_number_rows, sizeof(AttributeStruct));

	for (uint i = 0; i < m_number_rows; i++) {
		m_rows_attributes[i].width = m_rows_size;
	}

}

void Form::createForm() {	
	float title_size = 0;
	float bottom_size = 0;
	float body_size = m_height;

	if (m_visible_title) {
		title_size = 40;
		body_size -= title_size;
	}

	if (m_visible_bottom) {
		bottom_size = 70;
		body_size -= bottom_size;
	}

	float bottom_y = m_y;
	float body_y = bottom_y - bottom_size;
	float title_y = body_y - body_size;
	
	m_color = {1, 0.0f, 0.0f, 0.0f};
	m_border_color = {1, 1.0f, 1.0f, 1.0f};
	m_background_color = {0.7, 0.0f, 0.0f, 0.0f};
	
	if (m_is_popup) {
		m_background_rectangle = new Rectangle(PVR_LIST_TR_POLY, 0, 480, 640, 480, m_background_color, m_zIndex, 0, m_border_color, 0);
		subAdd(m_background_rectangle);
	}

	if (m_visible_title) {
		m_title_rectangle = new Rectangle(PVR_LIST_OP_POLY, m_x, title_y, m_width, title_size, m_title_background_color, m_zIndex + 2, 3, m_border_color, 0);
		subAdd(m_title_rectangle);
	}

	m_body_rectangle = new Rectangle(PVR_LIST_OP_POLY, m_x, body_y, m_width, body_size, m_color, m_zIndex + 1, 3, m_border_color, 0);
	subAdd(m_body_rectangle);

	if (m_visible_bottom) {
		m_bottom_rectangle = new Rectangle(PVR_LIST_OP_POLY, m_x, bottom_y, m_width, bottom_size, m_bottom_background_color, m_zIndex + 2, 3, m_border_color, 0);
		subAdd(m_bottom_rectangle);
	}

	if (m_title_label) {
		Vector title_vector;
		title_vector.x = m_x + m_width/2;
		title_vector.y = title_y - (title_size/2 + 2);
		title_vector.z = m_zIndex + ML_ITEM;
		
		m_title_label->setTranslate(title_vector);
		subAdd(m_title_label);
	}

	m_title_height = (uint)title_size;
	m_body_height = (uint)body_size;
	m_bottom_height = (uint)bottom_size;

	onViewIndexChangedEvent();
}
   
Font* Form::getTitleFont() {
	return m_title_font;
}

void Form::setTitle(const std::string &text) {
	m_title_label->setText(text);
}

void Form::setSize(float width, float height) {	
}

void Form::setPosition(float x, float y) {
	Vector vec = getTranslate();
	vec.x = x;
	vec.y = y;

	setTranslate(vec);
}

void Form::setColumnSize(uint column_number, float width) {
	m_columns_attributes[column_number - 1].width = width;
}

void Form::setColumnSpan(uint column_number, uint column_span) {
	m_columns_attributes[column_number - 1].span = column_span;
}

void Form::setRowSize(uint row_number, float height) {	
	m_rows_attributes[row_number - 1].height = height;
}

void Form::setRowSpan(uint row_number, uint row_span) {
	m_rows_attributes[row_number - 1].span = row_span;
}

void Form::addBodyObject(Drawable *drawable_ptr, uint object_type, uint column_number, uint row_number) {
	ObjectStruct *object = new ObjectStruct();
	object->object = drawable_ptr;
	object->type = object_type;
	object->column = column_number;
	object->row = row_number;
	object->view_index = m_current_view_index;

	m_objects.push_front(object);

	Vector body_position = m_body_rectangle->getTranslate();
	Vector object_position;

	int width = 0;
	int height = 0;

	for (uint i = 0; i < column_number; i++) {
		if (i > 0) {
			width += (m_columns_attributes[i-1].width > 0 ? m_columns_attributes[i-1].width : m_columns_size);
		}
	}

	for (uint i = 0; i < row_number; i++) {
		height += (m_rows_attributes[i].height > 0 ? m_rows_attributes[i].height : m_rows_size);
	}

	object_position.x = width + body_position.x + column_padding_left;
	object_position.y = body_position.y - m_body_height + height;
	object_position.z = body_position.z + ML_ITEM;
	object_position.w = body_position.w;

	drawable_ptr->setTranslate(object_position);

	this->subAdd(drawable_ptr);

	if (m_current_object_selected == nullptr) {
		m_current_column = column_number;
		m_current_row = row_number;
		setCursor(drawable_ptr);
	}
}

void Form::addBodyScene(Scene *scene_ptr, uint column_number, uint row_number) {
	addBodyObject(scene_ptr, ObjectTypeEnum::SCENE_TYPE, column_number, row_number);
}

void Form::addBodyForm(Form *form_ptr, uint column_number, uint row_number) {
	addBodyObject(form_ptr, ObjectTypeEnum::FORM_TYPE, column_number, row_number);
}

void Form::addBodyBanner(Banner *banner_ptr, uint column_number, uint row_number) {
	addBodyObject(banner_ptr, ObjectTypeEnum::BANNER_TYPE, column_number, row_number);
}

void Form::addBodyRectangle(Rectangle *rectangle_ptr, uint column_number, uint row_number) {
	addBodyObject(rectangle_ptr, ObjectTypeEnum::RECTANGLE_TYPE, column_number, row_number);
}

void Form::addBodyLabel(Label *label_ptr, uint column_number, uint row_number) {
	addBodyObject(label_ptr, ObjectTypeEnum::LABEL_TYPE, column_number, row_number);
}

void Form::addBodyItemMenu(ItemMenu *itemmenu_ptr, uint column_number, uint row_number) {
	addBodyObject(itemmenu_ptr, ObjectTypeEnum::ITEMMENU_TYPE, column_number, row_number);
}

void Form::addBodyOptionGroup(OptionGroup *optiongroup_ptr, uint column_number, uint row_number) {
	addBodyObject(optiongroup_ptr, ObjectTypeEnum::OPTIONGROUP_TYPE, column_number, row_number);
}

void Form::addBodyTextBox(TextBox *textbox_ptr, uint column_number, uint row_number) { 
	addBodyObject(textbox_ptr, ObjectTypeEnum::TEXTBOX_TYPE, column_number, row_number);
}

void Form::addBodyComboBox(ComboBox *combobox_ptr, uint column_number, uint row_number) { 
	addBodyObject(combobox_ptr, ObjectTypeEnum::COMBOBOX_TYPE, column_number, row_number);
}

void Form::addBodyCheckBox(CheckBox *checkbox_ptr, uint column_number, uint row_number) { 
	addBodyObject(checkbox_ptr, ObjectTypeEnum::CHECKBOX_TYPE, column_number, row_number);
}

Drawable* Form::getBodyObject(uint column_number, uint row_number) {
	Drawable *result = nullptr;

	auto is_ptr = [=](ObjectStruct *sp)
	{ 
		return sp->column == column_number && sp->row == row_number && sp->view_index == m_current_view_index;
	};

	auto it = std::find_if(m_objects.begin(), m_objects.end(), is_ptr);

	if (it != m_objects.end()) {
		result = (*it)->object;
	}
	
	return result;
}

Vector Form::getPositionXY(uint column_number, uint row_number) {
	Vector position = {0, 0, 0, 0};
	Vector body_position = m_body_rectangle->getTranslate();

	for (uint i = 0; i < column_number; i++) {
		if (i > 0) {
			position.x += (m_columns_attributes[i-1].width > 0 ? m_columns_attributes[i-1].width : m_columns_size);
		}
	}
	position.x += body_position.x;

	for (uint i = 0; i < row_number; i++) {
		position.y += (m_rows_attributes[i].height > 0 ? m_rows_attributes[i].height : m_rows_size);
	}
	position.y = body_position.y - m_body_height + position.y;

	position.z = body_position.z + ML_CURSOR;
	position.w = body_position.w;

	return position;
}

Vector Form::getBottomPositionXY(uint index) {
	Vector position = {0, 0, 0, 0};
	Vector bottom_position = m_bottom_rectangle->getTranslate();
	uint width = 64;

	for (uint i = 0; i <= index; i++) {
		if (i > 0) {
			position.x += width;
		}
	}
	position.x += bottom_position.x;
	position.y = bottom_position.y - m_bottom_height + position.y;

	position.z = bottom_position.z + ML_CURSOR;
	position.w = bottom_position.w;

	return position;
}

void Form::removeBodyObject(Drawable *object_ptr) {
	auto is_ptr = [=](ObjectStruct *sp) { return sp->object == object_ptr; };
	auto it = std::find_if(m_objects.begin(), m_objects.end(), is_ptr);

	if (it != m_objects.end()) {
		free((*it)->object);
		free(*it);
	}

	subRemove(object_ptr);
}

void Form::removeBodyScene(Scene *scene_ptr) {
	removeBodyObject(scene_ptr);
}

void Form::removeBodyForm(Form *form_ptr) {
	removeBodyObject(form_ptr);
}

void Form::removeBodyBanner(Banner *banner_ptr) {
	removeBodyObject(banner_ptr);
}

void Form::removeBodyRectangle(Rectangle *rectangle_ptr) {
	removeBodyObject(rectangle_ptr);
}

void Form::removeBodyLabel(Label *label_ptr) {
	removeBodyObject(label_ptr);
}

void Form::removeBodyItemMenu(ItemMenu *itemmenu_ptr) {
	removeBodyObject(itemmenu_ptr);
}

void Form::removeBodyOptionGroup(OptionGroup *optiongroup_ptr) {
	removeBodyObject(optiongroup_ptr);
}

void Form::removeBodyTextBox(TextBox *textbox_ptr) { 
	removeBodyObject(textbox_ptr);
}

void Form::removeBodyComboBox(ComboBox *combobox_ptr) { 
	removeBodyObject(combobox_ptr);
}

void Form::removeBodyCheckBox(CheckBox *checkbox_ptr) { 
	removeBodyObject(checkbox_ptr);
}

void Form::addBottomObject(Drawable *drawable_ptr, uint object_type) {
	ObjectStruct *object = new ObjectStruct();
	object->object = drawable_ptr;
	object->type = object_type;
	object->column = m_objects.size() + 1;
	object->row = 0;

	m_bottom_objects.push_front(object);

	Vector bottom_position = m_bottom_rectangle->getTranslate();
	Vector object_position;

	int width = (m_bottom_objects.size() - 1) * 64;
	int height = 64;
	float border = 4;

	object_position.x = width + bottom_position.x + border;
	object_position.y = bottom_position.y - height/2;
	object_position.z = bottom_position.z + ML_ITEM;
	object_position.w = bottom_position.w;

	if (object_type == LABEL_TYPE) {
		object_position.x += 64/2;
		drawable_ptr->setTranslate(object_position);

		((Label *)drawable_ptr)->setCenter(true);
	}
	else {
		drawable_ptr->setTranslate(object_position);
	}

	this->subAdd(drawable_ptr);
}

void Form::addBottomBanner(Banner *banner_ptr) {
	addBottomObject(banner_ptr, BANNER_TYPE);
}

void Form::addBottomLabel(Label *label_ptr) {
	addBottomObject(label_ptr, LABEL_TYPE);
}

void Form::removeBottomObject(Drawable *object_ptr) {
	auto is_ptr = [=](ObjectStruct *sp) { return sp->object == object_ptr; };
	auto it = std::find_if(m_bottom_objects.begin(), m_bottom_objects.end(), is_ptr);

	if (it != m_bottom_objects.end()) {
		free((*it)->object);
		free(*it);
	}

	subRemove(object_ptr);
}

void Form::removeBottomBanner(Banner *banner_ptr) {
	removeBottomObject(banner_ptr);
}

void Form::removeBottomLabel(Label *label_ptr) {
	removeBottomObject(label_ptr);
}

Drawable* Form::getBottomObject(uint index) {
	Drawable *result = nullptr;

	auto is_ptr = [=](ObjectStruct *sp)
	{ 
		return sp->column == index; 
	};

	auto it = std::find_if(m_bottom_objects.begin(), m_bottom_objects.end(), is_ptr);

	if (it != m_bottom_objects.end()) {
		result = (*it)->object;
	}
	
	return result;
}

Banner* Form::getBottomBanner(uint index) {
	Banner *banner = nullptr;
	Drawable *result = getBottomObject(index);

	if (result != nullptr) {
		banner = (Banner *)result;
	}
	result = nullptr;
	return banner;
}

Label* Form::getBottomLabel(uint index) {
	Label *label = nullptr;
	Drawable *result = getBottomObject(index);

	if (result != nullptr) {
		label = (Label *)result;
	}
	result = nullptr;
	return label;
}

void Form::show() {

}

void Form::hide() {

}

uint Form::getState() {
	return m_enable ? 1 : 0;
}

void Form::enable(bool enable) {

}

bool Form::isEnable() {
	return m_enable;
}

Drawable* Form::findNextNearestObject(int direction) {
	uint previous_current_row = m_current_row;
	uint previous_current_column = m_current_column;
	Drawable *drawable = findNextObject(direction, true);

	if (drawable == nullptr) {		
		switch(direction) {
			case SearchDirectionEnum::SDE_LEFT:
			case SearchDirectionEnum::SDE_RIGHT: {

				FIND_TO_UP:
				if (drawable == nullptr && m_current_row > 1) {
					m_current_column = previous_current_column;

					m_current_row--;
					drawable = findNextObject(direction, true);

					if (drawable == nullptr) {
						goto FIND_TO_UP;
					}
				}

				FIND_TO_DOWN:
				if (drawable == nullptr && m_current_row < m_number_rows) {
					m_current_column = previous_current_column;

					m_current_row++;
					drawable = findNextObject(direction, true);

					if (drawable == nullptr) {
						goto FIND_TO_DOWN;
					}
				}

				if (drawable == nullptr) {
					if ((drawable = findNextNearestObject(direction)) == nullptr) {
						m_current_column = previous_current_column;
						m_current_row = previous_current_row;
					}
				}
			}
			break;
			
			case SearchDirectionEnum::SDE_UP:
			case SearchDirectionEnum::SDE_DOWN: {
				
				FIND_TO_LEFT:
				if (drawable == nullptr && m_current_column > 1) {
					m_current_row = previous_current_row;

					m_current_column--;
					drawable = findNextObject(direction, true);

					if (drawable == nullptr) {
						goto FIND_TO_LEFT;
					}
				}

				FIND_TO_RIGHT:
				if (drawable == nullptr && m_current_column < m_number_columns) {
					m_current_row = previous_current_row;

					m_current_column++;
					drawable = findNextObject(direction, true);

					if (drawable == nullptr) {
						goto FIND_TO_RIGHT;
					}
				}

				if (drawable == nullptr) {
					if ((drawable = findNextNearestObject(direction)) == nullptr) {
						m_current_column = previous_current_column;
						m_current_row = previous_current_row;
					}
				}
			}
			break;

		}
	}

	return drawable;
}

Drawable* Form::findNextObject(int direction, bool start) {
	Drawable *drawable = nullptr;
	static bool jump = false;
	static Drawable *first_drawable = nullptr;

	if (start) {
		first_drawable = nullptr;
		jump = false;
	}
	else if (jump) {
		return nullptr;
	}

	switch (direction)
	{
		case SearchDirectionEnum::SDE_LEFT:
			m_current_column--;
			break;

		case SearchDirectionEnum::SDE_RIGHT:
			m_current_column++;
			break;

		case SearchDirectionEnum::SDE_UP:
			m_current_row--;
			break;

		case SearchDirectionEnum::SDE_DOWN:
			m_current_row++;
			break;
		
		default:
			break;
	}

	if (direction == SearchDirectionEnum::SDE_LEFT || direction == SearchDirectionEnum::SDE_RIGHT) {
		if (m_current_column < 1) {
			m_current_column = m_number_columns;
		}
		else if (m_current_column > m_number_columns) {
			m_current_column = 1;
		}
		
		Drawable *object = getBodyObject(m_current_column, m_current_row);
		if (object) {			
			if (first_drawable == nullptr) {
				first_drawable = object;
			}
			else if (first_drawable == object) {
				jump = true;
				return nullptr;
			}
			
			if (object->isReadOnly()) {
				drawable = findNextObject(direction, false);
			}
			else {
				drawable = object;
			}
		}
		object = nullptr;
	}
	else if (direction == SearchDirectionEnum::SDE_UP || direction == SearchDirectionEnum::SDE_DOWN) {
		if (m_current_row < 1) {
			m_current_row = m_number_rows;
		}
		else if (m_current_row > m_number_rows) {
			m_current_row = 1;
		}
		
		Drawable *object = getBodyObject(m_current_column, m_current_row);
		if (object) {
			if (first_drawable == nullptr) {
				first_drawable = object;
			}
			else if (first_drawable == object) {
				jump = true;
				return nullptr;
			}
			
			if (object->isReadOnly()) {				
				drawable = findNextObject(direction, false);
			}
			else {
				drawable = object;
			}
		}
		object = nullptr;
	}

	return drawable;
}

void Form::setCursorSize(float width, float height) {	
	if (m_cursor != nullptr) {
		m_cursor->setSize(width, height);
	}
}

void Form::setCursor(Drawable *drawable) {
	if (drawable && !drawable->isReadOnly()) {
		if (m_current_object_selected) {
			m_current_object_selected = nullptr;
		}
		m_current_object_selected = drawable;
		
		if (m_cursor_animation != nullptr) {
			m_cursor_animation->complete(m_cursor);
			
			delete m_cursor_animation;
			m_cursor_animation = nullptr;
		}

		if (m_cursor != nullptr) {
			subRemove(m_cursor);
			delete m_cursor;
			m_cursor = nullptr;
		}

		uint column_width = 0;
		uint row_height = 0;		

		if (m_columns_attributes[m_current_column-1].span > 1) {
			for (uint i = m_current_column-1; i < (m_current_column-1 + m_columns_attributes[m_current_column-1].span); i++) {
				column_width = m_columns_attributes[i].width;
			}
		}
		else if (m_columns_attributes[m_current_column-1].width > 0) {
			column_width = m_columns_attributes[m_current_column-1].width;
		}
		else {
			column_width = m_columns_size;
		}

		if (m_rows_attributes[m_current_row-1].height > 0) {
			row_height = m_rows_attributes[m_current_row-1].height;
		}
		else {
			row_height = m_rows_size;
		}

		float border_width = 3;
		float drawable_width = 0, drawable_height = 0;

		if (drawable->getObjectType() == ObjectTypeEnum::LABEL_TYPE) {
			Label *label = (Label *)drawable;
			label->getFont()->getTextSize(label->getText(), &drawable_width, &drawable_height);
			label = NULL;

			drawable_width += 4;
			drawable_height += 2;
		}
		else {
			drawable->getSize(&drawable_width, &drawable_height);
		}

		Vector drawable_position = drawable->getPosition();

		if (drawable_width > 0 && drawable_height > 0) {
			column_width = drawable_width + border_width + 2;
			row_height = drawable_height + border_width + 2;
		}		
		
		if (m_cursor == nullptr) {
			Color color = {0, 0.0f, 0.0f, 0.0f};
			Color border_color = {1, 1.0f, 1.0f, 0.1f};
			m_cursor = new Rectangle(PVR_LIST_TR_POLY, m_selector_translate.x, m_selector_translate.y, column_width - 4, row_height - 4, color
					, ML_POPUP + ML_CURSOR, border_width, border_color, 0);
			subAdd(m_cursor);
		}

		Vector body_position = m_body_rectangle->getTranslate();
		m_selector_translate.z = body_position.z + ML_CURSOR;
		m_selector_translate.w = column_width;
		m_selector_translate.x = drawable_position.x;
		m_selector_translate.y = drawable_position.y + (row_height/2) - border_width;

		if (drawable->getObjectType() == ObjectTypeEnum::ITEMMENU_TYPE || drawable->getObjectType() == ObjectTypeEnum::BANNER_TYPE) {
			m_selector_translate.y += drawable_height/2;
		}
		else if (drawable->getObjectType() == ObjectTypeEnum::LABEL_TYPE) {
			m_selector_translate.x -= 2;
			m_selector_translate.y -= (drawable_height/2 - (border_width + 2));
			m_selector_translate.w -= (border_width + 2);
		}

		if (m_cursor_animation_enable) {
			m_cursor_animation = new LogXYMover(m_selector_translate.x, m_selector_translate.y);
			m_cursor_animation->setFactor(6.0f);
			m_cursor->animAdd((Animation *)m_cursor_animation);
		}
		else {
			m_cursor->setTranslate(m_selector_translate);
		}
	}
}

void Form::inputEvent(int event_type, int key) {
	if (event_type != GenericMenu::Event::EvtKeypress)
		return;

	Drawable *drawable = nullptr;

	switch (key)
	{
		case GenericMenu::Event::KeyMiscX:
		{
		}
		break;

		case GenericMenu::Event::KeyCancel:
		{
		}
		break;

		// LEFT TRIGGER
		case GenericMenu::Event::KeyPgup:
		{
			if (m_current_view_index >= 0) {
				onGetObjectsCurrentViewEvent();
				m_current_view_index--;
				onViewIndexChangedEvent();

				m_current_row = 0;
				drawable = findNextNearestObject(SearchDirectionEnum::SDE_UP);

				m_current_column = 0;
				drawable = findNextNearestObject(SearchDirectionEnum::SDE_RIGHT);
			}
		}
		break;

		// RIGHT TRIGGER
		case GenericMenu::Event::KeyPgdn:
		{
			if (m_current_view_index < (int)m_bottom_objects.size()) {
				onGetObjectsCurrentViewEvent();
				m_current_view_index++;
				onViewIndexChangedEvent();

				m_current_row = 0;
				drawable = findNextNearestObject(SearchDirectionEnum::SDE_UP);

				m_current_column = 0;
				drawable = findNextNearestObject(SearchDirectionEnum::SDE_RIGHT);
			}
		}
		break;

		case GenericMenu::Event::KeyUp:
		{
			drawable = findNextNearestObject(SearchDirectionEnum::SDE_UP);
		}

		break;

		case GenericMenu::Event::KeyDown:
		{
			drawable = findNextNearestObject(SearchDirectionEnum::SDE_DOWN);
		}

		break;

		case GenericMenu::Event::KeyLeft:
		{
			drawable = findNextNearestObject(SearchDirectionEnum::SDE_LEFT);
		}
		break;

		case GenericMenu::Event::KeyRight:
		{
			drawable = findNextNearestObject(SearchDirectionEnum::SDE_RIGHT);			
		}
		break;

		case GenericMenu::Event::KeySelect:
		{
			if (m_current_object_selected) {
				m_current_object_selected->click();
			}
		}
		break;

		default:
		{
		}
		break;
	}

	bool changed = (drawable && m_current_object_selected != drawable ? true : false);

	setCursor(drawable);
	if (changed) {
		if (selected_event) {
			selected_event(drawable, 0, m_current_column, m_current_row);
		}
	}
}

void Form::onViewIndexChangedEvent() {
	if (view_index_changed_event != nullptr) {

		if (!m_visible_bottom) {
			m_current_view_index = 0;
			view_index_changed_event(this, m_current_view_index);
			return;
		}

		if (m_current_view_index < 0) {
			m_current_view_index = m_bottom_objects.size() - 1;
		}
		else if (m_current_view_index > (int)m_bottom_objects.size() - 1) {
			m_current_view_index = 0;
		}

		clearObjects();

		if (m_bottom_cursor_animation != nullptr) {
			m_bottom_cursor_animation->complete(m_bottom_cursor);
			delete m_bottom_cursor_animation;
			m_bottom_cursor_animation = nullptr;
		}

		if (m_bottom_cursor != nullptr) {
			subRemove(m_bottom_cursor);
			delete m_bottom_cursor;
			m_bottom_cursor = nullptr;
		}
		
		uint row_height = 64;
		uint column_width = 64;
		Vector bottom_position = m_bottom_rectangle->getTranslate();
		
		Color color = {0.1f, 1.0f, 1.0f, 0.1f};
		Color border_color = {1, 1.0f, 1.0f, 0.1f};
		m_bottom_cursor = new Rectangle(PVR_LIST_TR_POLY, m_bottom_selector_translate.x, m_bottom_selector_translate.y, column_width, row_height, color, bottom_position.z + 1, 3, border_color, 0);
		subAdd(m_bottom_cursor);

		Vector current_init_position = getBottomPositionXY(m_current_view_index);		
		m_bottom_selector_translate.z = bottom_position.z + 1;
		m_bottom_selector_translate.w = column_width;
		m_bottom_selector_translate.x = current_init_position.x + 4/2 + cursor_padding_left;
		m_bottom_selector_translate.y = current_init_position.y + row_height + 3;

		if (m_cursor_animation_enable) {
			m_bottom_cursor_animation = new LogXYMover(m_bottom_selector_translate.x, m_bottom_selector_translate.y);
			m_bottom_cursor_animation->setFactor(6.0f);
			m_bottom_cursor->animAdd((Animation *)m_bottom_cursor_animation);
		}
		else {
			m_bottom_cursor->setTranslate(m_bottom_selector_translate);			
		}

		view_index_changed_event(this, m_current_view_index);
	}
}

void Form::onGetObjectsCurrentViewEvent() {
	if (get_objects_current_view_event != nullptr) {
		uint loop_index = 0;
		for (auto it = m_objects.begin(); it != m_objects.end();) {			
			if ((*it)->object != nullptr && (*it)->view_index == m_current_view_index) {
				get_objects_current_view_event(loop_index, (*it)->object->getId(), (*it)->object, (*it)->type, (*it)->row, (*it)->column, (*it)->view_index);
			}
			it++;
			loop_index++;
		}
	}
}

void Form::selectedEvent(SelectedEventPtr func_ptr) {
	if (func_ptr) {
		selected_event = func_ptr;
	}
}

void Form::getObjectsCurrentViewEvent(GetObjectsCurrentViewEventPtr func_ptr) {
	if (func_ptr) {
		get_objects_current_view_event = func_ptr;
	}
}


extern "C"
{
	Form* TSU_FormCreate(int x, int y, uint width, uint height, bool is_popup, int z_index,
		bool visible_title, bool visible_bottom, Font *title_font,
		ViewIndexChangedEventPtr view_index_changed_event_ptr)
	{
		return new Form(x, y, width, height, is_popup, z_index, 
				visible_title, visible_bottom, title_font, view_index_changed_event_ptr);
	}

	void TSU_FormtSetAttributes(Form *form_ptr, uint number_columns, uint number_rows, uint columns_size, uint rows_size)
	{
		if (form_ptr != NULL)
		{
			form_ptr->setAttributes(number_columns, number_rows, columns_size, rows_size);
		}
	}

	void TSU_FormRemove(Form *form_ptr)
	{
		if (form_ptr != NULL)
		{
			form_ptr->getParent()->subRemove(form_ptr);
		}
	}

	void TSU_FormDestroy(Form **form_ptr)
	{
		if (*form_ptr != NULL)
		{
			delete *form_ptr;
			*form_ptr = NULL;
		}
	}

	void TSU_FormInputEvent(Form *form_ptr, int event_type, int key)
	{
		if (form_ptr != NULL)
		{
			form_ptr->inputEvent(event_type, key);
		}
	}

	void TSU_FormSelectedEvent(Form *form_ptr, SelectedEventPtr func_ptr)
	{
		if (form_ptr != NULL)
		{
			form_ptr->selectedEvent(func_ptr);
		}
	}

	void TSU_FormGetObjectsCurrentViewEvent(Form *form_ptr, GetObjectsCurrentViewEventPtr func_ptr)
	{
		if (form_ptr != NULL)
		{
			form_ptr->getObjectsCurrentViewEvent(func_ptr);
		}
	}

	void TSU_FormOnGetObjectsCurrentView(Form *form_ptr)
	{
		if (form_ptr != NULL)
		{
			form_ptr->onGetObjectsCurrentViewEvent();
		}		
	}

	Font* TSU_FormGetTitleFont(Form *form_ptr)
	{
		Font* font = NULL;

		if (form_ptr != NULL)
		{
			font = form_ptr->getTitleFont();
		}

		return font;
	}

	void TSU_FormSetTitle(Form *form_ptr, const char *text)
	{
		if (form_ptr != NULL)
		{
			form_ptr->setTitle(text);
		}
	}

	void TSU_FormSetSize(Form *form_ptr, float width, float height)
	{
		if (form_ptr != NULL)
		{
			form_ptr->setSize(width, height);
		}
	}

	void TSU_FormSetPosition(Form *form_ptr, float x, float y)
	{
		if (form_ptr != NULL)
		{
			form_ptr->setSize(x, y);
		}
	}

	void TSU_FormSetColumnSize(Form *form_ptr, uint column_number, float width)
	{
		if (form_ptr != NULL)
		{
			form_ptr->setColumnSize(column_number, width);
		}
	}

	void TSU_FormSetColumnSpan(Form *form_ptr, uint column_number, uint column_span)
	{
		if (form_ptr != NULL)
		{
			form_ptr->setColumnSpan(column_number, column_span);
		}
	}
	
	void TSU_FormSetRowSize(Form *form_ptr, uint row_number, float height)
	{
		if (form_ptr != NULL)
		{
			form_ptr->setRowSize(row_number, height);
		}
	}

	void TSU_FormSetRowSpan(Form *form_ptr, uint row_number, uint row_span)
	{
		if (form_ptr != NULL)
		{
			form_ptr->setRowSpan(row_number, row_span);
		}
	}

	void TSU_FormAddBodyScene(Form *form_ptr, Scene *scene_ptr, uint column_number, uint row_number)
	{
		if (form_ptr != NULL && scene_ptr != NULL)
		{
			form_ptr->addBodyScene(scene_ptr, column_number, row_number);
		}
	}
	
	void TSU_FormAddBodyForm(Form *form_ptr, Form *new_form_ptr, uint column_number, uint row_number)
	{
		if (form_ptr != NULL && new_form_ptr != NULL)
		{
			form_ptr->addBodyForm(new_form_ptr, column_number, row_number);
		}
	}
	
	void TSU_FormAddBodyBanner(Form *form_ptr, Banner *banner_ptr, uint column_number, uint row_number)
	{
		if (form_ptr != NULL && banner_ptr != NULL)
		{
			form_ptr->addBodyBanner(banner_ptr, column_number, row_number);
		}
	}

	void TSU_FormAddBodyRectangle(Form *form_ptr, Rectangle *rectangle_ptr, uint column_number, uint row_number)
	{
		if (form_ptr != NULL && rectangle_ptr != NULL)
		{
			form_ptr->addBodyRectangle(rectangle_ptr, column_number, row_number);
		}
	}
	
	void TSU_FormAddBodyLabel(Form *form_ptr, Label *label_ptr, uint column_number, uint row_number)
	{
		if (form_ptr != NULL && label_ptr != NULL)
		{
			form_ptr->addBodyLabel(label_ptr, column_number, row_number);
		}
	}
	
	void TSU_FormAddBodyItemMenu(Form *form_ptr, ItemMenu *itemmenu_ptr, uint column_number, uint row_number)
	{
		if (form_ptr != NULL && itemmenu_ptr != NULL)
		{
			form_ptr->addBodyItemMenu(itemmenu_ptr, column_number, row_number);
		}
	}

	void TSU_FormAddBodyOptionGroup(Form *form_ptr, OptionGroup *optiongroup_ptr, uint column_number, uint row_number)
	{
		if (form_ptr != NULL && optiongroup_ptr != NULL)
		{
			form_ptr->addBodyOptionGroup(optiongroup_ptr, column_number, row_number);
		}
	}

	void TSU_FormAddBodyTextBox(Form *form_ptr, TextBox *textbox_ptr, uint column_number, uint row_number)
	{
		if (form_ptr != NULL && textbox_ptr != NULL)
		{
			form_ptr->addBodyTextBox(textbox_ptr, column_number, row_number);
		}
	}
	
	void TSU_FormAddBodyComboBox(Form *form_ptr, ComboBox *combobox_ptr, uint column_number, uint row_number) 
	{
		if (form_ptr != NULL && combobox_ptr != NULL)
		{
			form_ptr->addBodyComboBox(combobox_ptr, column_number, row_number);
		}
	}

	void TSU_FormAddBodyCheckBox(Form *form_ptr, CheckBox *checkbox_ptr, uint column_number, uint row_number)
	{
		if (form_ptr != NULL && checkbox_ptr != NULL)
		{
			form_ptr->addBodyCheckBox(checkbox_ptr, column_number, row_number);
		}
	}

	Drawable* TSU_FormGetBodyObject(Form *form_ptr, uint column_number, uint row_number)
	{
		if (form_ptr != NULL)
		{
			return form_ptr->getBodyObject(column_number, row_number);
		}
		else
		{
			return NULL;
		}
	}
	
	void TSU_FormRemoveBodyScene(Form *form_ptr, Scene *scene_ptr)
	{
		if (form_ptr != NULL)
		{
			form_ptr->removeBodyScene(scene_ptr);
		}
	}
	
	void TSU_FormRemoveBodyForm(Form *form_ptr, Form *remove_form_ptr)
	{
		if (form_ptr != NULL)
		{
			form_ptr->removeBodyForm(remove_form_ptr);
		}
	}
	
	void TSU_FormRemoveBodyBanner(Form *form_ptr, Banner *banner_ptr)
	{
		if (form_ptr != NULL)
		{
			form_ptr->removeBodyBanner(banner_ptr);
		}
	}

	void TSU_FormRemoveBodyRectangle(Form *form_ptr, Rectangle *rectangle_ptr)
	{
		if (form_ptr != NULL)
		{
			form_ptr->removeBodyRectangle(rectangle_ptr);
		}
	}
	
	void TSU_FormRemoveBodyLabel(Form *form_ptr, Label *label_ptr)
	{
		if (form_ptr != NULL)
		{
			form_ptr->removeBodyLabel(label_ptr);
		}
	}
	
	void TSU_FormRemoveBodyItemMenu(Form *form_ptr, ItemMenu *itemmenu_ptr)
	{
		if (form_ptr != NULL)
		{
			form_ptr->removeBodyItemMenu(itemmenu_ptr);
		}
	}

	void TSU_FormRemoveBodyOptionGroup(Form *form_ptr, OptionGroup *optiongroup_ptr)
	{
		if (form_ptr != NULL)
		{
			form_ptr->removeBodyOptionGroup(optiongroup_ptr);
		}
	}

	void TSU_FormRemoveBodyTextBox(Form *form_ptr, TextBox *textbox_ptr)
	{
		if (form_ptr != NULL)
		{
			form_ptr->removeBodyTextBox(textbox_ptr);
		}
	}
	
	void TSU_FormRemoveBodyComboBox(Form *form_ptr, ComboBox *combobox_ptr)
	{
		if (form_ptr != NULL)
		{
			form_ptr->removeBodyComboBox(combobox_ptr);
		}
	}

	void TSU_FormRemoveBodyCheckBox(Form *form_ptr, CheckBox *checkbox_ptr)
	{
		if (form_ptr != NULL)
		{
			form_ptr->removeBodyCheckBox(checkbox_ptr);
		}
	}

	void TSU_FormAddBottomBanner(Form *form_ptr, Banner *banner_ptr)
	{
		if (form_ptr != NULL)
		{
			form_ptr->addBottomBanner(banner_ptr);
		}
	}
	
	Banner* TSU_FormGetBottomBanner(Form *form_ptr, uint index)
	{
		if (form_ptr != NULL)
		{
			return form_ptr->getBottomBanner(index);
		}
		else
		{
			return NULL;
		}
	}

	void TSU_FormAddBottomLabel(Form *form_ptr, Label *label_ptr)
	{
		if (form_ptr != NULL)
		{
			form_ptr->addBottomLabel(label_ptr);
		}
	}
	
	Label* TSU_FormGetBottomLabel(Form *form_ptr, uint index)
	{
		if (form_ptr != NULL)
		{
			return form_ptr->getBottomLabel(index);
		}
		else
		{
			return NULL;
		}
	}

	void TSU_FormRemoveBottomBanner(Form *form_ptr, Banner *banner_ptr)
	{
		if (form_ptr != NULL)
		{
			form_ptr->removeBottomBanner(banner_ptr);
		}
	}

	void TSU_FormRemoveBottomLabel(Form *form_ptr, Label *label_ptr)
	{
		if (form_ptr != NULL)
		{
			form_ptr->removeBottomLabel(label_ptr);
		}
	}
	
	void TSU_FormShow(Form *form_ptr)
	{
		if (form_ptr != NULL)
		{
			form_ptr->show();
		}
	}
	
	void TSU_FormHide(Form *form_ptr)
	{
		if (form_ptr != NULL)
		{
			form_ptr->hide();
		}
	}
	
	uint TSU_FormGetState(Form *form_ptr)
	{
		if (form_ptr != NULL)
		{
			return form_ptr->getState();
		}
	
		return 0;
	}
	
	void TSU_FormEnable(Form *form_ptr, bool enable)
	{
		if (form_ptr != NULL)
		{
			form_ptr->enable(enable);
		}
	}

	void TSU_FormIsEnable(Form *form_ptr)
	{
		if (form_ptr != NULL)
		{
			form_ptr->isEnable();
		}
	}

	void TSU_FormSetCursor(Form *form_ptr, Drawable *drawable_ptr)
	{
		if (form_ptr != NULL && drawable_ptr != NULL)
		{
			form_ptr->setCursor(drawable_ptr);
		}
	}

	void TSU_FormSetCursorSize(Form *form_ptr, float width, float height)
	{
		if (form_ptr != NULL)
		{
			form_ptr->setCursorSize(width, height);
		}
	}
	
}
