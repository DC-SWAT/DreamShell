/*
   Tsunami for KallistiOS ##version##

   label.h

   Copyright (C) 2002 Megan Potter

*/

#ifndef __TSUNAMI_DRW_LABEL_H
#define __TSUNAMI_DRW_LABEL_H

#include "../drawable.h"
#include "../font.h"

#include <string>

class Label : public Drawable {
public:
	Label(std::shared_ptr<Font> fh, const std::string &text,
	      int size, bool centered, bool smear);
	virtual ~Label();

	void setText(const std::string &text);
	void setFont(std::shared_ptr<Font> f);

	virtual void draw(int list);

private:
	std::shared_ptr<Font>	m_fh;
	std::string	m_text;
	int		m_size;
	bool		m_centered;
	bool		m_smear;
};

#endif	/* __TSUNAMI_DRW_LABEL_H */
