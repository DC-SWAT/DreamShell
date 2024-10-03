/*
 * untgz.c -- Display contents and extract files from a gzip'd TAR file
 *
 * written by "Pedro A. Aranda Guti\irrez" <paag@tid.es>
 * adaptation to Unix by Jean-loup Gailly <jloup@gzip.org>
 * various fixes by Cosmin Truta <cosmint@cs.ubbcluj.ro>
 */

#include "ds.h"
#include <time.h>
#include <errno.h>
#include <utime.h>

#include <zlib/zlib.h>
#include "console.h"


/* values used in typeflag field */

#define REGTYPE  '0'            /* regular file */
#define AREGTYPE '\0'           /* regular file */
#define LNKTYPE  '1'            /* link */
#define SYMTYPE  '2'            /* reserved */
#define CHRTYPE  '3'            /* character special */
#define BLKTYPE  '4'            /* block special */
#define DIRTYPE  '5'            /* directory */
#define FIFOTYPE '6'            /* FIFO special */
#define CONTTYPE '7'            /* reserved */

#define BLOCKSIZE 512

struct tar_header
{                               /* byte offset */
  char name[100];               /*   0 */
  char mode[8];                 /* 100 */
  char uid[8];                  /* 108 */
  char gid[8];                  /* 116 */
  char size[12];                /* 124 */
  char mtime[12];               /* 136 */
  char chksum[8];               /* 148 */
  char typeflag;                /* 156 */
  char linkname[100];           /* 157 */
  char magic[6];                /* 257 */
  char version[2];              /* 263 */
  char uname[32];               /* 265 */
  char gname[32];               /* 297 */
  char devmajor[8];             /* 329 */
  char devminor[8];             /* 337 */
  char prefix[155];             /* 345 */
                                /* 500 */
};

union tar_buffer {
  char               buffer[BLOCKSIZE];
  struct tar_header  header;
};

enum { TGZ_EXTRACT, TGZ_LIST, TGZ_INVALID };

static char *TGZfname          OF((const char *));
static void TGZnotfound        OF((const char *));

static int getoct              OF((char *, int));
static char *strtime           OF((time_t *));
static int setfiletime         OF((char *, time_t));
static int ExprMatch           OF((char *, char *));

static int makedir             OF((char *));
static int matchname           OF((int, int, char **, char *));

static void error              OF((const char *));
static int tar                 OF((gzFile, int, int, int, char **));

static void help               OF((int));
//int main                OF((int, char **));

char *prog;

const char *TGZsuffix[] = { "\0", ".tar", ".tar.gz", ".taz", ".tgz", NULL };

/* return the file name of the TGZ archive */
/* or NULL if it does not exist */

static char *TGZfname (const char *arcname)
{
  static char buffer[1024];
  int origlen,i;

  strcpy(buffer,arcname);
  origlen = strlen(buffer);

  for (i=0; TGZsuffix[i]; i++)
    {
       strcpy(buffer+origlen,TGZsuffix[i]);
       if (strcmp(buffer,F_OK) == 0)
         return buffer;
    }
  return NULL;
}


/* error message for the filename */

static void TGZnotfound (const char *arcname)
{
  int i;

  ds_printf("DS_ERROR: Couldn't find \n");
  for (i=0;TGZsuffix[i];i++)
    ds_printf((TGZsuffix[i+1]) ? "%s%s, " : "or %s%s\n", arcname, TGZsuffix[i]);
  //exit(1); 
}


/* convert octal digits to int */
/* on error return -1 */

static int getoct (char *p,int width)
{
  int result = 0;
  char c;

  while (width--)
    {
      c = *p++;
      if (c == 0)
        break;
      if (c == ' ')
        continue;
      if (c < '0' || c > '7')
        return -1;
      result = result * 8 + (c - '0');
    }
  return result;
}


/* convert time_t to string */
/* use the "YYYY/MM/DD hh:mm:ss" format */

static char *strtime (time_t *t)
{
  struct tm   *local;
  static char result[32];

  local = localtime(t);
  sprintf(result, "%4d/%02d/%02d %02d:%02d:%02d",
          local->tm_year+1900, local->tm_mon+1, local->tm_mday,
          local->tm_hour, local->tm_min, local->tm_sec);
  return result;
}


/* set file time */

