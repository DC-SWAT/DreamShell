/* DreamShell ##version##

   NAOMI MIE EEPROM save/restore for ISO Loader
   Copyright (C) 2026 SWAT

*/

#include "ds.h"
#include "isoldr.h"
#include "naomi/cart.h"

#include <dc/maple/mie.h>
#include <stdbool.h>
#include <string.h>

#define NAOMI_EEPROM_GAME_MAX   42
#define NAOMI_CALIB_AXES        7

#define NAOMI_CALIB_DEF_ZERO    0x0000

typedef struct naomi_calib_map {
	const char game_id[5];
	uint8_t game_size;
	uint8_t axis_off[NAOMI_CALIB_AXES];
	uint16_t axis_def[NAOMI_CALIB_AXES];
} naomi_calib_map_t;

#define NAOMI_CALIB_N1_OFFS { 16, 18, 20, 22, 24, 26, 28 }
#define NAOMI_CALIB_N1_DEFS { \
	NAOMI_CALIB_DEF_ZERO, NAOMI_CALIB_DEF_ZERO, NAOMI_CALIB_DEF_ZERO, \
	NAOMI_CALIB_DEF_ZERO, NAOMI_CALIB_DEF_ZERO, NAOMI_CALIB_DEF_ZERO, \
	NAOMI_CALIB_DEF_ZERO \
}
#define NAOMI_CALIB_N2_OFFS { 21, 23, 25, 27, 29, 31, 33 }
#define NAOMI_CALIB_N2_DEFS { \
	0x0080, 0x0080, 0x0080, \
	0x0080, 0x0080, 0x0080, \
	0x0080 \
}
#define NAOMI_CALIB_BBK_OFFS { 33, 32, 35, 36, 37, 38, 39 }
#define NAOMI_CALIB_BBK_DEFS { 0x0000, 0x0080, 0x001f, 0x00df, 0x0025, 0x0050, 0x0050 }
#define NAOMI_CALIB_BBK_RIGHT_MIN 34
#define NAOMI_CALIB_BAC_OFFS { 25, 24, 26, 20, 21, 22, 23 }
#define NAOMI_CALIB_BAC_DEFS { 0x0028, 0x0081, 0x00e1, 0x002c, 0x00c3, 0x0037, 0x00ce }
#define NAOMI_CALIB_BAU_OFFS { 17, 16, 18, 12, 13, 14, 15 }
#define NAOMI_CALIB_BAU_DEFS { 0x0020, 0x0080, 0x00e0, 0x0020, 0x00d0, 0x0020, 0x00d0 }

