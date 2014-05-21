/* DreamShell ##version##

   dreameye.c
   Copyright (C) 2009-2013 SWAT
*/


#include "ds.h"
#include <dc/maple.h>
#include <dc/maple/dreameye.h>


static int save_image(char *fn, uint8 *buf, int size) {
	
	file_t f;

	f = fs_open(fn, O_CREAT | O_WRONLY);

	if(f < 0) {
		ds_printf("DS_ERROR: Can't open file %s\n", fn);
		return 0;
	}

	fs_write(f, buf, size);
	fs_close(f);
	return 1;
}


int main(int argc, char *argv[]) {
    
    if(argc < 2) {
		ds_printf("Usage: dreameye options args\n"
	              "Options: \n"
	              " -g, --grab      -Grab images from dreameye and save to file\n"
	              " -e, --erase     -Erase images from dreameye\n"
	              " -c, --count     -Get images count\n\n"
	              "Arguments: \n"
	              " -f, --file      -File for save image\n"
				  " -d, --dir       -Directory for save all images\n"
				  " -n, --num       -Image number for grab or erase\n\n"
	              "Example: dreameye -g -n 1 -f /sd/image.jpg\n"
				  "         dreameye -g -d /sd");
        return CMD_NO_ARG; 
    } 

    int grab = 0, count = 0, erase = 0;
    char *file = NULL, *dir = NULL;
	int num = 0;
    char fn[MAX_FN_LEN];
	maple_device_t *dreameye;
    dreameye_state_t *state;
    uint8 *buf;
    int size, err, i;

	struct cfg_option options[] = {
		{"grab", 'g', NULL, CFG_BOOL, (void *) &grab, 0},
		{"count",  'c', NULL, CFG_BOOL, (void *) &count,  0},
		{"erase", 'e', NULL, CFG_BOOL, (void *) &erase, 0},
		{"file",  'f', NULL, CFG_STR,  (void *) &file,  0},
		{"num", 'n', NULL, CFG_INT,(void *) &num, 0},
		{"dir",  'd', NULL, CFG_STR,  (void *) &dir,  0},
		CFG_END_OF_LIST
	};
  
  	CMD_DEFAULT_ARGS_PARSER(options);
	
    dreameye = maple_enum_type(0, MAPLE_FUNC_CAMERA);

    if(!dreameye) {
        ds_printf("DS_ERROR: Couldn't find any attached devices, bailing out.\n");
        return CMD_ERROR;
    }

    state = (dreameye_state_t *)maple_dev_status(dreameye);


	if(count) {

		ds_printf("DS_POCESS: Attempting to grab the number of saved images...\n");
		dreameye_get_image_count(dreameye, 1);

		ds_printf("DS_INFO: Image Count is %s -- (%d)\n",
			   state->image_count_valid ? "valid" : "invalid", state->image_count);
	}


	if(grab) {

		ds_printf("DS_PROCESS: Attempting to grab the %s...\n", (num ? "image" : "images"));
		
		
		if(!num) {

			dreameye_get_image_count(dreameye, 1);

			if(!state->image_count_valid || !state->image_count) {
				ds_printf("DS_ERROR: No images avaible.");
				return CMD_ERROR;
			}

			for(i = 0; i < state->image_count; i++) {

				err = dreameye_get_image(dreameye, i, &buf, &size);

				if(err != MAPLE_EOK) {
					ds_printf("DS_ERROR: Error was: %d\n", err);
					return CMD_ERROR;
				}

				sprintf(fn, "%s/image_%d.jpg", dir, i+1);
				ds_printf("DS_OK: Image received successfully, size %d bytes. Writing to %s\n", size, fn);

				if(!save_image(fn, buf, size)) {
					ds_printf("DS_ERROR: Can't save image to %s\n", fn);
				}

				free(buf);
			}

		} else {

			err = dreameye_get_image(dreameye, num, &buf, &size);

			if(err != MAPLE_EOK) {
				ds_printf("DS_ERROR: Error was: %d\n", err);
				return CMD_ERROR;
			}

			ds_printf("DS_OK: Image received successfully, size %d bytes. Writing to %s\n", size, file);

			if(!save_image(file, buf, size)) {
				free(buf);
				return CMD_ERROR;
			}
			free(buf);
		}

		return CMD_OK;
	}

	if(erase) {

		ds_printf("DS_PROCESS: Erasing %s...\n", (num ? "all images" : "image"));

		err = dreameye_erase_image(dreameye, (num ? num : 0xFF), 1);

		if(err != MAPLE_EOK) {
			ds_printf("DS_ERROR: Error was: %d\n", err);
			return CMD_ERROR;
		}

		ds_printf("DS_OK: Complete.\n");
		return CMD_OK;
	}


    return CMD_OK;
}