static int setfiletime (char *fname,time_t ftime) {
//struct utimbuf settime;
//settime.actime = settime.modtime = ftime;
//return utime(fname,&settime);
return 0; 
}


/* regular expression matching */

#define ISSPECIAL(c) (((c) == '*') || ((c) == '/'))

static int ExprMatch (char *string,char *expr)
{
  while (1)
    {
      if (ISSPECIAL(*expr))
        {
          if (*expr == '/')
            {
              if (*string != '\\' && *string != '/')
                return 0;
              string ++; expr++;
            }
          else if (*expr == '*')
            {
              if (*expr ++ == 0)
                return 1;
              while (*++string != *expr)
                if (*string == 0)
                  return 0;
            }
        }
      else
        {
          if (*string != *expr)
            return 0;
          if (*expr++ == 0)
            return 1;
          string++;
        }
    }
}


/* recursive mkdir */
/* abort on ENOENT; ignore other errors like "directory already exists" */
/* return 1 if OK */
/*        0 on error */

static int makedir (char *newdir)
{
  char *buffer = strdup(newdir);
  char *p;
  int  len = strlen(buffer);

  if (len <= 0) {
    free(buffer);
    return 0;
  }
  if (buffer[len-1] == '/') {
    buffer[len-1] = '\0';
  }
  



#ifdef DS_ARCH_DC  
  if (fs_mkdir(buffer) == -1)
    {
      free(buffer);
      return 1;
    }

#elif DS_ARCH_PSP

  if (mkdir(buffer, 0777) == -1)
    {
      free(buffer);
      return 1;
    }

#else

  if (mkdir(buffer) == -1)
    {
      free(buffer);
      return 1;
    }

#endif

  p = buffer+1;
  while (1)
    {
      char hold;

      while(*p && *p != '\\' && *p != '/')
        p++;
      hold = *p;
      *p = 0;
      #ifdef DS_ARCH_DC
      if ((fs_mkdir(buffer) != 0) && (errno == ENOENT))
      #elif DS_ARCH_PSP
      if ((mkdir(buffer, 0777) != 0) && (errno == ENOENT))
      #else
      if ((mkdir(buffer) != 0) && (errno == ENOENT))
      #endif
        {
          ds_printf("DS_ERROR: Couldn't create directory %s\n", buffer); 
          free(buffer);
          return 0;
        }
      if (hold == 0)
        break;
      *p++ = hold;
    }
  free(buffer);
  return 1;
}


static int matchname (int arg,int argc,char **argv,char *fname)
{
  if (arg == argc)      /* no arguments given (untgz tgzarchive) */
    return 1;

  while (arg < argc)
    if (ExprMatch(fname,argv[arg++]))
      return 1;

  return 0; /* ignore this for the moment being */
}


/* tar file list or extract */

