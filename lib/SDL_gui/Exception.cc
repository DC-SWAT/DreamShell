#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <stdarg.h>

#include "SDL_gui.h"


extern "C"
{

int ds_printf(const char *fmt, ...); 

}


GUI_Exception::GUI_Exception(const char *fmt, ...)
{
	va_list marker;
	char temp[4096];

	va_start(marker, fmt);
	vsprintf(temp, fmt, marker);
	va_end(marker);

	message = strdup(temp);
	ds_printf("DS_ERROR: %s\n", message);
}

GUI_Exception::GUI_Exception(const GUI_Exception &err)
{
	message = strdup(err.message);
}

GUI_Exception::~GUI_Exception(void)
{
	free(message);
}

const char *GUI_Exception::GetMessage(void)
{
	return message;
}
