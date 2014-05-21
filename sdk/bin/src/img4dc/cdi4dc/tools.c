/* 
	:: C D I 4 D C ::
	
	By SiZiOUS, http://sbibuilder.shorturl.com/
	10 july 2006, v0.2b
	
	File : 	TOOLS.C
	Desc : 	Somes utilities for cdi4dc.
			
			Sorry comments are in french. If you need some help you can contact me at sizious[at]dc-france.com.
*/

#include "tools.h"

/* 
	Pour passer un tableau à plusieurs dimensions en paramètre à une fonction, il suffit fournir les différentes 
	dimensions dans le prototype de la fonction.
	La dimension la plus à gauche (et uniquement celle-ci) peut être omise dans le prototype de la fonction.
	
	Comme on sait que les tableaux décrivant les secteurs correspondent à une adresse suivie de la valeur à cette adresse, 
	on peut donc mettre ici 2.
*/

// remplir un buffer avec un tableau défini dans cdihead.h
void fill_buffer(unsigned char *buf, int total_size, int values_array_size, const unsigned int values_array[][2]) {
	int i, offset;
	
	// on remplit de zéro tout le buffer
	memset(buf, 0x0, total_size);
	
	// on rajoute nos valeurs
	for(i = 0 ; i < values_array_size ; i++) {
		offset = values_array[i][0]; // colonne 0 : offset		
		buf[offset] = values_array[i][1]; //colonne 1 : valeur
	}
}

// taille d'un fichier
unsigned long fsize(FILE *stream) {
	/* Renvoie la position du dernier octets du flot stream */
	unsigned long curpos, length;

	curpos = ftell(stream); /* garder la position courante */
	fseek(stream, 0L, SEEK_END);
	length = ftell(stream);
	fseek(stream, curpos, SEEK_SET); /* restituer la position */
	
	return length;
}

// vérifier si l'ISO passé en paramètre contient un IP.BIN
int check_iso_is_bootable(FILE* iso) {
	unsigned long curpos, length;
	unsigned char signature[33] = "SEGA SEGAKATANA SEGA ENTERPRISES";
	unsigned char buf[33];
	
	curpos = ftell(iso); /* garder la position courante */
	fseek(iso, 0L, SEEK_SET);
	
	fread(buf, sizeof(buf), 1, iso);
	buf[sizeof(buf) - 1] = '\0';
	
	fseek(iso, curpos, SEEK_SET); /* restituer la position */
	
	if (strcmp(buf, signature) == 0) return 1; // c'est ok, on a bien un IP.BIN
	
	return 0;
}

// récuperer le nom de volume de l'iso passé en paramètre.
void get_volumename(FILE* iso, char* volume_name) {
	unsigned long curpos;
	char result[32 + 1]; // 16 + 1 (pour le zéro)
	//char buf;
	int i = 0;
	
	curpos = ftell(iso); // garder la position courante
	
//#ifdef WIN32
	fseek(iso, 0x8028, SEEK_SET); //pour win32 oui... mais les autres ? edit : c'est standard
//#endif
	
	/*while(fread(&buf, 1, 1, iso) == 1) {
		//if (buf == 0x20) break; // il peut y'avoir des espaces dans un nom
		
		result[i] = buf;
		i++;
	}*/
	fread(&result, sizeof(result), 1, iso);
	// result[i] = '\0';
		
	fseek (iso, curpos, SEEK_SET); // restituer la position
	
	memcpy(volume_name, result, strlen(result) + 1); // + 1 pour le \0 !
}

// donne la taille utilisée par le CDI en fonction des secteurs de données.
int get_total_cdi_space_used(unsigned long data_sectors_count) {
	/* 	301 : nombre de secteurs audio
		11702 : msinfo
		150 : j'en sais rien, c'est comme ça */
	return (301 + 11702 + (data_sectors_count - 1)) - 150;
}

/* int get_track_cdi_data_space_used(unsigned long data_sectors_count) {
	return ((data_sectors_count - 1)) - 150;
} */

