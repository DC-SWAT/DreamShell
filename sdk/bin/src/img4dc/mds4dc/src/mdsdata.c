/*
	mdsdata.c - Media Descriptor Header Support Writer for Data/Data images format
	
	Created by [big_fury]SiZiOUS / Dreamcast-Scene 2007
	Version 0.1 - 05/06/07
	
	Based on specs by Henrik Stokseth <hensto AT c2i DOT net>
*/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include "mdfwrt.h"
#include "mdsdata.h"
#include "tools.h"

/* Header */

// �crire l'en-t�te du fichier MDS
void dd_write_mds_header(FILE* mds) {		
	write_array_block(mds, DD_MDS_HEADER_SIZE, DD_MDS_HEADER_ENTRIES, dd_mds_header);
}

/* Session 01 : Data (Mode 2 Form 1) */

// �crire les informations de la session CDDA (la premi�re)
void dd_write_data_session_infos(FILE* mds, int data_session_sectors_count, int boot_session_sectors_count) {
	unsigned char *buf;
	int i, msinfo;
	
	// init le buffer
	buf = (char *) malloc(DD_DATA_SESSION_INFOS_SIZE);
	fill_buffer(buf, DD_DATA_SESSION_INFOS_SIZE, DD_DATA_SESSION_INFOS_ENTRIES, dd_data_session_infos);
		
	// taille totale de la session CDDA
	buf[0x00] = data_session_sectors_count;
	buf[0x01] = data_session_sectors_count >> 8;
	buf[0x02] = data_session_sectors_count >> 16;
	buf[0x03] = data_session_sectors_count >> 24;
		
	// MSINFO entre la session audio et la session data
	msinfo = (MSINFO_OFFSET_BASE + data_session_sectors_count);
	
	// Exemple : MSINFO (11702 - 150) = 11552 (2D20) 11702 = 11400 (valeur d�calage) + 302 (taille track 1 audio)
	i = msinfo - TAO_OFFSET;
	buf[0x14] = i;
	buf[0x15] = i >> 8;
	buf[0x16] = i >> 16;
	buf[0x17] = i >> 24;
	
	// Exemple : 11702 (lba audio) + 348 (nb secteurs piste data) = 12050 ou (Taille piste (espace utilis�) en blocs 11220 - 150 = 12050) ...
	i = get_total_space_used_blocks(data_session_sectors_count, boot_session_sectors_count) - TAO_OFFSET;
	buf[0x18] = i; 
	buf[0x19] = i >> 8;
	buf[0x1a] = i >> 16;
	buf[0x1b] = i >> 24;
		
	// �crire dans le fichier
	fwrite(buf, DD_DATA_SESSION_INFOS_SIZE, 1, mds);
	free(buf);
}

// �crire la section de lead-in "track first" pour cdda
void dd_write_data_lead_in_track_first_infos(FILE* mds) {
	write_array_block(mds, DD_DATA_LEAD_IN_TRACK_FIRST_SIZE, DD_DATA_LEAD_IN_TRACK_FIRST_ENTRIES, dd_data_lead_in_track_first);
}

// �crire la section de lead-in "track last" pour cdda
void dd_write_data_lead_in_track_last_infos(FILE* mds) {
	write_array_block(mds, DD_DATA_LEAD_IN_TRACK_LAST_SIZE, DD_DATA_LEAD_IN_TRACK_LAST_ENTRIES, dd_data_lead_in_track_last);
}

// �crire la section de lead-in "track leadout" pour cdda
void dd_write_data_lead_in_track_leadout_infos(FILE* mds, int data_session_sectors_count) {
	unsigned char *buf;
	int m, s, f;
	
	// init le buffer
	buf = (char *) malloc(DD_DATA_LEAD_IN_TRACK_LEADOUT_SIZE);
	fill_buffer(buf, DD_DATA_LEAD_IN_TRACK_LEADOUT_SIZE, DD_DATA_LEAD_IN_TRACK_LEADOUT_ENTRIES, dd_data_lead_in_track_leadout);
	
	// lba2msf de la somme des blocs de la session 1 (cdda)
	lba_2_msf(data_session_sectors_count, &m, &s, &f);
	buf[0x05] = m;
	buf[0x06] = s;
	buf[0x07] = f;
	
	// �crire dans le fichier
	fwrite(buf, DD_DATA_LEAD_IN_TRACK_LEADOUT_SIZE, 1, mds);
	free(buf);
}

