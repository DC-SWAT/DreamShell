/** 
 * \file       SDL_gui.h
 * \brief      SDL GUI for DreamShell
 * \date       2005-2014
 * \author     SWAT
 * \copyright  http://www.dc-swat.ru
 */

#include "SDL.h"
#include "SDL_image.h"
#include "SDL_ttf.h"
#include "SDL_rtf.h"

#ifndef SDL_GUI_H
#define SDL_GUI_H

typedef void GUI_CallbackFunction(void *);

#ifdef __cplusplus


struct GUI_Rect
{
	int x, y, w, h;
	GUI_Rect(void) : x(0), y(0), w(0), h(0) { }
	GUI_Rect(int ww, int hh) : x(0), y(0), w(ww), h(hh) { }
	GUI_Rect(int xx, int yy, int ww, int hh) : x(xx), y(yy), w(ww), h(hh) { }
	GUI_Rect Adjust(int xx, int yy) const { return GUI_Rect(x + xx, y + yy, w, h); }
};

struct GUI_Color
{
	float r, g, b;
	GUI_Color(void) : r(0), g(0), b(0) { }
	GUI_Color(float rr, float gg, float bb) : r(rr), g(gg), b(bb) { }
};

class GUI_Exception
{
	protected:
		char *message;
	public:
		GUI_Exception(const char *fmt, ...);
		GUI_Exception(const GUI_Exception &err);
		virtual ~GUI_Exception(void);
		const char *GetMessage(void);
};

class GUI_Object
{
	private:
		int refcount;
		char *name;
	public:
		GUI_Object(const char *aname);
		virtual ~GUI_Object(void);
		
		void SetName(const char *s);
		const char *GetName(void);
		int CheckName(const char *aname);
		
		void IncRef(void);
		int DecRef(void);
		int GetRef(void);
		int Trash(void);
};

class GUI_Callback : public GUI_Object
{
	protected:
	public:
		GUI_Callback(const char *aname);
		~GUI_Callback(void);
		virtual void Call(GUI_Object *object)=0;
};

class GUI_Callback_C : public GUI_Callback
{
	protected:
		void *data;
		GUI_CallbackFunction *freefunc;
		GUI_CallbackFunction *function;
	public:
		GUI_Callback_C(GUI_CallbackFunction *function, GUI_CallbackFunction *freefunc, void *data);
		virtual ~GUI_Callback_C(void);
		virtual void Call(GUI_Object *object);
};

template <class T> class GUI_EventHandler : public GUI_Callback
{
	typedef void handler(GUI_Object *sender);
	typedef handler (T::*method);
	protected:
		method f;
		T *obj;
	public:
		GUI_EventHandler<T>(T *t, method m) : GUI_Callback(NULL) {
			// FIXME keeps a borrowed reference
			obj = t;
			f = m;
		}
		virtual void Call(GUI_Object *sender) {
			(obj->*f)(sender);
		}
};

class GUI_Surface : public GUI_Object
{
	protected:
		SDL_Surface *surface;
	public:
		GUI_Surface(const char *aname, SDL_Surface *image);
		GUI_Surface(const char *aname, int f, int w, int h, int d, int rm, int gm, int bm, int am);
		GUI_Surface(const char *fn);
		GUI_Surface(const char *fn, SDL_Rect *selection);
		virtual ~GUI_Surface(void);

		void DisplayFormat(void);
		void SetAlpha(Uint32 flag, Uint8 alpha);
		void SetColorKey(Uint32 c);
		void Blit(SDL_Rect *src_r, GUI_Surface *dst, SDL_Rect *dst_r);
		void UpdateRects(int n, SDL_Rect *rects);
		void UpdateRect(int x, int y, int w, int h);
		void Fill(SDL_Rect *r, Uint32 c);
		
		void BlitRGBA(SDL_Rect * srcrect, GUI_Surface * dst, SDL_Rect * dstrect);
		void Pixel(Sint16 x, Sint16 y, Uint32 c);
		void Line(Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 c);
		void LineAA(Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 c);
		void LineH(Sint16 x1, Sint16 x2, Sint16 y, Uint32 c);
		void LineV(Sint16 x1, Sint16 y1, Sint16 y2, Uint32 c);
		void ThickLine(Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint8 width, Uint32 c);
		void Rectagle(Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 c);
		void RectagleRouded(Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 rad, Uint32 c);
		void Box(Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 c);
		void BoxRouded(Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 rad, Uint32 c);
		void Circle(Sint16 x, Sint16 y, Sint16 rad, Uint32 c);
		void CircleAA(Sint16 x, Sint16 y, Sint16 rad, Uint32 c);
		void CircleFill(Sint16 x, Sint16 y, Sint16 rad, Uint32 c);
		void Arc(Sint16 x, Sint16 y, Sint16 rad, Sint16 start, Sint16 end, Uint32 c);
		void Ellipse(Sint16 x, Sint16 y, Sint16 rx, Sint16 ry, Uint32 c);
		void EllipseAA(Sint16 x, Sint16 y, Sint16 rx, Sint16 ry, Uint32 c);
		void EllipseFill(Sint16 x, Sint16 y, Sint16 rx, Sint16 ry, Uint32 c);
		void Pie(Sint16 x, Sint16 y, Sint16 rad, Sint16 start, Sint16 end, Uint32 c);
		void PieFill(Sint16 x, Sint16 y, Sint16 rad, Sint16 start, Sint16 end, Uint32 c);
		void Trigon(Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 x3, Sint16 y3, Uint32 c);
		void TrigonAA(Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 x3, Sint16 y3, Uint32 c);
		void TrigonFill(Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 x3, Sint16 y3, Uint32 c);
		void Polygon(const Sint16 * vx, const Sint16 * vy, int n, Uint32 c);
		void PolygonAA(const Sint16 * vx, const Sint16 * vy, int n, Uint32 c);
		void PolygonFill(const Sint16 * vx, const Sint16 * vy, int n, Uint32 c);
		void PolygonTextured(const Sint16 * vx, const Sint16 * vy, int n, GUI_Surface *texture, int texture_dx, int texture_dy);
		void Bezier(const Sint16 * vx, const Sint16 * vy, int n, int s, Uint32 c);
		
		int GetWidth(void);
		int GetHeight(void);
		Uint32 MapRGB(int r, int g, int b);
		Uint32 MapRGBA(int r, int g, int b, int a);
		int IsDoubleBuffered(void);
		int IsHardware(void);
		void Flip(void);
		SDL_Surface *GetSurface(void);
		void SaveBMP(const char *filename);
};

class GUI_Font : public GUI_Object
{
	public:
		GUI_Font(const char *aname);
		virtual ~GUI_Font(void);

		// draw text onto a surface
		virtual void DrawText(GUI_Surface *surface, const char *s, int x, int y);

		// create a new surface with text on it using the fastest method
		virtual GUI_Surface *RenderFast(const char *s, SDL_Color fg);

		// create a new surface with text on it using the best quality
		virtual GUI_Surface *RenderQuality(const char *s, SDL_Color fg);
		
		// return the size of the text when using this font
		virtual SDL_Rect GetTextSize(const char *s);
};

class GUI_FastFont : public GUI_Font
{
	protected:
		GUI_Surface *image;
		int char_width;
		int char_height;
	public:
		GUI_FastFont(const char *fn);
		virtual ~GUI_FastFont(void);

