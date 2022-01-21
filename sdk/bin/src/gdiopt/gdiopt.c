/** 
 * gdiopt.c 
 * Copyright (c) 2014-2015 SWAT
 */

#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

int bin2iso(const char *source, const char *target) {
    
    int   seek_header, seek_ecc, sector_size;
    long  i, source_length;
    char  buf[2352];
    const unsigned char SYNC_HEADER[12] = 
        {0, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0};
    
    FILE *fpSource, *fpTarget;

    fpSource = fopen(source, "rb");
    fpTarget = fopen(target, "wb");
    
    if ((fpSource==NULL) || (fpTarget==NULL)) {
        return -1;
    }
    
    fread(buf, sizeof(char), 16, fpSource);

    if (memcmp(SYNC_HEADER, buf, 12))
    {
        seek_header = 8;        
        seek_ecc = 280;
        sector_size = 2336;
    }
    else        
    {
        switch(buf[15])
        {
            case 2:
            {    
                seek_header = 24;    // Mode2/2352    
                seek_ecc = 280;
                sector_size = 2352;
                break;
            }

            case 1:
            {
                seek_header = 16;    // Mode1/2352
                seek_ecc = 288;
                sector_size = 2352;
                break;
            }

            default:
            {
                fclose(fpTarget);
                fclose(fpSource);
                return -1;
            }
        }
    }

    fseek(fpSource, 0L, SEEK_END);
    source_length = ftell(fpSource)/sector_size;
    fseek(fpSource, 0L, SEEK_SET);

    for(i=0; i<source_length; i++)
    {
		fseek(fpSource, seek_header, SEEK_CUR);
		fread(buf, sizeof(char), 2048, fpSource);  
		fwrite(buf, sizeof(char), 2048, fpTarget);
		fseek(fpSource, seek_ecc, SEEK_CUR);
    }

    fclose(fpTarget);
    fclose(fpSource);

    return 0;
}



int main(int argc, char *argv[]) {

	if(argc < 2) {
		printf("GDI optimizer v0.1 by SWAT\n");
		printf("Usage: %s input.gdi output.gdi", argv[0]);
		return 0;
	}
	
	FILE *fr, *fw;
	int i, rc, track_no, track_count;
	unsigned long start_lba, flags, sector_size, offset;
	char fn_old[256],  fn_new[256], cmd[256];
	char outfn[9] = "disk.gdi"; outfn[8] = '\0';
	char *out = outfn;
	
	fr = fopen(argv[1], "r");
	
	if(!fr) {
		printf("Can't open for read: %s\n", argv[1]);
		return -1;
	}
	
	if(argc > 2) {
		out = argv[2];
	}
	
	fw = fopen(out, "w");
	
	if(!fw) {
		printf("Can't open for write: %s\n", out);
		fclose(fr);
		return -1;
	}
	
	rc = fscanf(fr, "%d", &track_count);
	
	if(rc == 1) {
	
		fprintf(fw, "%d\n", track_count);
		
		for(i = 0; i < track_count; i++) {
	
			start_lba = flags = sector_size = offset = 0;
			memset(fn_new, 0, sizeof(fn_new));
			memset(fn_old, 0, sizeof(fn_old));
			
			rc = fscanf(fr, "%d %ld %ld %ld %s %ld", 
					&track_no, &start_lba, &flags, 
					&sector_size, fn_old, &offset);
				
			if(flags == 4) {
				int len = strlen(fn_old);
				strncpy(fn_new, fn_old, sizeof(fn_new));
				fn_new[len - 3] = 'i';
				fn_new[len - 2] = 's';
				fn_new[len - 1] = 'o';
				fn_new[len] = '\0';
				sector_size = 2048;
				
				printf("Converting %s to %s ...\n", fn_old, fn_new);
				
				if(bin2iso(fn_old, fn_new) < 0) {
					printf("Error!\n");
					fclose(fr);
					fclose(fw);
					return -1;
				}
				
				unlink(fn_old);
			}
			
			fprintf(fw, "%d %ld %ld %ld %s %ld\n", 
						track_no, start_lba, flags, sector_size, 
						(flags == 4 ? fn_new : fn_old), offset);
		}
	}
	fclose(fr);
	fclose(fw);
	unlink(argv[1]);
	return 0;
}