/*
	Permet d'�crire une information correspondant exactement � une piste cdda.
	
	mds               : Fichier MDS ouvert
	msinfo            : D�calage ou commence la piste
*/
void dd_write_data_track_infos(FILE* mds) {
	unsigned char *buf;
	int m, s, f;
	int ofs_extra, ofs_footer, track_offset;
	
	// init le buffer
	buf = (char *) malloc(DD_DATA_TRACK_SIZE);
	fill_buffer(buf, DD_DATA_TRACK_SIZE, DD_DATA_TRACK_ENTRIES, dd_data_track);
							
	// �crire dans le fichier
	fwrite(buf, DD_DATA_TRACK_SIZE, 1, mds);
	free(buf);
}

/* Session 02 : Boot Data Header Track */

// permet d'�crire les informations sur la session de donn�es.
void dd_write_boot_session_infos(FILE* mds, int data_session_sectors_count, int boot_session_sectors_count) {
	unsigned char *buf;
	int m, s, f;
	int boot_session_start_lba, boot_session_end_lba; // lba = msinfo !
	
	// init le buffer
	buf = (char *) malloc(DD_BOOT_SESSION_INFOS_SIZE);
	fill_buffer(buf, DD_BOOT_SESSION_INFOS_SIZE, DD_BOOT_SESSION_INFOS_ENTRIES, dd_boot_session_infos);
	
	// �crire le M:S:F correspondant au d�but de la seconde session de donn�es
	boot_session_start_lba = (data_session_sectors_count + MSINFO_OFFSET_BASE) - TAO_OFFSET;
	lba_2_msf(boot_session_start_lba, &m, &s, &f);
	buf[0x05] = m;
	buf[0x06] = s;
	buf[0x07] = f;
	
	// �crire le M:S:F correspondant � la fin de la seconde session de donn�es (session de boot)
	boot_session_end_lba = get_total_space_used_blocks(data_session_sectors_count, boot_session_sectors_count) - TAO_OFFSET;
	lba_2_msf(boot_session_end_lba, &m, &s, &f);
	buf[0x09] = m;
	buf[0x0a] = s;
	buf[0x0b] = f;
	
	// �crire dans le fichier
	fwrite(buf, DD_BOOT_SESSION_INFOS_SIZE, 1, mds);
	free(buf);
}

void dd_write_boot_lead_in_track_first_infos(FILE* mds) {
	unsigned char *buf;
	
	// init le buffer
	buf = (char *) malloc(DD_BOOT_LEAD_IN_TRACK_FIRST_SIZE);
	fill_buffer(buf, DD_BOOT_LEAD_IN_TRACK_FIRST_SIZE, DD_BOOT_LEAD_IN_TRACK_FIRST_ENTRIES, dd_boot_lead_in_track_first);
		
	// �crire dans le fichier
	fwrite(buf, DD_BOOT_LEAD_IN_TRACK_FIRST_SIZE, 1, mds);
	free(buf);
}

void dd_write_boot_lead_in_track_last_infos(FILE* mds) {
	unsigned char *buf;
	
	// init le buffer
	buf = (char *) malloc(DD_BOOT_LEAD_IN_TRACK_LAST_SIZE);
	fill_buffer(buf, DD_BOOT_LEAD_IN_TRACK_LAST_SIZE, DD_BOOT_LEAD_IN_TRACK_LAST_ENTRIES, dd_boot_lead_in_track_last);
	
	// �crire dans le fichier
	fwrite(buf, DD_BOOT_LEAD_IN_TRACK_LAST_SIZE, 1, mds);
	free(buf);
}

