
#include <kos.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include "SDL_gui.h"

#define NUM_FONT_FAMILIES 8

// Global pointer to the current RTF widget being processed.
// This is a workaround for the SDL_rtf library not supporting user data in callbacks.
static GUI_RTF *g_current_rtf = NULL;

typedef struct {
    const char *name;
    int r, g, b;
} ColorMapping;

static const ColorMapping color_map[] = {
    {"red", 239, 83, 80},
	{"green", 102, 187, 106},
	{"blue", 66, 165, 245},
    {"yellow", 255, 238, 88},
	{"purple", 171, 71, 188},
	{"orange", 255, 167, 38},
    {"black", 33, 33, 33},
	{"white", 250, 250, 250},
    {NULL, 0, 0, 0}
};

typedef struct {
    const char *name;
    int rtf_index;
} FontFamilyMapping;

static const FontFamilyMapping font_map[] = {
    {"default", 0}, {"arial", 0},
    {"roman", 1}, {"georgia", 1},
    {"swiss", 2}, {"verdana", 2},
    {"modern", 3}, {"m23", 3},
    {"script", 4}, {"comic", 4},
    {NULL, 0}
};

typedef struct {
    const char *name;
    const char *rtf_tag;
} AlignMapping;

static const AlignMapping align_map[] = {
    {"left", "\\ql"},
    {"center", "\\qc"},
    {"right", "\\qr"},
    {NULL, NULL}
};


static int find_color_index(const char *name) {
    for (int i = 0; color_map[i].name; ++i) {
        if (strcasecmp(name, color_map[i].name) == 0) {
            return i + 1;
        }
    }
    return 0;
}

static int find_font_index(const char *name) {
    for (int i = 0; font_map[i].name; ++i) {
        if (strcasecmp(name, font_map[i].name) == 0) {
            return font_map[i].rtf_index;
        }
    }
    return -1;
}

static const char* find_align_tag(const char *name) {
    for (int i = 0; align_map[i].name; ++i) {
        if (strcasecmp(name, align_map[i].name) == 0) {
            return align_map[i].rtf_tag;
        }
    }
    return NULL;
}

#define PIXELS_TO_TWIPS(px) ((px) * 20)

/**
 * @brief Converts a plain text string with simple markup into an RTF document.
 * 
 * This function takes a string and converts it into a valid RTF document string,
 * which can then be rendered by the RTF widget. It supports basic formatting
 * tags similar to BBCode. The tags can be enclosed in either square brackets []
 * or angle brackets <>.
 * 
 * Supported tags (examples use [], but <> works too):
 * - [b]...[/b]               - Bold text
 * - [i]...[/i]               - Italic text
 * - [u]...[/u]               - Underlined text
 * - [s]...[/s]               - Strikethrough text
 * - [size=N]...[/size]       - Font size in points
 * - [color=NAME]...[/color]   - Text color (e.g., red, green, blue)
 * - [font=FAMILY]...[/font]   - Font family (e.g., roman, swiss, modern)
 * - [align=X]...[/align]     - Text alignment (left, center, right)
 * - [indent=N]               - Left indent for the paragraph in pixels
 * - [firstline=N]            - First line indent for the paragraph in pixels
 * 
 * Newlines (\n) are converted to paragraphs. Special RTF characters like
 * '\', '{', and '}' are automatically escaped.
 * 
 * @param text The input string with markup.
 * @return A dynamically allocated string containing the RTF document. 
 *         The caller is responsible for freeing this memory.
 */
