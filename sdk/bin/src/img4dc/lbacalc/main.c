#include <stdio.h>
#include "tools.h"

#define VERSION "0.1"
#define DATE "26/05/07"

#define AUDIO_BLOCK_SIZE 2352
#define MSINFO_OFFSET 11400
#define MINIMAL_FILE_SIZE 300 * AUDIO_BLOCK_SIZE
#define TAO_OFFSET 150

unsigned long fsize(char* filename) {
	unsigned long curpos, length = 0;	
	FILE* f = fopen(filename, "rb");
	
	if (f != NULL) {
		curpos = ftell(f); // garder la position courante
		fseek(f, 0L, SEEK_END);
		length = ftell(f);
		fseek(f, curpos, SEEK_SET); // restituer la position
		fclose(f);
		if (length < MINIMAL_FILE_SIZE) {
			textColor(LIGHT_RED);
			printf("warning: \"%s\" file is violating the minimal track length !\n", filename);
			textColor(LIGHT_GRAY);
		}
	} else {
		textColor(LIGHT_RED);
		printf("warning: \"%s\" file doesn't exists !\n", filename);
		textColor(LIGHT_GRAY);
	}
	
	return length;
}

void print_usage(char* prgname_cmdline) {
	char* prg_name = extract_proggyname(prgname_cmdline);
	
	textColor(LIGHT_RED);
	printf("LBACALC");
	textColor(WHITE);
	printf(" - ");
	textColor(YELLOW);
	printf("%s", VERSION);
	textColor(WHITE);
	printf(" - by SiZ!^DCS %s\n", DATE);
	printf("http://sbibuilder.shorturl.com/\n\n");
	printf("This proggy was designed for computing ");
	textColor(LIGHT_RED);
	printf("MSINFO");
	textColor(WHITE);
	printf(" value to use with mkisofs when\n");
	printf("generating your data track, in case of you are using a lot of CDDA tracks.");
	textColor(LIGHT_RED);
	printf(" But\nwatch out : tracks must be in RAW format (*NOT* WAV).\n\n");
	textColor(LIGHT_BLUE);
	printf("Usage:");
	textColor(LIGHT_CYAN);
	printf(" %s <track_1.raw ... track_n.raw>\n\n", prg_name);
	textColor(LIGHT_BLUE);
	printf("Examples:\n");
	textColor(WHITE);
	printf("   %s t1.raw : The proggy prints for example 11702 (in case of a minimal\n                    track size of 710304 bytes, corresponding to 4 sec).\n\n", prg_name);
	printf("   %s t1.raw t2.raw t3.raw t4.raw : Example output : ", prg_name);
	textColor(LIGHT_RED);
	printf("12608");
	textColor(WHITE);
	printf(".\n");
	printf("\nNow, you can use the computed value in mkisofs:\n");
	printf("  mkisofs -C 0,");
	textColor(LIGHT_RED);
	printf("12608");
	textColor(WHITE);
	printf(" -V IMAGE -G IP.BIN -joliet -rock -l -o data.iso data\n");
	printf("\nAnd now, you can use your generated iso in your favorite selfboot proggy.\n\n");
	textColor(YELLOW);
	printf("SiZ!^DCS '07 ... Can you believe it ?\n");
	textColor(LIGHT_GRAY);
	
	free(prg_name);
}

int main(int argc, char* argv[]) {
	int i, msinfo = 0;
	
	if (argc > 1) { 
		for(i = 1 ; i < argc ; i++) {
			msinfo = msinfo + (fsize(argv[i]) / AUDIO_BLOCK_SIZE) + 2; 
			if (i < argc - 1) // pas la dernière piste
				msinfo = msinfo + TAO_OFFSET;
		}
		msinfo = msinfo + MSINFO_OFFSET;
		
		if (msinfo == 0) {
			textColor(LIGHT_RED);
			printf("error: audio session is empty !");
			textColor(LIGHT_GRAY);
			return 1;
		} else 
			printf("%d", msinfo);
			
	} else
		print_usage(argv[0]);
		
	return 0;
}

