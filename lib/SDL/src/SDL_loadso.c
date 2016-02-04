/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2004 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Library General Public
    License as published by the Free Software Foundation; either
    version 2 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Library General Public License for more details.

    You should have received a copy of the GNU Library General Public
    License along with this library; if not, write to the Free
    Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA

    Sam Lantinga
    slouken@libsdl.org
*/

#ifdef SAVE_RCSID
static char rcsid =
 "@(#) $Id: SDL_loadso.c,v 1.9 2004/01/05 01:34:33 slouken Exp $";
#endif

/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* System dependent library loading routines                           */

#include <stdio.h>
#if defined(USE_DLOPEN)
# include <dlfcn.h>
#elif defined(WIN32)
# include <windows.h>
#elif defined(__BEOS__)
# include <be/kernel/image.h>
#elif defined(macintosh)
# include <string.h>
# include <Strings.h>
# include <CodeFragments.h>
# include <Errors.h>
#elif defined(__MINT__) && defined(ENABLE_LDG)
# include <gem.h>
# include <ldg.h>
#else
/*#error Unsupported dynamic link environment*/
#endif /* system type */

#include "SDL_types.h"
#include "SDL_error.h"
#include "SDL_loadso.h"

void *SDL_LoadObject(const char *sofile)
{
	void *handle = NULL;
	const char *loaderror = "SDL_LoadObject() not implemented";
#if defined(USE_DLOPEN)
/* * */
	handle = dlopen(sofile, RTLD_NOW);
	loaderror = (char *)dlerror();
#elif defined(WIN32)
/* * */
	char errbuf[512];

	handle = (void *)LoadLibrary(sofile);

	/* Generate an error message if all loads failed */
	if ( handle == NULL ) {
		FormatMessage((FORMAT_MESSAGE_IGNORE_INSERTS |
					FORMAT_MESSAGE_FROM_SYSTEM),
				NULL, GetLastError(), 
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				errbuf, SDL_TABLESIZE(errbuf), NULL);
		loaderror = errbuf;
	}
#elif defined(__BEOS__)
/* * */
	image_id library_id;

	library_id = load_add_on(sofile);
	if ( library_id == B_ERROR ) {
		loaderror = "BeOS error";
	} else {
		handle = (void *)(library_id);
	}
#elif defined(macintosh)
/* * */
	CFragConnectionID library_id;
	Ptr mainAddr;
	Str255 errName;
	OSErr error;
	char psofile[512];

	strncpy(psofile, sofile, SDL_TABLESIZE(psofile));
	psofile[SDL_TABLESIZE(psofile)-1] = '\0';
	error = GetSharedLibrary(C2PStr(psofile), kCompiledCFragArch,
			kLoadCFrag, &library_id, &mainAddr, errName);
	switch (error) {
		case noErr:
			loaderror = NULL;
			break;
		case cfragNoLibraryErr:
			loaderror = "Library not found";
			break;
		case cfragUnresolvedErr:
			loaderror = "Unabled to resolve symbols";
			break;
		case cfragNoPrivateMemErr:
		case cfragNoClientMemErr:
			loaderror = "Out of memory";
			break;
		default:
			loaderror = "Unknown Code Fragment Manager error";
			break;
	}
	if ( loaderror == NULL ) {
		handle = (void *)(library_id);
	}
#elif defined(__MINT__) && defined(ENABLE_LDG)
/* * */
	handle = (void *)ldg_open((char *)sofile, ldg_global);
#endif /* system type */

	if ( handle == NULL ) {
		SDL_SetError("Failed loading %s: %s", sofile, loaderror);
	}
	return(handle);
}

void *SDL_LoadFunction(void *handle, const char *name)
{
	void *symbol = NULL;
	const char *loaderror = "SDL_LoadFunction not implemented";
#if defined(USE_DLOPEN)
/* * */
	symbol = dlsym(handle, name);
	if ( symbol == NULL ) {
		loaderror = (char *)dlerror();
	}
#elif defined(WIN32)
/* * */
	char errbuf[512];

	symbol = (void *)GetProcAddress((HMODULE)handle, name);
	if ( symbol == NULL ) {
		FormatMessage((FORMAT_MESSAGE_IGNORE_INSERTS |
					FORMAT_MESSAGE_FROM_SYSTEM),
				NULL, GetLastError(), 
				MAKELANGID(LANG_NEUTRAL, SUBLANG_DEFAULT),
				errbuf, SDL_TABLESIZE(errbuf), NULL);
		loaderror = errbuf;
	}
#elif defined(__BEOS__)
/* * */
	image_id library_id = (image_id)handle;
	if ( get_image_symbol(library_id,
		name, B_SYMBOL_TYPE_TEXT, &symbol) != B_NO_ERROR ) {
		loaderror = "Symbol not found";
	}
#elif defined(macintosh)
/* * */
	CFragSymbolClass class;
	CFragConnectionID library_id = (CFragConnectionID)handle;
	char pname[512];

	strncpy(pname, name, SDL_TABLESIZE(pname));
	pname[SDL_TABLESIZE(pname)-1] = '\0';
	if ( FindSymbol(library_id, C2PStr(pname),
	                (char **)&symbol, &class) != noErr ) {
		loaderror = "Symbol not found";
	}
#elif defined(__MINT__) && defined(ENABLE_LDG)
/* * */
	symbol = (void *)ldg_find((char *)name, (LDG *)handle);
#endif /* system type */

	if ( symbol == NULL ) {
		SDL_SetError("Failed loading %s: %s", name, loaderror);
	}
	return(symbol);
}

void SDL_UnloadObject(void *handle)
{
#if defined(__BEOS__)
	image_id library_id;
#elif defined(macintosh)
	CFragConnectionID library_id;
#endif
	if ( handle == NULL ) {
		return;
	}
#if defined(USE_DLOPEN)
/* * */
	dlclose(handle);
#elif defined(WIN32)
/* * */
	FreeLibrary((HMODULE)handle);
#elif defined(__BEOS__)
/* * */
	library_id = (image_id)handle;
	unload_add_on(library_id);
#elif defined(macintosh)
/* * */
	library_id = (CFragConnectionID)handle;
	CloseConnection(&library_id);
#elif defined(__MINT__) && defined(ENABLE_LDG)
/* * */
	ldg_close((LDG *)handle, ldg_global);
#endif /* system type */
}
