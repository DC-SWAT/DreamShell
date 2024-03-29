/*
	mdsdata.c - Media Descriptor Header Support Writer for Data/Data images format
	
	Created by [big_fury]SiZiOUS / Dreamcast-Scene 2007
	Version 0.1 - 05/06/07
	
	Based on specs by Henrik Stokseth <hensto AT c2i DOT net>
*/

#ifndef __MDSDATA__H__
#define __MDSDATA__H__

/* Header */

#define DD_MDS_HEADER_ENTRIES 25
#define DD_MDS_HEADER_SIZE 92
static const unsigned int dd_mds_header[DD_MDS_HEADER_ENTRIES][2] = {
		{0x00000, 0x4d}, {0x00001, 0x45}, {0x00002, 0x44}, {0x00003, 0x49}, {0x00004, 0x41}, {0x00005, 0x20}, {0x00006, 0x44},
		{0x00007, 0x45}, {0x00008, 0x53}, {0x00009, 0x43}, {0x0000a, 0x52}, {0x0000b, 0x49}, {0x0000c, 0x50}, {0x0000d, 0x54},
		{0x0000e, 0x4f}, {0x0000f, 0x52}, {0x00010, 0x01}, {0x00011, 0x04}, {0x00014, 0x02}, {0x00016, 0x02}, {0x00050, 0x58},
		{0x00058, 0x6a}, {0x00059, 0xff}, {0x0005a, 0xff}, {0x0005b, 0xff}
};

/* Donn�es GAP (les deux secteurs en fin de piste) */

#define DD_GAP_DUMMY_SECTOR_ENTRIES 6
#define DD_GAP_DUMMY_SECTOR_SIZE 2352
static const unsigned int dd_gap_dummy_sector[DD_GAP_DUMMY_SECTOR_ENTRIES][2] = {
		{0x00012, 0x20}, {0x00016, 0x20}, {0x0092c, 0x3f}, {0x0092d, 0x13}, {0x0092e, 0xb0}, {0x0092f, 0xbe}
};

/* Session 01 : Data (DATA en MODE 2 FORM 1) */

#define DD_DATA_SESSION_INFOS_ENTRIES 26
#define DD_DATA_SESSION_INFOS_SIZE 48
static const unsigned int dd_data_session_infos[DD_DATA_SESSION_INFOS_ENTRIES][2] = {
		{0x00000, 0xff}, {0x00001, 0xff}, {0x00002, 0xff}, {0x00003, 0xff}, {0x00004, 0x01}, {0x00006, 0x06}, {0x00007, 0x03},
		{0x00008, 0x01}, {0x0000a, 0x01}, {0x00010, 0x88}, {0x00014, 0xff}, {0x00015, 0xff}, {0x00016, 0xff}, {0x00017, 0xff},
		{0x00018, 0xff}, {0x00019, 0xff}, {0x0001a, 0xff}, {0x0001b, 0xff}, {0x0001c, 0x02}, {0x0001e, 0x04}, {0x0001f, 0x03},
		{0x00020, 0x02}, {0x00022, 0x02}, {0x00028, 0x68}, {0x00029, 0x02}, {0x0002e, 0x14}
};

#define DD_DATA_LEAD_IN_TRACK_FIRST_ENTRIES 4
#define DD_DATA_LEAD_IN_TRACK_FIRST_SIZE 80
static const unsigned int dd_data_lead_in_track_first[DD_DATA_LEAD_IN_TRACK_FIRST_ENTRIES][2] = {
		{0x00000, 0xa0}, {0x00005, 0x01}, {0x00006, 0x20}, {0x0004e, 0x14}
};

#define DD_DATA_LEAD_IN_TRACK_LAST_ENTRIES 3
#define DD_DATA_LEAD_IN_TRACK_LAST_SIZE 80
static const unsigned int dd_data_lead_in_track_last[DD_DATA_LEAD_IN_TRACK_LAST_ENTRIES][2] = {
		{0x00000, 0xa1}, {0x00005, 0x01}, {0x0004e, 0x14}
};

#define DD_DATA_LEAD_IN_TRACK_LEADOUT_ENTRIES 4
#define DD_DATA_LEAD_IN_TRACK_LEADOUT_SIZE 76
static const unsigned int dd_data_lead_in_track_leadout[DD_DATA_LEAD_IN_TRACK_LEADOUT_ENTRIES][2] = { /* 0xff � modif */
		{0x00000, 0xa2}, {0x00005, 0xff}, {0x00006, 0xff}, {0x00007, 0xff}
};

#define DD_DATA_TRACK_ENTRIES 12
#define DD_DATA_TRACK_SIZE 80
static const unsigned int dd_data_track[DD_DATA_TRACK_ENTRIES][2] = {
		{0x00000, 0xec}, {0x00002, 0x14}, {0x00004, 0x01}, {0x0000a, 0x02}, {0x0000c, 0xc0}, {0x0000d, 0x03}, {0x00010, 0x30},
		{0x00011, 0x09}, {0x00012, 0x02}, {0x00030, 0x01}, {0x00034, 0xf8}, {0x00035, 0x03}
};

/* Session 02 : Data (DATA en MODE 2 FORM 1) */

