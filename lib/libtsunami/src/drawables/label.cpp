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

Label::Label(Font *fh, const std::string &text,
	     int size, bool centered, bool smear) {
	
	setObjectType(ObjectTypeEnum::LABEL_TYPE);
	m_fh = fh;
	m_text = text;
	m_size = size;
	m_centered = centered;
	m_smear = smear;

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

void Label::draw(int list) {
	if (list != PLX_LIST_TR_POLY)
		return;

	const Vector & p = getPosition();
	Color t = getColor();

	m_fh->setSize(m_size);
	m_fh->setAlpha(t.a);
	m_fh->setColor(t.r, t.g, t.b);
	if (m_centered) {
		if (m_smear)
			m_fh->smearDrawCentered(p.x, p.y, p.z, m_text.c_str());
		else
			m_fh->drawCentered(p.x, p.y, p.z, m_text.c_str());
	} else {
		if (m_smear)
			m_fh->smearDraw(p.x, p.y, p.z, m_text.c_str());
		else
			m_fh->draw(p.x, p.y, p.z, m_text.c_str());
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


extern "C"
{
	Label *TSU_LabelCreate(Font *font_ptr, const char *text, int size, bool centered, bool smear)
	{
		if (font_ptr != NULL) {
			return new Label(font_ptr, text, size, centered, smear);
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
}