#include <stdio.h>
#include <stdlib.h>
#include "buffer.h"


int BufWrite (char *data, long data_size, struct buffer_s *buffer)
{
     long write_length;

     if (data_size > (buffer->size + (buffer->size - buffer->index - 1)))
        return 0;  // unimplemented

     if (buffer->index + data_size < buffer->size)  // 1 menos
     {
        memcpy ((buffer->ptr + buffer->index), data, data_size);
        buffer->index += data_size;
     }
     else
     {
         write_length = buffer->size - buffer->index;
         memcpy ((buffer->ptr + buffer->index), data, write_length);
         fwrite (buffer->ptr, buffer->size, 1, buffer->file);
         memcpy (buffer->ptr, data + write_length, data_size - write_length);
         buffer->index = data_size - write_length;
     }

return 1;
}

int BufWriteFlush (struct buffer_s *buffer)
{

     fwrite (buffer->ptr, buffer->index, 1, buffer->file);
     buffer->index = 0;

return 1;
}


int BufRead (char *data, long data_size, struct buffer_s *buffer, long filesize)
{
     long read_length, max_length, pos;

     if (data_size > (buffer->size + (buffer->size - buffer->index - 1)))
        return 0;  // unimplemented

     if (filesize == 0)  // no cuenta
     {
        max_length = buffer->size;
     }
     else
     {
        pos = ftell(buffer->file);
        if (pos > filesize) max_length = 0;
        else max_length = ((pos + buffer->size) > filesize) ? (filesize - pos) : buffer->size;
     }

     if (buffer->index == 0)
     {
        fread(buffer->ptr, max_length, 1, buffer->file);
     }

     if (buffer->index + data_size <= buffer->size)
     {
        memcpy (data, buffer->ptr + buffer->index, data_size);
        buffer->index += data_size;
        if (buffer->index >= buffer->size) buffer->index = 0;
     }
     else
     {
         read_length = buffer->size - buffer->index;
         memcpy (data, buffer->ptr + buffer->index, read_length);
         fread (buffer->ptr, max_length, 1, buffer->file);
         memcpy (data + read_length, buffer->ptr, data_size - read_length);
         buffer->index = data_size - read_length;
     }

return 1;
}

