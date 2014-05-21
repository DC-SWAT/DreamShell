/* 
	:: C D I 4 D C ::
	
	By SiZiOUS, http://sbibuilder.shorturl.com/
	5 july 2006, v0.2b
	
	File : 	CDIHEAD.H
	Desc : 	This file contains the CDI structure file.
			
			Sorry comments are in french. If you need some help you can contact me at sizious[at]dc-france.com.
*/

/* 
	Un CDI bootable pour Dreamcast de type Audio/Data est composé de plusieurs parties (dans l'ordre) :
	
		- 352 800 octets de zéros binaires (padding ?),
		- La piste audio (mode audio donc) de 302 secteurs (numérotés de 0 à 301) dans le cas d'un CDI avec la 
		  piste "audio.raw" du kit ECHELON.
		- Une piste "GAP" servant à délimiter la piste audio de la piste de données, sur 350408 bytes. Elle est de deux types
		  différents détaillés ci-dessous.
		- Une piste de données, en Mode 2 Form 1 (CD/XA), commencant au secteur (lba ou msinfo) 11702 (toujours pour audio.raw 
		  de ECHELON).
		- Quelques secteurs de padding (2 pour être exact)
		- Et enfin le vrai header du CDI de longueur variable.
		
	Une piste doit faire au minimum 302 secteurs. Si l'ISO est trop petit et que la piste fait en dessous de 302 secteurs,
	il faut rajouter des secteurs de padding pour arriver à 302. Si c'est supérieur à 302, on fait rien. En effet, 302 secteurs 
	est la taille minimale d'une piste. Certains graveurs ont du mal à graver ce genre de pistes si c'est inférieur.
	
	Voici la structure du CDI en schéma :

	+=================================================+
	|         352 800 octets de zéros binaires        |
	+=================================================+
	|     Piste audio : 302 secteurs min, 710 304     |
	+-------------------------------------------------+	
	 |            0x108 octets de zéros              |
	 +-----------------------------------------------+
	 |  --- La piste elle même (705 600 octets)---   |
	 +-----------------------------------------------+
	 |              11760 octets de zéros            |
	+=================================================+
	|                    piste GAP                    |
	+-------------------------------------------------+
	 |   1st piste : gap_dummy_sector 175 200 octets |
	 +-----------------------------------------------+
	 |   2nd piste : gap_dummy_sector2 : même taille |
	+=================================================+
	|  Données : au secteur 11702, taille bloc : 2336 |
	+-------------------------------------------------+
	 |             2048 octets de données            |
	 +-----------------------------------------------+
	 |                4 octets d'EDC                 |
	 +-----------------------------------------------+
	 |              172 octets d'ECC P               |
	 +-----------------------------------------------+
	 |              104 octets d'ECC Q               |
	+=================================================+
	|                2 secteurs GAP 1                 |
	+=================================================+
	|                  Header du CDI                  |
	+=================================================+
	 |             Nombre de sessions (2)            |
	 +-----------------------------------------------+
	 |         Nombre de pistes par sessions (1)     |
	 +-----------------------------------------------+
	 |                 Padding de 4                  |
	 +-----------------------------------------------+
	 |               Track start mark                |
	 /////////////////////////////////////////////////
	 ///           INFOS SUR LES PISTES            ///
	 /////////////////////////////////////////////////
	  |           Infos piste audio :               |
	  +---------------------------------------------+
	   |            Track start mark               |
	   +-------------------------------------------+
	   |       Valeur inconnue sur 2 octets        |
	   +-------------------------------------------+
	   |            0x0210 (2 octets)              |
	   +-------------------------------------------+
	   |    Longueur du nom de fichier (1 octet)   |
	   +-------------------------------------------+
	   |         Nom de fichier en absolu          |
	   +-------------------------------------------+
	   |         Valeurs diverses et variées...    |
	   +-------------------------------------------+
	   |         Nombre de secteurs (0x012e)       |
	   +-------------------------------------------+
	   |     Encore du padding... (~20 octets)     |
	   +-------------------------------------------+
	   |    Nombre de secteurs + 150 (0x01c4)      |
	   +-------------------------------------------+
	   |      Encore padding (~20 octets)          |
	   +-------------------------------------------+
	   |    Nombre de secteurs + 150 (0x01c4)      |
	   +-------------------------------------------+
	   |   Plein de valeurs ... (echantionnage...) |
	  +---------------------------------------------+
	  |             Infos piste datas :             |
	  +---------------------------------------------+
	   |        Même chose que la piste audio...   |
	  +---------------------------------------------+
	  |                 Header final                |
	  +---------------------------------------------+
	   |  même chose que les autres jusqu'au nom   |
	   +-------------------------------------------+
	   |   la taille totale de tous les secteurs   |
	   +-------------------------------------------+
	   |    le label du CD et sa taille (1 octet)  |
	   +-------------------------------------------+
	   |  un ID de version de fichier 0x80000006   |
	   +-------------------------------------------+
	   | Emplacement head : size(CDI) - size(head) |
	   +-------------------------------------------+
*/

