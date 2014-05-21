/* 
	:: C D I 4 D C ::
	
	By SiZiOUS, http://sbibuilder.shorturl.com/
	5 july 2006, v0.2b
	
	File : 	MAIN.C
	Desc : 	The main file for cd4dc.
			
			Sorry comments are in french. If you need some help you can contact me at sizious[at]dc-france.com.
			
	
	Changes :
		0.3b :	13 april 2007
				- Added a new image generation method : Data/Data images.
				- Removed the algo that checks if the ISO is a veritable 11702 because it fails.
				
		0.2b : 	6 july 2006
				- If the iso doesn't exists, cdi4dc don't create an empty CDI file. (Thx Fackue)
				- Fixed a problem with CD Labels with spaces (Thx Fackue)
				- Checks if the ISO is a veritable 11702 iso.
		
		0.1b : 	5 july 2006
				Initial release
*/

#include "cdibuild.h"
#include "tools.h"

#define BUILD_DATE "13 april 2007"
#define VERSION "0.3b"
#define WARNING_MSG "(TO A CD-RW PLEASE AGAIN... BETA VERSION !!!)"

unsigned long x, y; // position ou on doit placer le curseur avant d'écrire le pourcentage

void print_help(char* prgname) {	
	textColor(WHITE);
	printf("This proggy was written in order to replace the old (but good) Xeal's bin2boot.\n");
	printf("It generates a ");
	textColor(LIGHT_RED);
	printf("*REAL*");
	textColor(WHITE);
	printf(" valid CDI file from an ISO.\n\n");
	
	textColor(LIGHT_BLUE);
	printf("Syntax: ");
	textColor(LIGHT_CYAN);
	printf("%s <input.iso> <output.cdi> [-d]\n\n", prgname);
	
	textColor(LIGHT_BLUE);
	printf("Options:\n");
	textColor(LIGHT_CYAN);
	printf("  -d        : Generates a Data/Data image from a MSINFO 0 ISO.\n");
	printf("  <nothing> : Generates a Audio/Data image from a MSINFO 11702 ISO.\n\n");
	
	textColor(YELLOW);
	printf("This proggy's still dedicated to Ron even if today it isn't the 6 july !\n\n");
	printf("Greetings goes to:\n  BlackAura, Fackue, Xeal, DeXT, Heiko Eissfeldt and Joerg Schilling.\n\n");
	textColor(LIGHT_GREEN);
	printf("SiZ! for Dreamcast-Scene / %s... The legend will never die...\n", BUILD_DATE);
	
	textColor(LIGHT_GRAY);
}

void start_progressbar() {
	x =	whereX();
	y = whereY();
}

void padding_event(int sector_count) {
	gotoXY(x, y);
	textColor(LIGHT_RED);
	printf("%d block(s) : padding is needed...\n", sector_count);
	textColor(LIGHT_GRAY);
}

void writing_data_track_event(unsigned long current_pos, unsigned long total_iso_size) {
	gotoXY(x, y);
	
	float p1 = (float)current_pos / (float)total_iso_size;	
	unsigned long percent = p1 * 100;
	printf("[");
	textColor(LIGHT_RED);
	printf("%u/%u", current_pos, total_iso_size);
	textColor(LIGHT_GRAY);
	printf("] - ");
	textColor(LIGHT_MAGENTA);
	printf("%u%%\n", percent);
	textColor(LIGHT_GRAY);
}

void echo(char* label) {
	textColor(WHITE);
	printf(label);
	textColor(LIGHT_GRAY);
}

void print_head() {
	textColor(LIGHT_RED);
	printf("CDI4DC");
	textColor(WHITE);
	printf(" - ");
	textColor(YELLOW);
	printf(VERSION);
	textColor(WHITE);
	printf(" - (C)reated by [big_fury]SiZiOUS\n");
	printf("http://sbibuilder.shorturl.com/\n\n");
	textColor(LIGHT_GRAY);
}

