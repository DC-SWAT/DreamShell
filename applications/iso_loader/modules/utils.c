/* DreamShell ##version##

   utils.c - ISO Loader app utils
   Copyright (C) 2022 SWAT

*/

#include "ds.h"
#include "isoldr.h"
#include "utils.h"

/* Trim begin/end spaces and copy into output buffer */
void trim_spaces(char *input, char *output, int size) {
	char *p;
	char *o;

	p = input;
	o = output;
	
	while(*p == ' ' && input + size > p) {
		p++;
	}

	while(input + size > p) { 
		*o = *p;
		p++; 
		o++; 
	}
	
	*o = '\0';
	o--;

	while(*o == ' ' && o > output) {
		*o='\0';
		o--;
	}
}

char *trim_spaces2(char *txt)
{
	int32_t i;
	
	while(txt[0] == ' ')
	{
		txt++;
	}
	
	int32_t len = strlen(txt);
	
	for(i=len; i ; i--)
	{
		if(txt[i] > ' ') break;
		txt[i] = '\0';
	}
	
	return txt;
}

char *fix_spaces(char *str)
{
	if(!str) return NULL;
	
	int i, len = (int) strlen(str);
	
	for(i=0; i<len; i++)
	{
		if(str[i] == ' ') str[i] = '\\';
	}
	
	return str;
}

int conf_parse(isoldr_conf *cfg, const char *filename)
{
	file_t fd;
	int i;
	
	fd = fs_open(filename, O_RDONLY);
	
	if(fd == FILEHND_INVALID) 
	{
		ds_printf("DS_ERROR: Can't open %s\n", filename);
		return -1;
	}
	
	size_t size = fs_total(fd);
	
	char buf[512];
	char *optname = NULL, *value = NULL;
	
	if(fs_read(fd, buf, size) != size)
	{
		fs_close(fd);
		ds_printf("DS_ERROR: Can't read %s\n", filename);
		return -1;
	}
	
	fs_close(fd);
	
	while(1)
	{
		if(optname == NULL)
			optname = strtok(buf, "=");
		else
			optname = strtok('\0', "=");
		
		value = strtok('\0', "\n");
		
		if(optname == NULL || value == NULL) break;
		
		for(i=0; cfg[i].conf_type; i++)
		{
			if(strncasecmp(cfg[i].name, trim_spaces2(optname), 32)) continue;
			
			switch(cfg[i].conf_type)
			{
				case CONF_INT:
					*(int *)cfg[i].pointer = atoi(value);
					break;
					
				case CONF_STR:
					strcpy((char *)cfg[i].pointer, trim_spaces2(value));
					break;
			}
			break;
		}
	}
	
	return 0;
}

int getDeviceType(const char *dir) {

	if(!strncasecmp(dir, "/cd", 3)) {
		return APP_DEVICE_CD;
	} else if(!strncasecmp(dir, "/sd",   3)) {
		return APP_DEVICE_SD;
	} else if(!strncasecmp(dir, "/ide",  4)) {
		return APP_DEVICE_IDE;
	} else if(!strncasecmp(dir, "/pc",   3)) {
		return APP_DEVICE_PC;
//	} else if(!strncasecmp(dir, "/???", 5)) {
//		return APP_DEVICE_NET;
	} else {
		return -1;
	}
}

int checkGDI(char *filepath, const char *fmPath, char *dirname, char *filename) {
	memset(filepath, 0, MAX_FN_LEN);
	snprintf(filepath, MAX_FN_LEN, "%s/%s/%s.gdi", fmPath, dirname, filename);

	if(FileExists(filepath)) {
		memset(filepath, 0, MAX_FN_LEN);
		snprintf(filepath, MAX_FN_LEN, "%s/%s.gdi", dirname, filename);
		return 1;
	}
	
	return 0;
}
