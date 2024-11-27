#include "drawables/scene.h"

Scene::Scene() {
	setObjectType(ObjectTypeEnum::SCENE_TYPE);
	// Scene is always finished itself, because it doesn't draw
	// anything. This will make it check its sub-drawables for
	// finishedness before acquiescing.
	setFinished();
	subRemoveFinished();
}

Scene::~Scene() {
}

extern "C"
{
	Scene* TSU_SceneCreate()
	{
		return new Scene();
	}

	void TSU_SceneDestroy(Scene **scene_ptr)
	{
		if (*scene_ptr != NULL)
		{
			delete *scene_ptr;
			*scene_ptr = NULL;
		}
	}
}