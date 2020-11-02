/*
	:: C D I 4 D C ::

	By SiZiOUS, http://sbibuilder.shorturl.com/
	13 april 2007, v0.3b

	File : 	COMMON.C
	Desc : 	Common part of the CDI generation.

			Sorry comments are in french. If you need some help you can contact me at sizious[at]dc-france.com.
*/

#include "cdibuild.h"
#include "tools.h"

void writing_data_track_event(uint32_t current_pos, uint32_t total_iso_size);
void padding_event(int sector_count);

// ecrire la piste pregap entre l'audio et le data
int write_gap_tracks(FILE* cdi) {
	int i;
	unsigned char* buf;

    buf = (unsigned char *) malloc(gap_sector_size);

	// écrire Gap 1
	fill_buffer(buf, gap_sector_size, gap_dummy_sector_size, gap_dummy_sector);

	for(i = 0 ; i < gap_sector_count ; i++) {
		fwrite(buf, 1, gap_sector_size, cdi);
	}

	// écrire Gap 2
	fill_buffer(buf, gap_sector_size, gap_dummy_sector_2_size, gap_dummy_sector_2);

	for(i = 0 ; i < gap_sector_count ; i++) {
		fwrite(buf, 1, gap_sector_size, cdi);
	}

	free(buf);

	return 0;
}

// ecrire des pistes gap entre les données et l'header du cdi situé à la fin du fichier
void write_gap_end_tracks(FILE *cdi) {
	unsigned char *buf;
	int i;

    buf = (unsigned char *) malloc(gap_sector_size);
	fill_buffer(buf, gap_sector_size, gap_dummy_sector_size, gap_dummy_sector);

	fseek(cdi, ftell(cdi) - 8, SEEK_SET);

	// 2 secteurs de type GAP 1
	for(i = 0 ; i < 2 ; i++)
		fwrite(buf, 1, gap_sector_size, cdi);

	free(buf);
}

// ecrire la piste de données
int write_data_track(FILE* cdi, FILE* iso) {

	int length, block_count = 0;
	unsigned int address = EDC_ENCODE_ADDRESS;
	uint32_t iso_size;

	iso_size = fsize(iso);

	unsigned char buf[EDC_MODE_2_FORM_1_DATA_SECTOR_SIZE];

	// début piste data
	write_null_block(cdi, 8);

	block_count = 2; // un bloc de plus que la boucle + encore 1

	/*while( length = fread(buf + EDC_LOAD_OFFSET, EDC_SECTOR_SIZE_ENCODE, 1, iso) ) {
		THERE!!!*/


	while((length = fread(buf + EDC_LOAD_OFFSET, EDC_SECTOR_SIZE_ENCODE, 1, iso))) {
		block_count++;

		edc_encode_sector(buf, address);

		// on écrit le secteur encodé. On décale de 24 offset (load_offset) afin de ne pas écrire le début du secteur (inutile ici).
		fwrite(buf + 24, EDC_SECTOR_SIZE_DECODE - 16, 1, cdi);

		address++;
	}
	// Write out how many blocks we wrote
	writing_data_track_event((block_count - 2) * L2_RAW, iso_size);

	/*}*/

	// piste data trop petite ! Il faut qu'elle fasse au minimum 302 secteurs.
	if (block_count < 302) padding_event(block_count); // se produit qu'une fois. C'est juste pour l'afficher à l'utilisateur.

	// ici on commence vraiment l'opération
	while(block_count < 302) {
		write_null_block(cdi, EDC_SECTOR_SIZE_DECODE - 16); // on complete donc par des zéros binaires
		block_count++;
	}

	// ecrire deux secteurs GAP 1
	write_gap_end_tracks(cdi);

	return block_count;
}

// ecrire le début de l'header (contenant le nom du fichier)
void write_cdi_header_start(FILE* cdi, char* cdiname) {

	int i;
	unsigned char cdi_filename_length; // longeur de la chaine qui va suivre, elle contient le nom complet du fichier CDI.
	unsigned char head_track_start_mark_blocks[20]; // deux fois la même valeur (track_start_mark)
	uint16_t unknow, unknow2;

	// remplir le tableau contenant les octets représentant le début d'une piste
	for(i = 0 ; i < 20 ; i++) {
		head_track_start_mark_blocks[i] = track_start_mark[i % 10];
	}

	// ecrire deux fois l'en-tête de la piste
	fwrite(head_track_start_mark_blocks, sizeof(head_track_start_mark_blocks), 1, cdi);

	// valeur inconnue ... pour le moment. Il semblerait qu'elle soit importante et qu'elle dépend du nom du fichier enregistré dans l'header...
	unknow = 0x00AB; // pfff impossible de savoir ce que c'est !!!
	fwrite(&unknow, sizeof(unknow), 1, cdi);

	unknow2 = 0x0210; //constante... à quoi elle sert ? on s'en fout elle y est on la remet...
	fwrite(&unknow2, sizeof(unknow2), 1, cdi);

	cdi_filename_length = strlen(cdiname);
	fwrite(&cdi_filename_length, sizeof(cdi_filename_length), 1, cdi); // ecrire la taille
	fwrite(cdiname, cdi_filename_length, 1, cdi); // ecrire le nom du fichier destination CDI

	// ecrire la suite après le nom du fichier... (octets interminés)
	write_array_block(cdi, cdi_head_next_size, cdi_head_next_entries, cdi_head_next);
}

// ecrire la fin de l'header CDI (contient le nom du volume, la taille totale des secteurs du CDI et une valeur servant à calculer l'emplacement de l'header du CDI).
void write_cdi_head_end(FILE* cdi, char* volumename, uint32_t total_cdi_space_used, long cdi_end_image_tracks) {

	int i;
	uint8_t volumename_length;
	uint32_t cdi_header_pos;

	volumename_length = strlen(volumename);

	// ecrire la taille utilisée au total sur le disque
	fwrite(&total_cdi_space_used, sizeof(total_cdi_space_used), 1, cdi);

	// ecrire la taille du volume
	fwrite(&volumename_length, sizeof(volumename_length), 1, cdi);

	// ecrire le nom du volume
	fwrite(volumename, volumename_length, 1, cdi);

	// ecrire les octets de fin
	write_array_block(cdi, cdi_head_end_size, cdi_head_end_entries, cdi_header_end);

	// ecrire la position de l'header (les 4 derniers octets du fichier)
	cdi_header_pos = (ftell(cdi) + 4) - cdi_end_image_tracks; // ftell + 4 = taille du fichier image
	fwrite(&cdi_header_pos, sizeof(cdi_header_pos), 1, cdi);
}
