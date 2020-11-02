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

uint32_t fsize(FILE *stream) {
	/* Renvoie la position du dernier octets du flot stream */
    uint32_t curpos, length;

	curpos = ftell(stream); /* garder la position courante */
	fseek(stream, 0L, SEEK_END);
	length = ftell(stream);
	fseek(stream, curpos, SEEK_SET); /* restituer la position */
	
	return length;
}

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

// vérifier si l'ISO passé en paramètre contient un IP.BIN
int check_iso_is_bootable(FILE* iso) {
    uint32_t curpos, length;
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
    uint32_t curpos;
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
uint32_t get_total_cdi_space_used(uint32_t data_sectors_count) {
	/* 	301 : nombre de secteurs audio
		11702 : msinfo
		150 : j'en sais rien, c'est comme ça */
	return (301 + 11702 + (data_sectors_count - 1)) - 150;
}

/* int get_track_cdi_data_space_used(uint32_t data_sectors_count) {
	return ((data_sectors_count - 1)) - 150;
} */

// récuperer la valeur de MSINFO de l'iso
int get_iso_msinfo_value(FILE* iso) {
    uint32_t curpos;
    uint32_t lba;
	
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


