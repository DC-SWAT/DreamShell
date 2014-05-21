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

// ecrire size bytes de zéros dans le fichier cdi.
void write_null_block(FILE *target, int size) {
	unsigned char* buf;
	
	buf = (char *) malloc(size);
	memset(buf, 0x0, size);
	fwrite(buf, 1, size, target);
	free(buf);
}

// ecrire un tableau directement vers le fichier
void write_array_block(FILE* target, int array_size, const int array_entries, const unsigned int values_array[][2]) {
	unsigned char *buf;
	
	buf = (char *) malloc(array_size);
	fill_buffer(buf, array_size, array_entries, values_array);
	fwrite(buf, 1, array_size, target);
	free(buf);
}

/* 
void endian_swap_short(unsigned short *value) {
	unsigned short x = *value;
    x = (x>>8) | 
        (x<<8);
}

void endian_swap_int(unsigned int *value) {
	unsigned int x = *value;
    x = (x>>24) | 
        ((x<<8) & 0x00FF0000) |
        ((x>>8) & 0x0000FF00) |
        (x<<24);
}

// __int64 for MSVC, "long long" for gcc
void endian_swap_int64(unsigned long long *value) {
	unsigned long long x = *value;
	
    x = (x>>56) | 
        ((x<<40) & 0x00ff000000000000) |
        ((x<<24) & 0x0000ff0000000000) |
        ((x<<8)  & 0x000000ff00000000) |
        ((x>>8)  & 0x00000000ff000000) |
        ((x>>24) & 0x0000000000ff0000) |
        ((x>>40) & 0x000000000000ff00) |
        (x<<56);
}
 */

// conversion function: from logical block adresses  to minute,second,frame
int lba_2_msf(long lba, int *m, int* s, int* f) {

#ifdef  __follow_redbook__
	if (lba >= -150 && lba < 405000) {      /* lba <= 404849 */
#else
	if (lba >= -150) {
#endif
		lba += 150;
	} else if (lba >= -45150 && lba <= -151) {
		lba += 450150;
	} else
		return 1;

	*m = lba / 60 / 75;
		
	lba -= (*m)*60*75;
	*s = lba / 75;
	lba -= (*s)*75;
	*f = lba;

	return 0;
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

/* 
Renvoie l'équivalent "décimal" en hexadécimal d'un nombre décimal.
Exemple :
	0x45 = 45d
	0x75 = 75d
	
	Ce qui est évidemment faux mais c'est utile pour les secteurs d'un CD...
*/
unsigned int int_2_inthex(int dec) {
	unsigned long msb = dec / 10;
	return dec + (msb * 6);
}

// permet de récupérer la taille en bytes de l'espace utilisé sur le disque.
unsigned int get_total_space_used_bytes(int cdda_session_sectors_count, int data_session_sectors_count) {
	return 
		((cdda_session_sectors_count + MSINFO_OFFSET_BASE) * AUDIO_SECTOR_SIZE) + 
		((data_session_sectors_count + TAO_OFFSET) * DATA_SECTOR_SIZE);
}

// permet de récupérer la taille en blocks de l'espace utilisé sur le disque.
unsigned int get_total_space_used_blocks(int cdda_session_sectors_count, int data_session_sectors_count) {
	int lba = MSINFO_OFFSET_BASE + cdda_session_sectors_count; // lba <=> msinfo
	return (lba + data_session_sectors_count) + TAO_OFFSET;
}

// récuperer le nom de volume de l'iso passé en paramètre.
void get_volumename(FILE* iso, char* volume_name) {
	unsigned long curpos;
	char result[32 + 1]; // 16 + 1 (pour le zéro)
	int i = 0;
	
	curpos = ftell(iso); // garder la position courante
	fseek(iso, 0x8028, SEEK_SET); //pour win32 oui... mais les autres ? edit : c'est standard
	fread(&result, sizeof(result), 1, iso);	
	fseek (iso, curpos, SEEK_SET); // restituer la position
	memcpy(volume_name, result, strlen(result) + 1); // + 1 pour le \0 !
}

char* check_ext(char* in, char* ext) {
	char* p;
	char* buf;
	int i, j = 0;
		
	if((p = strrchr(in, '.')) != NULL) {
		// une extension est présente
		buf = (char*) malloc((strlen(in) - (p-in)) - 1); // va recevoir l'extension trouvée dans le nom
		for(i = (p-in) + 1 ; i < strlen(in) ; i++) {
			buf[j] = tolower(in[i]); // on converti tout en minuscule au passage
			j++;
		}
		// l'extension est extraite et présente dans buf.
		
		// maintenant on va comparer les extensions.
		if (strcasecmp(buf, ext) == 0) {// ok ! c'est la même extension
			free(buf);
			return in;
		} else {
			// non alors on va la rajouter en enlevant l'ancienne extension
			free(buf);	
			buf = (char*) malloc((p-in) + strlen(ext) + 1); // le radical du nom (sans l'extension)
			strncpy(buf, in, (p-in));
			buf[(p-in)] = '\0';
			sprintf(buf, "%s.%s", buf, ext);
			return buf;
		}
		
	} else {
		sprintf(in, "%s.%s", in, ext);
		return in;
	}
}

char* get_friendly_unit(float* size) {
	int i = 0;
	char* iec[9] = {"B ", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"};
	
	while ((*size / 1024) > 1) {
		*size = *size / 1024;
		i++;
	}
	
	char res[2];
	return iec[i];
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

