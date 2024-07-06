/*
   Tsunami for KallistiOS ##version##

   font.cpp

   Copyright (C) 2002 Megan Potter
   Copyright (C) 2024 Maniac Vera
   
*/

#include "font.h"
#include "tsunamiutils.h"

Font::Font(const std::filesystem::path &fn, int list) {
	m_list = list;
	m_font = nullptr;
	m_cxt = nullptr;
	if (!fn.empty()) {
		if (!loadFromFile(fn)) {
			assert( false );
		}
	} else {
		m_font = nullptr;
		m_cxt = nullptr;
	}
	m_a = m_r = m_g = m_b = 1.0f;
	m_ps = 24.0f;
}

Font::~Font() {
	if (m_cxt)
		plx_fcxt_destroy(m_cxt);
	if (m_font)
		plx_font_destroy(m_font);
}

bool Font::loadFromFile(const std::filesystem::path &fn) {
	if (m_cxt) {
		plx_fcxt_destroy(m_cxt);
		m_cxt = nullptr;
	}
	if (m_font) {
		plx_font_destroy(m_font);
		m_font = nullptr;
	}

	m_font = plx_font_load(fn.c_str());
	if (!m_font)
		return false;
	m_cxt = plx_fcxt_create(m_font, m_list);
	if (!m_cxt)
		return false;

	return true;
}

void Font::setFilter(int type) {
	plx_txr_setfilter(m_font->txr, type);
}

void Font::setColor(float r, float g, float b) {
	assert( m_cxt );

	m_r = r;
	m_g = g;
	m_b = b;
	plx_fcxt_setcolor4f(m_cxt, m_a, m_r, m_g, m_b);
}

void Font::setAlpha(float a) {
	assert( m_cxt );

	m_a = a;
	plx_fcxt_setcolor4f(m_cxt, m_a, m_r, m_g, m_b);
}

void Font::setSize(float size) {
	assert( m_cxt );

	m_ps = size;
	plx_fcxt_setsize(m_cxt, m_ps);
}

void Font::draw(float x, float y, float z, const std::string &text) {
	assert( m_cxt );

	plx_fcxt_setpos(m_cxt, x, y, z);
	plx_fcxt_begin(m_cxt);
	plx_fcxt_draw(m_cxt, text.c_str());
	plx_fcxt_end(m_cxt);
}

void Font::drawCharBegin(float x, float y, float z) {
	assert( m_cxt );

	plx_fcxt_setpos(m_cxt, x, y, z);
	plx_fcxt_begin(m_cxt);
}

Vector Font::drawCharGetPos() {
	assert( m_cxt );

	point_t p;
	plx_fcxt_getpos(m_cxt, &p);
	return Vector(p.x, p.y, p.z);
}

void Font::drawCharSetPos(const Vector & v) {
	assert( m_cxt );

	plx_fcxt_setpos(m_cxt, v.x, v.y, v.z);
}

float Font::drawChar(int ch) {
	assert( m_cxt );

	return plx_fcxt_draw_ch(m_cxt, ch);
}

void Font::drawCharEnd() {
	assert( m_cxt );

	plx_fcxt_end(m_cxt);
}

void Font::drawCentered(float x, float y, float z, const std::string &text) {
	assert( m_cxt );

	centerCoords(text, &x, &y);
	draw(x, y, z, text);
}

void Font::smearDraw(float x, float y, float z, const std::string &text) {
	assert( m_cxt );

	plx_fcxt_begin(m_cxt);

	// Draw the text itself
	plx_fcxt_setpos(m_cxt, x, y, z);
	plx_fcxt_draw(m_cxt, text.c_str());

	// Draw a nifty subtitle border
	// Guess all that time I spent on BakaSub wasn't wasted after all ^_-
	plx_fcxt_setcolor4f(m_cxt, m_a, 0.0f, 0.0f, 0.0f);
	plx_fcxt_setpos(m_cxt, x + 1, y + 0, z - 1.0f);
	plx_fcxt_draw(m_cxt, text.c_str());
	plx_fcxt_setpos(m_cxt, x + 1, y + 1, z - 1.0f);
	plx_fcxt_draw(m_cxt, text.c_str());
	plx_fcxt_setpos(m_cxt, x + 0, y + 1, z - 1.0f);
	plx_fcxt_draw(m_cxt, text.c_str());
	plx_fcxt_setpos(m_cxt, x - 1, y + 1, z - 1.0f);
	plx_fcxt_draw(m_cxt, text.c_str());
	plx_fcxt_setpos(m_cxt, x - 1, y + 0, z - 1.0f);
	plx_fcxt_draw(m_cxt, text.c_str());
	plx_fcxt_setpos(m_cxt, x - 1, y - 1, z - 1.0f);
	plx_fcxt_draw(m_cxt, text.c_str());
	plx_fcxt_setpos(m_cxt, x + 0, y + 0, z - 1.0f);
	plx_fcxt_draw(m_cxt, text.c_str());

	plx_fcxt_end(m_cxt);
	plx_fcxt_setcolor4f(m_cxt, m_a, m_r, m_g, m_b);
}

void Font::smearDrawCentered(float x, float y, float z, const std::string &text) {
	assert( m_cxt );

	centerCoords(text, &x, &y);
	smearDraw(x, y, z, text);
}

