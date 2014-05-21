/*
	mdsaudio.c - Media Descriptor Header Support Writer for Audio/Data images format
	
	Created by [big_fury]SiZiOUS / Dreamcast-Scene 2007
	Version 0.1 - 05/06/07
	
	Based on specs by Henrik Stokseth <hensto AT c2i DOT net>
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "mdfwrt.h"
#include "mdsaudio.h"
#include "tools.h"

/*
	+ EXEMPLE D'UN FICHIER MEDIA DESCRIPTOR "MDS" AUDIO/DATA
	
	Chaque fonction est à appeler dans l'ordre de ce fichier.
	
	Exemple : Cet exemple va générer un fichier MDS contenant :
		- Session Audio (CDDA) : 3 pistes
			 01 : 302 blocs
			 02 : 302 blocs
			 03 : 302 blocs
		- Session Données (DATA) : 1 piste (seulement)
		     01 : 352 blocs
	
	Début du code :
	
	int cdda_session_sectors_count = (302 + 302 + 302);
	int cdda_tracks_count = 3; // 3 pistes 
	int data_session_sectors_count = 352;
	
	FILE* mds = fopen("test.mds", "wb");
	
	write_mds_header(mds);
	write_cdda_session_infos(mds, cdda_session_sectors_count, cdda_tracks_count, data_session_sectors_count);
	write_cdda_lead_in_track_first_infos(mds);
	write_cdda_lead_in_track_last_infos(mds, cdda_tracks_count);
	write_cdda_lead_in_track_leadout_infos(mds, cdda_session_sectors_count, cdda_tracks_count);
	
	// écrire chaque piste CDDA
	write_cdda_track_infos(mds, 1, cdda_tracks_count, 0); // piste 1 lba: 0
	write_cdda_track_infos(mds, 2, cdda_tracks_count, 302); // piste 2 lba: 0 + 302 = 302
	write_cdda_track_infos(mds, 3, cdda_tracks_count, 604); // piste 3 lba: 0 + 302 + 302 = 604
	
	write_data_session_infos(mds, cdda_session_sectors_count);
	
	write_data_lead_in_track_first_infos(mds, cdda_tracks_count);
	write_data_lead_in_track_last_infos(mds, cdda_tracks_count);
	write_data_lead_in_track_leadout_infos(mds, cdda_session_sectors_count, data_session_sectors_count);
	
	write_data_track_infos(mds, cdda_session_sectors_count, cdda_tracks_count); // écrire la piste de données
	write_data_track_infos_header_start(mds);
	
	write_cdda_track_sectors_count(mds, 302); // piste 1
	write_cdda_track_sectors_count(mds, 302); // piste 2
	write_cdda_track_sectors_count(mds, 302); // piste 3
	
	write_data_track_sectors_count(mds, data_session_sectors_count);
	
	write_mds_footer(mds, cdda_tracks_count);
		
	fclose(mds);
*/

/*
	Audio/Data : Toutes les fonctions sont préfixées par ad_
*/

// écrire l'en-tête du fichier MDS
void ad_write_mds_header(FILE* mds) {		
	write_array_block(mds, AD_MDS_HEADER_SIZE, AD_MDS_HEADER_ENTRIES, ad_mds_header);
}