#ifndef __CDIHEAD__H__
#define __CDIHEAD__H__

// début de l'image : 352 800 bytes de zero binaires
static const unsigned int cdi_start_file_header = 352800;

// --- AUDIO TRACK ---

// une "piste" audio fait en tout 710 304 bytes ... (en fait la piste + les secteurs d'en-tête)
static const int cdi_audio_track_total_size = 710304;

// ... mais elle commence uniquement 108h bytes  après. Les 108h bytes avant de commencer la vraie piste audio sont des zéros binaires.
static const int cdi_audio_start_offset = 0x108;

// une piste audio se termine par 11760 octets de full zéro... ça correspond à cette adresse dans le bloc audio
static const int cdi_audio_end_padding = 0xaa8b0;

// --- GAP ---

// ce "gap" là est pour le cas d'une image data/data. Ce type d'image commence par une piste GAP de 350 400 bytes (soit 150 secteurs).
static const int gap_data_start_image_count = 150;

// le "gap" est une piste qui sépare la piste audio et la piste de datas.

static const int gap_sector_size = 2336;

// il y'a 75 secteurs GAP 1 et 75 secteurs GAP 2
static const int gap_sector_count = 75;

// un secteur GAP 1 est constitué de ça :
static const int gap_dummy_sector_size = 6;
static const unsigned int gap_dummy_sector[6][2] = {
		{0x002, 0x20}, 
		{0x006, 0x20}, 
		{0x91c, 0x3f}, 
		{0x91d, 0x13}, 
		{0x91e, 0xb0}, 
		{0x91f, 0xbe}
};