static char* MarkupToRTF(const char *text) {
	if (!text) {
		return NULL;
	}

	char header[1024];
	char *h_ptr = header;
	h_ptr += sprintf(h_ptr, "{\\rtf1\\ansi\\deff0{\\fonttbl{\\f0\\fswiss Arial;}{\\f1\\froman Georgia;}{\\f2\\fswiss Verdana;}{\\f3\\fmodern M23;}{\\f4\\fscript Comic Sans MS;}}{\\colortbl ;");
	for (int i = 0; color_map[i].name; ++i) {
		h_ptr += sprintf(h_ptr, "\\red%d\\green%d\\blue%d;", color_map[i].r, color_map[i].g, color_map[i].b);
	}
	h_ptr += sprintf(h_ptr, "}\\pard\\fs24 ");

	size_t buffer_size = strlen(header) + (strlen(text) * 3) + 16;
	char *rtf_text = (char *)malloc(buffer_size);
	if (!rtf_text) {
		return NULL;
	}

	char *q = rtf_text;
	strcpy(q, header);
	q += strlen(header);

	const char *p = text;
	int default_font_size = 24;
	int current_font_size = default_font_size;

	while (*p) {
		
		size_t current_size = q - rtf_text;
		if (current_size + 256 > buffer_size) {
			buffer_size *= 2;
			char *new_rtf_text = (char *)realloc(rtf_text, buffer_size);
			if (!new_rtf_text) {
				free(rtf_text);
				return NULL;
			}
			rtf_text = new_rtf_text;
			q = rtf_text + current_size;
		}

		if (*p == '[' || *p == '<') {
			char open_char = *p;
			char close_char = (open_char == '[') ? ']' : '>';

			const char* tag_start = p + 1;
			const char* tag_end = strchr(tag_start, close_char);

			if (tag_end) {

				char tag[64];
				size_t tag_len = tag_end - tag_start;

				if (tag_len < sizeof(tag) -1) {
					strncpy(tag, tag_start, tag_len);
					tag[tag_len] = '\0';
					bool is_closing = (*tag == '/');
					const char *tag_name = is_closing ? tag + 1 : tag;

					// Simple styles
					if (strcasecmp(tag_name, "b") == 0) q += sprintf(q, is_closing ? "\\b0 " : "\\b ");
					else if (strcasecmp(tag_name, "i") == 0) q += sprintf(q, is_closing ? "\\i0 " : "\\i ");
					else if (strcasecmp(tag_name, "u") == 0) q += sprintf(q, is_closing ? "\\ulnone " : "\\ul ");
					else if (strcasecmp(tag_name, "s") == 0) q += sprintf(q, is_closing ? "\\strike0 " : "\\strike ");

					// Tags with values
					else if (strncasecmp(tag_name, "size=", 5) == 0) {
						if (!is_closing) {
							current_font_size = atoi(tag_name + 5) * 2;
							q += sprintf(q, "\\fs%d ", current_font_size);
						}
					}
					else if (strcasecmp(tag_name, "size") == 0 && is_closing) {
						current_font_size = default_font_size;
						q += sprintf(q, "\\fs%d ", current_font_size);
					}
					else if (strncasecmp(tag_name, "color=", 6) == 0) {
						if (!is_closing) {
							int color_index = find_color_index(tag_name + 6);
							if (color_index > 0) q += sprintf(q, "{\\cf%d ", color_index);
						}
					}
					else if (strcasecmp(tag_name, "color") == 0 && is_closing) {
						*q++ = '}';
					}
					else if (strncasecmp(tag_name, "font=", 5) == 0) {
						if (!is_closing) {
							int font_index = find_font_index(tag_name + 5);
							if (font_index != -1) q += sprintf(q, "\\f%d ", font_index);
						}
					}
					else if (strcasecmp(tag_name, "font") == 0 && is_closing) {
						q += sprintf(q, "\\f0 ");
					}
					else if (strncasecmp(tag_name, "align=", 6) == 0) {
						if (!is_closing) {
							const char* align_tag = find_align_tag(tag_name + 6);
							if (align_tag) q += sprintf(q, "%s ", align_tag);
						}
					}
					else if (strcasecmp(tag_name, "align") == 0 && is_closing) {
						q += sprintf(q, "\\ql ");
					}
					else if (strncasecmp(tag_name, "indent=", 7) == 0) {
						if (!is_closing) q += sprintf(q, "\\li%d ", PIXELS_TO_TWIPS(atoi(tag_name + 7)));
					}
					else if (strncasecmp(tag_name, "firstline=", 10) == 0) {
						if (!is_closing) q += sprintf(q, "\\fi%d ", PIXELS_TO_TWIPS(atoi(tag_name + 10)));
					}
					else {
						// Not a known tag, treat as literal text
						memcpy(q, p, tag_end - p + 1);
						q += (tag_end - p + 1);
					}
					p = tag_end + 1;
					continue;
				}
			}
		}

		if (*p == '\\' && *(p + 1) == 'n') {
			q += sprintf(q, "\\par\\pard\\fs%d ", current_font_size);
			p++;
		}
		else if (*p == '\n') {
			q += sprintf(q, "\\par\\pard\\fs%d ", current_font_size);
		}
		else if (*p == '\\' || *p == '{' || *p == '}') {
			*q++ = '\\';
			*q++ = *p;
		}
		else {
			*q++ = *p;
		}
		p++;
	}
	*q++ = '}';
	*q = '\0';

	return rtf_text;
}

