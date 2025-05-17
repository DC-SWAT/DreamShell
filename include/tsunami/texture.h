/*
   Tsunami for KallistiOS ##version##

   texture.h

   Copyright (C) 2002 Megan Potter
   Copyright (C) 2024 Maniac Vera

*/

#ifndef __TSUNAMI_TEXTURE_H
#define __TSUNAMI_TEXTURE_H

// #include <plx/texture.h>
#include "../plx/texture.h"
#include <stdbool.h>

#ifdef __cplusplus

#include <filesystem>

class Texture {
public:
	Texture(const std::filesystem::path &fn, bool use_alpha, bool yflip = false, uint flags = 0);
	Texture(int w, int h, int fmt);
	Texture();
	virtual ~Texture();

	// Load this texture from a file (if it hasn't been done already)
	bool loadFromFile(const std::filesystem::path &fn, bool use_alpha, bool yflip, uint flags);
	
	// Submit one of the poly headers
	void sendHdr(int list);

	// Attribute sets
	enum FilterType {
		FilterNone = PLX_FILTER_NONE,
		FilterBilinear = PLX_FILTER_BILINEAR,
		FilterTrilinear1 = PVR_FILTER_TRILINEAR1,
		FilterTrilinear2 = PVR_FILTER_TRILINEAR2
	};
	void	setFilter(FilterType filter);

	enum UVMode {
		UVRepeat = PLX_UV_REPEAT,
		UVClamp = PLX_UV_CLAMP
	};
	void	setUVClamp(UVMode umode, UVMode vmode);

	// Accessors
	pvr_ptr_t	getPtr() const { return m_txr->ptr; }
	int		getW() const { return m_txr->w; }
	int		getH() const { return m_txr->h; }
	int		getPxlfmt() const { return m_txr->fmt; }
	operator plx_texture_t * const() { return m_txr; }

protected:

	plx_texture_t	* m_txr;
};

#else

typedef struct texture Texture;

#endif


#ifdef __cplusplus
extern "C"
{
#endif

Texture* TSU_TextureCreate(int w, int h, int fmt);
Texture* TSU_TextureCreateFromFile(const char *texture_path, bool use_alpha, bool yflip, unsigned int flags);
Texture* TSU_TextureCreateEmpty();
void TSU_TextureDestroy(Texture **texture_ptr);
bool TSU_TextureLoadFromFile(Texture *texture_ptr, const char *texture_path, bool use_alpha, bool yflip, unsigned int flags);

#ifdef __cplusplus
};
#endif

#endif	/* __TSUNAMI_TEXTURE_H */