static int tar (gzFile in,int action,int arg,int argc,char **argv)
{
  union  tar_buffer buffer;
  int    len;
  int    err;
  int    getheader = 1;
  int    remaining = 0;
  FILE   *outfile = NULL;
  char   fname[BLOCKSIZE];
  int    tarmode;
  time_t tartime;

  if (action == TGZ_LIST)
    ds_printf("    date      time     size                 file\n"
              " ---------- -------- --------- -----------------------------\n");
  while (1)
    {
      len = gzread(in, &buffer, BLOCKSIZE);
      if (len < 0)
        error(gzerror(in, &err));
      /*
       * Always expect complete blocks to process
       * the tar information.
       */
      if (len != BLOCKSIZE)
        {
          action = TGZ_INVALID; /* force error exit */
          remaining = 0;        /* force I/O cleanup */
        }

      /*
       * If we have to get a tar header
       */
      if (getheader == 1)
        {
          /*
           * if we met the end of the tar
           * or the end-of-tar block,
           * we are done
           */
          if ((len == 0) || (buffer.header.name[0] == 0)) break;

          tarmode = getoct(buffer.header.mode,8);
          tartime = (time_t)getoct(buffer.header.mtime,12);
          if (tarmode == -1 || tartime == (time_t)-1)
            {
              buffer.header.name[0] = 0;
              action = TGZ_INVALID;
            }

          strcpy(fname,buffer.header.name);

          switch (buffer.header.typeflag)
            {
            case DIRTYPE:
              if (action == TGZ_LIST)
                ds_printf("DS_INF: %s     <dir> %s\n",strtime(&tartime),fname);
              if (action == TGZ_EXTRACT)
                {
                  makedir(fname);
                  setfiletime(fname,tartime);
                }
              break;
            case REGTYPE:
            case AREGTYPE:
              remaining = getoct(buffer.header.size,12);
              if (remaining == -1)
                {
                  action = TGZ_INVALID;
                  break;
                }
              if (action == TGZ_LIST)
                ds_printf("DS_INF: %s %9d %s\n",strtime(&tartime),remaining,fname);
              else if (action == TGZ_EXTRACT)
                {
                  if (matchname(arg,argc,argv,fname))
                    {
                      outfile = fopen(fname,"wb");
                      if (outfile == NULL) {
                        /* try creating directory */
                        char *p = strrchr(fname, '/');
                        if (p != NULL) {
                          *p = '\0';
                          makedir(fname);
                          *p = '/';
                          outfile = fopen(fname,"wb");
                        }
                      }
                      if (outfile != NULL)
                        ds_printf("DS_PROCESS: Extracting %s\n",fname);
                      else
                        ds_printf("DS_ERROR: Couldn't create %s\n",fname);
                    }
                  else
                    outfile = NULL;
                }
              getheader = 0;
              break;
            default:
              if (action == TGZ_LIST)
                ds_printf("DS_INF: %s     <---> %s\n",strtime(&tartime),fname);
              break;
            }
        }
      else
        {
          unsigned int bytes = (remaining > BLOCKSIZE) ? BLOCKSIZE : remaining;

          if (outfile != NULL)
            {
              if (fwrite(&buffer,sizeof(char),bytes,outfile) != bytes)
                {
                  ds_printf("DS_ERROR: Error writing %s -- skipping\n",fname);
                  fclose(outfile);
                  outfile = NULL;
                  remove(fname);
                }
            }
          remaining -= bytes;
        }

      if (remaining == 0)
        {
          getheader = 1;
          if (outfile != NULL)
            {
              fclose(outfile);
              outfile = NULL;
              if (action != TGZ_INVALID)
                setfiletime(fname,tartime);
            }
        }

      /*
       * Abandon if errors are found
       */
      if (action == TGZ_INVALID)
        {
          ds_printf("DS_ERROR: Broken archive\n");
          break;
        }
    }

  if (gzclose(in) != Z_OK)
    ds_printf("DS_ERROR: failed gzclose\n");

  return 0;
}


/* ============================================================ */

static void help(int exitval)
{
ds_printf("Usage: untgz file.tgz            extract all files\n"
          "       untgz file.tgz fname ...  extract selected files\n"
          "       untgz -l file.tgz         list archive contents\n"); 
          //"       untgz -h                  display this help\n");
  //exit(exitval);
}

static void error(const char *msg)
{
  ds_printf("DS_ERROR: %s\n", msg); 
  //exit(1);
}


/* ============================================================ */



int main(int argc, char *argv[]) {
    int         action = TGZ_EXTRACT;
    int         arg = 1;
    char        *TGZfile;
    gzFile      f;

    prog = strrchr(argv[0],'\\');
    if (prog == NULL)
      {
        prog = strrchr(argv[0],'/');
        if (prog == NULL)
          {
            prog = strrchr(argv[0],':');
            if (prog == NULL)
              prog = argv[0];
            else
              prog++;
          }
        else
          prog++;
      }
    else
      prog++;

    if (argc == 1)
      help(0);

    if (strcmp(argv[arg],"-l") == 0)
      {
        action = TGZ_LIST;
        if (argc == ++arg)
          help(0);
      }
    else if (strcmp(argv[arg],"-h") == 0)
      {
        help(0);
      }

    if ((TGZfile = TGZfname(argv[arg])) == NULL)
      TGZnotfound(argv[arg]);

    ++arg;
    if ((action == TGZ_LIST) && (arg != argc))
      help(1);

/*
 *  Process the TGZ file
 */
    switch(action)
      {
      case TGZ_LIST:
      case TGZ_EXTRACT:
        f = gzopen(TGZfile,"rb");
        if (f == NULL)
          {
            ds_printf("DS_ERROR: Couldn't gzopen %s\n",TGZfile); 
            return CMD_ERROR;
          }
        tar(f, action, arg, argc, argv); 
      break;

      default:
        ds_printf("DS_ERROR: Unknown option\n");
        return CMD_ERROR;
      }

    return CMD_OK;
}