// écrire les informations de la session CDDA (la première)
void ad_write_cdda_session_infos(FILE* mds, int cdda_session_sectors_count, int cdda_tracks_count, int data_session_sectors_count) {
	unsigned char *buf;
	int i, msinfo;
	
	// init le buffer
	buf = (char *) malloc(AD_CDDA_SESSION_INFOS_SIZE);
	fill_buffer(buf, AD_CDDA_SESSION_INFOS_SIZE, AD_CDDA_SESSION_INFOS_ENTRIES, ad_cdda_session_infos);
		
	// taille totale de la session CDDA
	buf[0x00] = cdda_session_sectors_count;
	buf[0x01] = cdda_session_sectors_count >> 8;
	buf[0x02] = cdda_session_sectors_count >> 16;
	buf[0x03] = cdda_session_sectors_count >> 24;
	
	// nombre de pistes + 0x06 ...
	i = 0x06 + cdda_tracks_count;
	buf[0x06] = i;
	
	// nombre de pistes
	buf[0x0a] = cdda_tracks_count;
	
	// MSINFO entre la session audio et la session data
	msinfo = (MSINFO_OFFSET_BASE + cdda_session_sectors_count);
	
	// Exemple : MSINFO (11702 - 150) = 11552 (2D20) 11702 = 11400 (valeur décalage) + 302 (taille track 1 audio)
	i = msinfo - TAO_OFFSET;
	buf[0x14] = i;
	buf[0x15] = i >> 8;
	buf[0x16] = i >> 16;
	buf[0x17] = i >> 24;
	
	// Exemple : 11702 (lba audio) + 348 (nb secteurs piste data) = 12050 ou (Taille piste (espace utilisé) en blocs 11220 - 150 = 12050) ...
	i = get_total_space_used_blocks(cdda_session_sectors_count, data_session_sectors_count) - TAO_OFFSET;
	buf[0x18] = i; 
	buf[0x19] = i >> 8;
	buf[0x1a] = i >> 16;
	buf[0x1b] = i >> 24;
	
	// nombre de pistes ...
	i = 0x01 + cdda_tracks_count;
	buf[0x20] = i;
	buf[0x22] = i;
	
	// taille quelconque ...
	i = 616 + (80 * cdda_tracks_count);
	buf[0x28] = i;
	buf[0x29] = i >> 8;
	buf[0x2a] = i >> 16;
	buf[0x2b] = i >> 24;
	
	// écrire dans le fichier
	fwrite(buf, AD_CDDA_SESSION_INFOS_SIZE, 1, mds);
	free(buf);
}

// écrire la section de lead-in "track first" pour cdda
void ad_write_cdda_lead_in_track_first_infos(FILE* mds) {
	write_array_block(mds, AD_CDDA_LEAD_IN_TRACK_FIRST_SIZE, AD_CDDA_LEAD_IN_TRACK_FIRST_ENTRIES, ad_cdda_lead_in_track_first);
}

// écrire la section de lead-in "track last" pour cdda
void ad_write_cdda_lead_in_track_last_infos(FILE* mds, int cdda_tracks_count) {
	unsigned char *buf;
	
	// init le buffer
	buf = (char *) malloc(AD_CDDA_LEAD_IN_TRACK_LAST_SIZE);
	fill_buffer(buf, AD_CDDA_LEAD_IN_TRACK_LAST_SIZE, AD_CDDA_LEAD_IN_TRACK_LAST_ENTRIES, ad_cdda_lead_in_track_last);
	
	// nombre de pistes cdda
	buf[0x05] = cdda_tracks_count;
	
	// écrire dans le fichier
	fwrite(buf, AD_CDDA_LEAD_IN_TRACK_LAST_SIZE, 1, mds);
	free(buf);
}

// écrire la section de lead-in "track leadout" pour cdda
void ad_write_cdda_lead_in_track_leadout_infos(FILE* mds, int cdda_session_sectors_count) {
	unsigned char *buf;
	int m, s, f;
	
	// init le buffer
	buf = (char *) malloc(AD_CDDA_LEAD_IN_TRACK_LEADOUT_SIZE);
	fill_buffer(buf, AD_CDDA_LEAD_IN_TRACK_LEADOUT_SIZE, AD_CDDA_LEAD_IN_TRACK_LEADOUT_ENTRIES, ad_cdda_lead_in_track_leadout);
	
	// lba2msf de la somme des blocs de la session 1 (cdda)
	lba_2_msf(cdda_session_sectors_count, &m, &s, &f);
	buf[0x05] = m;
	buf[0x06] = s;
	buf[0x07] = f;
	
	// écrire dans le fichier
	fwrite(buf, AD_CDDA_LEAD_IN_TRACK_LEADOUT_SIZE, 1, mds);
	free(buf);
}

