#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "SDL_gui.h"

extern "C"
{
void AppGuiCallbackEnter(void);
void AppGuiCallbackLeave(void);
}

GUI_Callback::GUI_Callback(const char *aname)
: GUI_Object(aname)
{
}

GUI_Callback::~GUI_Callback()
{
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
		freefunc(data);
		data = NULL;
	}
}

void GUI_Callback_C::Call(GUI_Object *object)
{
	(void)object;
	if (function) {
		IncRef();
		AppGuiCallbackEnter();
		function(data);
		AppGuiCallbackLeave();
		DecRef();
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
	if (callback)
		callback->Call(0);
}

}
