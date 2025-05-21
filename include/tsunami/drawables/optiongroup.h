/*
   Tsunami for KallistiOS ##version##

   form.h

   Copyright (C) 2024-2025 Maniac Vera

*/

#ifndef __TSUNAMI_DRW_OPTION_GROUP_H
#define __TSUNAMI_DRW_OPTION_GROUP_H

#include "../drawable.h"
#include "../font.h"
#include "../color.h"
#include "../animation.h"
#include "../anims/logxymover.h"
#include "label.h"
#include "rectangle.h"
#include "triangle.h"

#ifdef __cplusplus

#include <string>
#include <vector>

typedef struct OptionValueStructure
{
	int32 key;
	std::string text;
} OptionValueStruct;

class OptionGroup : public Drawable
{
private:
	const float border_width = 2;
	const float m_padding_width = 32;
	const float m_padding_height = 6;
	Font *m_display_font;
	int m_control_state, m_previous_state, m_index_selected, m_previous_index;
	int *m_change_state;
	LogXYMover *m_text_animation;

	OptionValueStruct* m_option_selected;
	std::vector<OptionValueStruct *> m_options;
	OptionValueStruct* previousOption();
	OptionValueStruct* nextOption();

	Label *m_display_label;
	Rectangle *m_control_rectangle, *m_left_rectangle, *m_right_rectangle;
	Triangle *m_left_triangle, *m_right_triangle;
	Color m_body_color;

public:
	OptionGroup(Font *font_ptr, uint text_size, float width, float height, const Color &body_color);
	virtual ~OptionGroup();

	void setStates(int control_state, int previous_state, int *change_state);
	int getControlState();
	void setFocus(bool focus);
	void inputEvent(int event_type, int key);
	void setCursor(Drawable *drawable);
	void setSize(float width, float height);
	void setPosition(float x, float y);
	void addOption(int32 key, const std::string &text);
	void deleteOption(const std::string &text);
	void deleteOptionByKey(int32 key);
	void deleteOptionByIndex(int index);
	void clearOptions();
	void setDisplayText(const std::string &text);
	const std::string getTextSelected();
	int32 getKeySelected();
	int getIndexSelected();
	void selectOptionByIndex(int index);
	void selectOptionByKey(int32 key);
	void selectOptionByText(const char *text);
	void setOptionByText(const std::string &current_text, const std::string &new_text);
	void setOptionByKey(int32 key, const std::string &text);
	void setOptionByIndex(int index, const std::string &text);
	OptionValueStruct* getOptionByText(const std::string &text);
	OptionValueStruct* getOptionByKey(int32 key);
	OptionValueStruct* getOptionByIndex(int index);
	const std::string getText();
	int geIndex();
};

#else

typedef struct optionValueStructure OptionValueStructure;
typedef struct optionGroup OptionGroup;

#endif

#ifdef __cplusplus
extern "C"
{
#endif

	OptionGroup* TSU_OptionGroupCreate(Font *display_font, uint text_size, float width, float height, Color *body_color);
	void TSU_OptionGroupDestroy(OptionGroup **optiongroup_ptr);
	void TSU_OptionGroupSetStates(OptionGroup *optiongroup_ptr, int control_state, int previous_state, int *change_state);
	int TSU_OptionGroupGetControlState(OptionGroup *optiongroup_ptr);
	void TSU_OptionGroupSetFocus(OptionGroup *optiongroup_ptr, bool focus);
	void TSU_OptionGroupInputEvent(OptionGroup *optiongroup_ptr, int event_type, int key);
	void TSU_OptionGroupAdd(OptionGroup *optiongroup_ptr, int32 key, const char *text);
	void TSU_OptionGroupDelete(OptionGroup *optiongroup_ptr, const char *text);
	void TSU_OptionGroupDeleteByIndex(OptionGroup *optiongroup_ptr, int index);
	const char* TSU_OptionGroupGetTextSelected(OptionGroup *optiongroup_ptr);
	void TSU_OptionGroupSetDisplayText(OptionGroup *optiongroup_ptr, const char *text);
	void TSU_OptionGroupClear(OptionGroup *optiongroup_ptr);
	void TSU_OptionGroupSelectOptionByIndex(OptionGroup *optiongroup_ptr, int index);
	void TSU_OptionGroupSelectOptionByKey(OptionGroup *optiongroup_ptr, int32 key);
	void TSU_OptionGroupSelectOptionByText(OptionGroup *optiongroup_ptr, const char *text);
	int32 TSU_OptionGroupGetKeySelected(OptionGroup *optiongroup_ptr);
	int TSU_OptionGroupGetIndexSelected(OptionGroup *optiongroup_ptr);
	void TSU_OptionGroupSetOptionByText(OptionGroup *optiongroup_ptr, const char *curent_text, const char *new_text);
	void TSU_OptionGroupSetOptionByKey(OptionGroup *optiongroup_ptr, int32 key, const char *text);
	void TSU_OptionGroupSetOptionByIndex(OptionGroup *optiongroup_ptr, int index, const char *text);
	OptionValueStructure* TSU_OptionGroupGetOptionByText(OptionGroup *optiongroup_ptr, const char *text);
	OptionValueStructure* TSU_OptionGroupGetOptionByKey(OptionGroup *optiongroup_ptr, int32 key);
	OptionValueStructure* TSU_OptionGroupGetOptionByIndex(OptionGroup *optiongroup_ptr, int index);
	int32 TSU_OptionGroupGetKey(OptionValueStructure *option_ptr);
	const char* TSU_OptionGroupGetText(OptionValueStructure *option_ptr);	

#ifdef __cplusplus
};
#endif

#endif /* __TSUNAMI_DRW_OPTION_GROUP_H */
