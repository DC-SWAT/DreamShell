#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#ifdef _WIN32
#include <windows.h>
#endif
#include "common.h"

void error_exit(long errcode, char *string)
{

char errmessage[256];

#ifdef _WIN32
HWND hWnd = NULL;
#endif

    if (errcode != ERR_GENERIC)
       printf("\n%s: %s\n", string, strerror(errno));  // string is used as Filename

    switch(errcode)
          {
          case ERR_OPENIMAGE:
                   strcpy(errmessage, "File not found\n\nYou may have typed a wrong name or path to source CDI image");
                   break;
          case ERR_SAVETRACK:
                   strcpy(errmessage, "Could not save track");
                   break;
          case ERR_READIMAGE:
                   strcpy(errmessage, "Error reading image");
                   break;
          case ERR_PATH:
                   strcpy(errmessage, "Could not find destination path");
                   break;
          case ERR_GENERIC:
          default: strcpy(errmessage, string);          // string is used as Error message
          }

#ifdef _WIN32
    MessageBoxA(hWnd, errmessage, NULL, MB_OK | MB_ICONERROR);
    ExitProcess(0);
#else
    printf(errmessage);
    exit(EXIT_FAILURE);
#endif

}
