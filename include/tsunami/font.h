/*
   Tsunami for KallistiOS ##version##

   font.h

   Copyright (C) 2002 Megan Potter
   Copyright (C) 2024 Maniac Vera

*/

#ifndef __TSUNAMI_FONT_H
#define __TSUNAMI_FONT_H

// #include <plx/font.h>
#include "../plx/font.h"
#include "vector.h"
#include <stdbool.h>

#ifdef __cplusplus

#include <filesystem>
#include <string>

class Font {
public:
	Font(const std::filesystem::path &path, pvr_list_type_t list = PVR_LIST_TR_POLY);
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
	pvr_list_type_t	m_list;
	float		m_a, m_r, m_g, m_b;
	float		m_ps;
};

#else

typedef struct font Font;

#endif


#ifdef __cplusplus
extern "C"
{
#endif

Font* TSU_FontCreate(const char *path, pvr_list_type_t list);
void TSU_FontDestroy(Font **font_ptr);
bool TSU_FontLoadFromFile(Font *font_ptr, const char *path);
void TSU_FontSetFilter(Font *font_ptr, int type);

void TSU_FontSetColor(Font *font_ptr, float r, float g, float b);
void TSU_FontSetAlpha(Font *font_ptr, float a);
void TSU_FontSetSize(Font *font_ptr, float size);

void TSU_FontDraw(Font *font_ptr, float x, float y, float z, const char *text);
void TSU_FontDrawCentered(Font *font_ptr, float x, float y, float z, const char *text);
void TSU_FontSmearDraw(Font *font_ptr, float x, float y, float z, const char *text);
void TSU_FontSmearDrawCentered(Font *font_ptr, float x, float y, float z, const char *text);

void TSU_FontDrawCharBegin(Font *font_ptr, float x, float y, float z);
Vector TSU_FontDrawCharGetPos(Font *font_ptr);
void TSU_FontDrawCharSetPos(Font *font_ptr, const Vector *v);
float TSU_FontDrawChar(Font *font_ptr, int ch);
void TSU_FontDrawCharEnd(Font *font_ptr);

void TSU_FontGetCharExtents(Font *font_ptr, int c, float * l, float * u, float * r, float * d);
void TSU_FontGetTextSize(Font *font_ptr, const char *text, float * w, float * h);
void TSU_FontUpperleftCoords(Font *font_ptr, const char *text, float *x, float *y);
void TSU_FontCenterCoords(Font *font_ptr, const char *text, float *x, float *y);

#ifdef __cplusplus
};
#endif

#endif	/* __FONTHELPER_H */