static const naomi_calib_map_t naomi_calib_maps[] = {
	/* 18 Wheeler: American Pro Trucker */
	{ "BBK0", 40, NAOMI_CALIB_BBK_OFFS, NAOMI_CALIB_BBK_DEFS },
	/* Airline Pilots */
	{ "BAE0", 32, NAOMI_CALIB_N1_OFFS, NAOMI_CALIB_N1_DEFS },
	/* Jambo! Safari */
	{ "BAU0", 24, NAOMI_CALIB_BAU_OFFS, NAOMI_CALIB_BAU_DEFS },
	/* Crazy Taxi */
	{ "BAC0", 28, NAOMI_CALIB_BAC_OFFS, NAOMI_CALIB_BAC_DEFS },
	/* Tokyo Bus Guide */
	{ "BCJ0", 32, NAOMI_CALIB_N1_OFFS, NAOMI_CALIB_N1_DEFS },
	/* Alien Front */
	{ "BCQ0", 32, NAOMI_CALIB_N1_OFFS, NAOMI_CALIB_N1_DEFS },
	/* Ferrari F355 Challenge */
	{ "BAM0", 32, NAOMI_CALIB_N1_OFFS, NAOMI_CALIB_N1_DEFS },
	/* Ring Out 4x4 */
	{ "BAF0", 32, NAOMI_CALIB_N1_OFFS, NAOMI_CALIB_N1_DEFS },
	/* Wave Runner GP */
	{ "BDD0", 34, NAOMI_CALIB_N1_OFFS, NAOMI_CALIB_N1_DEFS },
	/* Zombie Revenge */
	{ "BAD0", 20, NAOMI_CALIB_N1_OFFS, NAOMI_CALIB_N1_DEFS },
	/* Super Major League 99 / World Series 99 */
	{ "BAS0", 32, NAOMI_CALIB_N1_OFFS, NAOMI_CALIB_N1_DEFS },
	/* Sega Marine Fishing */
	{ "BBS0", 32, NAOMI_CALIB_N1_OFFS, NAOMI_CALIB_N1_DEFS },
	/* Samba de Amigo */
	{ "BBE0", 32, NAOMI_CALIB_N1_OFFS, NAOMI_CALIB_N1_DEFS },
	/* World Kicks */
	{ "BBM0", 32, NAOMI_CALIB_N1_OFFS, NAOMI_CALIB_N1_DEFS },
	/* Sega Strike Fighter */
	{ "BBZ0", 32, NAOMI_CALIB_N1_OFFS, NAOMI_CALIB_N1_DEFS },
	/* Samba de Amigo ver. 2000 */
	{ "BCP0", 32, NAOMI_CALIB_N1_OFFS, NAOMI_CALIB_N1_DEFS },
	/* Crackin' DJ */
	{ "BCG0", 36, NAOMI_CALIB_N1_OFFS, NAOMI_CALIB_N1_DEFS },
	/* Gun Survivor 2 */
	{ "BDE0", 32, NAOMI_CALIB_N1_OFFS, NAOMI_CALIB_N1_DEFS },
	/* Monkey Ball */
	{ "BDF0", 32, NAOMI_CALIB_N1_OFFS, NAOMI_CALIB_N1_DEFS },
	/* World Series Baseball / Super Major League */
	{ "BDH0", 32, NAOMI_CALIB_N1_OFFS, NAOMI_CALIB_N1_DEFS },
	/* Shakatto Tambourine Cho Powerup Chu */
	{ "BDZ0", 32, NAOMI_CALIB_N1_OFFS, NAOMI_CALIB_N1_DEFS },
	/* Shakatto Tambourine */
	{ "BCS0", 32, NAOMI_CALIB_N1_OFFS, NAOMI_CALIB_N1_DEFS },
	/* Shootout Pool */
	{ "BEV0", 32, NAOMI_CALIB_N1_OFFS, NAOMI_CALIB_N1_DEFS },
	/* Shootout Pool Prize / The Medal */
	{ "BGQ0", 32, NAOMI_CALIB_N1_OFFS, NAOMI_CALIB_N1_DEFS },
	/* The King of Route 66 */
	{ "BEE0", 36, NAOMI_CALIB_N2_OFFS, NAOMI_CALIB_N2_DEFS },
	/* Club Kart: European Session */
	{ "BDB0", 32, NAOMI_CALIB_N1_OFFS, NAOMI_CALIB_N1_DEFS },
	/* Club Kart Prize */
	{ "BGR0", 32, NAOMI_CALIB_N1_OFFS, NAOMI_CALIB_N1_DEFS },
	/* Club Kart: European Session (2003) */
	{ "BHL0", 32, NAOMI_CALIB_N1_OFFS, NAOMI_CALIB_N1_DEFS },
	/* Sega Driving Simulator */
	{ "BAA0", 32, NAOMI_CALIB_N1_OFFS, NAOMI_CALIB_N1_DEFS },
	/* Wild Riders */
	{ "BDG0", 36, NAOMI_CALIB_N2_OFFS, NAOMI_CALIB_N2_DEFS },
	/* Soul Surfer */
	{ "BET0", 36, NAOMI_CALIB_N2_OFFS, NAOMI_CALIB_N2_DEFS },
	/* Initial D Arcade Stage */
	{ "BEM0", 36, NAOMI_CALIB_N2_OFFS, NAOMI_CALIB_N2_DEFS },
	/* Initial D Arcade Stage (Export) */
	{ "BFF0", 36, NAOMI_CALIB_N2_OFFS, NAOMI_CALIB_N2_DEFS },
	/* Initial D Arcade Stage Ver. 2 */
	{ "BFK0", 36, NAOMI_CALIB_N2_OFFS, NAOMI_CALIB_N2_DEFS },
	/* Initial D Arcade Stage Ver. 2 (Export) */
	{ "BFS0", 36, NAOMI_CALIB_N2_OFFS, NAOMI_CALIB_N2_DEFS },
	/* Initial D Arcade Stage Ver. 3 */
	{ "BHH0", 36, NAOMI_CALIB_N2_OFFS, NAOMI_CALIB_N2_DEFS },
	/* Initial D Arcade Stage Ver. 3 (Export) */
	{ "BHR0", 36, NAOMI_CALIB_N2_OFFS, NAOMI_CALIB_N2_DEFS },
};

