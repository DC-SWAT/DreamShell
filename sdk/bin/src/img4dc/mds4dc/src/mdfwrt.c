#include "mdfwrt.h"
#include "mdsaudio.h"
#include "mdsdata.h"
#include "config.h"
#include "tools.h"

extern unsigned int write_method;

// fonction permettant d'écrire une piste audio dans le mdf.
int write_audio_track(FILE* mdf, FILE* audio, unsigned int is_last_track) {
	int length, tao_padding = 0;
	int block_count = 0;
	unsigned char buf[AUDIO_SECTOR_SIZE];	
	
#ifdef APP_CONSOLE
	uint32_t total_size = fsize(audio);
	start_progressbar();
#endif

	// écriture début de la session audio
	write_null_block(mdf, AUDIO_SECTOR_START_SIZE);	
	
	fseek(audio, 0x0, SEEK_SET);
	while(length = fread(buf, sizeof(buf), 1, audio)) {
		fwrite(buf, sizeof(buf), 1, mdf);
		block_count++;	
#ifdef APP_CONSOLE
		writing_track_event(block_count * AUDIO_SECTOR_SIZE, total_size);
#endif
	}
	
	// piste inférieure à 4 secondes (300 secteurs) : on rajoute des secteurs
#ifdef APP_CONSOLE
	if (block_count < MINIMAL_TRACK_SIZE_BLOCKS) {
		padding_event(block_count);
		length = MINIMAL_TRACK_SIZE_BLOCKS * AUDIO_SECTOR_SIZE;
		printf("\n%*c", INFO_MSG_SIZE + 5, ' ');
		start_progressbar();
	}
#endif
	while(block_count < MINIMAL_TRACK_SIZE_BLOCKS) {
		write_null_block(mdf, AUDIO_SECTOR_SIZE); // on complète donc par des zéros binaires
		block_count++;
#ifdef APP_CONSOLE
		writing_track_event(block_count * AUDIO_SECTOR_SIZE, length);
#endif
	}

	// écrire le pregap (2 secteurs),  on complète donc par des zéros binaires
	block_count = block_count + 2;
	write_null_block(mdf, AUDIO_SECTOR_SIZE); // premier secteur
	write_null_block(mdf, AUDIO_SECTOR_SIZE - AUDIO_SECTOR_START_SIZE); // deuxième secteur. On enlève la taille du début de la piste audio !
	
	// si le disque est en mode TAO, alors on va rajouter 150 secteurs entre chaque piste, sauf la dernière (ou on ne fait rien)
	if ((write_method == TAO_WRITE_METHOD_MODE) && (!is_last_track)) {
		tao_padding = block_count + TAO_OFFSET; // sinon on rajoute 150 (TAO_OFFSET)
		
		while(block_count < tao_padding) {
			write_null_block(mdf, AUDIO_SECTOR_SIZE); // on complète donc par des zéros binaires
			block_count++;
		}
	}
	
#ifdef APP_CONSOLE
	uint32_t space_used = block_count * AUDIO_SECTOR_SIZE;
	writing_track_event_end(block_count, space_used);
#endif
	
	return block_count;
}

// fonction permettant d'écrire un block nul dans une piste data (pour faire du padding). C'est une fonction "privée" à ce fichier.
int write_data_track_null_block(FILE* mdf, unsigned int from_lba, unsigned int current_block_count, unsigned int count, int include_gap_entries) {
	int i, m, s, f;
	unsigned char buf[EDC_MODE_2_FORM_1_DATA_SECTOR_SIZE];

	memset(buf, 0x0, EDC_MODE_2_FORM_1_DATA_SECTOR_SIZE); // mettre tout à zéro
	
	// inclure les secteurs de "GAP"
	if (include_gap_entries)
		fill_buffer(buf, DD_GAP_DUMMY_SECTOR_SIZE, DD_GAP_DUMMY_SECTOR_ENTRIES, dd_gap_dummy_sector);
	
	// on met la signature de début d'un secteur DATA
	memcpy(buf, DATA_SECTOR_START_PATTERN, sizeof(DATA_SECTOR_START_PATTERN));
	
	from_lba = from_lba + current_block_count; // on calcule le LBA courant de notre fichier (from_lba indique le début de la piste DATA)
	
	// on va écrire count secteurs
	for(i = 0 ; i < count ; i++) {
		lba_2_msf(from_lba, &m, &s, &f);
		buf[0x0c] = int_2_inthex(m);
		buf[0x0d] = int_2_inthex(s);
		buf[0x0e] = int_2_inthex(f);
		buf[0x0f] = 0x02;
		
		fwrite(buf, EDC_SECTOR_SIZE_DECODE, 1, mdf);
		current_block_count++; // on incrémente le nombre de secteurs
		from_lba++; // on rajoute un secteur
	}
	
	return current_block_count; // puis on le renvoie (normalement incrémenté de count)
}

