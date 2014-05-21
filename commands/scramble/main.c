
#include "ds.h"

void scramble(char *src, char *dst);
void descramble(char *src, char *dst);

int main(int argc, char *argv[]) { 

    if(argc < 3) {
		ds_printf("Usage: %s -flag infile outfile\n");
		ds_printf("Flags: \n"
					"-s  -Scramble bin file\n"
					"-d  -Descramble bin file\n\n", argv[0]); 
		ds_printf("Example: %s -d /cd/1ST_READ.BIN /ram/unscramble.bin\n", argv[0]); 
		return CMD_NO_ARG; 
    } 
    

    if(!strncasecmp(argv[1], "-s", 2)) { 
                        
        ds_printf("DS_PROCESS: Scrambling...\n"); 
        scramble(argv[2], argv[3]); 
        ds_printf("DS_OK: Complete.\n"); 
        
    } else if(!strncasecmp(argv[1], "-d", 2)) {
          
        ds_printf("DS_PROCESS: Descrambling...\n"); 
        descramble(argv[2], argv[3]); 
        ds_printf("DS_OK: Complete.\n"); 
         
    } else {
        ds_printf("DS_ERROR: Uknown flag - '%s'\n", argv[1]);    
    }
    
    return CMD_OK;
} 
