/*
   Tsunami for KallistiOS ##version##

   label.h

   Copyright (C) 2002 Megan Potter
   Copyright (C) 2024 Maniac Vera

*/

#ifndef __TSUNAMI_DRW_LABEL_H
#define __TSUNAMI_DRW_LABEL_H

#include "../drawable.h"
#include "../font.h"

#ifdef __cplusplus

#include <string>

class Label : public Drawable {
public:
	Label(Font *fh, const std::string &text,
	      int size, bool centered, bool smear, bool fix_width);
	virtual ~Label();

	void setText(const std::string &text);
	void setFont(Font *f);
	Font* getFont();
	std::string fixTextWidth(const std::string &text);

	virtual void draw(pvr_list_type_t list);
	const std::string getText();
	void setSmear(bool smear);
	void setFixWidth(bool fiw_width);
	void setCenter(bool centered);
	bool isCentered() { return m_centered; }
	void getSize(float *x, float *y);
	
private:
	void refreshSize();
	void onMouseOver();

	Font 	*m_fh;
	std::string	m_text;
	int		m_size;
	bool	m_centered;
	bool	m_smear;
	bool	m_fix_width;
};

#else

typedef struct label Label;

#endif

#ifdef __cplusplus
extern "C"
{
#endif

Label* TSU_LabelCreate(Font *font_ptr, const char *text, int size, bool centered, bool smear, bool fix_width);
void TSU_LabelDestroy(Label **label_ptr);
void TSU_LabelSetText(Label *label_ptr, const char *text);
void TSU_LabelSetFont(Label *label_ptr, Font *font_ptr);
Font* TSU_LabelGetFont(Label *label_ptr);
const char* TSU_LabelGetText(Label *label_ptr);
void TSU_LabelSetCenter(Label *label_ptr, bool centered);
void TSU_LabelSetSmear(Label *label_ptr, bool smear);
void TSU_LabelSetFixWidth(Label *label_ptr, bool fix_width);
void TSU_LabelSetTranslate(Label *label_ptr, const Vector *v);
void TSU_LabelSetTint(Label *label_ptr, const Color *tint);
void TSU_LabelIsCentered(Label *label_ptr);
void TSU_LabelGetSize(Label *label_ptr, float *x, float *y);
void TSU_LabelSetSize(Label *label_ptr, float x, float y);
void TSU_LabelSetWindowState(Label *label_ptr, int window_state);
int TSU_LabelGetWindowState(Label *label_ptr);

#ifdef __cplusplus
};
#endif

#endif	/* __TSUNAMI_DRW_LABEL_H */
