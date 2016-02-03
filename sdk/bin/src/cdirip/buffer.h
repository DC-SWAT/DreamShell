#ifndef __BUFFER_H__
#define __BUFFER_H__

struct buffer_s {
       FILE *file;
       char *ptr;
       long index;
       long size;
       };

int BufRead (char *data, long size, struct buffer_s *buffer, long filesize);
int BufWrite (char *data, long size, struct buffer_s *buffer);
int BufWriteFlush (struct buffer_s *buffer);

#endif