// un secteur GAP 2 est constitué de ça :
static const int gap_dummy_sector_2_size = 101;
static const unsigned int gap_dummy_sector_2[101][2] = {
		{0x00008, 0x54}, {0x00009, 0x44}, {0x0000a, 0x49}, {0x0000b, 0x01}, {0x0000c, 0x50}, {0x0000d, 0x01}, {0x0000e, 0x02},
		{0x0000f, 0x02}, {0x00010, 0x02}, {0x00011, 0x80}, {0x00012, 0xff}, {0x00013, 0xff}, {0x00014, 0xff}, {0x00808, 0x78},
		{0x00809, 0x62}, {0x0080a, 0x21}, {0x0080b, 0x6d}, {0x00818, 0x93}, {0x00819, 0x78}, {0x0081a, 0x85}, {0x0081b, 0xf5},
		{0x0081c, 0x60}, {0x0081d, 0xf5}, {0x0081e, 0xf7}, {0x0081f, 0xf7}, {0x00820, 0xf7}, {0x00821, 0x0b}, {0x00822, 0xaa},
		{0x00823, 0xaa}, {0x00824, 0xaa}, {0x0085e, 0x88}, {0x0085f, 0xa6}, {0x00860, 0x63}, {0x00861, 0xb7}, {0x0086e, 0xc7},
		{0x0086f, 0x3c}, {0x00870, 0xcc}, {0x00871, 0xf4}, {0x00872, 0x30}, {0x00873, 0xf4}, {0x00874, 0xf5}, {0x00875, 0xf5},
		{0x00876, 0xf5}, {0x00877, 0x8b}, {0x00878, 0x55}, {0x00879, 0x55}, {0x0087a, 0x55}, {0x008b4, 0xf0}, {0x008b5, 0xc4},
		{0x008b6, 0x42}, {0x008b7, 0xda}, {0x008c6, 0x63}, {0x008c7, 0xb7}, {0x008c8, 0xd0}, {0x008c9, 0xf7}, {0x008ca, 0x59},
		{0x008cb, 0x26}, {0x008cc, 0xea}, {0x008cd, 0x66}, {0x008d0, 0xd1}, {0x008d2, 0xf3}, {0x008d3, 0x15}, {0x008d4, 0x4d},
		{0x008d5, 0xf5}, {0x008d6, 0xf8}, {0x008d7, 0x31}, {0x008d8, 0x7e}, {0x008d9, 0x2f}, {0x008da, 0x6b}, {0x008db, 0xcc},
		{0x008dc, 0x41}, {0x008dd, 0x80}, {0x008de, 0xe0}, {0x008df, 0xf2}, {0x008e0, 0x23}, {0x008e1, 0x40}, {0x008fa, 0x42},
		{0x008fb, 0xda}, {0x008fc, 0xcb}, {0x008fd, 0x22}, {0x008fe, 0x93}, {0x008ff, 0x5a}, {0x00900, 0x1a}, {0x00901, 0xa2},
		{0x00904, 0x7b}, {0x00906, 0x0c}, {0x00907, 0xbf}, {0x00908, 0x10}, {0x00909, 0xab}, {0x0090a, 0x05}, {0x0090b, 0xb2},
		{0x0090c, 0xe9}, {0x0090d, 0xaf}, {0x0090e, 0xdc}, {0x0090f, 0xcf}, {0x00910, 0x4e}, {0x00911, 0x0d}, {0x00912, 0x6e},
		{0x00913, 0xcf}, {0x00914, 0x77}, {0x00915, 0x04}
};

// --- DATA ---

static const int data_sector_size = 2336;

// --- CDI HEADER ---

struct cdi_header {
	unsigned short int track_count; // 0x02
	unsigned short int first_track_num; //0x01
	unsigned long padding; //0x00000000
};

static const unsigned int track_start_mark[10] = {
	0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0xFF, 0xFF, 0xFF, 0xFF
};


static const int sector1_size = 195;
static const int sector1_entries = 28;
/* {0x00006, 0x2e}, {0x00007, 0x01} : c'est 0x12e = 302 secteurs audio */
static const unsigned int sector1[28][2] = {
		{0x00000, 0x02}, {0x00002, 0x96}, {0x00006, 0x2e}, {0x00007, 0x01}, {0x00024, 0xc4}, {0x00025, 0x01}, {0x00038, 0x02},
		{0x00041, 0xc4}, {0x00042, 0x01}, {0x0005a, 0xff}, {0x0005b, 0xff}, {0x0005c, 0xff}, {0x0005d, 0xff}, {0x0005e, 0xff},
		{0x0005f, 0xff}, {0x00060, 0xff}, {0x00061, 0xff}, {0x00062, 0x01}, {0x00066, 0x80}, {0x0006a, 0x02}, {0x0006e, 0x10},
		{0x00072, 0x44}, {0x00073, 0xac}, {0x000a0, 0xff}, {0x000a1, 0xff}, {0x000a2, 0xff}, {0x000a3, 0xff}, {0x000bd, 0x01},
};