/*
	Permet d'écrire une information correspondant exactement à une piste cdda.
	
	mds               : Fichier MDS ouvert
	track_num         : Numéro de la piste qui va être écrit
	                    - exemple : 1 = première piste 
	cdda_tracks_count : Nombre de pistes cdda total
	msinfo            : Décalage ou commence la piste
*/
void ad_write_cdda_track_infos(FILE* mds, int track_num, int cdda_tracks_count, int msinfo) {
	unsigned char *buf;
	int m, s, f;
	int ofs_extra, ofs_footer, track_offset;
	
	// init le buffer
	buf = (char *) malloc(AD_CDDA_TRACK_SIZE);
	fill_buffer(buf, AD_CDDA_TRACK_SIZE, AD_CDDA_TRACK_ENTRIES, ad_cdda_track);
	
	// numéro de piste
	buf[0x04] = track_num;
	
	// lba_2_msf du décalage courant de cette piste
	lba_2_msf(msinfo, &m, &s, &f);
	buf[0x09] = m;
	buf[0x0a] = s;
	buf[0x0b] = f;
	
	// ofs_extra : start offset of this track's extra block.
	ofs_extra = (1040 + (cdda_tracks_count * 80)) + ((track_num - 1) * 8);
	buf[0x0c] = ofs_extra;
	buf[0x0d] = ofs_extra >> 8;
	buf[0x0e] = ofs_extra >> 16;
	buf[0x0f] = ofs_extra >> 24;
	
	// msinfo correspondant au début de cette piste.
	// exemple : 302 (blocs)
	buf[0x24] = msinfo;
	buf[0x25] = msinfo >> 8;
	buf[0x26] = msinfo >> 16;
	buf[0x27] = msinfo >> 24;
	
	// track_offset correspond à la taille en byte avant la piste courante.
	// exemple : 302 (blocs) * 2352 (taille audio) = 710304
	track_offset = msinfo * AUDIO_SECTOR_SIZE;
	buf[0x28] = track_offset;
	buf[0x29] = track_offset >> 8;
	buf[0x2a] = track_offset >> 16;
	buf[0x2b] = track_offset >> 24;
	
	// start offset of footer.
	ofs_footer = 1104 + (cdda_tracks_count * 88);
	buf[0x34] = ofs_footer;
	buf[0x35] = ofs_footer >> 8;
	buf[0x36] = ofs_footer >> 16;
	buf[0x37] = ofs_footer >> 24;
	
	// écrire dans le fichier
	fwrite(buf, AD_CDDA_TRACK_SIZE, 1, mds);
	free(buf);
}

// permet d'écrire les informations sur la session de données.
void ad_write_data_session_infos(FILE* mds, int cdda_session_sectors_count) {
	unsigned char *buf;
	int m, s, f;
	int data_session_start_lba; // lba = msinfo !
	
	// init le buffer
	buf = (char *) malloc(AD_DATA_SESSION_INFOS_SIZE);
	fill_buffer(buf, AD_DATA_SESSION_INFOS_SIZE, AD_DATA_SESSION_INFOS_ENTRIES, ad_data_session_infos);
	
	// écrire le M:S:F correspondant au début de la seconde session de données
	data_session_start_lba = (cdda_session_sectors_count + MSINFO_OFFSET_BASE) - TAO_OFFSET;
	lba_2_msf(data_session_start_lba, &m, &s, &f);
	buf[0x05] = m;
	buf[0x06] = s;
	buf[0x07] = f;
	
	// écrire dans le fichier
	fwrite(buf, AD_DATA_SESSION_INFOS_SIZE, 1, mds);
	free(buf);
}