static const uint8_t naomi_factory_bbk0[] = {
	0x30, 0x20, 0x01, 0x00, 0x31, 0x38, 0x57, 0x48, 0x45, 0x45, 0x4c, 0x45, 0x52, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x22, 0x11, 0x44, 0x33, 0x66, 0x55, 0x00, 0x00, 0x02, 0x00, 0x02, 0x00, 0x01, 0x00,
	0x80, 0x00, 0xce, 0x1f, 0xdf, 0x25, 0x50, 0x50
};

static const uint8_t naomi_factory_bac0[] = {
	0x90, 0x04, 0x10, 0xa3, 0x43, 0x52, 0x41, 0x5a, 0x59, 0x54, 0x41, 0x58, 0x49, 0x20, 0x41, 0x4d,
	0x33, 0x03, 0x03, 0x04, 0x2c, 0xc3, 0x37, 0xce, 0x81, 0x28, 0xe1, 0x00
};

static const uint8_t naomi_factory_bau0[] = {
	0xdd, 0xbe, 0x04, 0x00, 0x18, 0x08, 0x99, 0x19, 0x02, 0x02, 0x02, 0x02, 0x20, 0xd0, 0x20, 0xd0,
	0x80, 0x20, 0xe0, 0x00, 0x00, 0x00, 0x00, 0x00
};

static const uint8_t naomi_factory_bee0[] = {
	0x17, 0x05, 0x01, 0x00, 0x4b, 0x4f, 0x52, 0x36, 0x36, 0x00, 0x76, 0x98, 0x32, 0x54, 0x98, 0x10,
	0x12, 0x4d, 0x01, 0x00, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80, 0x00, 0x80,
	0x00, 0x80, 0x00, 0x00
};

static const struct {
	char game_id[4];
	uint8_t len;
	const uint8_t *data;
} naomi_game_factory[] = {
	{ "BBK0", sizeof(naomi_factory_bbk0), naomi_factory_bbk0 },
	{ "BAC0", sizeof(naomi_factory_bac0), naomi_factory_bac0 },
	{ "BAU0", sizeof(naomi_factory_bau0), naomi_factory_bau0 },
	{ "BEE0", sizeof(naomi_factory_bee0), naomi_factory_bee0 },
};

static void naomi_eeprom_make_path(const char *game_id, char *path, size_t path_size) {
	snprintf(path, path_size, "%s/firmware/eeprom/%.4s.bin", getenv("PATH"), game_id);
}

static bool naomi_eeprom_save_file(const char *game_id, const uint8_t *eeprom) {
	char path[NAME_MAX];
	file_t fd;

	naomi_eeprom_make_path(game_id, path, sizeof(path));
	fd = fs_open(path, O_CREAT | O_TRUNC | O_WRONLY);
	if(fd == FILEHND_INVALID) {
		ds_printf("DS_WARNING: Can't save EEPROM to %s\n", path);
		return false;
	}

	if(fs_write(fd, eeprom, MIE_EEPROM_SIZE) != MIE_EEPROM_SIZE) {
		fs_close(fd);
		ds_printf("DS_WARNING: EEPROM write failed for %s\n", path);
		return false;
	}

	fs_close(fd);
	ds_printf("DS_OK: Saved EEPROM for %.4s\n", game_id);
	return true;
}