int write_data_boot_track(FILE* mdf, FILE* iso, unsigned int start_lba) {
	int block_count = 0;
	int lba, m, s, f, length;
	unsigned int address = EDC_ENCODE_ADDRESS;
	unsigned char buf[EDC_MODE_2_FORM_1_DATA_SECTOR_SIZE];
			
	// écrire l'IP.BIN (secteur 0), le 17ème et 18ème secteur de l'ISO (18 secteurs à écrire en tout, numérotés de 0 à 17)
	fseek(iso, 0x0, SEEK_SET); // 17ème secteur commence à 0x8000. IP.BIN commence à 0x0.
	
	lba = start_lba;
	while((block_count < 18) && (fread(buf + EDC_LOAD_OFFSET, EDC_SECTOR_SIZE_ENCODE, 1, iso))) {
		lba = start_lba + block_count;
		
		edc_encode_sector(buf, address);
		
		// corriger la valeur de l'en-tête (4 bytes)
		/* Le MSF est bizarrement codé en décimal dans le fichier !
		   C'est à dire que dans l'en-tête, 0x54 = 54d, ce qui est évidemment faux ! ... 
		   Exemple : Pour un MSF égal à 02:54.02, la valeur 0x02 0x54 0x02 sera écrite... */
		lba_2_msf(lba, &m, &s, &f);
		buf[0x0c] = int_2_inthex(m);
		buf[0x0d] = int_2_inthex(s);
		buf[0x0e] = int_2_inthex(f);
		
		// on écrit le secteur encodé. On décale de LOAD_OFFSET afin d'enlever les données inutiles calculées par libedc.
		fwrite(buf, EDC_SECTOR_SIZE_DECODE, 1, mdf);
		
		// pour encoder le secteur suivant
		address++;
		
		// savoir combien de blocs ont été écrit
		block_count++;
	}
		
	//  (une piste DOIT faire au minimul 300 secteurs).
	if(block_count < MINIMAL_TRACK_SIZE_BLOCKS) {
		length = MINIMAL_TRACK_SIZE_BLOCKS - block_count;
		block_count = write_data_track_null_block(mdf, start_lba, block_count, length, NO_GAP_ENTRIES);
	}
	
	// écrire le pregap (2 secteurs)
	block_count = write_data_track_null_block(mdf, start_lba, block_count, 2, INCLUDE_GAP_ENTRIES);
	
	return block_count;
}

// ecrire la piste de données
int write_data_track(FILE* mdf, FILE* iso, unsigned int start_lba) {
    unsigned int length = 0, lba = 0, block_count = 0;
    int32_t m = 0, s = 0, f = 0;

	unsigned int address = EDC_ENCODE_ADDRESS;
	uint32_t iso_size;
	unsigned char buf[EDC_MODE_2_FORM_1_DATA_SECTOR_SIZE];
	
	fseek(iso, 0x0, SEEK_SET);
	iso_size = fsize(iso);
	
#ifdef APP_CONSOLE
	start_progressbar();
#endif

    while((length = fread(buf + EDC_LOAD_OFFSET, EDC_SECTOR_SIZE_ENCODE, 1, iso))) {
		lba = start_lba + block_count;
		
		edc_encode_sector(buf, address);
			
		// corriger la valeur de l'en-tête (4 bytes)
		/* Le MSF est bizarrement codé en décimal dans le fichier !
		   C'est à dire que dans l'en-tête, 0x54 = 54d, ce qui est évidemment faux ! ... 
		   Exemple : Pour un MSF égal à 02:54.02, la valeur 0x02 0x54 0x02 sera écrite... */
		lba_2_msf(lba, &m, &s, &f);
		buf[0x0c] = int_2_inthex(m);
		buf[0x0d] = int_2_inthex(s);
		buf[0x0e] = int_2_inthex(f);

		// la valeur du sous en-tête (8 bytes) est égale à 0 ... mais de temps en temps elle est égale à 0x80... je sais pas pourquoi.
		
		// on écrit le secteur encodé. On décale de LOAD_OFFSET afin d'enlever les données inutiles calculées par libedc.
		fwrite(buf, EDC_SECTOR_SIZE_DECODE, 1, mdf);
		
		// pour encoder le secteur suivant
		address++;
		
		// savoir combien de blocs ont été écrit
		block_count++; 
		
#ifdef APP_CONSOLE
		writing_track_event(block_count * L2_RAW, iso_size);
#endif
	}
		
	// piste data trop petite ! on fait du padding jusqu'à 300 secteurs (une piste DOIT faire au minimul 300 secteurs).
	if (block_count < MINIMAL_TRACK_SIZE_BLOCKS) {
#ifdef APP_CONSOLE
		padding_event(block_count);
		printf("\n%*c", INFO_MSG_SIZE + 1, ' ');
		start_progressbar();
#endif
		length = MINIMAL_TRACK_SIZE_BLOCKS - block_count;
		block_count = write_data_track_null_block(mdf, start_lba, block_count, length, NO_GAP_ENTRIES);
#ifdef APP_CONSOLE
		writing_track_event(block_count * EDC_MODE_2_FORM_1_DATA_SECTOR_SIZE, length);
#endif
	}
	
	// écrire le pregap (2 secteurs)
	block_count = write_data_track_null_block(mdf, start_lba, block_count, 2, INCLUDE_GAP_ENTRIES);
	
#ifdef APP_CONSOLE
	uint32_t space_used = (block_count * DATA_SECTOR_SIZE);
	writing_track_event_end(block_count, space_used);
#endif

	return block_count;
}
