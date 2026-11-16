/*
 * g1sum — HOLLY G1 BIOS checksum.
 *
 * Early-key rule from:
 * https://github.com/mathieulh/HOLLY-G1-CHECKSUM
 *
 * The first size-2 bytes are summed. The stored little-endian word must
 * match that sum, and the fold must be 0 on both sides of the stored 00 00.
 *
 * Copyright (C) 2026 SWAT
 * http://www.dc-swat.ru
 */

#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define DEFAULT_CHECKED_SIZE 0x400
#define STORED_LEN 2
#define PATCH_LEN 8
#define FOLD_BIT 0x1104
#define SPREAD_MAX 0x5555
#define TARGET 0x0000
#define MASK_16 0xFFFF
#define WRAP 0x10000
#define BYTE_COUNT 256
#define WRAPS_WHEN_FOLD_CLEAR 2
#define WRAPS_WHEN_FOLD_SET 1
#define INVERSE_NONE (-1)

static uint16_t spreads[BYTE_COUNT];
static int inverse[WRAP];
static uint8_t by_spread[BYTE_COUNT];

static uint16_t spread_byte(unsigned value) {
	value &= 0xFF;
	value = (value | (value << 4)) & 0x0F0F;
	value = (value | (value << 2)) & 0x3333;
	value = (value | (value << 1)) & 0x5555;
	return (uint16_t)value;
}

static void sort_by_spread(void) {
	int i;
	int j;

	for (i = 1; i < BYTE_COUNT; i++) {
		uint8_t byte = by_spread[i];

		j = i;
		while (j > 0 && spreads[by_spread[j - 1]] < spreads[byte]) {
			by_spread[j] = by_spread[j - 1];
			j--;
		}
		by_spread[j] = byte;
	}
}

static void init_tables(void) {
	int byte;

	for (byte = 0; byte < WRAP; byte++)
		inverse[byte] = INVERSE_NONE;

	for (byte = 0; byte < BYTE_COUNT; byte++) {
		spreads[byte] = spread_byte((unsigned)byte);
		inverse[spreads[byte]] = byte;
		by_spread[byte] = (uint8_t)byte;
	}
	sort_by_spread();
}

static void step(int *acc, int *fold, int byte) {
	*acc += spreads[byte] ^ *fold;
	if (*acc >= WRAP) {
		*acc &= MASK_16;
		*fold ^= FOLD_BIT;
	}
}

static void checksum(const uint8_t *data, int length, int *acc, int *fold) {
	int i;

	*acc = 0;
	*fold = 0;
	for (i = 0; i < length; i++)
		step(acc, fold, data[i]);
}

static int stored_value(const uint8_t *data, int checked) {
	return data[checked - 2] | (data[checked - 1] << 8);
}

static int passes(const uint8_t *data, int checked) {
	int acc;
	int fold;

	checksum(data, checked - STORED_LEN, &acc, &fold);
	return acc == stored_value(data, checked) && fold == 0;
}

static int solve_spreads(int need, int count, uint8_t *out) {
	int i;
	int min_spread;

	if (need < 0 || need > count * SPREAD_MAX)
		return 0;
	if (count == 1) {
		if (need >= WRAP || inverse[need] == INVERSE_NONE)
			return 0;
		out[0] = (uint8_t)inverse[need];
		return 1;
	}

	min_spread = need - (count - 1) * SPREAD_MAX;
	if (min_spread < 0)
		min_spread = 0;

	for (i = 0; i < BYTE_COUNT; i++) {
		uint8_t byte = by_spread[i];
		int value = spreads[byte];

		if (value < min_spread)
			break;
		if (value > need)
			continue;
		out[0] = byte;
		if (solve_spreads(need - value, count - 1, out + 1))
			return 1;
	}
	return 0;
}

static int encode(const uint8_t *raw, int count, int acc, int fold,
	uint8_t *out, int *got, int *got_fold) {
	int i;

	for (i = 0; i < count; i++) {
		int spread_value = spreads[raw[i]] ^ fold;
		int byte;

		if (spread_value >= WRAP || inverse[spread_value] == INVERSE_NONE)
			return 0;
		byte = inverse[spread_value];
		out[i] = (uint8_t)byte;
		step(&acc, &fold, byte);
	}
	*got = acc;
	*got_fold = fold;
	return 1;
}

static int needed_sum(int acc, int fold) {
	int wraps;

	if (fold == 0)
		wraps = WRAPS_WHEN_FOLD_CLEAR;
	else
		wraps = WRAPS_WHEN_FOLD_SET;
	return wraps * WRAP - acc;
}