static bool naomi_eeprom_load_file(const char *game_id, uint8_t *eeprom) {
	char path[NAME_MAX];
	file_t fd;
	ssize_t n;

	naomi_eeprom_make_path(game_id, path, sizeof(path));
	fd = fs_open(path, O_RDONLY);
	if(fd == FILEHND_INVALID) {
		return false;
	}

	if((size_t)fs_total(fd) != MIE_EEPROM_SIZE) {
		fs_close(fd);
		return false;
	}

	n = fs_read(fd, eeprom, MIE_EEPROM_SIZE);
	fs_close(fd);
	if(n != MIE_EEPROM_SIZE) {
		return false;
	}

	return true;
}

static size_t naomi_eeprom_rom_period(const uint8_t *data, size_t len) {
	size_t period, i;

	for(period = 1; period <= NAOMI_EEPROM_GAME_MAX && period < len; period++) {
		for(i = period; i < len; i++) {
			if(data[i] != data[i % period]) {
				break;
			}
		}
		if(i == len) {
			return period;
		}
	}

	if(len > NAOMI_EEPROM_GAME_MAX) {
		return NAOMI_EEPROM_GAME_MAX;
	}
	return len;
}

static const naomi_calib_map_t *naomi_calib_find(const char *game_id) {
	size_t i;

	for(i = 0; i < sizeof(naomi_calib_maps) / sizeof(naomi_calib_maps[0]); i++) {
		if(!strncmp(game_id, naomi_calib_maps[i].game_id, 4)) {
			return &naomi_calib_maps[i];
		}
	}

	return NULL;
}

static const uint8_t *naomi_game_factory_find(const char *game_id, size_t *len) {
	size_t i;

	for(i = 0; i < sizeof(naomi_game_factory) / sizeof(naomi_game_factory[0]); i++) {
		if(!strncmp(game_id, naomi_game_factory[i].game_id, 4)) {
			*len = naomi_game_factory[i].len;
			return naomi_game_factory[i].data;
		}
	}

	return NULL;
}

static bool naomi_eeprom_patch_calib(uint8_t *eeprom, const char *game_id,
		const uint8_t *rom_defaults, size_t def_len,
		const mie_analog_calib_t *calib) {
	const naomi_calib_map_t *map;
	const uint16_t values[NAOMI_CALIB_AXES] = {
		calib->wheel.min, calib->wheel.center, calib->wheel.max,
		calib->accel.min, calib->accel.max,
		calib->brake.min, calib->brake.max
	};
	uint8_t game_len;
	uint8_t *game;
	size_t i;
	bool patched = false;

	game_len = eeprom[38];
	if(game_len == 0 || game_len == 0xff || game_len > NAOMI_EEPROM_GAME_MAX) {
		return false;
	}

	game = eeprom + 44;

	map = naomi_calib_find(game_id);
	if(map) {
		for(i = 0; i < NAOMI_CALIB_AXES; i++) {
			size_t off = map->axis_off[i];
			uint8_t val;

			if(off >= game_len) {
				continue;
			}

			val = (uint8_t)(values[i] >> 8);
			game[off] = val;
			game[off + game_len] = val;
			patched = true;
		}
		if(!strncmp(game_id, "BBK0", 4) && NAOMI_CALIB_BBK_RIGHT_MIN < game_len) {
			uint8_t center = (uint8_t)(calib->wheel.center >> 8);

			game[NAOMI_CALIB_BBK_RIGHT_MIN] = center;
			game[NAOMI_CALIB_BBK_RIGHT_MIN + game_len] = center;
			patched = true;
		}
	}
	else {
		size_t cmp_len = def_len > game_len ? game_len : def_len;

		if(memcmp(game, rom_defaults, cmp_len) != 0) {
			return false;
		}

		if(def_len >= 2 && def_len <= game_len) {
			int vi = 0;

			for(i = 0; i + 1 < def_len && vi < NAOMI_CALIB_AXES; i++) {
				uint8_t val;

				if(rom_defaults[i] != 0x00 || rom_defaults[i + 1] != 0x80) {
					continue;
				}

				val = (uint8_t)(values[vi++] >> 8);
				game[i + 1] = val;
				game[i + 1 + game_len] = val;
				patched = true;
				i++;
			}
		}
	}

	if(patched) {
		mie_eeprom_fix_crc(eeprom);
	}

	return patched;
}

