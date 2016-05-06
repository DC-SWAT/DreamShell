#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "SDL_gui.h"
#include "SDL_gfxBlitFunc.h"
#include "SDL_gfxPrimitives.h"

extern "C"
{
SDL_Surface *SDL_ImageLoad(const char *filename, SDL_Rect *selection);
}

GUI_Surface::~GUI_Surface(void)
{
	if (surface)
		SDL_FreeSurface(surface);
}

GUI_Surface::GUI_Surface(const char *aname, SDL_Surface *image)
: GUI_Object(aname)
{
	//assert(image != NULL);
	if(image)
		surface = image;
}

GUI_Surface::GUI_Surface(const char *fn)
: GUI_Object(fn)
{
	surface = IMG_Load(fn);
	if (surface == NULL) {
		/*throw*/ GUI_Exception("Failed to load image \"%s\" (%s)", fn, IMG_GetError());
	}/* else {
		SDL_Surface *tmp = SDL_DisplayFormat(surface);
		SDL_FreeSurface(surface);
		surface = tmp;
	}*/
}

GUI_Surface::GUI_Surface(const char *fn, SDL_Rect *selection)
: GUI_Object(fn)
{
	surface = SDL_ImageLoad(fn, selection);
	if (surface == NULL) {
		/*throw*/ GUI_Exception("Failed to load image \"%s\" (%s)", fn, IMG_GetError());
	}/* else {
		SDL_Surface *tmp = SDL_DisplayFormat(surface);
		SDL_FreeSurface(surface);
		surface = tmp;
	}*/
}


GUI_Surface::GUI_Surface(const char *aname, int f, int w, int h, int d, int rm, int bm, int gm, int am)
: GUI_Object(aname)
{
	surface = SDL_AllocSurface(f, w, h, d, rm, bm, gm, am);
	if (surface == NULL)
		/*throw*/ GUI_Exception("Failed to allocate surface (f=%d, w=%d, h=%d, d=%d)", f, w, h, d);
}

void GUI_Surface::Blit(SDL_Rect *src_r, GUI_Surface *dst, SDL_Rect *dst_r)
{
	SDL_BlitSurface(surface, src_r, dst->surface, dst_r);
}

void GUI_Surface::UpdateRect(int x, int y, int w, int h)
{
	SDL_UpdateRect(surface, x, y, w, h);
}

void GUI_Surface::UpdateRects(int n, SDL_Rect *rects)
{
	SDL_UpdateRects(surface, n, rects);
}

void GUI_Surface::Fill(SDL_Rect *r, Uint32 c)
{
	SDL_FillRect(surface, r, c);
}

void GUI_Surface::BlitRGBA(SDL_Rect * srcrect, GUI_Surface * dst, SDL_Rect * dstrect)
{
	SDL_gfxBlitRGBA(surface, srcrect, dst->surface, dstrect);
}

void GUI_Surface::Pixel(Sint16 x, Sint16 y, Uint32 c)
{
	pixelColor(surface, x, y, c);
}

void GUI_Surface::Line(Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 c)
{
	lineColor(surface, x1, y1, x2, y2, c);
}

void GUI_Surface::LineAA(Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 c)
{
	aalineColor(surface, x1, y1, x2, y2, c);
}

void GUI_Surface::LineH(Sint16 x1, Sint16 x2, Sint16 y, Uint32 c)
{
	hlineColor(surface, x1, x2, y, c);
}

void GUI_Surface::LineV(Sint16 x1, Sint16 y1, Sint16 y2, Uint32 c)
{
	vlineColor(surface, x1, y1, y2, c);
}

void GUI_Surface::ThickLine(Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint8 width, Uint32 c)
{
	thickLineColor(surface, x1, y1, x2, y2, width, c);
}

void GUI_Surface::Rectagle(Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 c)
{
	rectangleColor(surface, x1, y1, x2, y2, c);
}

void GUI_Surface::RectagleRouded(Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 rad, Uint32 c)
{
	roundedRectangleColor(surface, x1, y1, x2, y2, rad, c);
}

void GUI_Surface::Box(Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 c)
{
	boxColor(surface, x1, y1, x2, y2, c);
}

void GUI_Surface::BoxRouded(Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 rad, Uint32 c)
{
	roundedBoxColor(surface, x1, y1, x2, y2, rad, c);
}

void GUI_Surface::Circle(Sint16 x, Sint16 y, Sint16 rad, Uint32 c)
{
	circleColor(surface, x, y, rad, c);
}