// cdi header audio sector
static const int sector2_size = 195;
static const int sector2_entries = 31;
static const unsigned int sector2[31][2] = {
		{0x00000, 0x02}, {0x00002, 0x96}, {0x00006, 0x9c}, {0x00007, 0x04}, {0x00010, 0x02}, {0x00018, 0x01}, {0x00020, 0xb6},
		{0x00021, 0x2d}, {0x00038, 0x01}, {0x0003c, 0x04}, {0x0005a, 0xff}, {0x0005b, 0xff}, {0x0005c, 0xff}, {0x0005d, 0xff}, 
		{0x0005e, 0xff}, {0x0005f, 0xff}, {0x00060, 0xff}, {0x00061, 0xff}, {0x00062, 0x01}, {0x00066, 0x80}, {0x0006a, 0x02}, 
		{0x0006e, 0x10}, {0x00072, 0x44}, {0x00073, 0xac}, {0x000a0, 0xff}, {0x000a1, 0xff}, {0x000a2, 0xff}, {0x000a3, 0xff}, 
		{0x000b0, 0x02}, {0x000b8, 0xb6}, {0x000b9, 0x2d},
};

static const int cdi_head_data_data_track_sector_size = 195;
static const int cdi_head_data_data_track_sector_entries = 27;
static const unsigned int cdi_head_data_data_track_sector[27][2] = {
		{0x00000, 0x02}, {0x00002, 0x96}, {0x00006, 0x9c}, {0x00007, 0x04}, {0x00010, 0x02}, {0x00038, 0x01}, {0x0003c, 0x04}, 
		{0x0005a, 0xff}, {0x0005b, 0xff}, {0x0005c, 0xff}, {0x0005d, 0xff}, {0x0005e, 0xff}, {0x0005f, 0xff}, {0x00060, 0xff}, 
		{0x00061, 0xff}, {0x00062, 0x01}, {0x00066, 0x80}, {0x0006a, 0x02}, {0x0006e, 0x10}, {0x00072, 0x44}, {0x00073, 0xac}, 
		{0x000a0, 0xff}, {0x000a1, 0xff}, {0x000a2, 0xff}, {0x000a3, 0xff}, {0x000b0, 0x02}, {0x000bd, 0x01},
};

static const unsigned int cdi_head_data_data_track_2_sector_size = 195;
static const unsigned int cdi_head_data_data_track_2_sector_entries = 31;
static const unsigned int cdi_head_data_data_track_2_sector[31][2] = {
		{0x00000, 0x02}, {0x00002, 0x96}, {0x00006, 0x2e}, {0x00007, 0x01}, {0x00010, 0x02}, {0x00018, 0x01}, {0x00024, 0xc4}, 
		{0x00025, 0x01}, {0x00038, 0x01}, {0x0003c, 0x04}, {0x00041, 0xc4}, {0x00042, 0x01}, {0x0005a, 0xff}, {0x0005b, 0xff}, 
		{0x0005c, 0xff}, {0x0005d, 0xff}, {0x0005e, 0xff}, {0x0005f, 0xff}, {0x00060, 0xff}, {0x00061, 0xff}, {0x00062, 0x01}, 
		{0x00066, 0x80}, {0x0006a, 0x02}, {0x0006e, 0x10}, {0x00072, 0x44}, {0x00073, 0xac}, {0x000a0, 0xff}, {0x000a1, 0xff}, 
		{0x000a2, 0xff}, {0x000a3, 0xff}, {0x000b0, 0x02}, 
};

static const int cdi_head_next_size = 31;
static const int cdi_head_next_entries = 6;
static const unsigned int cdi_head_next[6][2] = {
	{0x0000b, 0x02}, {0x00016, 0x80}, {0x00017, 0x40}, {0x00018, 0x7e}, {0x00019, 0x05}, {0x0001d, 0x98}
};

static const int cdi_head_end_size = 42;
static const int cdi_head_end_entries = 4;
static const unsigned int cdi_header_end[4][2] = {
		{0x00001, 0x01}, {0x00005, 0x01}, {0x00026, 0x06}, {0x00029, 0x80}
};

#endif // __CDIHEAD_H__
