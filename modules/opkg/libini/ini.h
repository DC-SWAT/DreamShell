#ifndef __INI_H
#define __INI_H

#include <stdlib.h>

struct INI;

struct INI *ini_open(const char *file);
struct INI *ini_open_mem(const char *buf, size_t len);

void ini_close(struct INI *ini);

/* Jump to the next section.
 * if 'name' is set, the pointer passed as argument
 * points to the name of the section. 'name_len' is set to the length
 * of the char array.
 * XXX: the pointer will be invalid as soon as ini_close() is called.
 *
 * Returns:
 * 	-EIO if an error occured while reading the file,
 * 	0 if no more section can be found,
 * 	1 otherwise.
 */
int ini_next_section(struct INI *ini, const char **name, size_t *name_len);

/* Read a key/value pair.
 * 'key' and 'value' must be valid pointers. The pointers passed as arguments
 * will point to the key and value read. 'key_len' and 'value_len' are
 * set to the length of their respective char arrays.
 * XXX: the pointers will be invalid as soon as ini_close() is called.
 *
 * Returns:
 *  -EIO if an error occured while reading the file,
 *  0 if no more key/value pairs can be found,
 *  1 otherwise.
 */
int ini_read_pair(struct INI *ini,
			const char **key, size_t *key_len,
			const char **value, size_t *value_len);

#endif