void GUI_Surface::CircleAA(Sint16 x, Sint16 y, Sint16 rad, Uint32 c)
{
	aacircleColor(surface, x, y, rad, c);
}

void GUI_Surface::CircleFill(Sint16 x, Sint16 y, Sint16 rad, Uint32 c)
{
	filledCircleColor(surface, x, y, rad, c);
}

void GUI_Surface::Arc(Sint16 x, Sint16 y, Sint16 rad, Sint16 start, Sint16 end, Uint32 c)
{
	arcColor(surface, x, y, rad, start, end, c);
}

void GUI_Surface::Ellipse(Sint16 x, Sint16 y, Sint16 rx, Sint16 ry, Uint32 c)
{
	ellipseColor(surface, x, y, rx, ry, c);
}

void GUI_Surface::EllipseAA(Sint16 x, Sint16 y, Sint16 rx, Sint16 ry, Uint32 c)
{
	aaellipseColor(surface, x, y, rx, ry, c);
}

void GUI_Surface::EllipseFill(Sint16 x, Sint16 y, Sint16 rx, Sint16 ry, Uint32 c)
{
	filledEllipseColor(surface, x, y, rx, ry, c);
}

void GUI_Surface::Pie(Sint16 x, Sint16 y, Sint16 rad, Sint16 start, Sint16 end, Uint32 c)
{
	pieColor(surface, x, y, rad, start, end, c);
}

void GUI_Surface::PieFill(Sint16 x, Sint16 y, Sint16 rad, Sint16 start, Sint16 end, Uint32 c)
{
	filledPieColor(surface, x, y, rad, start, end, c);
}

void GUI_Surface::Trigon(Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 x3, Sint16 y3, Uint32 c)
{
	trigonColor(surface, x1, y1, x2, y2, x3, y3, c);
}

void GUI_Surface::TrigonAA(Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 x3, Sint16 y3, Uint32 c)
{
	aatrigonColor(surface, x1, y1, x2, y2, x3, y3, c);
}

void GUI_Surface::TrigonFill(Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 x3, Sint16 y3, Uint32 c)
{
	filledTrigonColor(surface, x1, y1, x2, y2, x3, y3, c);
}

void GUI_Surface::Polygon(const Sint16 * vx, const Sint16 * vy, int n, Uint32 c)
{
	polygonColor(surface, vx, vy, n, c);
}

void GUI_Surface::PolygonAA(const Sint16 * vx, const Sint16 * vy, int n, Uint32 c)
{
	aapolygonColor(surface, vx, vy, n, c);
}

void GUI_Surface::PolygonFill(const Sint16 * vx, const Sint16 * vy, int n, Uint32 c)
{
	filledPolygonColor(surface, vx, vy, n, c);
}

void GUI_Surface::PolygonTextured(const Sint16 * vx, const Sint16 * vy, int n, GUI_Surface *texture, int texture_dx, int texture_dy)
{
	texturedPolygon(surface, vx, vy, n, texture->surface, texture_dx, texture_dy);
}

void GUI_Surface::Bezier(const Sint16 * vx, const Sint16 * vy, int n, int s, Uint32 c)
{
	bezierColor(surface, vx, vy, n, s, c);
}

int GUI_Surface::GetWidth(void)
{
	return surface->w;
}

int GUI_Surface::GetHeight(void)
{
	return surface->h;
}

SDL_Surface *GUI_Surface::GetSurface(void)
{
	return surface;
}

Uint32 GUI_Surface::MapRGB(int r, int g, int b)
{
	return SDL_MapRGB(surface->format, r, g, b);
}

Uint32 GUI_Surface::MapRGBA(int r, int g, int b, int a)
{
	return SDL_MapRGBA(surface->format, r, g, b, a);
}

int GUI_Surface::IsDoubleBuffered(void)
{
	return (surface->flags & SDL_DOUBLEBUF) != 0;
}

int GUI_Surface::IsHardware(void)
{
	return (surface->flags & SDL_HWSURFACE) != 0;
}

void GUI_Surface::Flip(void)
{
	SDL_Flip(surface);
}

void GUI_Surface::DisplayFormat(void)
{
	SDL_Surface *temp = SDL_DisplayFormat(surface);
	if (!temp)
		/*throw*/ GUI_Exception("Failed to format surface for display: %s", SDL_GetError());
	SDL_FreeSurface(surface);
	surface = temp;
}

void GUI_Surface::SetAlpha(Uint32 flag, Uint8 alpha)
{
	SDL_SetAlpha(surface, flag, alpha);
}