void ad_write_data_lead_in_track_first_infos(FILE* mds, int cdda_tracks_count) {
	unsigned char *buf;
	
	// init le buffer
	buf = (char *) malloc(AD_DATA_LEAD_IN_TRACK_FIRST_SIZE);
	fill_buffer(buf, AD_DATA_LEAD_IN_TRACK_FIRST_SIZE, AD_DATA_LEAD_IN_TRACK_FIRST_ENTRIES, ad_data_lead_in_track_first);
	
	// nombre de pistes cdda
	buf[0x05] = cdda_tracks_count + 0x01;
	
	// écrire dans le fichier
	fwrite(buf, AD_DATA_LEAD_IN_TRACK_FIRST_SIZE, 1, mds);
	free(buf);
}

void ad_write_data_lead_in_track_last_infos(FILE* mds, int cdda_tracks_count) {
	unsigned char *buf;
	
	// init le buffer
	buf = (char *) malloc(AD_DATA_LEAD_IN_TRACK_LAST_SIZE);
	fill_buffer(buf, AD_DATA_LEAD_IN_TRACK_LAST_SIZE, AD_DATA_LEAD_IN_TRACK_LAST_ENTRIES, ad_data_lead_in_track_last);
	
	// nombre de pistes cdda
	buf[0x05] = cdda_tracks_count + 0x01;
	
	// écrire dans le fichier
	fwrite(buf, AD_DATA_LEAD_IN_TRACK_LAST_SIZE, 1, mds);
	free(buf);
}

void ad_write_data_lead_in_track_leadout_infos(FILE* mds, int cdda_session_sectors_count, int data_session_sectors_count) {
	unsigned char *buf;
	int m, s, f;
	int data_session_end_lba;
	
	// init le buffer
	buf = (char *) malloc(AD_DATA_LEAD_IN_TRACK_LEADOUT_SIZE);
	fill_buffer(buf, AD_DATA_LEAD_IN_TRACK_LEADOUT_SIZE, AD_DATA_LEAD_IN_TRACK_LEADOUT_ENTRIES, ad_data_lead_in_track_leadout);
	
	// écrire le M:S:F correspondant à la fin de la seconde session de données
	data_session_end_lba = (cdda_session_sectors_count + MSINFO_OFFSET_BASE) + data_session_sectors_count;
	lba_2_msf(data_session_end_lba, &m, &s, &f);
	buf[0x05] = m;
	buf[0x06] = s;
	buf[0x07] = f;
	
	// écrire dans le fichier
	fwrite(buf, AD_DATA_LEAD_IN_TRACK_LEADOUT_SIZE, 1, mds);
	free(buf);
}

// permet d'écrire les infos sur la piste de données (le bloc de 80 bytes).
void ad_write_data_track_infos(FILE* mds, int cdda_session_sectors_count, int cdda_tracks_count) {
	unsigned char *buf;
	int m, s, f;
	int msinfo, ofs_extra, cdda_session_offset, ofs_footer;
	
	// init le buffer
	buf = (char *) malloc(AD_DATA_TRACK_SIZE);
	fill_buffer(buf, AD_DATA_TRACK_SIZE, AD_DATA_TRACK_ENTRIES, ad_data_track);
	
	// nombre de pistes audio + 1 (en fait il s'agit du numéro de piste data)
	buf[0x04] = cdda_tracks_count + 0x01;
	
	// début de la piste data en M:S:F
	msinfo = cdda_session_sectors_count + MSINFO_OFFSET_BASE;
	lba_2_msf(msinfo, &m, &s, &f);
	buf[0x09] = m;
	buf[0x0a] = s;
	buf[0x0b] = f;
	
	// ofs_extra selon la doc de Henrik...
	ofs_extra = 1088 + (88 * cdda_tracks_count);
	buf[0x0c] = ofs_extra;
	buf[0x0d] = ofs_extra >> 8;
	buf[0x0e] = ofs_extra >> 16;
	buf[0x0f] = ofs_extra >> 24;
	
	// msinfo correspondant au début de cette piste.
	// exemple : 302 (blocs)
	buf[0x24] = msinfo;
	buf[0x25] = msinfo >> 8;
	buf[0x26] = msinfo >> 16;
	buf[0x27] = msinfo >> 24;
	
	// cdda_session_offset correspond à la taille en byte avant la session audio précédente.
	// exemple : 302 (blocs) * 2352 (taille audio) = 710304
	cdda_session_offset = cdda_session_sectors_count * AUDIO_SECTOR_SIZE;
	buf[0x28] = cdda_session_offset;
	buf[0x29] = cdda_session_offset >> 8;
	buf[0x2a] = cdda_session_offset >> 16;
	buf[0x2b] = cdda_session_offset >> 24;
	
	// start offset of footer.
	ofs_footer = 1104 + (cdda_tracks_count * 88);
	buf[0x34] = ofs_footer;
	buf[0x35] = ofs_footer >> 8;
	buf[0x36] = ofs_footer >> 16;
	buf[0x37] = ofs_footer >> 24;
	
	// écrire dans le fichier
	fwrite(buf, AD_DATA_TRACK_SIZE, 1, mds);
	free(buf);
}