		virtual void DrawText(GUI_Surface *surface, const char *s, int x, int y);
		virtual GUI_Surface *RenderFast(const char *s, SDL_Color fg);
		virtual GUI_Surface *RenderQuality(const char *s, SDL_Color fg);
		virtual SDL_Rect GetTextSize(const char *s);
		GUI_Surface *GetFontImage(void);
};

class GUI_TrueTypeFont : public GUI_Font
{
	protected:
		TTF_Font *ttf;
	public:
		GUI_TrueTypeFont(const char *fn, int size);
		virtual ~GUI_TrueTypeFont(void);

		virtual GUI_Surface *RenderFast(const char *s, SDL_Color fg);
		virtual GUI_Surface *RenderQuality(const char *s, SDL_Color fg);
		virtual SDL_Rect GetTextSize(const char *s);
};

class GUI_Container;

class GUI_Layout : public GUI_Object
{
	public:
		GUI_Layout(const char *aname);
		virtual ~GUI_Layout(void);
		
		virtual void Layout(GUI_Container *container);
};

class GUI_VBoxLayout : public GUI_Layout
{
	public:
		GUI_VBoxLayout(const char *aname);
		virtual ~GUI_VBoxLayout(void);
		
		virtual void Layout(GUI_Container *container);
};

class GUI_Window : public GUI_Object
{
	protected:
		GUI_Rect area;
		GUI_Window *parent;
		GUI_Window **children;
	public:
		GUI_Window(const char *aname, GUI_Window *p, int x, int y, int w, int h);
		virtual ~GUI_Window(void);

		GUI_Rect GetArea(void) const { return area; }
		virtual void UpdateAll(void);
		virtual void Update(const GUI_Rect &r);
		virtual GUI_Window *CreateWindow(const char *aname, int x, int y, int w, int h);
		virtual void DrawImage(const GUI_Surface *image, const GUI_Rect &src_r, const GUI_Rect &dst_r);
		virtual void DrawRect(const GUI_Rect &r, const GUI_Color &c);
		virtual void DrawLine(int x1, int y1, int x2, int y2, const GUI_Color &c);
		virtual void DrawPixel(int x, int y, const GUI_Color &c);
		virtual void FillRect(const GUI_Rect &r, const GUI_Color &c);
};

class GUI_SDLScreen : public GUI_Window
{
	protected:
		SDL_Surface *surface;
	public:
		GUI_SDLScreen(const char *aname, SDL_Surface *surf);
		virtual ~GUI_SDLScreen(void);

		virtual void UpdateAll(void);
		virtual void Update(const GUI_Rect &r);
		virtual void DrawImage(const GUI_Surface *image, const GUI_Rect &src_r, const GUI_Rect &dst_r);
		virtual void DrawRect(const GUI_Rect &r, const GUI_Color &c);
		virtual void DrawLine(int x1, int y1, int x2, int y2, const GUI_Color &c);
		virtual void DrawPixel(int x, int y, const GUI_Color &c);
		virtual void FillRect(const GUI_Rect &r, const GUI_Color &c);
};

class GUI_Widget;

class GUI_Drawable : public GUI_Object
{
	protected:
		// FIXME make these private
		int flags;
		int flag_delta;
		int focused;
		int wtype;
		SDL_Rect area;
		GUI_Callback *status_callback;
		void Keep(GUI_Widget **target, GUI_Widget *source);
		SDL_Rect Adjust(const SDL_Rect *area);
	public:
		GUI_Drawable(const char *aname, int x, int y, int w, int h);
		virtual ~GUI_Drawable(void);
		
		void DoUpdate(int force);
		virtual void Update(int force);
		virtual void Draw(GUI_Surface *image, const SDL_Rect *sr, const SDL_Rect *dr);
		virtual void Erase(const SDL_Rect *dr);
		virtual void Fill(const SDL_Rect *dr, SDL_Color c);
		virtual int Event(const SDL_Event *event, int xoffset, int yoffset);
		virtual void Clicked(int x, int y);
		virtual void ContextClicked(int x, int y);
		virtual void Highlighted(int x, int y);
		virtual void unHighlighted(int x, int y);
		virtual void RemoveWidget(GUI_Widget *widget);
		virtual void Notify(int mask);
		virtual GUI_Drawable *GetParent(void);
		
		int GetWType(void);
		int GetWidth(void);
		int GetHeight(void);
		void SetWidth(int w);
		void SetHeight(int h);
		void TileImage(GUI_Surface *surface, const SDL_Rect *area, int x_offset, int y_offset);
		void CenterImage(GUI_Surface *surface, const SDL_Rect *area, int x_offset, int y_offset);
		void MarkChanged(void);
		void WriteFlags(int andmask, int ormask);
		void SetFlags(int mask);
		void ClearFlags(int mask);
		SDL_Rect GetArea(void);
		void SetPosition(int x, int y);
		void SetSize(int w, int h);
		void SetStatusCallback(GUI_Callback *callback);
		int GetFlagDelta(void);
		int GetFlags(void);
};

class GUI_Widget : public GUI_Drawable
{
	protected:
		GUI_Drawable *parent;
	public:
		GUI_Widget(const char *name, int x, int y, int w, int h);
		virtual ~GUI_Widget(void);
		
		void SetAlign(int align);
		void SetTransparent(int trans);
		void SetEnabled(int flag);
		void SetState(int state);
		int GetState(void);
		int GetWType(void);

		virtual void Update(int force);
		virtual void Draw(GUI_Surface *image, const SDL_Rect *sr, const SDL_Rect *dr);
		virtual void Erase(const SDL_Rect *dr);
		virtual void Fill(const SDL_Rect *dr, SDL_Color c);
		virtual void DrawWidget(const SDL_Rect *dr);

		void SetParent(GUI_Drawable *aparent);
		GUI_Drawable *GetParent(void);
};

class GUI_TextEntry : public GUI_Widget
{
	protected:
		GUI_Font *font;
		SDL_Color textcolor;
		GUI_Surface *normal_image;
		GUI_Surface *highlight_image;
		GUI_Surface *focus_image;
		GUI_Callback *focus_callback;
		GUI_Callback *unfocus_callback;
		int align;
		size_t buffer_size;
		size_t buffer_index;
		char *buffer;
	public:
		GUI_TextEntry(const char *name, int x, int y, int w, int h, GUI_Font *font, int size);
		~GUI_TextEntry(void);

		void SetFont(GUI_Font *afont);
		void SetTextColor(int r, int g, int b);
		void SetText(const char *text);
		const char *GetText(void);
		void SetNormalImage(GUI_Surface *surface);
		void SetHighlightImage(GUI_Surface *surface);
		void SetFocusImage(GUI_Surface *surface);
		void SetFocusCallback(GUI_Callback *callback);
		void SetUnfocusCallback(GUI_Callback *callback);

		virtual void Update(int force);
		virtual void Clicked(int x, int y);
		virtual void Highlighted(int x, int y);
		virtual void unHighlighted(int x, int y);
		virtual int Event(const SDL_Event *event, int xoffset, int yoffset);
};


class GUI_Label : public GUI_Widget
{
	protected:
		char *text;
		GUI_Font *font;
		SDL_Color textcolor;
	public:
		GUI_Label(const char *aname, int x, int y, int w, int h, GUI_Font *afont, const char *s);
		virtual ~GUI_Label(void);

