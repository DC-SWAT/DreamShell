/** 
 * mkbios.c 
 * Copyright (c) 2022 SWAT
 */

#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>
#include <unistd.h>

#define BIOS_OFFSET 65536

int main(int argc, char *argv[]) {

    if (argc < 3) {
        printf("BIOS maker v0.1 by SWAT\n");
        printf("Usage: %s file.bios program.bin", argv[0]);
        return 0;
    }

    FILE *fr, *fw;

    fw = fopen(argv[1], "r+b");

    if (!fw) {
        printf("Can't open for read: %s\n", argv[1]);
        return -1;
    }

    fr = fopen(argv[2], "r");

    if (!fr) {
        printf("Can't open for read: %s\n", argv[2]);
        fclose(fw);
        return -1;
    }

    fseek(fr, 0L, SEEK_END);
    size_t size = ftell(fr);
    fseek(fr, 0L, SEEK_SET);

    uint8_t *buff = (uint8_t *) malloc(size);

    if (!buff) {
        printf("Malloc failed for %zu bytes\n", size);
        fclose(fr);
        fclose(fw);
        return -1;
    }

    printf("Writing to %s by offset %d size %zu\n", argv[1], BIOS_OFFSET, size);

    fread(buff, sizeof(char), size, fr);
    fclose(fr);

    fseek(fw, BIOS_OFFSET, SEEK_SET);
    fwrite(buff, sizeof(char), size, fw);
    fclose(fw);

    free(buff);
    return 0;
}
