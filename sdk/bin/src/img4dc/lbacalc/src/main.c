/* 
	:: L B A C A L C ::
	
	By SiZiOUS
	www.sizious.com - sizious (at) gmail (dot) com - @sizious - fb.com/sizious
	
	File:	main.c
	Desc:	Program entry point.
*/
#include <stdio.h>
#include "console.h"
#include "fileutil.h"
#include "version.h"
#include "main.h"

void print_usage() {
	printf_colored(LIGHT_RED, "LBACALC for IMG4DC");
	printf_colored(WHITE, " - ");
	printf_colored(YELLOW, "%s", get_version());
	printf_colored(WHITE, " - Written by SiZiOUS\nwww.sizious.com - sizious (at) gmail (dot) com - @sizious - fb.com/sizious\n\n");
	
	printf_colored(WHITE, "This proggy was designed for computing the ");
	printf_colored(LIGHT_RED, "MSINFO");
	printf_colored(WHITE, " value to use with mkisofs\nwhen generating the data track, when using a lot of CD-DA (Audio) tracks.\n");
	printf_colored(LIGHT_RED, "CD-DA tracks must be in RAW format, *NOT* in Wave form (i.e. WAV).\n\n");
	printf_colored(LIGHT_BLUE, "Usage:");
	printf_colored(LIGHT_CYAN, " %s <track_1.raw ... track_n.raw>\n\n", get_program_name());
	printf_colored(LIGHT_BLUE, "Examples:\n");
	printf_colored(WHITE, "   %s t1.raw : The proggy prints for example 11702 (in case of a minimal\n\t\t    track size of 710304 bytes, corresponding to 4 sec).\n\n   %s t1.raw t2.raw t3.raw t4.raw : Example output : ", get_program_name(), get_program_name());
	printf_colored(LIGHT_RED, "12608");
	printf_colored(WHITE, ".\n\nNow, you can use the computed value in mkisofs:\n");
	printf_colored(LIGHT_GRAY, "  mkisofs -C 0,");
	printf_colored(LIGHT_RED, "12608");
	printf_colored(LIGHT_GRAY, " -V IMAGE -G IP.BIN -joliet -rock -l -o data.iso data\n\n");
	printf_colored(YELLOW, "SiZiOUS '15 ... Can you *STILL* believe it ?\n");
}

int main(int argc, char *argv[]) {
	int i;
	uint32_t msinfo = 0, filesize = 0;
	char *filename;

	set_program_name(argv[0]);

	if (argc > 1) {
		for(i = 1 ; i < argc ; i++) {
			filename = argv[i];
			filesize = fsize(filename);
			
			if (filesize == -1) {
				printf_colored(LIGHT_RED, "error: \"%s\" doesn't exist!\n", filename);
				return 2;
			} else if (filesize < MINIMAL_FILE_SIZE) {
				printf_colored(LIGHT_RED, "warning: \"%s\" is violating the minimal track length!\n", filename);
			}
			
			msinfo += (filesize / AUDIO_BLOCK_SIZE) + 2; 
			if (i < (argc - 1)) { // not for the latest track			
				msinfo += TAO_OFFSET;
			}
		}
		msinfo += MSINFO_OFFSET;
		
		if (msinfo <= MSINFO_OFFSET) {
			printf_colored(LIGHT_RED, "error: the audio session seems to be empty!");
			return 1;
		} else {
			printf("%lu", msinfo);			
		}
			
	} else {
		print_usage();		
	}
		
	return 0;
}
