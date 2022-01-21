#include <stdio.h>
#include "imgwrite.h"
#include "mdsaudio.h"
#include "mdsdata.h"
#include "mdfwrt.h"
#include "tools.h"
#include "config.h"

extern unsigned int image_format;
extern int image_creation_okay;

unsigned int cdda_session_sectors_count = 0; // pour la session audio
unsigned int data_session_sectors_count = 0; // pour la session data
unsigned int data_boothead_session_sectors_count = 0; // pour la session boot data (session 2) dans le cas d'un disque DATA/DATA
unsigned int data_start_lba = 0;

void write_audio_data_image(FILE* mds, FILE* mdf, FILE* iso, int cdda_tracks_count, char* audio_files_array[]) {
	int i, j, track_postype;
	FILE* raw = NULL; // audio track
	
	AUDIOTRACKINFOS audio_infos[cdda_tracks_count];

#ifdef APP_CONSOLE
	info_msg("Creating target MDF");
#endif

	/* ECRITURE DU MDF */
	if (mds != NULL) {
	
#ifdef APP_CONSOLE
		printf("OK\n");
		
		info_msg("Writing audio track(s)");
#endif

		/* Ecriture des pistes CDDA en session 1 */
		if (image_format == AUDIO_DATA_CUSTOM_CDDA_IMAGE_FORMAT) {
			
			// on va �crire chaque piste audio dans le fichier.
			for(i = 0 ; i < cdda_tracks_count ; i++) {
				raw = fopen(audio_files_array[i], "rb");
				
#ifdef APP_CONSOLE
				textColor(YELLOW);
				if (i == 0) 
					printf("%02d", i+1); 
				else 
					printf("%*c%02d", INFO_MSG_SIZE + 1, ' ', i+1);
				textColor(LIGHT_GRAY);
				printf(": ");
#endif
				if (raw != NULL) {
					if (i == cdda_tracks_count - 1) track_postype = IS_LAST_TRACK; else track_postype = IS_NORMAL_TRACK;
					audio_infos[i].sectors_count = write_audio_track(mdf, raw, track_postype);
					audio_infos[i].lba = cdda_session_sectors_count;
					cdda_session_sectors_count = cdda_session_sectors_count + audio_infos[i].sectors_count;
					fclose(raw);
				} else {
					image_creation_okay = 0;
#ifdef APP_CONSOLE
					warning_msg("Track file not found !\n");
#endif
				}
				
			}
			
		} else { 
			// AUDIO_DATA_IMAGE_FORMAT
			// une fausse piste audio de 302 secteurs est �crite.
			audio_infos[0].lba = 0;
			audio_infos[0].sectors_count = MINIMAL_TRACK_SIZE_BLOCKS + 2;
			cdda_session_sectors_count = audio_infos[0].sectors_count;
			write_null_block(mdf, AUDIO_FAKE_TRACK_SIZE);
#ifdef APP_CONSOLE
			printf("Fake track written successfully\n");
#endif
		}
		
#ifdef APP_CONSOLE
		info_msg("Opening source ISO");
#endif

		/* Ecriture de la piste DATA en session 2 */
		if (iso != NULL) {
#ifdef APP_CONSOLE
			printf("OK\n");
			
			info_msg("Writing datas track");
#endif
			data_start_lba = cdda_session_sectors_count + MSINFO_OFFSET_BASE;
			data_session_sectors_count = write_data_track(mdf, iso, data_start_lba);
			fclose(iso);
		} else {
			image_creation_okay = 0;
#ifdef APP_CONSOLE
			warning_msg("Source ISO not found !");
#endif
		}
		
		// �criture du MDF termin�e.
		fclose(mdf);
	} else {
		image_creation_okay = 0;
#ifdef APP_CONSOLE
		warning_msg("Failed !\n");
#endif
	}

#ifdef APP_CONSOLE
	info_msg("Creating target MDS");
#endif

	/* ECRITURE DU MDS */
	if (mds != NULL) {
		// en-t�te
		ad_write_mds_header(mds);
		
		// session 1 (audio)
		ad_write_cdda_session_infos(mds, cdda_session_sectors_count, cdda_tracks_count, data_session_sectors_count);
		ad_write_cdda_lead_in_track_first_infos(mds);
		ad_write_cdda_lead_in_track_last_infos(mds, cdda_tracks_count);
		ad_write_cdda_lead_in_track_leadout_infos(mds, cdda_session_sectors_count);
		
		for(i = 0 ; i < cdda_tracks_count ; i++)
			ad_write_cdda_track_infos(mds, (i + 1), cdda_tracks_count, audio_infos[i].lba);
		
		// session 2 (data)
		ad_write_data_session_infos(mds, cdda_session_sectors_count);
		
		ad_write_data_lead_in_track_first_infos(mds, cdda_tracks_count);
		ad_write_data_lead_in_track_last_infos(mds, cdda_tracks_count);
		ad_write_data_lead_in_track_leadout_infos(mds, cdda_session_sectors_count, data_session_sectors_count);
		
		ad_write_data_track_infos(mds, cdda_session_sectors_count, cdda_tracks_count);
		ad_write_data_track_infos_header_start(mds);
		
		for(i = 0 ; i < cdda_tracks_count ; i++)
			ad_write_cdda_track_sectors_count(mds, audio_infos[i].sectors_count);
		
		ad_write_data_track_sectors_count(mds, data_session_sectors_count);
		
		// pied
		ad_write_mds_footer(mds, cdda_tracks_count);
		
		// fermeture du MDS
		fclose(mds);
		
#ifdef APP_CONSOLE
		printf("OK\n");
#endif
	} else {
		image_creation_okay = 0;
#ifdef APP_CONSOLE
		warning_msg("Failed !\n");
#endif
	}
	
#ifdef APP_CONSOLE
	info_msg("Total space used");
	start_progressbar();
	unsigned int total_space_blocks = get_total_space_used_blocks(cdda_session_sectors_count, data_session_sectors_count);
	unsigned int total_space_bytes = get_total_space_used_bytes(cdda_session_sectors_count, data_session_sectors_count);
	writing_track_event_end(total_space_blocks, total_space_bytes);
#endif
	
}