void ad_write_data_track_infos_header_start(FILE* mds) {
	write_array_block(mds, AD_DATA_TRACK_INFOS_HEADER_START_SIZE, AD_DATA_TRACK_INFOS_HEADER_START_ENTRIES, ad_data_track_infos_header_start);
}

void ad_write_cdda_track_sectors_count(FILE* mds, unsigned int cdda_sectors_count) {
	unsigned char* buf;
	
	buf = (char *) malloc(sizeof(unsigned int));
	
	buf[0x0] = cdda_sectors_count;
	buf[0x1] = cdda_sectors_count >> 8;
	buf[0x2] = cdda_sectors_count >> 16;
	buf[0x3] = cdda_sectors_count >> 24;
	
	fwrite(buf, sizeof(unsigned int), 1, mds);
	write_null_block(mds, 4); // c'est sur 8 bytes normalement, pour le support des DVD...  mais on grave des CD, nous !
	
	free(buf);
}

void ad_write_data_track_sectors_count(FILE* mds, unsigned int data_session_sectors_count) {
	unsigned char* buf;
	
	// init le buffer
	buf = (char *) malloc(AD_DATA_TRACK_INFOS_HEADER_END_SIZE);
	fill_buffer(buf, AD_DATA_TRACK_INFOS_HEADER_END_SIZE, AD_DATA_TRACK_INFOS_HEADER_END_ENTRIES, ad_data_track_infos_header_end);

	buf[0x30] = data_session_sectors_count;
	buf[0x31] = data_session_sectors_count >> 8;
	buf[0x32] = data_session_sectors_count >> 16;
	buf[0x33] = data_session_sectors_count >> 24;
	
	// écrire dans le fichier
	fwrite(buf, AD_DATA_TRACK_INFOS_HEADER_END_SIZE, 1, mds);
	free(buf);
}

void ad_write_mds_footer(FILE* mds, int cdda_tracks_count) {
	unsigned char* buf;
	int ofs_footer;
	
	// init le buffer
	buf = (char *) malloc(AD_MDS_FOOTER_SIZE);
	fill_buffer(buf, AD_MDS_FOOTER_SIZE, AD_MDS_FOOTER_ENTRIES, ad_mds_footer);

	ofs_footer = 1120 + (cdda_tracks_count * 88);
	buf[0x00] = ofs_footer;
	buf[0x01] = ofs_footer >> 8;
	buf[0x02] = ofs_footer >> 16;
	buf[0x03] = ofs_footer >> 24;
	
	// écrire dans le fichier
	fwrite(buf, AD_MDS_FOOTER_SIZE, 1, mds);
	free(buf);
}

