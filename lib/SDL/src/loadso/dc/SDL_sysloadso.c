/*
    SDL - Simple DirectMedia Layer
    Copyright (C) 1997-2006 Sam Lantinga

    This library is free software; you can redistribute it and/or
    modify it under the terms of the GNU Lesser General Public
    License as published by the Free Software Foundation; either
    version 2.1 of the License, or (at your option) any later version.

    This library is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
    Lesser General Public License for more details.

    You should have received a copy of the GNU Lesser General Public
    License along with this library; if not, write to the Free Software
    Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301  USA

    Sam Lantinga
    slouken@libsdl.org
*/
#include "SDL_config.h"



/* * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * * */
/* System dependent library loading routines                           */

#include "SDL_loadso.h"
#include <kos.h>

void *SDL_LoadObject(const char *sofile)
{
	return library_open(sofile, sofile);
}

void *SDL_LoadFunction(void *handle, const char *name)
{
    export_sym_t *sym; 
	
    if((sym = export_lookup(name)) != NULL) {
		return (void*)sym->ptr;
	}
    else return NULL; 
}

void SDL_UnloadObject(void *handle)
{
    library_close((klibrary_t*)handle);
}