void write_data_data_image(FILE* mds, FILE* mdf, FILE* iso) {

#ifdef APP_CONSOLE
	info_msg("Creating target MDF");
#endif

	/* ECRITURE DU MDF */
	if (mds != NULL) {
	
#ifdef APP_CONSOLE
		printf("OK\n");
		info_msg("Opening source ISO");
#endif

		/* Ecriture de la piste DATA en session 1 */
		if (iso != NULL) {
		
#ifdef APP_CONSOLE
			printf("OK\n");
			info_msg("Writing datas track");
#endif
			data_session_sectors_count = write_data_track(mdf, iso, 0);
			
#ifdef APP_CONSOLE
			info_msg("Writing boot track");
#endif

			/* Ecriture de la piste BOOT DATA en session 2 */
			data_start_lba = data_session_sectors_count + MSINFO_OFFSET_BASE;
			data_boothead_session_sectors_count = write_data_boot_track(mdf, iso, data_start_lba);
			
#ifdef APP_CONSOLE
			printf("OK\n");
#endif

			fclose(iso);
		} else {
			image_creation_okay = 0;
#ifdef APP_CONSOLE
			warning_msg("Source ISO not found !");
#endif
		}
		
		// �criture du MDF termin�e.
		fclose(mdf);
	} else {
		image_creation_okay = 0;
#ifdef APP_CONSOLE
		warning_msg("Failed !\n");
#endif
	}
			
#ifdef APP_CONSOLE
	info_msg("Creating target MDS");
#endif

	/* ECRITURE DU MDS */
	if (mds != NULL) {
		// en-t�te
		dd_write_mds_header(mds);
		
		// session 1
		dd_write_data_session_infos(mds, data_session_sectors_count, data_boothead_session_sectors_count);
		dd_write_data_lead_in_track_first_infos(mds);
		dd_write_data_lead_in_track_last_infos(mds);
		dd_write_data_lead_in_track_leadout_infos(mds, data_session_sectors_count);
		dd_write_data_track_infos(mds);
		
		// session 2
		dd_write_boot_session_infos(mds, data_session_sectors_count, data_boothead_session_sectors_count);
		dd_write_boot_lead_in_track_first_infos(mds);
		dd_write_boot_lead_in_track_last_infos(mds);
		dd_write_boot_lead_in_track_leadout_infos(mds, data_session_sectors_count, data_boothead_session_sectors_count);
		dd_write_boot_track_infos(mds, data_session_sectors_count);
		dd_write_boot_track_infos_header(mds, data_session_sectors_count);
		
		// pied
		dd_write_mds_footer(mds);
		
		// fermeture du MDS
		fclose(mds);
		
#ifdef APP_CONSOLE
		printf("OK\n");
#endif
	} else {
		image_creation_okay = 0;
#ifdef APP_CONSOLE
		warning_msg("Failed !\n");
#endif
	}
	
#ifdef APP_CONSOLE
	info_msg("Total space used");
	start_progressbar();
	unsigned int total_space_blocks = get_total_space_used_blocks(cdda_session_sectors_count, data_session_sectors_count);
	unsigned int total_space_bytes = get_total_space_used_bytes(cdda_session_sectors_count, data_session_sectors_count);
	writing_track_event_end(total_space_blocks, total_space_bytes);
#endif
}