		void SetFont(GUI_Font *font);
		void SetTextColor(int r, int g, int b);
		void SetText(const char *s);
		char *GetText();
		virtual void DrawWidget(const SDL_Rect *dr);
};

class GUI_Picture : public GUI_Widget
{
	protected:
		GUI_Surface *image;
		GUI_Widget *caption;
	public:
		GUI_Picture(const char *aname, int x, int y, int w, int h);
		GUI_Picture(const char *aname, int x, int y, int w, int h, GUI_Surface *an_image);
		virtual ~GUI_Picture(void);

		void SetImage(GUI_Surface *an_image);
		void SetCaption(GUI_Widget *a_caption);
		
		virtual void Update(int force);
		virtual int Event(const SDL_Event *event, int xoffset, int yoffset);
		virtual void DrawWidget(const SDL_Rect *dr);
};

class GUI_FastLabel : public GUI_Picture
{
	protected:
		GUI_Font *font;
		SDL_Color textcolor;
	public:
		GUI_FastLabel(const char *aname, int x, int y, int w, int h, GUI_Font *afont, const char *s);
		virtual ~GUI_FastLabel(void);

		void SetFont(GUI_Font *font);
		void SetTextColor(int r, int g, int b);
		void SetText(const char *s);
};

class GUI_AbstractButton : public GUI_Widget
{
	protected:
		GUI_Widget *caption;
		GUI_Widget *caption2;
		GUI_Callback *click;
		GUI_Callback *context_click;
		GUI_Callback *hover;
		GUI_Callback *unhover;
		virtual GUI_Surface *GetCurrentImage(void);
	public:
		GUI_AbstractButton(const char *aname, int x, int y, int w, int h);
		virtual ~GUI_AbstractButton(void);

		virtual void RemoveWidget(GUI_Widget *widget);
		void SetCaption(GUI_Widget *widget);
		void SetCaption2(GUI_Widget *widget);
		GUI_Widget *GetCaption();
		GUI_Widget *GetCaption2();
		void SetClick(GUI_Callback *handler);
		void SetContextClick(GUI_Callback *handler);
		void SetMouseover(GUI_Callback *handler);
		void SetMouseout(GUI_Callback *handler);
		
		virtual void Erase(const SDL_Rect *dr);
		virtual void Fill(const SDL_Rect *dr, SDL_Color c);
		virtual void Update(int force);
		virtual void Notify(int mask);
		virtual void Clicked(int x, int y);
		virtual void ContextClicked(int x, int y);
		virtual void Highlighted(int x, int y);
		virtual void unHighlighted(int x, int y);
};

class GUI_Button : public GUI_AbstractButton
{
	protected:
		GUI_Surface *normal;
		GUI_Surface *highlight;
		GUI_Surface *pressed;
		GUI_Surface *disabled;
		virtual GUI_Surface *GetCurrentImage(void);
	public:
		GUI_Button(const char *aname, int x, int y, int w, int h);
		virtual ~GUI_Button(void);

		void SetNormalImage(GUI_Surface *surface);
		void SetHighlightImage(GUI_Surface *surface);
		void SetPressedImage(GUI_Surface *surface);
		void SetDisabledImage(GUI_Surface *surface);
};

class GUI_ToggleButton : public GUI_AbstractButton
{
	protected:
		GUI_Surface *off_normal;
		GUI_Surface *off_highlight;
		GUI_Surface *on_normal;
		GUI_Surface *on_highlight;
		virtual GUI_Surface *GetCurrentImage(void);
	public:
		GUI_ToggleButton(const char *aname, int x, int y, int w, int h);
		virtual ~GUI_ToggleButton(void);

		virtual void Clicked(int x, int y);
		virtual void Highlighted(int x, int y);
		virtual void unHighlighted(int x, int y);
		void SetOnNormalImage(GUI_Surface *surface);
		void SetOffNormalImage(GUI_Surface *surface);
		void SetOnHighlightImage(GUI_Surface *surface);
		void SetOffHighlightImage(GUI_Surface *surface);
};

class GUI_ProgressBar : public GUI_Widget
{
	protected:
		GUI_Surface *image1;
		GUI_Surface *image2;
		double value;
	public:
		GUI_ProgressBar(const char *aname, int x, int y, int w, int h);
		virtual ~GUI_ProgressBar(void);
		
		void SetImage1(GUI_Surface *surface);
		void SetImage2(GUI_Surface *surface);
		void SetPosition(double value);

		virtual void Update(int force);
};

class GUI_ScrollBar : public GUI_Widget
{
	protected:
		GUI_Surface *background;
		GUI_Surface *knob;
		GUI_Callback *moved_callback;
		int position_x;
		int position_y;
		int tracking_on;
		int tracking_start_x;
		int tracking_start_y;
		int tracking_pos_x;
		int tracking_pos_y;
	public:
		GUI_ScrollBar(const char *aname, int x, int y, int w, int h);
		virtual ~GUI_ScrollBar(void);

		void SetKnobImage(GUI_Surface *image);
		GUI_Surface *GetKnobImage();
		void SetBackgroundImage(GUI_Surface *image);
		int GetHorizontalPosition(void);
		int GetVerticalPosition(void);
		void SetHorizontalPosition(int value);
		void SetVerticalPosition(int value);
		void SetMovedCallback(GUI_Callback *callback);

		virtual void Update(int force);
		virtual void Erase(const SDL_Rect *rp);
		virtual int Event(const SDL_Event *event, int xoffset, int yoffset);
};

class GUI_Container : public GUI_Widget
{
	protected:
		int n_widgets;
		int s_widgets;
		GUI_Widget **widgets;
		int x_offset;
		int y_offset;
		int bg_center;
		GUI_Surface *background;
		SDL_Color bgcolor;
		virtual void UpdateLayout(void);
	public:
		GUI_Container(const char *name, int x, int y, int w, int h);
		virtual ~GUI_Container(void);

		int ContainsWidget(GUI_Widget *widget);
		void AddWidget(GUI_Widget *widget);
		virtual void RemoveWidget(GUI_Widget *widget);
		void RemoveAllWidgets();
		int GetWidgetCount(void);
		GUI_Widget *GetWidget(int index);
		int IsVisibleWidget(GUI_Widget *widget);

		void SetBackground(GUI_Surface *image);
		void SetBackgroundCenter(GUI_Surface *image);
		void SetBackgroundColor(SDL_Color c);
		void SetXOffset(int value);
		void SetYOffset(int value);
		int GetXOffset();
		int GetYOffset();
		virtual void SetEnabled(int flag);
		virtual void Draw(GUI_Surface *image, const SDL_Rect *sr, const SDL_Rect *dr);
		virtual void Fill(const SDL_Rect *dr, SDL_Color c);
		virtual void Erase(const SDL_Rect *rp);
};

class GUI_Panel : public GUI_Container
{
	protected:
		GUI_Layout *layout;
		virtual void Update(int force);
		virtual void UpdateLayout(void);
	public:
		GUI_Panel(const char *name, int x, int y, int w, int h);
		virtual ~GUI_Panel(void);

		void SetLayout(GUI_Layout *a_layout);
		virtual int Event(const SDL_Event *event, int xoffset, int yoffset);
};



