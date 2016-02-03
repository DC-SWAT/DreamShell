
#ifndef OPK_H
#define OPK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdlib.h>

struct OPK;

struct OPK *opk_open(const char *opk_filename);
void opk_close(struct OPK *opk);

/* Open the next meta-data file.
 * if 'filename' is set, the pointer passed as argument will
 * be set to point to a string corresponding to the filename.
 * XXX: the pointer will be invalid as soon as opk_close() is called.
 *
 * Returns:
 *  -EIO if the file cannot be read,
 *  -ENOMEM if the buffer cannot be allocated,
 *  0 if no more meta-data file can be found,
 *  1 otherwise.
 */
int opk_open_metadata(struct OPK *opk, const char **filename);

/* Read a key/value pair.
 * 'key_chars' and 'val_chars' must be valid pointers. The pointers passed
 * as arguments will point to the key and value read. 'key_size' and
 * 'val_size' are set to the length of their respective char arrays.
 * XXX: the pointers will be invalid as soon as opk_close() is called.
 *
 * Returns:
 *  -EIO if the file cannot be read,
 *  0 if no more key/value pairs can be found,
 *  1 otherwise.
 */
int opk_read_pair(struct OPK *opk,
		const char **key_chars, size_t *key_size,
		const char **val_chars, size_t *val_size);

/* Extract the file with the given filename from the OPK archive.
 * The 'data' pointer is set to an allocated buffer containing
 * the file's content. The 'size' variable is set to the length
 * of the buffer, in bytes.
 *
 * Returns:
 *  -ENOENT if the file to extract is not found,
 *  -ENOMEM if the buffer cannot be allocated,
 *  -EIO if the file cannot be read,
 *  0 otherwise.
 */
int opk_extract_file(struct OPK *opk,
			const char *name, void **data, size_t *size);

#ifdef __cplusplus
}
#endif

#endif /* OPK_H */
