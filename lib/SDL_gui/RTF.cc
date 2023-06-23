
#include <kos.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "SDL_gui.h"

extern "C" {
	
	SDL_Surface *GetScreen();
	
	static char FontList[8][NAME_MAX];

	/* Note, this is only one way of looking up fonts */
	static int FontFamilyToIndex(RTF_FontFamily family) {
		switch(family) {
			case RTF_FontDefault:
				return 0;
			case RTF_FontRoman:
				return 1;
			case RTF_FontSwiss:
				return 2;
			case RTF_FontModern:
				return 3;
			case RTF_FontScript:
				return 4;
			case RTF_FontDecor:
				return 5;
			case RTF_FontTech:
				return 6;
			case RTF_FontBidi:
				return 7;
			default:
				return 0;
		}
	}

	static Uint16 UTF8_to_UNICODE(const char *utf8, int *advance) {
		int i = 0;
		Uint16 ch;

		ch = ((const unsigned char *)utf8)[i];
		if ( ch >= 0xF0 ) {
			ch  =  (Uint16)(utf8[i]&0x07) << 18;
			ch |=  (Uint16)(utf8[++i]&0x3F) << 12;
			ch |=  (Uint16)(utf8[++i]&0x3F) << 6;
			ch |=  (Uint16)(utf8[++i]&0x3F);
		} else if ( ch >= 0xE0 ) {
			ch  =  (Uint16)(utf8[i]&0x3F) << 12;
			ch |=  (Uint16)(utf8[++i]&0x3F) << 6;
			ch |=  (Uint16)(utf8[++i]&0x3F);
		} else if ( ch >= 0xC0 ) {
			ch  =  (Uint16)(utf8[i]&0x3F) << 6;
			ch |=  (Uint16)(utf8[++i]&0x3F);
		}
		*advance = (i+1);
		return ch;
	}
	
	static void *CreateFont(const char *name, RTF_FontFamily family, int charset, int size, int style) {
		int index;
		TTF_Font *font;

		index = FontFamilyToIndex(family);
		if (!FontList[index][0])
			index = 0;

		font = TTF_OpenFont(FontList[index], size);
		if (font) {
			int TTF_style = TTF_STYLE_NORMAL;
			if ( style & RTF_FontBold )
				TTF_style |= TTF_STYLE_BOLD;
			if ( style & RTF_FontItalic )
				TTF_style |= TTF_STYLE_ITALIC;
			if ( style & RTF_FontUnderline )
				TTF_style |= TTF_STYLE_UNDERLINE;
			TTF_SetFontStyle(font, style);
		}

		/* FIXME: What do we do with the character set? */

		return font;
	}

	static int GetLineSpacing(void *_font) {
		TTF_Font *font = (TTF_Font *)_font;
		return TTF_FontLineSkip(font);
	}

	static int GetCharacterOffsets(void *_font, const char *text, int *byteOffsets, int *pixelOffsets, int maxOffsets) {
		TTF_Font *font = (TTF_Font *)_font;
		int i = 0;
		int bytes = 0;
		int pixels = 0;
		int advance;
		Uint16 ch;
		while ( *text && i < maxOffsets ) {
			byteOffsets[i] = bytes;
			pixelOffsets[i] = pixels;
			++i;

			ch = UTF8_to_UNICODE(text, &advance);
			text += advance;
			bytes += advance;
			TTF_GlyphMetrics(font, ch, NULL, NULL, NULL, NULL, &advance);
			pixels += advance;
		}
		if ( i < maxOffsets ) {
			byteOffsets[i] = bytes;
			pixelOffsets[i] = pixels;
		}
		return i;
	}

	static SDL_Surface *RenderText(void *_font, const char *text, SDL_Color fg) {
		TTF_Font *font = (TTF_Font *)_font;
		return TTF_RenderUTF8_Blended(font, text, fg);
	}

	static void FreeFont(void *_font) {
		TTF_Font *font = (TTF_Font *)_font;
		TTF_CloseFont(font);
	}
}


GUI_RTF::GUI_RTF(const char *aname, const char *file, const char *default_font, int x, int y, int w, int h)
: GUI_Widget(aname, x, y, w, h)
{
	SetupFonts(default_font);
	ctx = RTF_CreateContext(&font);
	/*
	file_t f = fs_open(file, O_RDONLY);
	uint32 bsize = fs_total(f);
	uint8 *buff = (uint8*) malloc(bsize);
	fs_read(f, buff, bsize);
	fs_close(f);
	
	SDL_RWops *rw = SDL_RWFromMem(buff, bsize);
	*/
	if(ctx == NULL || RTF_Load(ctx, file) < 0) {
		assert(0);
	}
	
	//if(buff) free(buff);
	SetupSurface();
}