#define DD_BOOT_SESSION_INFOS_ENTRIES 16
#define DD_BOOT_SESSION_INFOS_SIZE 164
static const unsigned int dd_boot_session_infos[DD_BOOT_SESSION_INFOS_ENTRIES][2] = { /* 0xff � modif */
		{0x00002, 0x50}, {0x00004, 0xb0}, {0x00005, 0xff}, {0x00006, 0xff}, {0x00007, 0xff}, {0x00008, 0x02}, {0x00009, 0xff},
		{0x0000a, 0xff}, {0x0000b, 0xff}, {0x00052, 0x50}, {0x00054, 0xc0}, {0x00055, 0x50}, {0x00059, 0x61}, {0x0005a, 0x1a},
		{0x0005b, 0x3c}, {0x000a2, 0x14}
};

#define DD_BOOT_LEAD_IN_TRACK_FIRST_ENTRIES 4
#define DD_BOOT_LEAD_IN_TRACK_FIRST_SIZE 80
static const unsigned int dd_boot_lead_in_track_first[DD_BOOT_LEAD_IN_TRACK_FIRST_ENTRIES][2] = {
		{0x00000, 0xa0}, {0x00005, 0x02}, {0x00006, 0x20}, {0x0004e, 0x14}
};

#define DD_BOOT_LEAD_IN_TRACK_LAST_ENTRIES 3
#define DD_BOOT_LEAD_IN_TRACK_LAST_SIZE 80
static const unsigned int dd_boot_lead_in_track_last[DD_BOOT_LEAD_IN_TRACK_LAST_ENTRIES][2] = {
		{0x00000, 0xa1}, {0x00005, 0x02}, {0x0004e, 0x14}
};

#define DD_BOOT_LEAD_IN_TRACK_LEADOUT_ENTRIES 4
#define DD_BOOT_LEAD_IN_TRACK_LEADOUT_SIZE 76
static const unsigned int dd_boot_lead_in_track_leadout[DD_BOOT_LEAD_IN_TRACK_LEADOUT_ENTRIES][2] = { /* 0xff � modif */
		{0x00000, 0xa2}, {0x00005, 0xff}, {0x00006, 0xff}, {0x00007, 0xff}
};

#define DD_BOOT_TRACK_ENTRIES 22
#define DD_BOOT_TRACK_SIZE 80
static const unsigned int dd_boot_track[DD_BOOT_TRACK_ENTRIES][2] = { /* 0xff et 0xee � modif */
		{0x00000, 0xec}, {0x00002, 0x14}, {0x00004, 0x02}, {0x00009, 0xff}, {0x0000a, 0xff}, {0x0000b, 0xff}, {0x0000c, 0xf0},
		{0x0000d, 0x03}, {0x00010, 0x30}, {0x00011, 0x09}, {0x00012, 0x02}, {0x00024, 0xff}, {0x00025, 0xff}, {0x00026, 0xff},
		{0x00027, 0xff}, {0x00028, 0xee}, {0x00029, 0xee}, {0x0002a, 0xee}, {0x0002b, 0xee}, {0x00030, 0x01}, {0x00034, 0xf8},
		{0x00035, 0x03}
};

#define DD_BOOT_TRACK_INFOS_HEADER_ENTRIES 8
#define DD_BOOT_TRACK_INFOS_HEADER_SIZE 80
static const unsigned int dd_boot_track_infos_header[DD_BOOT_TRACK_INFOS_HEADER_ENTRIES][2] = {
		{0x00018, 0x96}, {0x0001c, 0xff}, {0x0001d, 0xff}, {0x0001e, 0xff}, {0x0001f, 0xff}, {0x00048, 0x96}, {0x0004c, 0x2e},
		{0x0004d, 0x01}
};

/* Footer */

#define DD_MDS_FOOTER_ENTRIES 7
#define DD_MDS_FOOTER_SIZE 22
static const unsigned int dd_mds_footer[DD_MDS_FOOTER_ENTRIES][2] = {
		{0x00000, 0x08}, {0x00001, 0x04}, {0x00010, 0x2a}, {0x00011, 0x2e}, {0x00012, 0x6d}, {0x00013, 0x64}, {0x00014, 0x66},
};

void dd_write_mds_header(FILE* mds);
void dd_write_data_session_infos(FILE* mds, int data_session_sectors_count, int boot_session_sectors_count);
void dd_write_data_lead_in_track_first_infos(FILE* mds);
void dd_write_data_lead_in_track_last_infos(FILE* mds);
void dd_write_data_lead_in_track_leadout_infos(FILE* mds, int data_session_sectors_count);
void dd_write_data_track_infos(FILE* mds);
void dd_write_boot_session_infos(FILE* mds, int data_session_sectors_count, int boot_session_sectors_count);
void dd_write_boot_lead_in_track_first_infos(FILE* mds);
void dd_write_boot_lead_in_track_last_infos(FILE* mds);
void dd_write_boot_lead_in_track_leadout_infos(FILE* mds, int data_session_sectors_count, int boot_session_sectors_count);
void dd_write_boot_track_infos(FILE* mds, int data_session_sectors_count);
void dd_write_boot_track_infos_header(FILE* mds, int data_session_sectors_count);
void dd_write_mds_footer(FILE* mds);

#endif // __MDSDATA__H__