static size_t naomi_eeprom_game_size(const char *game_id, size_t rom_len) {
	const naomi_calib_map_t *map = naomi_calib_find(game_id);

	if(map && map->game_size > rom_len) {
		return map->game_size;
	}
	return rom_len;
}

static void naomi_eeprom_build_default(const char *game_id, const uint8_t *game_defaults,
		size_t rom_len, uint8_t *out, bool exact_game) {
	uint8_t system[16];
	uint8_t game_buf[NAOMI_EEPROM_GAME_MAX];
	uint16_t crc;
	size_t game_len;
	size_t copy_len;
	size_t game_off;
	size_t i;

	if(exact_game) {
		game_len = rom_len;
	}
	else {
		game_len = naomi_eeprom_game_size(game_id, rom_len);
	}
	memset(game_buf, 0, sizeof(game_buf));
	copy_len = rom_len < game_len ? rom_len : game_len;
	if(copy_len > NAOMI_EEPROM_GAME_MAX) {
		copy_len = NAOMI_EEPROM_GAME_MAX;
	}
	memcpy(game_buf, game_defaults, copy_len);

	if(!exact_game) {
		const naomi_calib_map_t *map = naomi_calib_find(game_id);
		size_t ai;

		if(map) {
			for(ai = 0; ai < NAOMI_CALIB_AXES; ai++) {
				size_t off = map->axis_off[ai];

				if(off < game_len) {
					game_buf[off] = (uint8_t)(map->axis_def[ai] & 0xff);
				}
			}
		}
	}

	memset(out, 0xff, MIE_EEPROM_SIZE);

	system[0] = 0x10; /* attract sound on (upper nibble) */
	memcpy(system + 1, game_id, 4); /* game serial from ROM header @ 0x134 */
	system[5] = 0x09; /* system settings prefix, BIOS default */
	system[6] = 0x10; /* common coin chute, 2-player cabinet */
	system[7] = 0x00; /* coin assignment #1 (zero-based) */
	system[8] = 0x01; /* coin 1 rate (manual coin setting) */
	system[9] = 0x01; /* coin 2 rate (manual coin setting) */
	system[10] = 0x01; /* credit rate (manual coin setting) */
	system[11] = 0x00; /* bonus credit rate (manual coin setting) */
	system[12] = 0x11; /* sequence text offset 1 (packed nibble) */
	system[13] = 0x11; /* sequence text offset 2 (packed nibble) */
	system[14] = 0x11; /* sequence text offset 3 (packed nibble) */
	system[15] = 0x11; /* sequence text offset 4 (packed nibble) */

	crc = mie_eeprom_crc16(system, sizeof(system));
	out[0] = (uint8_t)(crc & 0xff);
	out[1] = (uint8_t)(crc >> 8);
	memcpy(out + 2, system, sizeof(system));
	out[18] = out[0];
	out[19] = out[1];
	memcpy(out + 20, system, sizeof(system));

	if(game_len == 0 || game_len > NAOMI_EEPROM_GAME_MAX) {
		return;
	}

	crc = mie_eeprom_crc16(game_buf, game_len);
	out[36] = (uint8_t)(crc & 0xff);
	out[37] = (uint8_t)(crc >> 8);
	out[38] = (uint8_t)game_len;
	out[39] = (uint8_t)game_len;
	out[40] = out[36];
	out[41] = out[37];
	out[42] = out[38];
	out[43] = out[39];

	game_off = 44;
	memcpy(out + game_off, game_buf, game_len);
	memcpy(out + game_off + game_len, game_buf, game_len);

	for(i = game_off + game_len * 2; i < MIE_EEPROM_SIZE; i++) {
		out[i] = 0xff;
	}
}

