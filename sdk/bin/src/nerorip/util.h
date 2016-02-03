/*
 * This file is part of nerorip. (c)2011 Joe Balough
 *
 * Nerorip is free software: you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * Nerorip is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with nerorip.  If not, see <http://www.gnu.org/licenses/>.
 *
 */
#ifndef UTIL_H
#define UTIL_H

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h> // strerror()
#include <stdint.h> // uintXX_t types
#include <byteswap.h> // for bswap_XX functions
#include <stdarg.h> // for printf wrapper

#define WEBSITE "https://github.com/scallopedllama/nerorip"
#define VERSION "0.4"


/**
 * Increments verbosity one tick
 * @author Joe Balough
 */
void inc_verbosity();

/**
 * Decrements verbosity one tick
 * @author Joe Balough
 */
void dec_verbosity();

/**
 * Returns the current verbosity
 * @author Joe Balough
 */
inline int get_verbosity();


/**
 * printf() wrapper
 *
 * Only prints when verbose enabled.
 * @param int verbosity
 *   Only prints the message if global verbosity is >= passed verbosity
 * @param char *format, ...
 *   What you'd normally pass off to printf
 * @return int
 *   Total number of characters written (0 if verbosity not met)
 * @author Joe Balough
 */
int ver_printf(int verbosity, char *format, ...);


/**
 * File reading convenience functions
 *
 * These functions read an unsigned integer of the indicated size from the passed file.
 * Before returning, convert the number from little endian to big endian.
 * If the read failed, it will return 0.
 *
 * @param FILE*
 *   A file pointer to the file from which the value should be read.
 * @return mixed
 *   Returns the byteswapped value read
 * @author Joe Balough
 */
uint8_t fread8u(FILE*);
uint16_t fread16u(FILE*);
uint32_t fread32u(FILE*);
uint64_t fread64u(FILE*);


size_t fwrite16u(uint16_t value, FILE* output);
size_t fwrite32u(uint32_t value, FILE* output);

/**
 * Writes a wav header to the passed file
 * Based on the file specification found at https://ccrma.stanford.edu/courses/422/projects/WaveFormat/
 *
 * @param FILE* output_file
 *   The file to which the header should be written
 * @param unsigned int length
 *   The length of the audio data that will be in the file
 * @author Joe Balough
 */
void fwrite_wav_header(FILE *output_file, unsigned int length);

/**
 * Writes an aiff header to the passed file
 * Based on the writeaiffheader() function in the cidrip project.
 *
 * @param FILE* output_file
 *   The file to which the header should be written
 * @param unsigned int length
 *   The length of the audio data that will be in the file in sectors
 * @author Joe Balough, DeXT/Lawrence Williams
 */
void fwrite_aiff_header(FILE *output_file, unsigned int sectors_length);

/**
 * "Swaps" the data passed in the buffer
 * Essentially, it takes the first two bytes, swaps them,
 * then goes to the next two bytes and repeats until done with the buffer.
 *
 * @param uint8_t *buffer
 *   The buffer of data to swap
 * @param unsigned int length
 *   The length of the buffer to swap
 * @author Joe Balough
 */
void swap_buffer(uint8_t *buffer, unsigned int length);

#endif
