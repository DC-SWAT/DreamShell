/*
   Tsunami for KallistiOS ##version##

   font.h

   Copyright (C) 2002 Megan Potter

*/

#ifndef __TSUNAMI_FONT_H
#define __TSUNAMI_FONT_H

#include <plx/font.h>
#include "vector.h"

#include <filesystem>
#include <string>

class Font {
public:
	Font(const std::filesystem::path &path, int list = PVR_LIST_TR_POLY);
	virtual ~Font();

	bool loadFromFile(const std::filesystem::path &path);

	void setFilter(int type);

	void setColor(float r, float g, float b);
	void setAlpha(float a);
	void setSize(float size);

	void draw(float x, float y, float z, const std::string &text);
	void drawCentered(float x, float y, float z, const std::string &text);
	void smearDraw(float x, float y, float z, const std::string &text);
	void smearDrawCentered(float x, float y, float z, const std::string &text);

	void drawCharBegin(float x, float y, float z);
	Vector drawCharGetPos();
	void drawCharSetPos(const Vector &v);
	float drawChar(int ch);
	void drawCharEnd();

	void getCharExtents(int c, float * l, float * u, float * r, float * d);
	void getTextSize(const std::string &text, float * w, float * h);
	void upperleftCoords(const std::string &text, float *x, float *y);
	void centerCoords(const std::string &text, float *x, float *y);

	operator plx_font_t * () const { return m_font; }
	operator plx_fcxt_t * () const { return m_cxt; }

private:
	plx_font_t	* m_font;
	plx_fcxt_t	* m_cxt;
	int		m_list;
	float		m_a, m_r, m_g, m_b;
	float		m_ps;
};

#endif	/* __FONTHELPER_H */
