/*
   Tsunami for KallistiOS ##version##

   label.cpp

   Copyright (C) 2002 Megan Potter
   Copyright (C) 2024 Maniac Vera
   
*/

#include <plx/list.h>
#include "drawables/label.h"
#include "tsunamiutils.h"
#include <cstring>

extern "C" {
	uint8 SDL_GetMouseState (int *x, int *y);
}

Label::Label(Font *fh, const std::string &text,
	     int size, bool centered, bool smear, bool fix_width) {
	setObjectType(ObjectTypeEnum::LABEL_TYPE);
	m_fh = fh;
	m_text = text;
	m_size = size;
	m_centered = centered;
	m_smear = smear;
	m_fix_width = fix_width;
	refreshSize();
}

Label::~Label() {
}

void Label::setText(const std::string &text) {
	m_text = text;
}

void Label::setFont(Font *f) {
	m_fh = f;
}

Font* Label::getFont() {
	m_fh->setSize(m_size);
	return m_fh;
}

std::string Label::fixTextWidth(const std::string &text) {
	std::string draw_text = "";
	if (m_fix_width && m_width > 0 && !text.empty()) {		
		float tw, th;
		for (size_t i = 0; i < text.size(); i++) {
			draw_text += text[i];
			m_fh->getTextSize(draw_text, &tw, &th);

			if (tw >= m_width) {
				draw_text.pop_back();
				break;
			}
		}
	}
	else {
		draw_text = text;
	}

	return draw_text;
}

void Label::onMouseOver() {
	if (m_mouse_over_function != nullptr && getWindowState() == InputEventState::getGlobalWindowState()
		&& !isReadOnly() && m_height > 0 && m_width > 0) {
		int x, y;
		SDL_GetMouseState(&x, &y);
		Vector pos = getPosition();

		float drawable_width = 0, drawable_height = 0;
		getFont()->getTextSize(getText(), &drawable_width, &drawable_height);
		
		if (isCentered()) {
			pos.x -= drawable_width / 2;
			pos.y += drawable_height / 2;
		}

		if ((x >= pos.x) && (x < pos.x + drawable_width) && (y <= pos.y) && (y > pos.y - drawable_height)) {
			m_mouse_over_function(this, getObjectType(), getId());
		}
	}
}

void Label::draw(int list) {
	if (list != PLX_LIST_TR_POLY)
		return;
	
	const Vector & p = getPosition();
	Color t = getColor();

	m_fh->setSize(m_size);
	m_fh->setAlpha(t.a);
	m_fh->setColor(t.r, t.g, t.b);
	onMouseOver();

	if (m_centered) {
		if (m_smear)
			m_fh->smearDrawCentered(p.x, p.y, p.z, m_text.c_str());
		else
			m_fh->drawCentered(p.x, p.y, p.z, m_text.c_str());
	} else {
		std::string draw_text = fixTextWidth(m_text.c_str());
		if (m_smear)
			m_fh->smearDraw(p.x, p.y, p.z, draw_text.c_str());
		else
			m_fh->draw(p.x, p.y, p.z, draw_text.c_str());
	}
}

const std::string Label::getText() {
	return m_text;
}

void Label::setCenter(bool centered) {
	m_centered = centered;
}

void Label::setSmear(bool smear) {
	m_smear = smear;
}

void Label::setFixWidth(bool fix_width) {
	m_fix_width = fix_width;
}

void Label::getSize(float *width, float *height) { 
	refreshSize();
	*width = m_width;
	*height = m_height;
}

void Label::refreshSize() {
	m_fh->setSize(m_size);
	float tw, th;
	m_fh->getTextSize(m_text, &tw, &th);

	m_width = tw;
	m_height = th;
}

extern "C"
{
	Label *TSU_LabelCreate(Font *font_ptr, const char *text, int size, bool centered, bool smear, bool fix_width)
	{
		if (font_ptr != NULL) {
			return new Label(font_ptr, text, size, centered, smear, fix_width);
		}
		else {
			return NULL;
		}
	}

	void TSU_LabelDestroy(Label **label_ptr)
	{
		if (*label_ptr != NULL) {
			delete *label_ptr;
			*label_ptr = NULL;
		}
	}

	void TSU_LabelSetText(Label *label_ptr, const char *text)
	{
		if (label_ptr != NULL) {
			label_ptr->setText(text);
		}
	}

	void TSU_LabelSetFont(Label *label_ptr, Font *font_ptr)
	{
		if (label_ptr != NULL && font_ptr != NULL) {
			label_ptr->setFont(font_ptr);
		}
	}

	Font* TSU_LabelGetFont(Label *label_ptr)
	{
		if (label_ptr != NULL) {
			return label_ptr->getFont();
		}
		else {
			return NULL;
		}
	}	

	const char* TSU_LabelGetText(Label *label_ptr)
	{
		if (label_ptr != NULL) {
			static char text[255] = {0};
			const std::string &src = label_ptr->getText();

			strncpy(text, src.c_str(), sizeof(text) - 1);
			text[sizeof(text) - 1] = '\0';

			return text;
		}
		else {
			return NULL;
		}
	}

	void TSU_LabelSetCenter(Label *label_ptr, bool centered)
	{
		if (label_ptr != NULL) {
			label_ptr->setCenter(centered);
		}
	}

	void TSU_LabelSetSmear(Label *label_ptr, bool smear)
	{
		if (label_ptr != NULL) {
			label_ptr->setSmear(smear);
		}
	}

	void TSU_LabelSetFixWidth(Label *label_ptr, bool fix_width)
	{
		if (label_ptr != NULL) {
			label_ptr->setFixWidth(fix_width);
		}
	}

	void TSU_LabelSetTranslate(Label *label_ptr, const Vector *v)
	{
		if (label_ptr != NULL) {
			label_ptr->setTranslate(*v);
		}
	}

	void TSU_LabelSetTint(Label *label_ptr, const Color *tint)
	{
		if (label_ptr != NULL) {
			label_ptr->setTint(*tint);
		}
	}

	void TSU_LabelIsCentered(Label *label_ptr)
	{
		if (label_ptr != NULL) {
			label_ptr->isCentered();
		}
	}

	void TSU_LabelGetSize(Label *label_ptr, float *x, float *y)
	{
		if (label_ptr != NULL) {
			return label_ptr->getSize(x, y);
		}
	}

	void TSU_LabelSetSize(Label *label_ptr, float x, float y)
	{
		if (label_ptr != NULL) {
			return label_ptr->setSize(x, y);
		}
	}

	void TSU_LabelSetWindowState(Label *label_ptr, int window_state)
	{
		if (label_ptr != NULL)
		{
			label_ptr->setWindowState(window_state);
		}
	}

	int TSU_LabelGetWindowState(Label *label_ptr)
	{
		if (label_ptr != NULL)
		{
			return label_ptr->getWindowState();
		}

		return 0;
	}
}