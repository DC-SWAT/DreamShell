/* DreamShell ##version##

   module.c - dreameye module
   Copyright (C)2009-2016 SWAT 

*/          

#include "ds.h"
#include "drivers/dreameye.h"

DEFAULT_MODULE_EXPORTS_CMD(dreameye, "Dreameye camera");

static int save_image(const char *fn, uint8 *buf, int size) {
	
	file_t fd;

	fd = fs_open(fn, O_CREAT | O_WRONLY);

	if(fd == FILEHND_INVALID) {
		ds_printf("DS_ERROR: Can't open file for write: %s\n", fn);
		return -1;
	}

	fs_write(fd, buf, size);
	fs_close(fd);
	return 0;
}


int builtin_dreameye_cmd(int argc, char *argv[]) {
    
    if(argc < 2) {
		ds_printf("Usage: %s options args...\n"
					"Options: \n"
					" -g, --grab    -Grab images from dreameye and save to file\n"
					" -e, --erase   -Erase images from dreameye\n"
					" -c, --count   -Get images count\n"
					" -p, --param   -Get param\n\n"
					"Arguments: \n"
					" -f, --file    -File for save image\n"
					" -d, --dir     -Directory for save all images\n"
					" -n, --num     -Image number for grab or erase (doesn't set it for all)\n\n"
					"Example: %s -g -n 2 -f /sd/photo.jpg\n"
					"         %s -g -d /sd", argv[0], argv[0], argv[0]);
		return CMD_NO_ARG; 
    } 

	/* Arguments */
	int grab = 0, count = 0, erase = 0, param = 0;
	char *file = NULL, *dir = NULL;
	int num = -1;
	
	/* Buffers */
	char fn[NAME_MAX];
	uint8 *buf;
	int size, err, i;
	
	/* Device state */
	maple_device_t *dreameye;
	dreameye_state_t *state;

	struct cfg_option options[] = {
		{"grab",  'g', NULL, CFG_BOOL, (void *) &grab,  0},
		{"count", 'c', NULL, CFG_BOOL, (void *) &count, 0},
		{"erase", 'e', NULL, CFG_BOOL, (void *) &erase, 0},
		{"param", 'p', NULL, CFG_BOOL, (void *) &param, 0},
		{"file",  'f', NULL, CFG_STR,  (void *) &file,  0},
		{"num",   'n', NULL, CFG_INT,  (void *) &num,   0},
		{"dir",   'd', NULL, CFG_STR,  (void *) &dir,   0},
		CFG_END_OF_LIST
	};
  
  	CMD_DEFAULT_ARGS_PARSER(options);
	
	dreameye = maple_enum_type(0, MAPLE_FUNC_CAMERA);

	if(!dreameye) {
		ds_printf("DS_ERROR: Couldn't find any attached devices, bailing out.\n");
		return CMD_ERROR;
	}

	state = (dreameye_state_t *)maple_dev_status(dreameye);
	
	if(param) {
		
		uint16 val;

		while(++num < 0x20) {
			dreameye_get_param(dreameye, DREAMEYE_COND_REG_JANGGU, num & 0xff, &val);
			ds_printf("0x%02x = 0x%02x\n", num & 0xff, val & 0xff);
		}

		return CMD_OK;
	}

	/* Get count option */
	if(count) {

		ds_printf("DS_PROCESS: Attempting to grab the number of saved images...\n");
		dreameye_get_image_count(dreameye, 1);

		if(state->image_count_valid && state->image_count > 0) {
			ds_printf("DS_INFO: Count of images: %d\n", state->image_count);
		} else {
			ds_printf("DS_INFO: No images avaible.\n");
		}
	}

	/* Grap data option */
	if(grab) {

		ds_printf("DS_PROCESS: Attempting to grab the %s...\n", (num > -1 ? "image" : "all images"));
		
		if(num < 0) {

			dreameye_get_image_count(dreameye, 1);

			if(!state->image_count_valid || !state->image_count) {
				ds_printf("DS_ERROR: No images avaible.\n");
				return CMD_ERROR;
			}

			for(i = 2; i < state->image_count + 2; i++) {

				err = dreameye_get_image(dreameye, i, &buf, &size);

				if(err != MAPLE_EOK) {
					ds_printf("DS_ERROR: No image data at index: %d\n", i);
					break;
				}

				sprintf(fn, "%s/photo_%d.jpg", dir, i);
				ds_printf("DS_OK: Image received successfully. Writing %d bytes to %s\n", size, fn);

				if(save_image(fn, buf, size) < 0) {
					ds_printf("DS_ERROR: Couldn't save image to %s\n", fn);
				}

				free(buf);
			}

		} else {

			if(num < 2) {
				err = dreameye_get_video_frame(dreameye, num, &buf, &size);
			} else {
				err = dreameye_get_image(dreameye, num, &buf, &size);
			}

			if(err != MAPLE_EOK) {
				ds_printf("DS_ERROR: No image data at index: %d\n", num);
				return CMD_ERROR;
			}

			ds_printf("DS_OK: Image received successfully. Writing %d bytes to %s\n", size, file);

			if(save_image(file, buf, size) < 0) {
				ds_printf("DS_ERROR: Couldn't save image to %s\n", file);
				free(buf);
				return CMD_ERROR;
			}

			free(buf);
		}

		ds_printf("DS_OK: Complete.\n");
		return CMD_OK;
	}

	/* Erase option */
	if(erase) {
		
		if(num < 2 || num > 33) {
			ds_printf("DS_ERROR: Wrong image index: %d\n", num);
			return CMD_ERROR;
		}

		ds_printf("DS_PROCESS: Erasing %s...\n", (num > 1 ? "image" : "all images"));

		err = dreameye_erase_image(dreameye, (num > 1 ? num : 0xFF), 1);

		if(err != MAPLE_EOK) {
			
			if(num > -1) {
				ds_printf("DS_ERROR: Couldn't erase image at index: %d\n", num);
			} else {
				ds_printf("DS_ERROR: Couldn't erase all images\n");
			}
			
			return CMD_ERROR;
		}

		ds_printf("DS_OK: Complete.\n");
		return CMD_OK;
	}

	return (!count ? CMD_NO_ARG : CMD_OK);
}
