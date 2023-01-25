#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "SDL_gui.h"

extern "C" 
{
	int GUI_Object2Trash(GUI_Object *object);
}

GUI_Object::GUI_Object(const char *aname)
{
	//assert(aname != NULL);
	
	if(aname == NULL) {
		name = NULL;
	} else {
		name = strdup(aname);
	}
	refcount = 1;
}

GUI_Object::~GUI_Object(void)
{
	if(name != NULL)
		free(name);
}

void GUI_Object::IncRef(void)
{
	refcount++;
}

int GUI_Object::DecRef(void)
{
	if (--refcount <= 0)
	{
		delete this;
		return 1;
	}
	return 0;
}

int GUI_Object::GetRef(void)
{
	return refcount;
}


int GUI_Object::Trash(void)
{
	return GUI_Object2Trash(this);
}


const char *GUI_Object::GetName(void)
{
	return name;
}


void GUI_Object::SetName(const char *s)
{
	if(name != NULL)
		free(name);
		
	name = strdup(s);
}

int GUI_Object::CheckName(const char *aname)
{
	return strcmp(name, aname);
}

extern "C" 
{

GUI_Object *GUI_ObjectCreate(const char *s)
{
	return new GUI_Object(s);
}

const char *GUI_ObjectGetName(GUI_Object *object)
{
	return object->GetName();
}

void GUI_ObjectSetName(GUI_Object *object, const char *s)
{
	object->SetName(s);
}

void GUI_ObjectIncRef(GUI_Object *object)
{
	if (object)
		object->IncRef();
}

int GUI_ObjectDecRef(GUI_Object *object)
{
	if (object)
		return object->DecRef();
	return 0;
}

int GUI_ObjectGetRef(GUI_Object *object)
{
	if (object)
		return object->GetRef();
	return 0;
}

int GUI_ObjectTrash(GUI_Object *object) {
	if (object)
		return object->Trash();
	return 0;
}

int GUI_ObjectKeep(GUI_Object **target, GUI_Object *source)
{
	if (source != *target)
	{
		GUI_ObjectIncRef(source);
		GUI_ObjectDecRef(*target);
		(*target) = source;
		return 1;
	}
	return 0;
}

}