// ScrollPanel by Thorsten Riess

class GUI_ScrollPanel : public GUI_Container
{
	protected:
		int maxx;
		int maxy;
		int offsetx;
		int offsety;
		void AdjustScrollbar(GUI_Object *sender);
		GUI_Panel *panel;
		GUI_ScrollBar *scrollbar_x,*scrollbar_y;
		GUI_Button *button_up,*button_down,*button_left,*button_right;
	public:
		GUI_ScrollPanel(const char *aname, int x, int y, int w, int h);
		virtual ~GUI_ScrollPanel(void);
		void AddItem(GUI_Widget *w);
		void RemoveItem(GUI_Widget *w);
		virtual void Update(int force);
		virtual int Event(const SDL_Event *event, int xoffset, int yoffset);
};



class GUI_CardStack : public GUI_Container
{
	protected:
		int visible_index;
		virtual void Update(int force);
	public:
		GUI_CardStack(const char *name, int x, int y, int w, int h);
		virtual ~GUI_CardStack(void);

		void Next(void);
		void Prev(void);
		int GetIndex(void);
		void ShowIndex(int index);
		void Show(const char *aname);

		virtual int IsVisibleWidget(GUI_Widget *widget);
		virtual int Event(const SDL_Event *event, int xoffset, int yoffset);
};

class GUI_AbstractTable : public GUI_Container
{
	public:
		GUI_AbstractTable(const char *aname, int x, int y, int w, int h);
		virtual ~GUI_AbstractTable(void);

		virtual int GetRowCount(void);
		virtual int GetRowSize(int row);
		virtual int GetColumnCount(void);
		virtual int GetColumnSize(int column);
		virtual void DrawCell(int column, int row, const SDL_Rect *dr);

		virtual void Update(int force);
		virtual int Event(const SDL_Event *event, int xoffset, int yoffset);
};

class GUI_ListBox : public GUI_AbstractTable
{
	protected:
		int item_count;
		int item_max;
		char **items;
		GUI_Font *font;
		SDL_Color textcolor;
	public:
		GUI_ListBox(const char *aname, int x, int y, int w, int h, GUI_Font *a_font);
		virtual ~GUI_ListBox(void);

		virtual int GetRowCount(void);
		virtual int GetRowSize(int row);
		virtual int GetColumnCount(void);
		virtual int GetColumnSize(int column);
		virtual void DrawCell(int column, int row, const SDL_Rect *dr);

		void SetFont(GUI_Font *font);
		void SetTextColor(int r, int g, int b);
		void AddItem(const char *s);
		void RemoveItem(int n);
};

// GUI_RTF and GUI_FileManager by SWAT

class GUI_RTF : public GUI_Widget
{
	protected:
		RTF_FontEngine font;
		RTF_Context *ctx;
		GUI_Surface *surface;
		Uint32 color;
		int offset;
		void SetupFonts(const char *default_font);
		void SetupSurface();
	public:
		GUI_RTF(const char *aname, const char *file, const char *default_font, int x, int y, int w, int h);
		GUI_RTF(const char *aname, SDL_RWops *src, int freesrc, const char *default_font, int x, int y, int w, int h);
		virtual ~GUI_RTF(void);
		
		int GetFullHeight();
		void SetOffset(int value);
		
		void SetBgColor(int r, int g, int b);
		int SetFont(RTF_FontFamily family, const char *file);
		
		/* Get the title of an RTF document */
		const char *GetTitle();

		/* Get the subject of an RTF document */
		const char *GetSubject();

		/* Get the author of an RTF document */
		const char *GetAuthor();
		
		virtual void DrawWidget(const SDL_Rect *dr);
};


class GUI_FileManager : public GUI_Container
{
protected:
		int rescan;
		int joy_sel;
		char cur_path[MAX_FN_LEN];
		GUI_Rect item_area;
		GUI_Surface *item_normal;
		GUI_Surface *item_highlight;
		GUI_Surface *item_pressed;
		GUI_Surface *item_disabled;
		GUI_CallbackFunction *item_click;
		GUI_CallbackFunction *item_context_click;
		GUI_CallbackFunction *item_mouseover;
		GUI_CallbackFunction *item_mouseout;
		GUI_Font *item_label_font;
		SDL_Color item_label_clr;
		GUI_Panel *panel;
		GUI_ScrollBar *scrollbar;
		GUI_Button *button_up;
		GUI_Button *button_down;
		void calc_item_offset(GUI_Widget *widget, Uint16 *x, Uint16 *y);
		void AdjustScrollbar(GUI_Object * sender);
		void ScrollbarButtonEvent(GUI_Object * sender);
		void Build();
		/*
		void ItemEvent(GUI_Object * sender, GUI_CallbackFunction *func);
		void ItemClickEvent(GUI_Object * sender);
		void ItemMouseoverEvent(GUI_Object * sender);
		void ItemMouseoutEvent(GUI_Object * sender);
		*/
	public:
		GUI_FileManager(const char *name, const char *path, int x, int y, int w, int h);
		virtual ~GUI_FileManager(void);
		void Scan();
		void ReScan();
		void Resize(int w, int h);
		void AddItem(const char *name, int size, int time, int attr);
		GUI_Button *GetItem(int index);
		GUI_Panel *GetItemPanel();
		void SetPath(const char *path);
		const char *GetPath();
		void ChangeDir(const char *name, int size);
		void SetItemSurfaces(GUI_Surface *normal, GUI_Surface *highlight, GUI_Surface *pressed, GUI_Surface *disabled);
		void SetItemLabel(GUI_Font *font, int r, int g, int b);
		void SetItemSize(const SDL_Rect *item_r);
		void SetItemClick(GUI_CallbackFunction *func);
		void SetItemContextClick(GUI_CallbackFunction *func);
		void SetItemMouseover(GUI_CallbackFunction *func);
		void SetItemMouseout(GUI_CallbackFunction *func);
		void SetScrollbar(GUI_Surface *knob, GUI_Surface *background);
		void SetScrollbarButtonUp(GUI_Surface *normal, GUI_Surface *highlight, GUI_Surface *pressed, GUI_Surface *disabled);
		void SetScrollbarButtonDown(GUI_Surface *normal, GUI_Surface *highlight, GUI_Surface *pressed, GUI_Surface *disabled);
		void RemoveScrollbar();
		void RestoreScrollbar();
		virtual void Update(int force);
		virtual int Event(const SDL_Event *event, int xoffset, int yoffset);
};



class GUI_Screen;

// GUI_MouseSprite by Thorsten Riess
/*
class GUI_Mouse : public GUI_Surface
{
	protected:
		SDL_Rect odst;
		SDL_Rect osrc;
		SDL_Surface *bgsave;
		Uint16 x;
		Uint16 y;
	public:
		GUI_Mouse(char *aname, SDL_Surface *image);
		virtual ~GUI_Mouse(void);
		void Draw(GUI_Screen *scr,int x,int y);
};*/

