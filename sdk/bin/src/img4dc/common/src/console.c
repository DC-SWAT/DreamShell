#include "console.h"

#ifndef WIN32
#include <libgen.h>
#include <stdlib.h>
#ifdef __APPLE__
#include <limits.h>
#else
#include <linux/limits.h>
#endif
#define _MAX_FNAME 4096
#endif

char program_name[_MAX_FNAME];

char * get_program_name() {
	return program_name;
}

void set_program_name(char *argv0) {
#ifdef WIN32
	char drive[_MAX_DRIVE];
	char dir[_MAX_DIR];
	char fname[_MAX_FNAME];
	char ext[_MAX_EXT];
	errno_t err;

	err = _splitpath_s( argv0, drive, _MAX_DRIVE, dir, _MAX_DIR, fname, _MAX_FNAME, ext, _MAX_EXT );
	if (err == 0) {
		strcpy(program_name, fname);
	}

#else
	strcpy(program_name, basename(argv0));
#endif
}

// Changer la couleur à l'emplacement du curseur
void text_color(int color) {
#ifdef WIN32
	int bkgnd = BLACK;
    SetConsoleTextAttribute( GetStdHandle(STD_OUTPUT_HANDLE), color + (bkgnd << 4) );
#else
    // Not implemented yet
#endif
}

// Extracted from the GNU C Library.
// http://stackoverflow.com/questions/4867229/code-for-printf-function-in-c/4867288#4867288
int printf_colored(int color, const char *format, ...) {
	va_list arg;
	int done;
	
	text_color(color);	
	
	va_start(arg, format);
	done = vfprintf(stdout, format, arg);
	va_end(arg);
	
	// Default console color
	text_color(LIGHT_GRAY);
	
	return done;
}

// Yeah, this function was duplicated... Maybe this can be refactored
int printf_stderr_colored(int color, const char *format, ...) {
	va_list arg;
	int done;

	text_color(color);
	
	va_start(arg, format);
	done = vfprintf(stderr, format, arg);
	va_end(arg);

	// Default console color
	text_color(LIGHT_GRAY);

	return done;	
}

// aller à la position (X;Y) dans une appli Win32 Console
void gotoXY(int x, int y) {
#ifdef WIN32
	COORD pos;
	
	pos.X = x;
	pos.Y = y;
	SetConsoleCursorPosition( GetStdHandle(STD_OUTPUT_HANDLE), pos );
#else
    // Not implemented
#endif
}
	
// donne la position X du curseur
int whereX() {
#ifdef WIN32
    CONSOLE_SCREEN_BUFFER_INFO info;

    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
    return info.dwCursorPosition.X;
#else
    return 0;
#endif
}

// donne la position Y du curseur
int whereY() {
#ifdef WIN32
    CONSOLE_SCREEN_BUFFER_INFO info;

    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
    return info.dwCursorPosition.Y;
#else
    return 0;
#endif
}

// permet d'avoir un chemin absolu à partir d'un chemin relatif, utilisation de l'API GetFullPathName
void get_full_filename(char *in, char *result) {
#ifndef WIN32
    char buffer[PATH_MAX];
    if(realpath(in, buffer)) {
        strcpy(result, buffer);
    }
#else
	long retval = 0;
    char buffer[_MAX_PATH] = ""; 
    char *lpPart[_MAX_PATH] = {NULL};
	
	/* 	DWORD GetFullPathName (LPCTSTR lpszFile, DWORD cchPath, LPCTSTR lpszPath, LPTSTR* ppszFilePart)
	
		lpszFile 		: 	Pointeur vers une chaine de caractères contenant un nom de fichier valide.
		cchPath 			: 	Taille en octets (ANSI) ou en caractères (Unicode) du tampon contenant le nom du périphérique et le chemin.
		lpszPath 		: 	Pointeur vers une chaine de caractères chargée de recevoir le nom du périphérique et le chemin associé au 
							fichier.
		ppszFilePart 		: 	Pointeur vers une variable chargée de récupérer l'adresse (dans lpszPath) de la chaine de caractères constituant 
							la dernière partie du chemin du fichier.
		Code de retour 	: 	Longueur en octets ou en caractères de la chaine à copier dans lpszPath si la fonction a été exécutée avec succès.
							Dans le cas contraire la fonction retourne 0. */
							
	retval = GetFullPathName(in, _MAX_PATH, buffer, lpPart);
	if (retval != 0) strcpy(result, buffer);
#endif
}
