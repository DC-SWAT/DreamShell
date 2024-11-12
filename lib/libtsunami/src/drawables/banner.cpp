/*
   Tsunami for KallistiOS ##version##

   banner.cpp

   Copyright (C) 2002 Megan Potter
   Copyright (C) 2024 Maniac Vera
   
*/

#include "drawables/banner.h"

Banner::Banner(int list, Texture *texture) {
	setObjectType(ObjectTypeEnum::BANNER_TYPE);
	m_list = list;
	m_texture = texture;

	m_u1 = m_v2 = 0.0f;
	m_u2 = m_v4 = 0.0f;
	m_u3 = m_v1 = 1.0f;
	m_u4 = m_v3 = 1.0f;
	m_w = m_h = -1.0f;
}

Banner::~Banner() {
}

void Banner::setTexture(Texture *txr) {
	m_texture = txr;
}

Texture* Banner::getTexture() {
	return m_texture;
}

void Banner::setTextureType(int texture_type) {
	m_list = texture_type;
}

int Banner::getTextureType() {
	return m_list;
}

void Banner::setUV(float u1, float v1, float u2, float v2, float u3, float v3, float u4, float v4) {
	m_u1 = u1;
	m_v1 = v1;
	m_u2 = u2;
	m_v2 = v2;
	m_u3 = u3;
	m_v3 = v3;
	m_u4 = u4;
	m_v4 = v4;
}

void Banner::setSize(float w, float h) {
	m_w = w;
	m_h = h;
}

void Banner::draw(int list) {
	if (list != m_list || !m_texture)
		return;

	m_texture->sendHdr(list);

	float w, h;
	if (m_w != -1 && m_h != -1) {
		w = m_w;
		h = m_h;
	} else {
		w = m_texture->getW();
		h = m_texture->getH();
	}

	const Vector & sv = getScale();
	w *= sv.x;
	h *= sv.y;

	const Vector & tv = getPosition();

	plx_vertex_t vert;
	if (list == PLX_LIST_TR_POLY) {
		vert.argb = getColor();
	} else {
		Color t = getColor(); 
		t.a = 1.0f;
		vert.argb = t;
	}
	vert.oargb = 0;

	vert.flags = PLX_VERT;
	vert.x = tv.x-w/2;
	vert.y = tv.y+h/2;
	vert.z = tv.z;
	vert.u = m_u1;
	vert.v = m_v1;
	plx_prim(&vert, sizeof(vert));

	vert.y = tv.y-h/2;
	vert.u = m_u2;
	vert.v = m_v2;
	plx_prim(&vert, sizeof(vert));

	vert.x = tv.x+w/2;
	vert.y = tv.y+h/2;
	vert.u = m_u3;
	vert.v = m_v3;
	plx_prim(&vert, sizeof(vert));

	vert.flags = PLX_VERT_EOS;
	vert.y = tv.y-h/2;
	vert.u = m_u4;
	vert.v = m_v4;
	plx_prim(&vert, sizeof(vert));

	Drawable::draw(list);
}


extern "C"
	{
	Banner* TSU_BannerCreate(int list, Texture *texture_ptr)
	{
		if (texture_ptr != NULL) {
			return new Banner(list, texture_ptr);
		}
		else {
			return NULL;
		}
	}

	void TSU_BannerDestroy(Banner **banner_ptr)
	{
		if (*banner_ptr != NULL) {
			delete *banner_ptr;
			*banner_ptr = NULL;
		}
	}

	void TSU_BannerDestroyAll(Banner **banner_ptr)
	{
		if (banner_ptr != NULL) {
			Texture *texture_ptr = (*banner_ptr)->getTexture();

			delete *banner_ptr;
			*banner_ptr = NULL;

			if (texture_ptr != NULL) {
				delete texture_ptr;
				texture_ptr = NULL;
			}
		}
	}

	void TSU_BannerSetTexture(Banner *banner_ptr, Texture *texture_ptr)
	{
		if (banner_ptr != NULL && texture_ptr != NULL) {
			banner_ptr->setTexture(texture_ptr);
		}
	}

	Texture* TSU_BannerGetTexture(Banner *banner_ptr)
	{
		if (banner_ptr != NULL) {
			return banner_ptr->getTexture();
		}
		else {
			return NULL;
		}
	}

	void TSU_BannerSetTextureType(Banner *banner_ptr, int texture_type)
	{
		if (banner_ptr != NULL && texture_type >= 0) {
			banner_ptr->setTextureType(texture_type);
		}
	}

	void TSU_BannerSetUV(Banner *banner_ptr, float u1, float v1, float u2, float v2, float u3, float v3, float u4, float v4)
	{
		if (banner_ptr != NULL) {
			banner_ptr->setUV(u1, v1, u2, v2, u3, v3, u4, v4);
		}
	}

	void TSU_BannerSetSize(Banner *banner_ptr, float w, float h)
	{
		if (banner_ptr != NULL) {
			banner_ptr->setSize(w, h);
		}
	}
}