class GUI_Screen : public GUI_Drawable
{
	protected:
		GUI_Surface *screen_surface;
		GUI_Surface *background;
		GUI_Widget *contents;
		GUI_Widget *focus_widget;
		//GUI_Mouse *mouse;
		GUI_Widget **joysel;
		int joysel_size;
		int joysel_cur;
		int joysel_enabled;
		Uint32 background_color;
		void find_widget_rec(GUI_Container *p);
		void calc_parent_offset(GUI_Widget *widget, Uint16 *x, Uint16 *y);
		virtual void FlushUpdates(void);
		virtual void UpdateRect(const SDL_Rect *r);
		virtual void Update(int force);
	public:
		GUI_Screen(const char *aname, SDL_Surface *surface);
		virtual ~GUI_Screen(void);

		virtual void Draw(GUI_Surface *image, const SDL_Rect *sr, const SDL_Rect *dr);
		virtual void Erase(const SDL_Rect *dr);
		virtual void Fill(const SDL_Rect *dr, SDL_Color c);
		virtual int Event(const SDL_Event *event, int xoffset, int yoffset);
		virtual void RemoveWidget(GUI_Widget *widget);
		
		void SetContents(GUI_Widget *widget);
		void SetBackground(GUI_Surface *image);
		void SetFocusWidget(GUI_Widget *item);
		void ClearFocusWidget(void);
		void SetJoySelectState(int value);
		void SetBackgroundColor(SDL_Color c);
		GUI_Widget *GetFocusWidget(void);
		GUI_Surface *GetSurface(void);

		//void SetMouse(GUI_Mouse *m);
		//void DrawMouse(void);
};

class GUI_RealScreen : public GUI_Screen
{
	protected:
		int n_updates;
		SDL_Rect *updates;
		virtual void FlushUpdates(void);
		virtual void UpdateRect(const SDL_Rect *r);
		virtual void Update(int force);
	public:
		GUI_RealScreen(const char *aname, SDL_Surface *surface);
		virtual ~GUI_RealScreen(void);
};

#else

typedef void GUI_Object;
typedef struct guiSurface GUI_Surface;
typedef struct guiScreen GUI_Screen;
typedef struct guiFont GUI_Font;
typedef struct guiWidget GUI_Widget;
typedef struct guiCallback GUI_Callback;
typedef struct guiLayout GUI_Layout;
typedef struct guiWindow GUI_Window;
typedef struct guiRect GUI_Rect;
typedef struct guiColor GUI_Color;
typedef struct guiListBox GUI_ListBox;
//typedef struct guiMouse GUI_Mouse;

#endif

/* flags */

#define WIDGET_PRESSED         0x00000001
#define WIDGET_INSIDE          0x00000002
#define WIDGET_HIDDEN          0x00000004
#define WIDGET_CHANGED         0x00000008
#define WIDGET_TRANSPARENT     0x00000010
#define WIDGET_HAS_FOCUS       0x00000020
#define WIDGET_WANTS_FOCUS     0x00000040
#define WIDGET_TURNED_ON       0x00000080

#define WIDGET_ALIGN_MASK      0x00000F00
#define WIDGET_HORIZ_MASK      0x00000300
#define WIDGET_HORIZ_RIGHT     0x00000100
#define WIDGET_HORIZ_LEFT      0x00000200
#define WIDGET_HORIZ_CENTER    0x00000300
#define WIDGET_VERT_MASK       0x00000C00
#define WIDGET_VERT_TOP        0x00000400
#define WIDGET_VERT_BOTTOM     0x00000800
#define WIDGET_VERT_CENTER     0x00000C00

#define WIDGET_DISABLED        0x00001000

#define SCREEN_DEBUG_BLIT      0x10000000
#define SCREEN_DEBUG_UPDATE    0x20000000

#define WIDGET_TYPE_OTHER 			0
#define WIDGET_TYPE_BUTTON 			1
#define WIDGET_TYPE_SCROLLBAR 		2
#define WIDGET_TYPE_CONTAINER 		3
#define WIDGET_TYPE_CARDSTACK 		4
#define WIDGET_TYPE_TEXTENTRY 		5

