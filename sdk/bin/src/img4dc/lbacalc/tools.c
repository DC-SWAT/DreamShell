#include "tools.h"

char* extract_proggyname(char* in) {
	char* p, *prgname;
	int i, j;
	
	if((p = strrchr(in, '.')) != NULL) { // l'extension est présente
		int cpyend = p-in; // on va copier jusqu'à là
		int cpystart = 0;
		int lgth;
		
		if((p = strrchr(in, '\\')) != NULL) // le dernier slash séparateur, le nom est entre les deux
			cpystart = (p-in) + 1;
		
		lgth = (cpyend - cpystart);
		prgname = (char*) malloc(lgth + 1); // + 1 pour \0
		for(i = cpystart, j = 0 ; i < cpyend ; i++, j++)
			prgname[j] = in[i];
		prgname[lgth + 1] = '\0';
				
		return prgname;
	}
	
	return in;
}

// uniquement pour Windows
#ifdef WIN32

#include <windows.h>
#include <wincon.h>

// changer la couleur à l'emplacement du curseur
void textColor(int color) {
	int bkgnd = 0; // noir
    SetConsoleTextAttribute(GetStdHandle (STD_OUTPUT_HANDLE), color + (bkgnd << 4));
}

#else

// pour les autres systèmes d'exploitation

#include "tools.h"
#include <curses.h>

void textcolor(int attr, int fg, int bg) {	
	char command[13];

	//Command is the control command to the terminal
	sprintf(command, "%c[%d;%d;%dm", 0x1B, attr, fg + 30, bg + 40);
	printf("%s", command);
}

void textColor(int color) {
	textcolor(0, color, WHITE);
}

#endif
