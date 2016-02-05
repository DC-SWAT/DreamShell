#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "SDL_gui.h"

extern "C"
{
int ds_printf(const char *fmt, ...); 
}

GUI_Callback::GUI_Callback(const char *aname)
: GUI_Object(aname)
{
	//data = NULL;
	//freefunc = NULL;
}
/*
GUI_Callback::GUI_Callback(const char *aname, GUI_CallbackFunction *ffunc, void *data)
: GUI_Object(aname)
{
	data = data;
	freefunc = ffunc;
}*/

GUI_Callback::~GUI_Callback()
{
	/*
	if(freefunc != NULL && data != NULL) {
		ds_printf("FREE: %p\n", data);
		freefunc(data);
		data = NULL;
	}*/
}


GUI_Callback_C::GUI_Callback_C(GUI_CallbackFunction *func, GUI_CallbackFunction *ffunc, void *p)
: GUI_Callback(NULL)
{
	function = func;
	freefunc = ffunc;
	data = p;
}


GUI_Callback_C::~GUI_Callback_C()
{
	if(freefunc != NULL && data != NULL) {
		//ds_printf("FREE_C: %p\n", data);
		freefunc(data);
		data = NULL;
	}
}


void GUI_Callback_C::Call(GUI_Object *object)
{
	if (function) {
		IncRef();
//		ds_printf("GUI_Callback_C::Call: %p %p\n", function, data);
		function(data);
		if(GetRef() == 1) {
			Trash();
		} else {
			DecRef();
		}
	}
}


extern "C"
{

GUI_Callback *GUI_CallbackCreate(GUI_CallbackFunction *function, GUI_CallbackFunction *freefunc, void *data)
{
	return new GUI_Callback_C(function, freefunc, data);
}

void GUI_CallbackCall(GUI_Callback *callback)
{
	callback->Call(0);
}


}