static int find_patch(int acc, int fold, int room, uint8_t *patch, int *length) {
	int need;
	int count;

	if (acc == TARGET && fold == 0) {
		*length = 0;
		return 1;
	}

	need = needed_sum(acc, fold);
	for (count = 1; count <= room; count++) {
		uint8_t raw[PATCH_LEN];
		uint8_t encoded[PATCH_LEN];
		int got;
		int got_fold;

		if (!solve_spreads(need, count, raw))
			continue;
		if (!encode(raw, count, acc, fold, encoded, &got, &got_fold))
			continue;
		if (got == TARGET && got_fold == 0) {
			memcpy(patch, encoded, (size_t)count);
			*length = count;
			return 1;
		}
	}
	return 0;
}

static int patch_room(int checked) {
	int room;

	room = checked - STORED_LEN;
	if (room > PATCH_LEN)
		return PATCH_LEN;
	return room;
}

static void apply_patch(uint8_t *data, int checked, int room,
	const uint8_t *patch, int length) {
	int area = checked - STORED_LEN;
	int start = area - length;
	int i;

	for (i = 0; i < room; i++)
		data[area - room + i] = 0;
	for (i = 0; i < length; i++)
		data[start + i] = patch[i];
	data[area] = TARGET & 0xFF;
	data[area + 1] = (TARGET >> 8) & 0xFF;
}

static uint8_t *read_image(const char *path, size_t *size) {
	FILE *file;
	long length;
	uint8_t *data;

	file = fopen(path, "rb");
	if (file == NULL)
		return NULL;
	if (fseek(file, 0, SEEK_END) != 0) {
		fclose(file);
		return NULL;
	}
	length = ftell(file);
	if (length < 0) {
		fclose(file);
		return NULL;
	}
	if (fseek(file, 0, SEEK_SET) != 0) {
		fclose(file);
		return NULL;
	}
	data = malloc((size_t)length);
	if (data == NULL) {
		fclose(file);
		return NULL;
	}
	if (length > 0 && fread(data, 1, (size_t)length, file) != (size_t)length) {
		free(data);
		fclose(file);
		return NULL;
	}
	fclose(file);
	*size = (size_t)length;
	return data;
}

static int write_image(const char *path, const uint8_t *data, size_t size) {
	FILE *file;

	file = fopen(path, "wb");
	if (file == NULL)
		return 0;
	if (fwrite(data, 1, size, file) != size) {
		fclose(file);
		return 0;
	}
	if (fclose(file) != 0)
		return 0;
	return 1;
}

static void usage(void) {
	fprintf(stderr, "usage: g1sum [-s size] image\n");
}

static int parse_size(const char *text, int *size) {
	char *end;
	long value;

	if (text[0] == '\0')
		return 0;
	errno = 0;
	value = strtol(text, &end, 0);
	if (end == text || *end != '\0' || errno != 0)
		return 0;
	if (value < STORED_LEN || value > INT_MAX)
		return 0;
	*size = (int)value;
	return 1;
}

int main(int argc, char **argv) {
	int checked = DEFAULT_CHECKED_SIZE;
	const char *path = NULL;
	uint8_t *data;
	size_t size;
	int room;
	int area;
	int acc;
	int fold;
	uint8_t patch[PATCH_LEN];
	int length;
	int i;

	for (i = 1; i < argc; i++) {
		if (strcmp(argv[i], "-s") == 0) {
			if (i + 1 >= argc || !parse_size(argv[i + 1], &checked)) {
				fprintf(stderr, "g1sum: bad size\n");
				usage();
				return 2;
			}
			i++;
			continue;
		}
		if (argv[i][0] == '-') {
			usage();
			return 2;
		}
		if (path != NULL) {
			usage();
			return 2;
		}
		path = argv[i];
	}
	if (path == NULL) {
		usage();
		return 2;
	}

	data = read_image(path, &size);
	if (data == NULL) {
		fprintf(stderr, "G1 checksum: cannot read %s\n", path);
		return 1;
	}
	if (size < (size_t)checked) {
		fprintf(stderr, "G1 checksum: image is shorter than 0x%X\n", checked);
		free(data);
		return 1;
	}

	init_tables();
	if (passes(data, checked)) {
		printf("G1 checksum OK\n");
		free(data);
		return 0;
	}

	room = patch_room(checked);
	area = checked - STORED_LEN;
	for (i = 0; i < room; i++)
		data[area - room + i] = 0;
	checksum(data, area, &acc, &fold);
	if (!find_patch(acc, fold, room, patch, &length)) {
		fprintf(stderr, "G1 checksum: no patch in the last %d bytes\n", room);
		free(data);
		return 1;
	}

	apply_patch(data, checked, room, patch, length);
	if (!passes(data, checked)) {
		fprintf(stderr, "G1 checksum: patch did not verify\n");
		free(data);
		return 1;
	}
	if (!write_image(path, data, size)) {
		fprintf(stderr, "G1 checksum: cannot write %s\n", path);
		free(data);
		return 1;
	}

	printf("G1 checksum patched at 0x%X:", area - length);
	for (i = 0; i < length; i++)
		printf(" %02X", patch[i]);
	printf("\n");
	free(data);
	return 0;
}