void GUI_Surface::SetColorKey(Uint32 c)
{
	if (SDL_SetColorKey(surface, SDL_RLEACCEL | SDL_SRCCOLORKEY, c) < 0)
		/*throw*/ GUI_Exception("Failed to set color key for surface: %s", SDL_GetError());
}

void GUI_Surface::SaveBMP(const char *filename)
{
	SDL_SaveBMP(surface, filename);
}

void GUI_Surface::SavePNG(const char *filename)
{
	SDL_SavePNG(surface, filename);
}

extern "C"
{


void GUI_SurfaceSaveBMP(GUI_Surface *src, const char *filename)
{
	src->SaveBMP(filename);
}

void GUI_SurfaceSavePNG(GUI_Surface *src, const char *filename)
{
	src->SavePNG(filename);
}

GUI_Surface *GUI_SurfaceLoad(const char *fn)
{
	return new GUI_Surface(fn);
}

GUI_Surface *GUI_SurfaceLoad_Rect(const char *fn, SDL_Rect *selection)
{
	return new GUI_Surface(fn, selection);
}


void GUI_SurfaceFlip(GUI_Surface *src)
{
	src->Flip();
}

GUI_Surface *GUI_SurfaceCreate(const char *aname, int f, int w, int h, int d, int rm, int gm, int bm, int am)
{
	return new GUI_Surface(aname, f, w, h, d, rm, gm, bm, am);
}

GUI_Surface *GUI_SurfaceFrom(const char *aname, SDL_Surface *src) {
	return new GUI_Surface(aname, src);
}

void GUI_SurfaceBlit(GUI_Surface *src, SDL_Rect *src_r, GUI_Surface *dst, SDL_Rect *dst_r)
{
	src->Blit(src_r, dst, dst_r);
}

void GUI_SurfaceUpdateRects(GUI_Surface *surface, int n, SDL_Rect *rects)
{
	surface->UpdateRects(n, rects);
}

void GUI_SurfaceUpdateRect(GUI_Surface *surface, int x, int y, int w, int h)
{
	surface->UpdateRect(x,y,w,h);
}

void GUI_SurfaceFill(GUI_Surface *surface, SDL_Rect *r, Uint32 c)
{
	surface->Fill(r, c);
}

int GUI_SurfaceGetWidth(GUI_Surface *surface)
{
	return surface->GetWidth();
}

int GUI_SurfaceGetHeight(GUI_Surface *surface)
{
	return surface->GetHeight();
}

Uint32 GUI_SurfaceMapRGB(GUI_Surface *surface, int r, int g, int b)
{
	return surface->MapRGB(r, g, b);
}

Uint32 GUI_SurfaceMapRGBA(GUI_Surface *surface, int r, int g, int b, int a)
{
	return surface->MapRGBA(r, g, b, a);
}

SDL_Surface *GUI_SurfaceGet(GUI_Surface *surface)
{
	return surface->GetSurface();
}

void GUI_SurfaceSetAlpha(GUI_Surface *surface, Uint32 flag, Uint8 alpha)
{
	surface->SetAlpha(flag, alpha);
}


void GUI_SurfaceBlitRGBA(GUI_Surface *surface, SDL_Rect * srcrect, GUI_Surface * dst, SDL_Rect * dstrect)
{
	surface->BlitRGBA(srcrect, dst, dstrect);
}

void GUI_SurfacePixel(GUI_Surface *surface, Sint16 x, Sint16 y, Uint32 c)
{
	surface->Pixel(x, y, c);
}

void GUI_SurfaceLine(GUI_Surface *surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 c)
{
	surface->Line(x1, y1, x2, y2, c);
}

void GUI_SurfaceLineAA(GUI_Surface *surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 c)
{
	surface->LineAA(x1, y1, x2, y2, c);
}

void GUI_SurfaceLineH(GUI_Surface *surface, Sint16 x1, Sint16 x2, Sint16 y, Uint32 c)
{
	surface->LineH(x1, x2, y, c);
}

void GUI_SurfaceLineV(GUI_Surface *surface, Sint16 x1, Sint16 y1, Sint16 y2, Uint32 c)
{
	surface->LineV(x1, y1, y2, c);
}

void GUI_SurfaceThickLine(GUI_Surface *surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint8 width, Uint32 c)
{
	surface->ThickLine(x1, y1, x2, y2, width, c);
}

void GUI_SurfaceRectagle(GUI_Surface *surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 c)
{
	surface->Rectagle(x1, y1, x2, y2, c);
}

void GUI_SurfaceRectagleRouded(GUI_Surface *surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 rad, Uint32 c)
{
	surface->RectagleRouded(x1, y1, x2, y2, rad, c);
}

void GUI_SurfaceBox(GUI_Surface *surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Uint32 c)
{
	surface->Box(x1, y1, x2, y2, c);
}

void GUI_SurfaceBoxRouded(GUI_Surface *surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 rad, Uint32 c)
{
	surface->BoxRouded(x1, y1, x2, y2, rad, c);
}

void GUI_SurfaceCircle(GUI_Surface *surface, Sint16 x, Sint16 y, Sint16 rad, Uint32 c)
{
	surface->Circle(x, y, rad, c);
}

void GUI_SurfaceCircleAA(GUI_Surface *surface, Sint16 x, Sint16 y, Sint16 rad, Uint32 c)
{
	surface->CircleAA(x, y, rad, c);
}

void GUI_SurfaceCircleFill(GUI_Surface *surface, Sint16 x, Sint16 y, Sint16 rad, Uint32 c)
{
	surface->CircleFill(x, y, rad, c);
}

void GUI_SurfaceArc(GUI_Surface *surface, Sint16 x, Sint16 y, Sint16 rad, Sint16 start, Sint16 end, Uint32 c)
{
	surface->Arc(x, y, rad, start, end, c);
}

void GUI_SurfaceEllipse(GUI_Surface *surface, Sint16 x, Sint16 y, Sint16 rx, Sint16 ry, Uint32 c)
{
	surface->Ellipse(x, y, rx, ry, c);
}

void GUI_SurfaceEllipseAA(GUI_Surface *surface, Sint16 x, Sint16 y, Sint16 rx, Sint16 ry, Uint32 c)
{
	surface->EllipseAA(x, y, rx, ry, c);
}

void GUI_SurfaceEllipseFill(GUI_Surface *surface, Sint16 x, Sint16 y, Sint16 rx, Sint16 ry, Uint32 c)
{
	surface->EllipseFill(x, y, rx, ry, c);
}

void GUI_SurfacePie(GUI_Surface *surface, Sint16 x, Sint16 y, Sint16 rad, Sint16 start, Sint16 end, Uint32 c)
{
	surface->Pie(x, y, rad, start, end, c);
}

void GUI_SurfacePieFill(GUI_Surface *surface, Sint16 x, Sint16 y, Sint16 rad, Sint16 start, Sint16 end, Uint32 c)
{
	surface->PieFill(x, y, rad, start, end, c);
}

void GUI_SurfaceTrigon(GUI_Surface *surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 x3, Sint16 y3, Uint32 c)
{
	surface->Trigon(x1, y1, x2, y2, x3, y3, c);
}

void GUI_SurfaceTrigonAA(GUI_Surface *surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 x3, Sint16 y3, Uint32 c)
{
	surface->TrigonAA(x1, y1, x2, y2, x3, y3, c);
}

void GUI_SurfaceTrigonFill(GUI_Surface *surface, Sint16 x1, Sint16 y1, Sint16 x2, Sint16 y2, Sint16 x3, Sint16 y3, Uint32 c)
{
	surface->TrigonFill(x1, y1, x2, y2, x3, y3, c);
}

void GUI_SurfacePolygon(GUI_Surface *surface, const Sint16 * vx, const Sint16 * vy, int n, Uint32 c)
{
	surface->Polygon(vx, vy, n, c);
}

void GUI_SurfacePolygonAA(GUI_Surface *surface, const Sint16 * vx, const Sint16 * vy, int n, Uint32 c)
{
	surface->PolygonAA(vx, vy, n, c);
}

void GUI_SurfacePolygonFill(GUI_Surface *surface, const Sint16 * vx, const Sint16 * vy, int n, Uint32 c)
{
	surface->PolygonFill(vx, vy, n, c);
}

void GUI_SurfacePolygonTextured(GUI_Surface *surface, const Sint16 * vx, const Sint16 * vy, int n, GUI_Surface *texture, int texture_dx, int texture_dy)
{
	surface->PolygonTextured(vx, vy, n, texture, texture_dx, texture_dy);
}

void GUI_SurfaceBezier(GUI_Surface *surface, const Sint16 * vx, const Sint16 * vy, int n, int s, Uint32 c)
{
	surface->Bezier(vx, vy, n, s, c);
}


};