void create_audio_data_image(FILE* infp, FILE* outfp, char* outfilename) {
	char volume_name[32];
	int i, data_blocks_count;
	float space_used;
	
	/*i = get_iso_msinfo_value(infp);
	if (i != 11702) { // 11702 pour notre MSINFO !
		textColor(LIGHT_RED);
		printf("Warning: The ISO's LBA seems to be invalid ! Value : %d\n", i);
		textColor(LIGHT_GRAY);
	}*/
		
	echo("Image method............: ");
	printf("Audio/Data\n");
	
	get_volumename(infp, &volume_name);
	echo("Volume name.............: ");
	printf("%s\n", volume_name);
	
	// ecrire piste audio
	echo("Writing audio track.....: ");
	write_cdi_audio_track(outfp);
	printf("OK\n");
	
	// ecrire les tracks GAP
	echo("Writing pregap tracks...: ");
	write_gap_tracks(outfp);
	printf("OK\n");
	
	//ecrire section data
	echo("Writing datas track.....: ");
	
	start_progressbar();
	
	data_blocks_count = write_data_track(outfp, infp);
	
	space_used = (float)(get_total_cdi_space_used(data_blocks_count) * data_sector_size) / 1024 / 1024;
	gotoXY(x, y);
	textColor(LIGHT_CYAN);
	printf("\n%25c %d ", ' ',  data_blocks_count);
	textColor(LIGHT_GRAY);
	printf("block(s) written (");
	textColor(LIGHT_GREEN);
	printf("%5.2fMB ", space_used);
	textColor(LIGHT_GRAY);
	printf("used)\n");
	
	// ecrire en tête (le footer)
	echo("Writing CDI header......: ");
	write_audio_cdi_header(
		outfp, 
		outfilename, 
		volume_name, 
		data_blocks_count, 
		get_total_cdi_space_used(data_blocks_count)
	);
	printf("OK\n");
}

void create_data_data_image(FILE* infp, FILE* outfp, char* outfilename) {
	char volume_name[32];
	int i, data_blocks_count;
	float space_used;
	
	echo("Image method............: ");
	printf("Data/Data\n");
	
	get_volumename(infp, &volume_name);
	echo("Volume name.............: ");
	printf("%s\n", volume_name);
	
	// écrire le début du CDI data/data (GAP 1)
	echo("Writing data pregap.....: ");
	write_data_gap_start_track(outfp);
	printf("OK\n");
	
	//ecrire section data
	echo("Writing datas track.....: ");
	
	start_progressbar();
	
	data_blocks_count = write_data_track(outfp, infp);
	
	space_used = (float)(get_total_cdi_space_used(data_blocks_count) * data_sector_size) / 1024 / 1024;
	gotoXY(x, y);
	textColor(LIGHT_CYAN);
	printf("\n%25c %d ", ' ',  data_blocks_count);
	textColor(LIGHT_GRAY);
	printf("block(s) written (");
	textColor(LIGHT_GREEN);
	printf("%5.2fMB ", space_used);
	textColor(LIGHT_GRAY);
	printf("used)\n");
	
	// ecrire les tracks GAP
	echo("Writing pregap tracks...: ");
	write_gap_tracks(outfp);
	printf("OK\n");

	// ecrire la piste 2 head.
	echo("Writing header track....: ");
	write_data_header_boot_track(outfp, infp);
	printf("OK\n");
	
	// ecrire en tête (le footer)
	echo("Writing CDI header......: ");
	write_data_cdi_header(
		outfp, 
		outfilename, 
		volume_name, 
		data_blocks_count, 
		get_total_cdi_space_used(data_blocks_count)
	);
	printf("OK\n");
}

// let's go my friends
int main(int argc, char *argv[]) {
	char outfilename[256];
	FILE *outfp, *infp;
	
	print_head();
	
	if (argc < 3) {
		print_help(argv[0]);
		return 1;
	}
	
	infp = fopen(argv[1], "rb");
	
	if (infp == NULL) {
		textColor(LIGHT_RED);
		perror("Error when opening file");
		textColor(LIGHT_GRAY);
		return 2;
	}
	
	if (check_iso_is_bootable(infp) == 0) {
		textColor(LIGHT_RED);
		printf("Warning: %s seems not to be bootable\n\n", argv[1]);
		textColor(LIGHT_GRAY);
	}
	
	strcpy(outfilename, argv[2]);
	
	outfp = fopen(outfilename, "wb");
	
	if (outfp == NULL) {
		textColor(LIGHT_RED);
		perror("Error when writing file");
		textColor(LIGHT_GRAY);
		return 2;
	}
	
	// choisir la méthode
	
	if ((argc == 4) && (strcmp(argv[3], "-d") == 0))
		create_data_data_image(infp, outfp, outfilename);
	else
		create_audio_data_image(infp, outfp, outfilename);	
	
	textColor(LIGHT_CYAN);
	printf("\nWoohoo... All done OK!\nYou can burn it now %s...\n", WARNING_MSG);
	
	fclose(outfp);
	fclose(infp);
	
	textColor(LIGHT_GRAY);
	
	return 0;
}
