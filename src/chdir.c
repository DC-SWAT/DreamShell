/* DreamShell ##version## 
 * chdir.c
 * (c) 2004-2014 SWAT 
 */
#include "ds.h"


/* read a pathname based on the current directory and turn it into an abs one, we
   don't check for validity, or really do anything except handle ..'s and .'s */

int makeabspath_wd(char *buff, char *path, char *dir, size_t size) {

	int numtxts;
	int i;
	char *txts[32];		/* max of 32...should be nuff */
	char *rslash;

	numtxts = 0;

	/* check if path is already absolute */
	if (path[0] == '/') {
		strncpy(buff, path, size);
		return 0;
	}

	/* split the path into tokens */
	for (numtxts = 0; numtxts < 32;) {
		if ((txts[numtxts] = strsep(&path, "/")) == NULL)
			break;
		if (*txts[numtxts] != '\0')
			numtxts++;
	}

	/* start from the current directory */
	strncpy(buff, dir, size);

	for (i = 0; i < numtxts; i++) {
		if (strcmp(txts[i], "..") == 0) {
			if ((rslash = strrchr(buff, '/')) != NULL)
				*rslash = '\0';
		} else if (strcmp(txts[i], ".") == 0) {
			/* do nothing */
		} else {
			if (buff[strlen(buff) - 1] != '/')
				strncat(buff, "/", size - 1 - strlen(buff));
			strncat(buff, txts[i], size - 1 - strlen(buff));
		}
	}

	/* make sure it's not empty */
	if (buff[0] == '\0') {
		buff[0] = '/';
		buff[1] = '\0';
	}

	return 0;
}


// Deprecated
int makeabspath(char *buff, char *path, size_t size) {
    //return makeabspath_wd(buff, path, cwd, size);
	realpath(path, buff);
	return 1;
}



const char *relativeFilePath(const char *rel, const char *file) {
    
    if(file[0] == '/') return file;
    
    char *rslash, *dir, *ret;
    char fn[NAME_MAX];
    char buff[NAME_MAX];

    if((rslash = strrchr(rel, '/')) != NULL) {
               
        strncpy(buff, file, NAME_MAX);
        dir = substring(rel, 0, strlen(rel) - strlen(rslash));
        makeabspath_wd(fn, buff, dir, NAME_MAX);
        
        fn[strlen(fn)] = '\0';
        ret = strdup(fn);
        //ds_printf("Directory: '%s' File: '%s' Out: '%s'", dir, rslash, ret, fn);
        free(dir);
        
    } else {
        return file;
    }
    
    return ret;
}



int relativeFilePath_wb(char *buff, const char *rel, const char *file) {
    
    if(file[0] == '/') {
       strncpy(buff, file, NAME_MAX);
       return 1;
    }
/*    
    char *rslash, *dir;

    if((rslash = strrchr(rel, '/')) != NULL) {
        dir = substring(rel, 0, strlen(rel) - strlen(rslash));
        makeabspath_wd(buff, file, dir, NAME_MAX);
        ds_printf("Directory: '%s' File: '%s' Out: '%s'", dir, rel, buff);
*/

    makeabspath_wd(buff, (char*)file, getFilePath(rel), NAME_MAX);
    //ds_printf("Directory: '%s' File: '%s' Out: '%s'", path, rel, buff);
    return 1;
}



char *getFilePath(const char *file) {
     
    char *rslash;
    
    if((rslash = strrchr(file, '/')) != NULL) {
        return substring(file, 0, strlen(file) - strlen(rslash));
    }
    
    return NULL;
}

/* change the current directory (dir is an absolute path for now) */
// Deprecated
int ds_chdir(char *dir) {
	return fs_chdir(dir);
}