void dd_write_boot_lead_in_track_leadout_infos(FILE* mds, int data_session_sectors_count, int boot_session_sectors_count) {
	unsigned char *buf;
	int m, s, f;
	int boot_session_end_lba;
	
	// init le buffer
	buf = (char *) malloc(DD_BOOT_LEAD_IN_TRACK_LEADOUT_SIZE);
	fill_buffer(buf, DD_BOOT_LEAD_IN_TRACK_LEADOUT_SIZE, DD_BOOT_LEAD_IN_TRACK_LEADOUT_ENTRIES, dd_boot_lead_in_track_leadout);
	
	// �crire le M:S:F correspondant � la fin de la seconde session de donn�es
	boot_session_end_lba = (data_session_sectors_count + MSINFO_OFFSET_BASE) + boot_session_sectors_count;
	lba_2_msf(boot_session_end_lba, &m, &s, &f);
	buf[0x05] = m;
	buf[0x06] = s;
	buf[0x07] = f;
	
	// �crire dans le fichier
	fwrite(buf, DD_BOOT_LEAD_IN_TRACK_LEADOUT_SIZE, 1, mds);
	free(buf);
}

// permet d'�crire les infos sur la piste de donn�es (le bloc de 80 bytes).
void dd_write_boot_track_infos(FILE* mds, int data_session_sectors_count) {
	unsigned char *buf;
	int m, s, f;
	int msinfo, ofs_extra, data_session_offset;
	
	// init le buffer
	buf = (char *) malloc(DD_BOOT_TRACK_SIZE);
	fill_buffer(buf, DD_BOOT_TRACK_SIZE, DD_BOOT_TRACK_ENTRIES, dd_boot_track);
		
	// d�but de la piste data en M:S:F
	msinfo = data_session_sectors_count + MSINFO_OFFSET_BASE;
	lba_2_msf(msinfo, &m, &s, &f);
	buf[0x09] = m;
	buf[0x0a] = s;
	buf[0x0b] = f;	
		
	// msinfo correspondant au d�but de cette piste.
	// exemple : 302 (blocs)
	buf[0x24] = msinfo;
	buf[0x25] = msinfo >> 8;
	buf[0x26] = msinfo >> 16;
	buf[0x27] = msinfo >> 24;
	
	// cdda_session_offset correspond � la taille en byte avant la session audio pr�c�dente.
	// exemple : 302 (blocs) * 2352 (taille audio) = 710304
	data_session_offset = data_session_sectors_count * AUDIO_SECTOR_SIZE;
	buf[0x28] = data_session_offset;
	buf[0x29] = data_session_offset >> 8;
	buf[0x2a] = data_session_offset >> 16;
	buf[0x2b] = data_session_offset >> 24;
	
	// �crire dans le fichier
	fwrite(buf, DD_BOOT_TRACK_SIZE, 1, mds);
	free(buf);
}

void dd_write_boot_track_infos_header(FILE* mds, int data_session_sectors_count) {	
	unsigned char *buf;
	
	// init le buffer
	buf = (char *) malloc(DD_BOOT_TRACK_INFOS_HEADER_SIZE);
	fill_buffer(buf, DD_BOOT_TRACK_INFOS_HEADER_SIZE, DD_BOOT_TRACK_INFOS_HEADER_ENTRIES, dd_boot_track_infos_header);

	// nombre de secteurs de la piste 1 (data)
	buf[0x1c] = data_session_sectors_count;
	buf[0x1d] = data_session_sectors_count >> 8;
	buf[0x1e] = data_session_sectors_count >> 16;
	buf[0x1f] = data_session_sectors_count >> 24;
	
	// �crire dans le fichier
	fwrite(buf, DD_BOOT_TRACK_SIZE, 1, mds);
	free(buf);
}

/* Footer */

void dd_write_mds_footer(FILE* mds) {
	unsigned char* buf;
	
	// init le buffer
	buf = (char *) malloc(DD_MDS_FOOTER_SIZE);
	fill_buffer(buf, DD_MDS_FOOTER_SIZE, DD_MDS_FOOTER_ENTRIES, dd_mds_footer);
	
	// �crire dans le fichier
	fwrite(buf, DD_MDS_FOOTER_SIZE, 1, mds);
	free(buf);
}