// récuperer la valeur de MSINFO de l'iso
int get_iso_msinfo_value(FILE* iso) {
	unsigned long curpos;
	unsigned long lba;
	
	curpos = ftell(iso); // garder la position courante
	
	fseek(iso, 0x809E, SEEK_SET);
	fread(&lba, sizeof(lba), 1, iso); // récuperer la valeur
	
	fseek (iso, curpos, SEEK_SET); // restituer la position
	
	return lba - 0x1c; //1C ? je sais pas ce que c'est. un ISO avec un MSINFO de 0 a cette valeur donc bon (avec mkisofs). Je cherche pas plus loin.
}

// ecrire size bytes de zéros dans le fichier cdi.
void write_null_block(FILE *cdi, int size) {
	unsigned char* buf;
	
	buf = (char *) malloc(size);
	memset(buf, 0x0, size);
	fwrite(buf, 1, size, cdi);
	free(buf);
}

// ecrire un tableau directement vers le fichier
void write_array_block(FILE* cdi, int array_size, const int array_entries, const unsigned int values_array[][2]) {
	unsigned char *buf;
	
	buf = (char *) malloc(array_size);
	fill_buffer(buf, array_size, array_entries, values_array);
	fwrite(buf, 1, array_size, cdi);
	free(buf);
}

// uniquement pour Windows
#ifdef WIN32

#include <windows.h>
#include <wincon.h>
#define BUFSIZE 4096

// permet d'avoir un chemin absolu à partir d'un chemin relatif, utilisation de l'API GetFullPathName
void get_full_filename(char* in, char* result) {
	long retval = 0;
    char buffer[BUFSIZE] = ""; 
    char *lpPart[BUFSIZE] = {NULL};
	
	/* 	DWORD GetFullPathName (LPCTSTR lpszFile, DWORD cchPath, LPCTSTR lpszPath, LPTSTR* ppszFilePart)
	
		lpszFile 		: 	Pointeur vers une chaine de caractères contenant un nom de fichier valide.
		cchPath 			: 	Taille en octets (ANSI) ou en caractères (Unicode) du tampon contenant le nom du périphérique et le chemin.
		lpszPath 		: 	Pointeur vers une chaine de caractères chargée de recevoir le nom du périphérique et le chemin associé au 
							fichier.
		ppszFilePart 		: 	Pointeur vers une variable chargée de récupérer l'adresse (dans lpszPath) de la chaine de caractères constituant 
							la dernière partie du chemin du fichier.
		Code de retour 	: 	Longueur en octets ou en caractères de la chaine à copier dans lpszPath si la fonction a été exécutée avec succès.
							Dans le cas contraire la fonction retourne 0. */
							
	retval = GetFullPathName(in, BUFSIZE, buffer, lpPart);
	if (retval != 0) strcpy(result, buffer);
}
 
// aller à la position (X;Y) dans une appli Win32 Console
void gotoXY(int x, int y) {
	COORD pos;
	
	pos.X = x;
	pos.Y = y;
	SetConsoleCursorPosition(GetStdHandle(STD_OUTPUT_HANDLE), pos);
}
	
// changer la couleur à l'emplacement du curseur
void textColor(int color) {
	int bkgnd = 0; // noir
    SetConsoleTextAttribute(GetStdHandle (STD_OUTPUT_HANDLE), color + (bkgnd << 4));
}

// donne la position X du curseur
int whereX() {
    CONSOLE_SCREEN_BUFFER_INFO info;

    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
    return info.dwCursorPosition.X;
}

// donne la position Y du curseur
int whereY() {
    CONSOLE_SCREEN_BUFFER_INFO info;

    GetConsoleScreenBufferInfo(GetStdHandle(STD_OUTPUT_HANDLE), &info);
    return info.dwCursorPosition.Y;
}

#else

// pour les autres systèmes d'exploitation

#include "tools.h"
#include <curses.h>

void gotoXY(int x, int y) {
	
}

void textcolor(int attr, int fg, int bg) {	
	char command[13];

	//Command is the control command to the terminal
	sprintf(command, "%c[%d;%d;%dm", 0x1B, attr, fg + 30, bg + 40);
	printf("%s", command);
}

void textColor(int color) {
	textcolor(0, color, WHITE);
}

int whereX() {
}

int whereY() {
}

#endif
