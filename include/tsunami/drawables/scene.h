/*
   Tsunami for KallistiOS ##version##

   scene.h

   Copyright (C) 2002 Megan Potter
   Copyright (C) 2024 Maniac Vera

*/

#ifndef __TSUNAMI_DRW_SCENE_H
#define __TSUNAMI_DRW_SCENE_H

#include "../drawable.h"

#ifdef __cplusplus

/**
  Scene provides a container class which you can use as a top-level drawable
  in your scene setup. It doesn't draw anything itself, so its finishedness
  always depends on its sub-drawables.
 */
class Scene : public Drawable {
public:
	Scene();
	virtual ~Scene();
};

#else

typedef struct scene Scene;

#endif

#ifdef __cplusplus
extern "C"
{
#endif

Scene* TSU_SceneCreate();
void TSU_SceneDestroy(Scene **scene_ptr);

#ifdef __cplusplus
};
#endif

#endif	/* __TSUNAMI_DRW_SCENE_H */
