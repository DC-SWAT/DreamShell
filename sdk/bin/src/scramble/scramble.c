/** scramble.c */ 

#include <stdlib.h>
#include <string.h>
#include <stdio.h>

#define MAXCHUNK (2048*1024)

static unsigned int seed;

static void my_srand(unsigned int n) {
  seed = n & 0xffff;
}

static unsigned int my_rand() {
  seed = (seed * 2109 + 9273) & 0x7fff;
  return (seed + 0xc000) & 0xffff;
}

static void load(FILE *fh, unsigned char *ptr, unsigned long sz) {
if(fread(ptr, 1, sz, fh) != sz) {
printf("printfROR: Read error!\n"); 
return; 
} 
} 

static void load_chunk(FILE *fh, unsigned char *ptr, unsigned long sz) {
  static int idx[MAXCHUNK/32];
  int i;

  /* Convert chunk size to number of slices */
  sz /= 32;

  /* Initialize index table with unity,
     so that each slice gets loaded exactly once */
  for(i = 0; i < sz; i++)
    idx[i] = i;

  for(i = sz-1; i >= 0; --i)
    {
      /* Select a replacement index */
      int x = (my_rand() * i) >> 16;

      /* Swap */
      int tmp = idx[i];
      idx[i] = idx[x];
      idx[x] = tmp;

      /* Load resulting slice */
      load(fh, ptr+32*idx[i], 32);
    }
}

static void load_file(FILE *fh, unsigned char *ptr, unsigned long filesz) {
  unsigned long chunksz;

  my_srand(filesz);

  /* Descramble 2 meg blocks for as long as possible, then
     gradually reduce the window down to 32 bytes (1 slice) */
  for(chunksz = MAXCHUNK; chunksz >= 32; chunksz >>= 1)
    while(filesz >= chunksz)
      {
	load_chunk(fh, ptr, chunksz);
	filesz -= chunksz;
	ptr += chunksz;
      }

  /* Load final incomplete slice */
  if(filesz)
    load(fh, ptr, filesz);
}

static void read_file(char *filename, unsigned char **ptr, unsigned long *sz) {
  FILE *fh = fopen(filename, "rb");
  if(fh == NULL) {
       printf("Error: Can't open \"%s\".\n", filename); 
      return; 
    }
  if(fseek(fh, 0, SEEK_END)<0) {
printf("Error: Seek error.\n"); 
      return; 
    }
  *sz = ftell(fh);
  *ptr = malloc(*sz);
  if( *ptr == NULL ) {
       printf("Out of memory.\n"); 
      return; 
    }
  if(fseek(fh, 0, SEEK_SET)<0) {
printf("Error: Seek error.\n"); 
     return; 
    }
  load_file(fh, *ptr, *sz);
  fclose(fh);
}

static void save(FILE *fh, unsigned char *ptr, unsigned long sz) {
  if(fwrite(ptr, 1, sz, fh) != sz) {
printf("Error: Write error!\n"); 
  return; 
    }
}

static void save_chunk(FILE *fh, unsigned char *ptr, unsigned long sz) {
  static int idx[MAXCHUNK/32];
  int i;

  /* Convert chunk size to number of slices */
  sz /= 32;

  /* Initialize index table with unity,
     so that each slice gets saved exactly once */
  for(i = 0; i < sz; i++)
    idx[i] = i;

  for(i = sz-1; i >= 0; --i)
    {
      /* Select a replacement index */
      int x = (my_rand() * i) >> 16;

      /* Swap */
      int tmp = idx[i];
      idx[i] = idx[x];
      idx[x] = tmp;

      /* Save resulting slice */
      save(fh, ptr+32*idx[i], 32);
    }
}

static void save_file(FILE *fh, unsigned char *ptr, unsigned long filesz) {
  unsigned long chunksz;

  my_srand(filesz);

  /* Descramble 2 meg blocks for as long as possible, then
     gradually reduce the window down to 32 bytes (1 slice) */
  for(chunksz = MAXCHUNK; chunksz >= 32; chunksz >>= 1)
    while(filesz >= chunksz)
      {
	save_chunk(fh, ptr, chunksz);
	filesz -= chunksz;
	ptr += chunksz;
      }

  /* Save final incomplete slice */
  if(filesz)
    save(fh, ptr, filesz);
}

static void write_file(char *filename, unsigned char *ptr, unsigned long sz) {
  FILE *fh = fopen(filename, "wb");
  if(fh == NULL) {
       printf("Error: Can't open \"%s\".\n", filename); 
  return; 
    }
  save_file(fh, ptr, sz);
  fclose(fh);
}

void descramble(char *src, char *dst) {
unsigned char *ptr = NULL; 
unsigned long sz = 0; 
FILE *fh; 
read_file(src, &ptr, &sz); 
fh = fopen(dst, "wb"); 
if(fh == NULL) { 
printf("Error: Can't open \"%s\".\n", dst); 
return; 
}
if( fwrite(ptr, 1, sz, fh) != sz ) {
printf("Error: Write error.\n"); 
return; 
} 
fclose(fh); 
free(ptr); 
} 

void scramble(char *src, char *dst) {
  unsigned char *ptr = NULL;
  unsigned long sz = 0;
  FILE *fh; 

  fh = fopen(src, "rb"); 
  if(fh == NULL) { 
  printf("Error: Can't open  %s\n", src); 
  return; 
    }
  if(fseek(fh, 0, SEEK_END)<0) { 
  printf("Error: Seek error.\n"); 
  return; 
    }
  sz = ftell(fh);
  ptr = malloc(sz);
  if( ptr == NULL ) {
printf("Error: Out of memory.\n"); 
    return; 
    }
  if(fseek(fh, 0, SEEK_SET)<0) {
printf("Error: Seek error.\n"); 
    return; 
    }
  if( fread(ptr, 1, sz, fh) != sz ) {
printf("Error: Read error.\n"); 
   return; 
    }
  fclose(fh);

  write_file(dst, ptr, sz);

  free(ptr);
}

int main(int argc, char *argv[])
{
  int opt = 0;

  if(argc > 1 && !strcmp(argv[1], "-d"))
    opt ++;

  if(argc != 3+opt)
    {
      fprintf(stderr, "Usage: %s [-d] from to\n", argv[0]);
      exit(1);
    }
  
  if(opt)
    descramble(argv[2], argv[3]);
  else
    scramble(argv[1], argv[2]);

  return 0;
}