GUI_RTF::GUI_RTF(const char *aname, SDL_RWops *src, int freesrc, const char *default_font, int x, int y, int w, int h)
: GUI_Widget(aname, x, y, w, h)
{

	memset(&FontList[0][0], 0, sizeof(FontList));
	SetupFonts(default_font);
	ctx = RTF_CreateContext(&font);
	
	if(ctx == NULL || RTF_Load_RW(ctx, src, freesrc) < 0) {
		assert(0);
	}
	SetupSurface();
}

GUI_RTF::~GUI_RTF()
{
	RTF_FreeContext(ctx);
	surface->DecRef();
}


void GUI_RTF::SetupFonts(const char *default_font) {
	font.version = RTF_FONT_ENGINE_VERSION;
    font.CreateFont = CreateFont;
    font.GetLineSpacing = GetLineSpacing;
    font.GetCharacterOffsets = GetCharacterOffsets;
    font.RenderText = RenderText;
    font.FreeFont = FreeFont;
	
	if(FontList[0][0] != '/') {
		int i;
		for(i = 0; i < 8; i++) {
			if(default_font != NULL) {
				strcpy(FontList[i], default_font);
			} else {
				sprintf(FontList[i], "%s/fonts/ttf/arial_lite.ttf", getenv("PATH"));
			}
		}
	}
}

void GUI_RTF::SetupSurface() {
	offset = 0;
	SDL_Surface *screen = GetScreen();
	surface = GUI_SurfaceCreate("rtf_render", 
								screen->flags, 
								area.w, area.h, 
								screen->format->BitsPerPixel,
								screen->format->Rmask, 
								screen->format->Gmask, 
								screen->format->Bmask, 
								screen->format->Amask); 
	SetBgColor(255, 255, 255);
}

int GUI_RTF::SetFont(RTF_FontFamily family, const char *file) {
	
	int i = FontFamilyToIndex(family);
	
	if(i > -1 && i < 8) {
		strcpy(FontList[i], file);
		MarkChanged();
		return 1;
	}
	return 0;
}

int GUI_RTF::GetFullHeight() {
	return RTF_GetHeight(ctx, area.w);
}

void GUI_RTF::SetOffset(int value) {
	offset = value;
	MarkChanged();
}

void GUI_RTF::SetBgColor(int r, int g, int b) {
	color = surface->MapRGB(r, g, b);
	MarkChanged();
}

/* Get the title of an RTF document */
const char *GUI_RTF::GetTitle() {
	return RTF_GetTitle(ctx);
}

/* Get the subject of an RTF document */
const char *GUI_RTF::GetSubject() {
	return RTF_GetSubject(ctx);
}

/* Get the author of an RTF document */
const char *GUI_RTF::GetAuthor() {
	return RTF_GetAuthor(ctx);
}

void GUI_RTF::DrawWidget(const SDL_Rect *clip) {
	
	if (parent == 0)
		return;
	
	if (ctx && surface) {
		surface->Fill(NULL, color);
		RTF_Render(ctx, surface->GetSurface(), NULL, offset);
		SDL_Rect dr;
		SDL_Rect sr;
			
		sr.w = dr.w = surface->GetWidth();
		sr.h = dr.h = surface->GetHeight();
		sr.x = sr.y = 0;
		
		dr.x = area.x;
		dr.y = area.y + (area.h - dr.h) / 2;

		//if (GUI_ClipRect(&sr, &dr, clip))
			parent->Draw(surface, &sr, &dr);
	}
}

extern "C"
{

	GUI_Widget *GUI_RTF_Load(const char *name, const char *file, const char *default_font, int x, int y, int w, int h) {
		return new GUI_RTF(name, file, default_font, x, y, w, h);
	}
	
	GUI_Widget *GUI_RTF_LoadRW(const char *name, SDL_RWops *src, int freesrc, const char *default_font, int x, int y, int w, int h) {
		return new GUI_RTF(name, src, freesrc, default_font, x, y, w, h);
	}

	int GUI_RTF_GetFullHeight(GUI_Widget *widget) {
		return ((GUI_RTF *)widget)->GetFullHeight();
	}

	void GUI_RTF_SetOffset(GUI_Widget *widget, int value) {
		((GUI_RTF *)widget)->SetOffset(value);
	}
	
	void GUI_RTF_SetBgColor(GUI_Widget *widget, int r, int g, int b) {
		((GUI_RTF *)widget)->SetBgColor(r, g, b);
	}
	
	int GUI_RTF_SetFont(GUI_Widget *widget, RTF_FontFamily family, const char *file) {
		return ((GUI_RTF *)widget)->SetFont(family, file);
	}

	const char *GUI_RTF_GetTitle(GUI_Widget *widget) {
		return ((GUI_RTF *)widget)->GetTitle();
	}

	const char *GUI_RTF_GetSubject(GUI_Widget *widget) {
		return ((GUI_RTF *)widget)->GetSubject();
	}

	const char *GUI_RTF_GetAuthor(GUI_Widget *widget) {
		return ((GUI_RTF *)widget)->GetAuthor();
	}
}