void Font::getCharExtents(int c, float * l, float * u, float * r, float * d) {
	assert( m_cxt );

	plx_fcxt_setsize(m_cxt, m_ps);
	plx_fcxt_char_metrics(m_cxt, c, l, u, r, d);
}

void Font::getTextSize(const std::string &text, float * w, float * h) {
	assert( m_cxt );

	float left, right, bot, top;

	plx_fcxt_setsize(m_cxt, m_ps);
	plx_fcxt_str_metrics(m_cxt, text.c_str(), &left, &top, &right, &bot);

	*w = left+right;
	*h = top+bot;
}

void Font::upperleftCoords(const std::string &text, float *x, float *y) {
	assert( m_cxt );

	float left, right, bot, top;

	plx_fcxt_setsize(m_cxt, m_ps);
	plx_fcxt_str_metrics(m_cxt, text.c_str(), &left, &top, &right, &bot);

	*y = *y + top;
}

void Font::centerCoords(const std::string &text, float *x, float *y) {
	assert( m_cxt );

	float left, right, bot, top;

	plx_fcxt_setsize(m_cxt, m_ps);
	plx_fcxt_str_metrics(m_cxt, text.c_str(), &left, &top, &right, &bot);

	*x = *x - (left+right)/2;
	*y = *y + (bot+top)/2;
}

extern "C"
{
	Font* TSU_FontCreate(const char *path, int list)
	{
		if (path != NULL) {
			return new Font(path, list);
		}
		else {
			return NULL;
		}
	}

	void TSU_FontDestroy(Font **font_ptr)
	{
		if (*font_ptr != NULL) {
			delete *font_ptr;
			*font_ptr = NULL;
		}
	}

	bool TSU_FontLoadFromFile(Font *font_ptr, const char *path)
	{
		if (font_ptr != NULL && path != NULL) {
			return font_ptr->loadFromFile(path);
		}
		else {
			return false;
		}
	}

	void TSU_FontSetFilter(Font *font_ptr, int type)
	{
		if (font_ptr != NULL) {
			font_ptr->setFilter(type);
		}
	}

	void TSU_FontSetColor(Font *font_ptr, float r, float g, float b)
	{
		if (font_ptr != NULL) {
			font_ptr->setColor(r, g, b);
		}
	}

	void TSU_FontSetAlpha(Font *font_ptr, float a)
	{
		if (font_ptr != NULL) {
			font_ptr->setAlpha(a);
		}
	}

	void TSU_FontSetSize(Font *font_ptr, float size)
	{
		if (font_ptr != NULL) {
			font_ptr->setSize(size);
		}		
	}

	void TSU_FontDraw(Font *font_ptr, float x, float y, float z, const char *text)
	{
		if (font_ptr != NULL) {
			font_ptr->draw(x, y, z, text);
		}
	}

	void TSU_FontDrawCentered(Font *font_ptr, float x, float y, float z, const char *text)
	{
		if (font_ptr != NULL) {
			font_ptr->drawCentered(x, y, z, text);
		}
	}

	void TSU_FontSmearDraw(Font *font_ptr, float x, float y, float z, const char *text)
	{
		if (font_ptr != NULL) {
			font_ptr->smearDraw(x, y, z, text);
		}
	}

	void TSU_FontSmearDrawCentered(Font *font_ptr, float x, float y, float z, const char *text)
	{
		if (font_ptr != NULL) {
			font_ptr->smearDrawCentered(x, y, z, text);
		}
	}

	void TSU_FontDrawCharBegin(Font *font_ptr, float x, float y, float z)
	{
		if (font_ptr != NULL) {
			font_ptr->drawCharBegin(x, y, z);
		}
	}

	Vector TSU_FontDrawCharGetPos(Font *font_ptr)
	{
		return font_ptr->drawCharGetPos();
	}

	void TSU_FontDrawCharSetPos(Font *font_ptr, const Vector *v)
	{
		if (font_ptr != NULL) {
			font_ptr->drawCharSetPos(*v);
		}
	}

	float TSU_FontDrawChar(Font *font_ptr, int ch)
	{
		if (font_ptr != NULL) {
			return font_ptr->drawChar(ch);
		}
		else {
			return 0.0;
		}
	}

	void TSU_FontDrawCharEnd(Font *font_ptr)
	{
		if (font_ptr != NULL) {
			font_ptr->drawCharEnd();
		}
	}

	void TSU_FontGetCharExtents(Font *font_ptr, int c, float * l, float * u, float * r, float * d)
	{
		if (font_ptr != NULL) {
			font_ptr->getCharExtents(c, l, u, r, d);
		}
	}

	void TSU_FontGetTextSize(Font *font_ptr, const char *text, float * w, float * h)
	{
		if (font_ptr != NULL) {
			font_ptr->getTextSize(text, w, h);
		}
	}

	void TSU_FontUpperleftCoords(Font *font_ptr, const char *text, float *x, float *y)
	{
		if (font_ptr != NULL) {
			font_ptr->upperleftCoords(text, x, y);
		}
	}

	void TSU_FontCenterCoords(Font *font_ptr, const char *text, float *x, float *y)
	{
		if (font_ptr != NULL) {
			font_ptr->centerCoords(text, x, y);
		}
	}
}