#ifndef __MDFWRT__H__
#define __MDFWRT__H__

#include "libedc.h"
#include "tools.h"

#define INCLUDE_GAP_ENTRIES 1
#define NO_GAP_ENTRIES 0

#define MINIMAL_TRACK_SIZE_BLOCKS 300
#define MSINFO_OFFSET_BASE 11400
#define TAO_OFFSET 150

#define AUDIO_FAKE_TRACK_SIZE 0xAD6A0

#define AUDIO_SECTOR_SIZE 2352
#define DATA_SECTOR_SIZE 2336

#define DATA_SECTOR_START_PATTERN "\000\377\377\377\377\377\377\377\377\377\377"
#define AUDIO_SECTOR_START_SIZE 0x18

// méthode d'écriture
#define TAO_WRITE_METHOD_MODE 0
#define DAO_WRITE_METHOD_MODE 1

// permet de savoir si la piste est la dernière de la session (pour rajouter ou pas TAO_OFFSET, soit 150 blocks)
#define IS_NORMAL_TRACK 0
#define IS_LAST_TRACK 1

// format de l'image qui va être créé
#define UNKNOW_IMAGE_FORMAT 0
#define AUDIO_DATA_CUSTOM_CDDA_IMAGE_FORMAT 1 // méthode audio/data, l'utilisateur rajoute des pistes audios
#define AUDIO_DATA_IMAGE_FORMAT 2             // méthode audio/data, pas de pistes audios supplémentaires
#define DATA_DATA_IMAGE_FORMAT 3              // méthode data/data

int write_audio_track(FILE* mdf, FILE* audio, unsigned int is_last_track);
int write_data_track(FILE* mdf, FILE* iso, unsigned int start_lba);
int write_data_boot_track(FILE* mdf, FILE* iso, unsigned int start_lba);

#endif // __MDFWRT__H__
