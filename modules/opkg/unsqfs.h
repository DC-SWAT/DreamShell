#ifndef PRIVATE_H
#define PRIVATE_H

struct PkgData;

struct PkgData *opk_sqfs_open(const char *image_name);
void opk_sqfs_close(struct PkgData *pdata);
int opk_sqfs_extract_file(struct PkgData *pdata, const char *name,
		void **out_data, size_t *out_size);
int opk_sqfs_get_metadata(struct PkgData *pdata, const char **filename);

#endif