#ifdef __cplusplus
extern "C" {
#endif


/* GUI API */

void GUI_SetScreen(GUI_Screen *);
GUI_Screen *GUI_GetScreen(void);

/*
int GUI_Init(void);
void GUI_Run(void);
void GUI_Quit(void);

int GUI_MustLock(void);
int GUI_Lock(void);
int GUI_Unlock(void);

void GUI_SetThread(Uint32 id);
int GUI_GetRunning(void);
void GUI_SetRunning(int value);

GUI_Mouse *GUI_MouseCreate(char *name, SDL_Surface * sf);
*/

int GUI_ClipRect(SDL_Rect *sr, SDL_Rect *dr, const SDL_Rect *clip);
void GUI_TriggerUpdate(void);


/* Object API */

GUI_Object *GUI_ObjectCreate(const char *s);
const char *GUI_ObjectGetName(GUI_Object *object);
void GUI_ObjectSetName(GUI_Object *object, const char *s);
void GUI_ObjectIncRef(GUI_Object *object);
int GUI_ObjectDecRef(GUI_Object *object);
int GUI_ObjectGetRef(GUI_Object *object);
int GUI_ObjectTrash(GUI_Object *object);
int GUI_ObjectKeep(GUI_Object **target, GUI_Object *source);

/* Surface API */
void GUI_SurfaceSaveBMP(GUI_Surface *src, const char *filename);
GUI_Surface *GUI_SurfaceLoad(const char *fn);
GUI_Surface *GUI_SurfaceLoad_Rect(const char *fn, SDL_Rect *selection);
GUI_Surface *GUI_SurfaceCreate(const char *aname, int f, int w, int h, int d, int rm, int gm, int bm, int am);
GUI_Surface *GUI_SurfaceFrom(const char *aname, SDL_Surface *src);
void GUI_SurfaceBlit(GUI_Surface *src, SDL_Rect *src_r, GUI_Surface *dst, SDL_Rect *dst_r);
void GUI_SurfaceUpdateRects(GUI_Surface *surface, int n, SDL_Rect *rects);
void GUI_SurfaceUpdateRect(GUI_Surface *surface, int x, int y, int w, int h);
void GUI_SurfaceFill(GUI_Surface *surface, SDL_Rect *r, Uint32 c);
void GUI_SurfaceBlitRGBA(GUI_Surface *surface, SDL_Rect * srcrect, GUI_Surface * dst, SDL_Rect * dstrect);
void GUI_SurfacePixel(GUI_Surface *surface, Sint16 x, Sint16 y, Uint32 c);
void GUI_SurfaceLine(GUI_Surface *surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 c);
void GUI_SurfaceLineAA(GUI_Surface *surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 c);
void GUI_SurfaceLineH(GUI_Surface *surface, Sint16 x1, Sint16 x2, Sint16 y, Uint32 c);
void GUI_SurfaceLineV(GUI_Surface *surface, Sint16 x1, Sint16 y1, Sint16 y2, Uint32 c);
void GUI_SurfaceThickLine(GUI_Surface *surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint8 width, Uint32 c);
void GUI_SurfaceRectagle(GUI_Surface *surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 c);
void GUI_SurfaceRectagleRouded(GUI_Surface *surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 rad, Uint32 c);
void GUI_SurfaceBox(GUI_Surface *surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 c);
void GUI_SurfaceBoxRouded(GUI_Surface *surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 rad, Uint32 c);
void GUI_SurfaceCircle(GUI_Surface *surface, Sint16 x, Sint16 y, Sint16 rad, Uint32 c);
void GUI_SurfaceCircleAA(GUI_Surface *surface, Sint16 x, Sint16 y, Sint16 rad, Uint32 c);
void GUI_SurfaceCircleFill(GUI_Surface *surface, Sint16 x, Sint16 y, Sint16 rad, Uint32 c);
void GUI_SurfaceArc(GUI_Surface *surface, Sint16 x, Sint16 y, Sint16 rad, Sint16 start, Sint16 end, Uint32 c);
void GUI_SurfaceEllipse(GUI_Surface *surface, Sint16 x, Sint16 y, Sint16 rx, Sint16 ry, Uint32 c);
void GUI_SurfaceEllipseAA(GUI_Surface *surface, Sint16 x, Sint16 y, Sint16 rx, Sint16 ry, Uint32 c);
void GUI_SurfaceEllipseFill(GUI_Surface *surface, Sint16 x, Sint16 y, Sint16 rx, Sint16 ry, Uint32 c);
void GUI_SurfacePie(GUI_Surface *surface, Sint16 x, Sint16 y, Sint16 rad, Sint16 start, Sint16 end, Uint32 c);
void GUI_SurfacePieFill(GUI_Surface *surface, Sint16 x, Sint16 y, Sint16 rad, Sint16 start, Sint16 end, Uint32 c);
void GUI_SurfaceTrigon(GUI_Surface *surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 x3, Sint16 y3, Uint32 c);
void GUI_SurfaceTrigonAA(GUI_Surface *surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 x3, Sint16 y3, Uint32 c);
void GUI_SurfaceTrigonFill(GUI_Surface *surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 x3, Sint16 y3, Uint32 c);
void GUI_SurfacePolygon(GUI_Surface *surface, const Sint16 * vx, const Sint16 * vy, int n, Uint32 c);
void GUI_SurfacePolygonAA(GUI_Surface *surface, const Sint16 * vx, const Sint16 * vy, int n, Uint32 c);
void GUI_SurfacePolygonFill(GUI_Surface *surface, const Sint16 * vx, const Sint16 * vy, int n, Uint32 c);
void GUI_SurfacePolygonTextured(GUI_Surface *surface, const Sint16 * vx, const Sint16 * vy, int n, GUI_Surface *texture, int texture_dx, int texture_dy);
void GUI_SurfaceBezier(GUI_Surface *surface, const Sint16 * vx, const Sint16 * vy, int n, int s, Uint32 c);
int GUI_SurfaceGetWidth(GUI_Surface *surface);
int GUI_SurfaceGetHeight(GUI_Surface *surface);
Uint32 GUI_SurfaceMapRGB(GUI_Surface *surface, int r, int g, int b);
Uint32 GUI_SurfaceMapRGBA(GUI_Surface *surface, int r, int g, int b, int a);
//void GUI_SurfaceFlip(GUI_Surface *src);
SDL_Surface *GUI_SurfaceGet(GUI_Surface *surface);
void GUI_SurfaceSetAlpha(GUI_Surface *surface, Uint32 flag, Uint8 alpha);



/* Font API */

GUI_Surface *GUI_FontRenderFast(GUI_Font *font, const char *s, SDL_Color fg);
GUI_Surface *GUI_FontRenderQuality(GUI_Font *font, const char *s, SDL_Color fg);
void GUI_FontDrawText(GUI_Font *font, GUI_Surface *surface, const char *s, int x, int y);
SDL_Rect GUI_FontGetTextSize(GUI_Font *font, const char *s);

/* Widget API */

void GUI_WidgetUpdate(GUI_Widget *widget, int force);
void GUI_WidgetDraw(GUI_Widget *widget, GUI_Surface *image, const SDL_Rect *sr, const SDL_Rect *dr);
void GUI_WidgetErase(GUI_Widget *widget, const SDL_Rect *dr);
void GUI_WidgetFill(GUI_Widget *widget, const SDL_Rect *dr, SDL_Color c);
int GUI_WidgetEvent(GUI_Widget *widget, const SDL_Event *event, int xoffset, int yoffset);
void GUI_WidgetClicked(GUI_Widget *widget, int x, int y);
void GUI_WidgetContextClicked(GUI_Widget *widget, int x, int y);
void GUI_WidgetSetAlign(GUI_Widget *widget, int align);
void GUI_WidgetMarkChanged(GUI_Widget *widget);
void GUI_WidgetSetTransparent(GUI_Widget *widget, int trans);
void GUI_WidgetSetEnabled(GUI_Widget *widget, int flag);
void GUI_WidgetTileImage(GUI_Widget *widget, GUI_Surface *surface, const SDL_Rect *area, int x_offset, int y_offset);
void GUI_WidgetSetFlags(GUI_Widget *widget, int mask);
void GUI_WidgetClearFlags(GUI_Widget *widget, int mask);
void GUI_WidgetSetState(GUI_Widget *widget, int state);
int GUI_WidgetGetState(GUI_Widget *widget);
SDL_Rect GUI_WidgetGetArea(GUI_Widget *widget);
void GUI_WidgetSetPosition(GUI_Widget *widget, int x, int y);
void GUI_WidgetSetSize(GUI_Widget *widget, int w, int h);
int GUI_WidgetGetType(GUI_Widget *widget);
int GUI_WidgetGetFlags(GUI_Widget *widget);
GUI_Widget *GUI_WidgetGetParent(GUI_Widget *widget);

/* Container API */

int GUI_ContainerContains(GUI_Widget *container, GUI_Widget *widget);
void GUI_ContainerAdd(GUI_Widget *container, GUI_Widget *widget);
void GUI_ContainerRemove(GUI_Widget *container, GUI_Widget *widget);
void GUI_ContainerRemoveAll(GUI_Widget *container);
int GUI_ContainerGetCount(GUI_Widget *container);
void GUI_ContainerRemoveAll(GUI_Widget *container);
GUI_Widget *GUI_ContainerGetChild(GUI_Widget *container, int index);
void GUI_ContainerSetEnabled(GUI_Widget *container, int flag);
int GUI_ContainerIsVisibleWidget(GUI_Widget *container, GUI_Widget *widget);

/* Screen API */

GUI_Screen *GUI_ScreenCreate(int w, int h, int d, int f);
void GUI_ScreenSetContents(GUI_Screen *screen, GUI_Widget *contents);
void GUI_ScreenSetBackground(GUI_Screen *screen, GUI_Surface *surface);
void GUI_ScreenSetFocusWidget(GUI_Screen *screen, GUI_Widget *item);
void GUI_ScreenClearFocusWidget(GUI_Screen *screen);
void GUI_ScreenSetBackgroundColor(GUI_Screen *screen, SDL_Color c);
GUI_Widget *GUI_ScreenGetFocusWidget(GUI_Screen *screen);
//void GUI_ScreenDrawMouse(GUI_Screen *screen); 
void GUI_ScreenSetJoySelectState(GUI_Screen *screen, int value);
void GUI_ScreenEvent(GUI_Screen *screen, const SDL_Event *event, 
                     int xoffset, int yoffset); 
void GUI_ScreenUpdate(GUI_Screen *screen, int force); 
void GUI_ScreenDoUpdate(GUI_Screen *screen, int force); 
void GUI_ScreenDraw(GUI_Screen *screen, GUI_Surface *image, 
                    const SDL_Rect *src_r, const SDL_Rect *dst_r);
void GUI_ScreenFill(GUI_Screen *screen, const SDL_Rect *dst_r, SDL_Color c);
void GUI_ScreenErase(GUI_Screen *screen, const SDL_Rect *area); 
//void GUI_ScreenSetMouse(GUI_Screen *screen, GUI_Mouse *m);
GUI_Surface *GUI_ScreenGetSurface(GUI_Screen *screen);

GUI_Screen *GUI_RealScreenCreate(const char *aname, SDL_Surface *surface);
void GUI_RealScreenUpdate(GUI_Screen *screen, int force);
void GUI_RealScreenUpdateRect(GUI_Screen *screen, const SDL_Rect *r);
void GUI_RealScreenFlushUpdates(GUI_Screen *screen);



/* Window Widget API */
GUI_Window *CreateWindow(const char *aname, int x, int y, int w, int h);
void GUI_WindowUpdateAll(GUI_Window *win);
void GUI_WindowUpdate(GUI_Window *win, const GUI_Rect *r);
void GUI_WindowDrawImage(GUI_Window *win, const GUI_Surface *image, const GUI_Rect *src_r, const GUI_Rect *dst_r);
void GUI_WindowDrawRect(GUI_Window *win, const GUI_Rect *r, const GUI_Color *c);
void GUI_WindowDrawLine(GUI_Window *win, int x1, int y1, int x2, int y2, const GUI_Color *c);
void GUI_WindowDrawPixel(GUI_Window *win, int x, int y, const GUI_Color *c);
void GUI_WindowFillRect(GUI_Window *win, const GUI_Rect *r, const GUI_Color *c);


/* Button Widget */

GUI_Widget *GUI_ButtonCreate(const char *name, int x, int y, int w, int h);
int GUI_ButtonCheck(GUI_Widget *widget);
void GUI_ButtonSetNormalImage(GUI_Widget *widget, GUI_Surface *surface);
void GUI_ButtonSetHighlightImage(GUI_Widget *widget, GUI_Surface *surface);
void GUI_ButtonSetPressedImage(GUI_Widget *widget, GUI_Surface *surface);
void GUI_ButtonSetDisabledImage(GUI_Widget *widget, GUI_Surface *surface);
void GUI_ButtonSetCaption(GUI_Widget *widget, GUI_Widget *caption);
void GUI_ButtonSetCaption2(GUI_Widget *widget, GUI_Widget *caption);
GUI_Widget *GUI_ButtonGetCaption(GUI_Widget *widget);
GUI_Widget *GUI_ButtonGetCaption2(GUI_Widget *widget);
void GUI_ButtonSetClick(GUI_Widget *widget, GUI_Callback *callback);
void GUI_ButtonSetContextClick(GUI_Widget *widget, GUI_Callback *callback);
void GUI_ButtonSetMouseover(GUI_Widget *widget, GUI_Callback *callback);
void GUI_ButtonSetMouseout(GUI_Widget *widget, GUI_Callback *callback);

/* Callback API */

GUI_Callback *GUI_CallbackCreate(GUI_CallbackFunction *function, GUI_CallbackFunction *freefunc, void *data);
GUI_Callback *GUI_LuaCallbackCreate(int appId, const char *luastring);
void GUI_CallbackCall(GUI_Callback *callback);

/* FastFont object */

GUI_Font *GUI_FontLoadBitmap(char *fn);

/* TTF Font object */

GUI_Font *GUI_FontLoadTrueType(char *fn, int size);

/* Panel Widget API */

GUI_Widget *GUI_PanelCreate(const char *name, int x, int y, int w, int h);
int GUI_PanelCheck(GUI_Widget *widget);
void GUI_PanelSetBackground(GUI_Widget *widget, GUI_Surface *surface);
void GUI_PanelSetBackgroundCenter(GUI_Widget *widget, GUI_Surface *surface);
void GUI_PanelSetBackgroundColor(GUI_Widget *widget, SDL_Color c);
void GUI_PanelSetXOffset(GUI_Widget *widget, int value);
void GUI_PanelSetYOffset(GUI_Widget *widget, int value);
int GUI_PanelGetXOffset(GUI_Widget *widget);
int GUI_PanelGetYOffset(GUI_Widget *widget);
void GUI_PanelSetLayout(GUI_Widget *widget, GUI_Layout *layout);

/* VBox Layout object */

GUI_Layout *GUI_VBoxLayoutCreate(void);

/* ToggleButton Widget API */

GUI_Widget *GUI_ToggleButtonCreate(const char *name, int x, int y, int w, int h);
int GUI_ToggleButtonCheck(GUI_Widget *widget);
void GUI_ToggleButtonSetOnNormalImage(GUI_Widget *widget, GUI_Surface *surface);
void GUI_ToggleButtonSetOnHighlightImage(GUI_Widget *widget, GUI_Surface *surface);
void GUI_ToggleButtonSetOffNormalImage(GUI_Widget *widget, GUI_Surface *surface);
void GUI_ToggleButtonSetOffHighlightImage(GUI_Widget *widget, GUI_Surface *surface);
void GUI_ToggleButtonSetCaption(GUI_Widget *widget, GUI_Widget *caption);
GUI_Widget *GUI_ToggleButtonGetCaption(GUI_Widget *widget);
void GUI_ToggleButtonSetClick(GUI_Widget *widget, GUI_Callback *callback);
void GUI_ToggleButtonSetContextClick(GUI_Widget *widget, GUI_Callback *callback);
void GUI_ToggleButtonSetMouseover(GUI_Widget *widget, GUI_Callback *callback);
void GUI_ToggleButtonSetMouseout(GUI_Widget *widget, GUI_Callback *callback);

/* Label Widget API */

GUI_Widget *GUI_LabelCreate(const char *name, int x, int y, int w, int h, GUI_Font *font, const char *text);
int GUI_LabelCheck(GUI_Widget *widget);
void GUI_LabelSetFont(GUI_Widget *widget, GUI_Font *font);
void GUI_LabelSetTextColor(GUI_Widget *widget, int r, int g, int b);
void GUI_LabelSetText(GUI_Widget *widget, const char *text);
char *GUI_LabelGetText(GUI_Widget *widget);

/* Picture Widget API */

GUI_Widget *GUI_PictureCreate(const char *name, int x, int y, int w, int h, GUI_Surface *image);
int GUI_PictureCheck(GUI_Widget *widget);
void GUI_PictureSetImage(GUI_Widget *widget, GUI_Surface *image);
void GUI_PictureSetCaption(GUI_Widget *widget, GUI_Widget *caption);

/* ProgressBar widget API */

GUI_Widget *GUI_ProgressBarCreate(const char *name, int x, int y, int w, int h);
int GUI_ProgressBarCheck(GUI_Widget *widget);
void GUI_ProgressBarSetImage1(GUI_Widget *widget, GUI_Surface *image);
void GUI_ProgressBarSetImage2(GUI_Widget *widget, GUI_Surface *image);
void GUI_ProgressBarSetPosition(GUI_Widget *widget, double value);

/* ScrollBar widget API */

GUI_Widget *GUI_ScrollBarCreate(const char *name, int x, int y, int w, int h);
int GUI_ScrollBarCheck(GUI_Widget *widget);
void GUI_ScrollBarSetKnobImage(GUI_Widget *widget, GUI_Surface *image);
GUI_Surface *GUI_ScrollBarGetKnobImage(GUI_Widget *widget);
void GUI_ScrollBarSetBackgroundImage(GUI_Widget *widget, GUI_Surface *image);
void GUI_ScrollBarSetPosition(GUI_Widget *widget, int value); // Deprecated
int GUI_ScrollBarGetPosition(GUI_Widget *widget); // Deprecated
void GUI_ScrollBarSetMovedCallback(GUI_Widget *widget, GUI_Callback *callback);
int GUI_ScrollBarGetHorizontalPosition(GUI_Widget *widget);
int GUI_ScrollBarGetVerticalPosition(GUI_Widget *widget);
void GUI_ScrollBarSetHorizontalPosition(GUI_Widget *widget, int value);
void GUI_ScrollBarSetVerticalPosition(GUI_Widget *widget, int value);

/* CardStack widget API */

GUI_Widget *GUI_CardStackCreate(const char *name, int x, int y, int w, int h);
int GUI_CardStackCheck(GUI_Widget *widget);
void GUI_CardStackSetBackground(GUI_Widget *widget, GUI_Surface *surface);
void GUI_CardStackSetBackgroundColor(GUI_Widget *widget, SDL_Color c);
void GUI_CardStackNext(GUI_Widget *widget);
void GUI_CardStackPrev(GUI_Widget *widget);
void GUI_CardStackShowIndex(GUI_Widget *widget, int index);
void GUI_CardStackShow(GUI_Widget *widget, const char *name);
int GUI_CardStackGetIndex(GUI_Widget *widget);

/* TextEntry widget API */

GUI_Widget *GUI_TextEntryCreate(const char *name, int x, int y, int w, int h, GUI_Font *font, int size);
int GUI_TextEntryCheck(GUI_Widget *widget);
void GUI_TextEntrySetFont(GUI_Widget *widget, GUI_Font *font);
void GUI_TextEntrySetTextColor(GUI_Widget *widget, int r, int g, int b);
void GUI_TextEntrySetText(GUI_Widget *widget, const char *text);
const char *GUI_TextEntryGetText(GUI_Widget *widget);
void GUI_TextEntrySetNormalImage(GUI_Widget *widget, GUI_Surface *surface);
void GUI_TextEntrySetHighlightImage(GUI_Widget *widget, GUI_Surface *surface);
void GUI_TextEntrySetFocusImage(GUI_Widget *widget, GUI_Surface *surface);
void GUI_TextEntrySetFocusCallback(GUI_Widget *widget, GUI_Callback *callback);
void GUI_TextEntrySetUnfocusCallback(GUI_Widget *widget, GUI_Callback *callback);


/* ListBox Widget API */ 
GUI_ListBox *GUI_CreateListBox(const char *aname, int x, int y, int w, int h, GUI_Font *afont);
int GUI_ListBoxGetRowCount(GUI_ListBox *list);
int GUI_ListBoxGetRowSize(GUI_ListBox *list, int row);
int GUI_ListBoxGetColumnCount(GUI_ListBox *list);
int GUI_ListBoxGetColumnSize(GUI_ListBox *list, int column);
void GUI_ListBoxSetFont(GUI_ListBox *list, GUI_Font *afont);
void GUI_ListBoxSetTextColor(GUI_ListBox *list, int r, int g, int b);
void GUI_ListBoxDrawCell(GUI_ListBox *list, int column, int row, const SDL_Rect *r);
void GUI_ListBoxAddItem(GUI_ListBox *list, const char *s);
void GUI_ListBoxRemoveItem(GUI_ListBox *list, int n);


/* RTF Widget API */ 
GUI_Widget *GUI_RTF_Load(const char *name, const char *file, const char *default_font, int x, int y, int w, int h);
GUI_Widget *GUI_RTF_LoadRW(const char *name, SDL_RWops *src, int freesrc, const char *default_font, int x, int y, int w, int h);
int GUI_RTF_GetFullHeight(GUI_Widget *widget);
void GUI_RTF_SetOffset(GUI_Widget *widget, int value);
void GUI_RTF_SetBgColor(GUI_Widget *widget, int r, int g, int b);
int GUI_RTF_SetFont(GUI_Widget *widget, RTF_FontFamily family, const char *file);
const char *GUI_RTF_GetTitle(GUI_Widget *widget);
const char *GUI_RTF_GetSubject(GUI_Widget *widget);
const char *GUI_RTF_GetAuthor(GUI_Widget *widget);


/* FileManager Widget API */

/* Callback data for item events */
typedef struct dirent_fm {
	dirent_t ent;
	GUI_Object *obj;
	int index;
} dirent_fm_t;

GUI_Widget *GUI_FileManagerCreate(const char *name, const char *path, int x, int y, int w, int h);
void GUI_FileManagerResize(GUI_Widget *widget, int w, int h);
void GUI_FileManagerSetPath(GUI_Widget *widget, const char *path);
const char *GUI_FileManagerGetPath(GUI_Widget *widget);
void GUI_FileManagerChangeDir(GUI_Widget *widget, const char *name, int size);
void GUI_FileManagerScan(GUI_Widget *widget);
void GUI_FileManagerAddItem(GUI_Widget *widget, const char *name, int size, int time, int attr);
GUI_Widget *GUI_FileManagerGetItem(GUI_Widget *widget, int index);
GUI_Widget *GUI_FileManagerGetItemPanel(GUI_Widget *widget);
void GUI_FileManagerSetItemSurfaces(GUI_Widget *widget, GUI_Surface *normal, GUI_Surface *highlight, GUI_Surface *pressed, GUI_Surface *disabled);
void GUI_FileManagerSetItemLabel(GUI_Widget *widget, GUI_Font *font, int r, int g, int b);
void GUI_FileManagerSetItemSize(GUI_Widget *widget, const SDL_Rect *item_r);
void GUI_FileManagerSetItemClick(GUI_Widget *widget, GUI_CallbackFunction *func);
void GUI_FileManagerSetItemContextClick(GUI_Widget *widget, GUI_CallbackFunction *func);
void GUI_FileManagerSetItemMouseover(GUI_Widget *widget, GUI_CallbackFunction *func);
void GUI_FileManagerSetItemMouseout(GUI_Widget *widget, GUI_CallbackFunction *func);
void GUI_FileManagerSetScrollbar(GUI_Widget *widget, GUI_Surface *knob, GUI_Surface *background);
void GUI_FileManagerSetScrollbarButtonUp(GUI_Widget *widget, GUI_Surface *normal, GUI_Surface *highlight, GUI_Surface *pressed, GUI_Surface *disabled);
void GUI_FileManagerSetScrollbarButtonDown(GUI_Widget *widget, GUI_Surface *normal, GUI_Surface *highlight, GUI_Surface *pressed, GUI_Surface *disabled);
int GUI_FileManagerEvent(GUI_Widget *widget, const SDL_Event *event, int xoffset, int yoffset);
void GUI_FileManagerUpdate(GUI_Widget *widget, int force);
void GUI_FileManagerRemoveScrollbar(GUI_Widget *widget);
void GUI_FileManagerRestoreScrollbar(GUI_Widget *widget);


#ifdef __cplusplus
};
#endif

#endif
