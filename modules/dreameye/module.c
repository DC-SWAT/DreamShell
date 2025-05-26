/* DreamShell ##version##

   module.c - dreameye module
   Copyright (C) 2015, 2023, 2025 SWAT

*/

#include "ds.h"
#include "drivers/dreameye.h"

int builtin_dreameye_cmd(int argc, char *argv[]);
void dreameye_cleanup_all_states(void);

DEFAULT_MODULE_HEADER(dreameye);

int lib_open(klibrary_t *lib) {
	AddCmd(lib_get_name(), "Dreameye camera", (CmdHandler *) builtin_dreameye_cmd);
	return nmmgr_handler_add(&ds_dreameye_hnd.nmmgr);
}

int lib_close(klibrary_t *lib) {
	RemoveCmd(GetCmdByName(lib_get_name()));
	dreameye_cleanup_all_states();
	return nmmgr_handler_remove(&ds_dreameye_hnd.nmmgr);
}

static int save_image(const char *fn, uint8 *buf, int size) {

	file_t fd;

	fd = fs_open(fn, O_CREAT | O_TRUNC | O_WRONLY);

	if(fd == FILEHND_INVALID) {
		ds_printf("DS_ERROR: Can't open file for write: %s\n", fn);
		return -1;
	}

	fs_write(fd, buf, size);
	fs_close(fd);
	return 0;
}


int builtin_dreameye_cmd(int argc, char *argv[]) {

	/* Arguments */
	int grab = 0, count = 0, erase = 0, show = 0, hide = 0, stop = 0;
	int video = 0, isp = DREAMEYE_ISP_MODE_SIF, fmt = DREAMEYE_FRAME_FMT_YUV420P;
	char *file = NULL, *dir = NULL;
	int num = -1;

    if(argc < 2) {
		ds_printf("Usage: %s options args...\n"
					"Options: \n"
					" -g, --grab    -Grab images from dreameye and save to file\n"
					" -e, --erase   -Erase images from dreameye\n"
					" -c, --count   -Get images count\n"
					" -v, --video   -Setup video camera for capturing\n"
					" -s, --stop    -Stop video camera capturing\n"
					" -w, --show    -Show preview\n"
					" -h, --hide    -Hide preview\n\n", argv[0]);
		ds_printf("Arguments: \n"
					" -f, --file    -File for save image\n"
					" -d, --dir     -Directory for save all images\n"
					" -n, --num     -Image number (unset for all, 0 and 1 is video fb's)\n"
					" -i, --isp     -ISP mode for capture (0-4), default %d (320x240)\n"
					" -t, --fmt     -Pixel format for capture (0-3), default %d (yuv420p)\n\n", isp, fmt);
		ds_printf("Example: %s -g -n 2 -f /sd/photo.jpg\n"
					"         %s -g -n 0 -f /sd/frame0.raw\n"
					"         %s -g -d /sd", argv[0], argv[0], argv[0]);
		return CMD_NO_ARG; 
    }

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
		{"file",  'f', NULL, CFG_STR,  (void *) &file,  0},
		{"num",   'n', NULL, CFG_INT,  (void *) &num,   0},
		{"dir",   'd', NULL, CFG_STR,  (void *) &dir,   0},
		{"video", 'v', NULL, CFG_BOOL, (void *) &video, 0},
		{"stop",  's', NULL, CFG_BOOL, (void *) &stop,  0},
		{"isp",   'i', NULL, CFG_INT,  (void *) &isp,   0},
		{"fmt",   't', NULL, CFG_INT,  (void *) &fmt,   0},
		{"show",  'w', NULL, CFG_BOOL, (void *) &show,  0},
		{"hide",  'h', NULL, CFG_BOOL, (void *) &hide,  0},
		CFG_END_OF_LIST
	};

  	CMD_DEFAULT_ARGS_PARSER(options);

	dreameye = maple_enum_type(0, MAPLE_FUNC_CAMERA);

	if(!dreameye) {
		ds_printf("DS_ERROR: Couldn't find any attached devices, bailing out.\n");
		return CMD_ERROR;
	}

	state = (dreameye_state_t *)maple_dev_status(dreameye);

	if(video) {
		ds_printf("DS_PROCESS: Setting up video mode: isp=%d fmt=%d\n", isp, fmt);
		dreameye_setup_video_camera(dreameye, isp, fmt);
		return CMD_OK;
	}

	if(stop) {
		ds_printf("DS_PROCESS: Stop video capture\n");
		dreameye_stop_video_camera(dreameye);
		return CMD_OK;
	}

	if(show) {
		ds_printf("DS_PROCESS: Initializing preview\n");
		dreameye_preview_t p = {
			.isp_mode = isp,
    		.bpp = 12,
			.fullscreen = 1,
			.scale = 1,
			.x = 0,
			.y = 0,
			.callback = NULL
		};
		if (dreameye_preview_init(dreameye, &p) < 0) {
			return CMD_ERROR;
		}
		return CMD_OK;
	}

	if(hide) {
		ds_printf("DS_PROCESS: Shutting down preview\n");
		dreameye_preview_shutdown(dreameye);
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

		if (num > 1) {
			ds_printf("DS_PROCESS: Attempting to grab image: %d\n", num);
		} else if(num < 0) {
			ds_printf("DS_PROCESS: Attempting to grab all images\n");
		} else {
			ds_printf("DS_PROCESS: Attempting to capture video frame: %d\n", num);
		}

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
				ds_printf("DS_PROCESS: Writing %d bytes to %s\n", size, fn);

				if(save_image(fn, buf, size) < 0) {
					ds_printf("DS_ERROR: Couldn't write to %s\n", fn);
				}

				free(buf);
			}

		} else {

			if(num < 2) {
				err = dreameye_get_video_frame(dreameye, &buf, &size);
			} else {
				err = dreameye_get_image(dreameye, num, &buf, &size);
			}

			if(err != MAPLE_EOK) {
				ds_printf("DS_ERROR: No image data at index: %d\n", num);
				return CMD_ERROR;
			}

			ds_printf("DS_PROCESS: Writing %d bytes to %s\n", size, file);

			if(save_image(file, buf, size) < 0) {
				ds_printf("DS_ERROR: Couldn't write to %s\n", file);
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
