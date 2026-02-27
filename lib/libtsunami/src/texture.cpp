/*
   Tsunami for KallistiOS ##version##

   texture.cpp

   Copyright (C) 2002 Megan Potter
   Copyright (C) 2024 Maniac Vera
   
*/

#include "texture.h"
#include <kos/dbglog.h>
#include <cassert>

Texture::Texture(const std::filesystem::path &fn, bool use_alpha, bool yflip, uint flags) {
	m_txr = nullptr;
	if (!loadFromFile(fn, use_alpha, yflip, flags)) {
		assert( false );
	}
}

Texture::Texture(int w, int h, int fmt) {
	m_txr = plx_txr_canvas(w, h, fmt);
	if (m_txr == nullptr) {
		dbglog(DBG_WARNING, "Texture::loadFromFile: Can't allocate %dx%dcanvas texture\n", w, h);
		assert( false );
	}
}

Texture::Texture() {
	m_txr = nullptr;
}

Texture::~Texture() {
	if (m_txr != nullptr)
		plx_txr_destroy(m_txr);
}

void Texture::setFilter(FilterType filter) {
	plx_txr_setfilter(m_txr, filter);
}

void Texture::setUVClamp(UVMode umode, UVMode vmode) {
	plx_txr_setuvclamp(m_txr, umode, vmode);
}

// Submit one of the poly headers
void Texture::sendHdr(pvr_list_type_t list) {
	plx_txr_send_hdr(m_txr, list, 0);
}

bool Texture::loadFromFile(const std::filesystem::path &fn, bool use_alpha, bool yflip, uint flags) {
	flags |= yflip ? PVR_TXRLOAD_INVERT_Y : flags;
	m_txr = plx_txr_load(fn.c_str(), use_alpha, flags);

	if (m_txr == nullptr) {
		// dbglog(DBG_WARNING, "Texture::loadFromFile: Can't load '%s'\n", fn.c_str());
		return false;
	} else
		return true;
}

extern "C"
{

	Texture* TSU_TextureCreate(int w, int h, int fmt)
	{
		return new Texture(w, h, fmt);
	}

	Texture* TSU_TextureCreateFromFile(const char *texture_path, bool use_alpha, bool yflip, uint flags)
	{
		if (texture_path != NULL) {
			return new Texture(texture_path, use_alpha, yflip, flags);
		}
		else {
			return NULL;
		}
	}

	Texture* TSU_TextureCreateEmpty()
	{
		return new Texture();
	}

	void TSU_TextureDestroy(Texture **texture_ptr)
	{
		if (*texture_ptr != NULL) {
			delete *texture_ptr;
			*texture_ptr = NULL;
		}
	}

	bool TSU_TextureLoadFromFile(Texture *texture_ptr, const char *texture_path, bool use_alpha, bool yflip, uint flags)
	{
		if (texture_ptr != NULL) {
			return texture_ptr->loadFromFile(texture_path, use_alpha, yflip, flags);
		}
		else {
			return false;
		}
	}

}