/*
   Tsunami for KallistiOS ##version##

   banner.h

   Copyright (C) 2002 Megan Potter
   Copyright (C) 2024 Maniac Vera

*/

#ifndef __TSUNAMI_DRW_BANNER_H
#define __TSUNAMI_DRW_BANNER_H

// #include <plx/list.h>
// #include <plx/sprite.h>
#include "../../plx/list.h"
#include "../../plx/sprite.h"
#include "../drawable.h"
#include "../texture.h"
#include "../color.h"

#ifdef __cplusplus

#include <memory>

/** Banner -- a texture banner drawable. This drawable takes a texture (and
    optional UV coordinates) and draws a banner. */
class Banner : public Drawable {
public:
	Banner(int list, Texture *texture);
	virtual ~Banner();

	void setTexture(Texture *txr);
	Texture* getTexture();

	void setTextureType(int texture_type);
	int getTextureType();

	// Points are:   2  4
	//               1  3
	void setUV(float u1, float v1, float u2, float v2, float u3, float v3, float u4, float v4);

	void setSize(float w, float h);

	virtual void draw(int list);

private:
	int			m_list;
	Texture *m_texture;

	float	m_u1, m_v1, m_u2, m_v2;
	float	m_u3, m_v3, m_u4, m_v4;
	float	m_w, m_h;
};

#else

typedef struct banner Banner;

#endif

#ifdef __cplusplus
extern "C"
{
#endif

Banner* TSU_BannerCreate(int list, Texture *texture_ptr);
void TSU_BannerDestroy(Banner **banner_ptr);
void TSU_BannerDestroyAll(Banner **banner_ptr);
void TSU_BannerSetTexture(Banner *banner_ptr, Texture *texture_ptr);
void TSU_BannerSetTextureType(Banner *banner_ptr, int texture_type);
void TSU_BannerSetUV(Banner *banner_ptr, float u1, float v1, float u2, float v2, float u3, float v3, float u4, float v4);
void TSU_BannerSetSize(Banner *banner_ptr, float w, float h);

#ifdef __cplusplus
};
#endif

#endif	/* __TSUNAMI_DRW_BANNER_H */