extern "C" {
	
	SDL_Surface *GetScreen();

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
		if (ch >= 0xF0) {
			ch = (Uint16)(utf8[i] & 0x07) << 18;
			ch |= (Uint16)(utf8[++i] & 0x3F) << 12;
			ch |= (Uint16)(utf8[++i] & 0x3F) << 6;
			ch |= (Uint16)(utf8[++i] & 0x3F);
		}
		else if (ch >= 0xE0) {
			ch = (Uint16)(utf8[i] & 0x3F) << 12;
			ch |= (Uint16)(utf8[++i] & 0x3F) << 6;
			ch |= (Uint16)(utf8[++i] & 0x3F);
		}
		else if (ch >= 0xC0) {
			ch = (Uint16)(utf8[i] & 0x3F) << 6;
			ch |= (Uint16)(utf8[++i] & 0x3F);
		}
		*advance = (i + 1);
		return ch;
	}

	static void *CreateFont(const char *name, RTF_FontFamily family, int charset, int size, int style) {
		int index;
		TTF_Font *font;

		if (!g_current_rtf) {
			return NULL;
		}

		index = FontFamilyToIndex(family);

		if (!g_current_rtf->FontList[index][0])
			index = 0;

		font = TTF_OpenFont(g_current_rtf->FontList[index], size);

		if (font) {
			int TTF_style = TTF_STYLE_NORMAL;
			if (style & RTF_FontBold)
				TTF_style |= TTF_STYLE_BOLD;
			if (style & RTF_FontItalic)
				TTF_style |= TTF_STYLE_ITALIC;
			if (style & RTF_FontUnderline)
				TTF_style |= TTF_STYLE_UNDERLINE;

			TTF_SetFontStyle(font, TTF_style);
		}

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
		while (*text && i < maxOffsets) {
			byteOffsets[i] = bytes;
			pixelOffsets[i] = pixels;
			++i;

			ch = UTF8_to_UNICODE(text, &advance);
			text += advance;
			bytes += advance;
			TTF_GlyphMetrics(font, ch, NULL, NULL, NULL, NULL, &advance);
			pixels += advance;
		}
		if (i < maxOffsets) {
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
	surface = NULL;
	offset = 0;
	FontList = new char[NUM_FONT_FAMILIES][NAME_MAX];
	SetupFonts(default_font);

	g_current_rtf = this;
	ctx = RTF_CreateContext(&font);

	if(ctx) {
		RTF_Load(ctx, file);
	}

	g_current_rtf = NULL;
	SetupSurface();
}


GUI_RTF::GUI_RTF(const char *aname, SDL_RWops *src, int freesrc, const char *default_font, int x, int y, int w, int h)
: GUI_Widget(aname, x, y, w, h)
{
	surface = NULL;
	offset = 0;
	FontList = new char[NUM_FONT_FAMILIES][NAME_MAX];
	SetupFonts(default_font);

	g_current_rtf = this;
	ctx = RTF_CreateContext(&font);

	if(ctx) {
		RTF_Load_RW(ctx, src, freesrc);
	}

	g_current_rtf = NULL;
	SetupSurface();
}

GUI_RTF::GUI_RTF(const char *aname, const char *text, int x, int y, int w, int h, const char *default_font)
: GUI_Widget(aname, x, y, w, h)
{
	ctx = NULL;
	surface = NULL;
	offset = 0;
	FontList = new char[NUM_FONT_FAMILIES][NAME_MAX];
	SetupFonts(default_font);
	SetText(text);
	SetupSurface();
}

GUI_RTF::~GUI_RTF()
{
	if(ctx) RTF_FreeContext(ctx);
	if(surface) surface->DecRef();
	delete[] FontList;
}


void GUI_RTF::SetupFonts(const char *default_font) {
	font.version = RTF_FONT_ENGINE_VERSION;
    font.CreateFont = CreateFont;
    font.GetLineSpacing = GetLineSpacing;
    font.GetCharacterOffsets = GetCharacterOffsets;
    font.RenderText = RenderText;
    font.FreeFont = FreeFont;

	memset(FontList, 0, sizeof(char) * NUM_FONT_FAMILIES * NAME_MAX);
	
	const char *base_path = getenv("PATH");

	if (default_font != NULL) {
		for (int i = 0; i < NUM_FONT_FAMILIES; i++) {
			strncpy(FontList[i], default_font, NAME_MAX - 1);
		}
	}
	else {
		snprintf(FontList[FontFamilyToIndex(RTF_FontDefault)], NAME_MAX, "%s/fonts/ttf/arial_lite.ttf", base_path);
		snprintf(FontList[FontFamilyToIndex(RTF_FontRoman)],   NAME_MAX, "%s/fonts/ttf/georgia.ttf", base_path);
		snprintf(FontList[FontFamilyToIndex(RTF_FontSwiss)],   NAME_MAX, "%s/fonts/ttf/verdana.ttf", base_path);
		snprintf(FontList[FontFamilyToIndex(RTF_FontModern)],  NAME_MAX, "%s/fonts/ttf/m23.ttf", base_path);
		snprintf(FontList[FontFamilyToIndex(RTF_FontScript)],  NAME_MAX, "%s/fonts/ttf/comic_lite.ttf", base_path);
		snprintf(FontList[FontFamilyToIndex(RTF_FontDecor)],   NAME_MAX, "%s/fonts/ttf/arial_lite.ttf", base_path);
		snprintf(FontList[FontFamilyToIndex(RTF_FontTech)],    NAME_MAX, "%s/fonts/ttf/arial_lite.ttf", base_path);
		snprintf(FontList[FontFamilyToIndex(RTF_FontBidi)],    NAME_MAX, "%s/fonts/ttf/arial_lite.ttf", base_path);
	}
}

void GUI_RTF::SetupSurface() {
	SDL_Surface *screen = GetScreen();

	if(surface) {
		surface->DecRef();
	}
	else {
		color = SDL_MapRGB(screen->format, 255, 255, 255);
	}
	surface = GUI_SurfaceCreate("rtf_render",
								screen->flags,
								area.w, area.h,
								screen->format->BitsPerPixel,
								screen->format->Rmask,
								screen->format->Gmask,
								screen->format->Bmask,
								screen->format->Amask);
}

int GUI_RTF::SetFont(RTF_FontFamily family, const char *file) {

	int i = FontFamilyToIndex(family);

	if(i > -1 && i < NUM_FONT_FAMILIES) {
		strncpy(FontList[i], file, NAME_MAX - 1);
		MarkChanged();
		return 1;
	}
	return 0;
}

int GUI_RTF::GetFullHeight() {
	g_current_rtf = this;
	int height = RTF_GetHeight(ctx, area.w);
	g_current_rtf = NULL;
	return height;
}

void GUI_RTF::SetText(const char *text) {

	g_current_rtf = this;

	if (ctx) {
		RTF_FreeContext(ctx);
	}

	ctx = RTF_CreateContext(&font);

	if (ctx) {
		char *rtf_text = MarkupToRTF(text);
		if (rtf_text) {
			SDL_RWops *rw = SDL_RWFromMem(rtf_text, strlen(rtf_text));
			RTF_Load_RW(ctx, rw, 1);
			free(rtf_text);
		}
	}
	g_current_rtf = NULL;
	MarkChanged();
}

void GUI_RTF::SetSize(int w, int h) {
	area.w = w;
	area.h = h;
	SetupSurface();
	MarkChanged();
}

void GUI_RTF::SetHeight(int h) {
	SetSize(area.w, h);
}

void GUI_RTF::SetWidth(int w) {
	SetSize(w, area.h);
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
		g_current_rtf = this;
		RTF_Render(ctx, surface->GetSurface(), NULL, offset);
		g_current_rtf = NULL;
		SDL_Rect dr;
		SDL_Rect sr;

		sr.w = dr.w = surface->GetWidth();
		sr.h = dr.h = surface->GetHeight();
		sr.x = sr.y = 0;

		dr.x = area.x;
		dr.y = area.y;

		if (GUI_ClipRect(&sr, &dr, clip))
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

	GUI_Widget *GUI_RTF_CreateFromText(const char *name, const char *text, const char *default_font, int x, int y, int w, int h) {
		return new GUI_RTF(name, text, x, y, w, h, default_font);
	}

	int GUI_RTF_GetFullHeight(GUI_Widget *widget) {
		return ((GUI_RTF *)widget)->GetFullHeight();
	}

	void GUI_RTF_SetOffset(GUI_Widget *widget, int value) {
		((GUI_RTF *)widget)->SetOffset(value);
	}

	void GUI_RTF_SetText(GUI_Widget *widget, const char *text) {
		((GUI_RTF *)widget)->SetText(text);
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