static bool naomi_eeprom_read_cart(const char *rom_file, char *game_id,
		uint8_t *rom_defaults, size_t *rom_defaults_len,
		uint8_t *game_blob, size_t *game_blob_len) {
	naomi_cart_header_t cart_hdr;
	const uint8_t *factory;
	file_t fd;
	size_t period;
	size_t factory_len = 0;

	*game_blob_len = 0;

	fd = fs_open(rom_file, O_RDONLY);
	if(fd == FILEHND_INVALID) {
		ds_printf("DS_ERROR: Can't open NAOMI ROM %s\n", rom_file);
		return false;
	}

	if(fs_read(fd, &cart_hdr, sizeof(cart_hdr)) != sizeof(cart_hdr)) {
		fs_close(fd);
		ds_printf("DS_ERROR: Can't read NAOMI ROM header\n");
		return false;
	}

	if(!naomi_cart_valid(&cart_hdr)) {
		fs_close(fd);
		ds_printf("DS_ERROR: Invalid NAOMI ROM header\n");
		return false;
	}

	memcpy(game_id, cart_hdr.serial, 4);
	game_id[4] = '\0';

	period = naomi_eeprom_rom_period(cart_hdr.EEPROM_init_val,
			sizeof(cart_hdr.EEPROM_init_val));
	memcpy(rom_defaults, cart_hdr.EEPROM_init_val, period);
	*rom_defaults_len = period;
	fs_close(fd);

	factory = naomi_game_factory_find(game_id, &factory_len);
	if(factory && factory_len > 0 && factory_len <= NAOMI_EEPROM_GAME_MAX) {
		memcpy(game_blob, factory, factory_len);
		*game_blob_len = factory_len;
		ds_printf("DS_OK: Using game EEPROM record for %.4s\n", game_id);
	}

	return true;
}

void isoldr_naomi_eeprom_prepare(isoldr_info_t *info) {
	uint8_t current[MIE_EEPROM_SIZE];
	uint8_t prepared[MIE_EEPROM_SIZE];
	uint8_t rom_defaults[NAOMI_EEPROM_GAME_MAX];
	uint8_t game_blob[NAOMI_EEPROM_GAME_MAX];
	char target_id[5];
	char current_id[5];
	char rom_path[NAME_MAX];
	size_t rom_defaults_len;
	size_t game_blob_len;

	if(mie_port0_mode() != MIE_PORT0_JVS) {
		return;
	}

	snprintf(rom_path, sizeof(rom_path), "/%s%s", info->fs_dev, info->image_file);
	if(!naomi_eeprom_read_cart(rom_path, target_id, rom_defaults, &rom_defaults_len,
			game_blob, &game_blob_len)) {
		return;
	}

	if(!mie_get_eeprom(current)) {
		ds_printf("DS_WARNING: MIE EEPROM read failed\n");
		return;
	}

	if(current[3] == 'B') {
		memcpy(current_id, current + 3, 4);
		current_id[4] = '\0';

		if(!memcmp(current + 3, target_id, 4)) {
			return;
		}

		naomi_eeprom_save_file(current_id, current);
	}

	if(naomi_eeprom_load_file(target_id, prepared)) {
		ds_printf("DS_OK: Restored EEPROM for %.4s\n", target_id);
	}
	else {
		if(game_blob_len > 0) {
			naomi_eeprom_build_default(target_id, game_blob, game_blob_len, prepared, true);
		}
		else {
			naomi_eeprom_build_default(target_id, rom_defaults, rom_defaults_len, prepared, false);
		}

		if(!mie_analog_calib_valid()) {
			ds_printf("DS_WARNING: DS calibration patch skipped for %.4s, calib invalid\n",
					target_id);
		}
		else if(!naomi_eeprom_patch_calib(prepared, target_id, rom_defaults,
				rom_defaults_len, mie_analog_calib_get())) {
			ds_printf("DS_WARNING: DS calibration patch failed for %.4s\n", target_id);
		}
		else {
			ds_printf("DS_OK: Applied DS calibration to %.4s defaults\n", target_id);
		}

		ds_printf("DS_OK: Initialized EEPROM defaults for %.4s\n", target_id);
	}

	if(!mie_set_eeprom(prepared)) {
		ds_printf("DS_ERROR: MIE EEPROM write failed\n");
	}
}
