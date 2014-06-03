/* DreamShell ##version##

   module.c - opkg module
   Copyright (C)2013-2014 SWAT 
*/          

#include "ds.h"
#include "opk.h"


DEFAULT_MODULE_EXPORTS_CMD(opkg, "OPKG Package Manager");


static int display_info(char *package_path) {
	
	struct OPK *opk;
	const char *key, *val;
	const char *metadata_name;
	size_t skey, sval;
	
	opk = opk_open(package_path);
	//EXPT_GUARD_ASSIGN(opk, opk_open(package_path), opk = NULL);

	if (!opk) {
		ds_printf("Failed to open %s\n", package_path);
		return CMD_ERROR;
	}
	
	ds_printf("=== %s\n", package_path);

	while (opk_open_metadata(opk, &metadata_name) > 0) {
		
		ds_printf("\nMetadata file: %s\n\n", metadata_name);
		
		while(opk_read_pair(opk, &key, &skey, &val, &sval) && key)
			ds_printf(" %.*s: %.*s\n", (int) skey, key, (int) sval, val);
	}

	opk_close(opk);
	ds_printf("\n");
	
	return CMD_OK;
}


static int extract_file_to(char *package_path, char *extract_file, char *dest) {

	struct OPK *opk;
	file_t fd;
	void *data;
	size_t size;

	opk = opk_open(package_path);

	if(opk == NULL) {
		ds_printf("DS_ERROR: Failed to open %s\n", package_path);
		return CMD_ERROR;
	}

	ds_printf("DS_PROCESS: Extracting %s ...\n", extract_file);

	if(opk_extract_file(opk, extract_file, &data, &size) < 0) {
		opk_close(opk);
		return CMD_ERROR;
	}

	fd = fs_open(dest, O_WRONLY | O_CREAT);

	if(fd < 0) {
		ds_printf("DS_ERROR: Can't open %s for write extracted file\n", dest);
		free(data);
		opk_close(opk);
		return CMD_ERROR;
	}

	fs_write(fd, data, size);
	fs_close(fd);

	free(data);
	opk_close(opk);
	return CMD_OK;
}


static int install_package(char *package_path) {

	struct OPK *opk;
	file_t fd;
	void *data;
	size_t size;
	const char *key, *val;
	const char *metadata_name;
	char dest[MAX_FN_LEN], lua_file[128];
	char *file_list, *file, *filepath, *tmpval;
	size_t skey, sval;

	opk = opk_open(package_path);

	if(opk == NULL) {
		ds_printf("DS_ERROR: Failed to open %s\n", package_path);
		return CMD_ERROR;
	}
	
	while (opk_open_metadata(opk, &metadata_name) > 0) {
		
		file_list = NULL;
		memset(lua_file, 0, sizeof(lua_file));
		
		while(opk_read_pair(opk, &key, &skey, &val, &sval) && key) {
			
			if(!strncasecmp(key, "DS-Files", 8) || !strncasecmp(key, "DS-Data", 7)) {
				
				file_list = strndup((char *)val, sval);
				
			} else if(!strncasecmp(key, "Name", 4)) {
				
				ds_printf("DS_PROCESS: Installing %.*s package...\n", (int)sval, val);
				
			} else if(!strncasecmp(key, "DS-InstallScript", 16) && sval) {
				
				strncpy(lua_file, val, sval);
			}
		}
		
		if(file_list != NULL) {
			
			tmpval = file_list;
			
			while((filepath = strsep(&tmpval, " ;\t\n")) != NULL) {

				file = strrchr(filepath, '/');
				if(!file) file = filepath;
				else file++;
				
				ds_printf("DS_PROCESS: Extracting '%s' to '%s' ...\n", file, filepath);
				sprintf(dest, "%s/%.*s", getenv("PATH"), strlen(filepath) - strlen(file) - 1, filepath);
				
				if(!DirExists(dest) && mkpath(dest)) {
					ds_printf("DS_ERROR: Can't create directory '%s'\n", dest);
					free(file_list);
					opk_close(opk);
					return CMD_ERROR;
				}

				if(opk_extract_file(opk, file, &data, &size) < 0) {
					if(opk_extract_file(opk, filepath, &data, &size) < 0) {
						opk_close(opk);
						return CMD_ERROR;
					}
				}
				
				sprintf(dest, "%s/%s", getenv("PATH"), filepath);
				fd = fs_open(dest, O_CREAT | O_TRUNC | O_WRONLY);

				if(fd < 0) {
					ds_printf("DS_ERROR: Can't open %s for write: %d\n", dest, errno);
					free(file_list);
					free(data);
					opk_close(opk);
					return CMD_ERROR;
				}

				fs_write(fd, data, size);
				fs_close(fd);
				free(data);
			}
			
			free(file_list);
			
		} else {
			ds_printf("DS_WARNING: Can't find 'DS-Files' or 'DS-Data' in metadata file\n");
		}
	}
	
	if(lua_file[0] != 0 && !opk_extract_file(opk, lua_file, &data, &size)) {
		tmpval = (char*)data;
		tmpval[size-1] = '\0';
		ds_printf("DS_PROCESS: Executing '%s'...\n", lua_file);
		LuaDo(LUA_DO_STRING, tmpval, GetLuaState());
		free(data);
	}
	
	opk_close(opk);
	ds_printf("DS_OK: Installation completed!\n");
	return CMD_OK;
}



static int uninstall_package(char *package_path) {

	struct OPK *opk;
	void *data;
	size_t size;
	const char *key, *val;
	const char *metadata_name;
	char dest[MAX_FN_LEN], lua_file[128];
	char *file_list, *filepath, *tmpval;
	size_t skey, sval;

	opk = opk_open(package_path);

	if(opk == NULL) {
		ds_printf("DS_ERROR: Failed to open %s\n", package_path);
		return CMD_ERROR;
	}
	
	while (opk_open_metadata(opk, &metadata_name) > 0) {
		
		file_list = NULL;
		memset(lua_file, 0, sizeof(lua_file));
		
		while(opk_read_pair(opk, &key, &skey, &val, &sval) && key) {
			
			if(!strncasecmp(key, "DS-Files", 8) || !strncasecmp(key, "DS-Data", 7)) {
				
				file_list = strndup((char *)val, sval);
				
			} else if(!strncasecmp(key, "Name", 4)) {
				
				ds_printf("DS_PROCESS: Uninstalling %.*s package...\n", (int)sval, val);
				
			} else if(!strncasecmp(key, "DS-UninstallScript", 18) && sval) {
				
				strncpy(lua_file, val, sval);
			}
		}
		
		if(file_list != NULL) {
			
			tmpval = file_list;
			
			while((filepath = strsep(&tmpval, " ;\t\n")) != NULL) {
				
				sprintf(dest, "%s/%s", getenv("PATH"), filepath);
				ds_printf("DS_PROCESS: Deleting '%s' ...\n", dest);
				
				if(fs_unlink(dest) != 0) {
					ds_printf("DS_ERROR: Error when deleting a file: %d\n", errno);
					free(file_list);
					return CMD_ERROR;
				}
			}
			
			free(file_list);
			
		} else {
			ds_printf("DS_WARNING: Can't find 'DS-Files' or 'DS-Data' in metadata file\n");
		}
	}
	
	if(lua_file[0] != 0 && !opk_extract_file(opk, lua_file, &data, &size)) {
		tmpval = (char*)data;
		tmpval[size-1] = '\0';
		ds_printf("DS_PROCESS: Executing '%s'...\n", lua_file);
		LuaDo(LUA_DO_STRING, tmpval, GetLuaState());
		free(data);
	}
	
	opk_close(opk);
	ds_printf("DS_OK: Uninstallation completed!\n");
	return CMD_OK;
}



int builtin_opkg_cmd(int argc, char *argv[]) { 
	
    if(argc < 2) {
		ds_printf("Usage: %s options args\n"
					"Options: \n"
					" -i, --install        -Installing package\n"
					" -u, --uninstall      -Uninstalling package\n"
					" -m, --metadata       -Display package metadata\n"
					" -v, --version        -Display opkg module version\n\n"
					"Arguments: \n"
					" -f, --file           -File .opk\n"
					" -e, --extract        -Extract one file\n"
					" -d, --dest           -Destination file/path\n\n"
					"Example: %s -m -f package.opk\n\n", argv[0], argv[0]);
        return CMD_NO_ARG; 
    }
     
	int install = 0, uninstall = 0, metadata = 0, version = 0;
	char *file = NULL, *dest = NULL, *extractf = NULL;

	struct cfg_option options[] = {
		{"install",   'i', NULL, CFG_BOOL, (void *) &install,	0},
		{"uninstall", 'u', NULL, CFG_BOOL, (void *) &uninstall,	0},
		{"metadata",  'm', NULL, CFG_BOOL, (void *) &metadata,	0},
		{"version",   'v', NULL, CFG_BOOL, (void *) &version,	0},
		{"file",      'f', NULL, CFG_STR,  (void *) &file,		0},
		{"dest",      'd', NULL, CFG_STR,  (void *) &dest,		0},
		{"extract",   'e', NULL, CFG_STR,  (void *) &extractf,	0},
		CFG_END_OF_LIST
	};
	
	CMD_DEFAULT_ARGS_PARSER(options);
		
	if(version) {
		ds_printf("OPKG module version: %d.%d.%d build %d\n", VER_MAJOR, VER_MINOR, VER_MICRO, VER_BUILD);
		return CMD_OK;
	}
	
	if(file == NULL) {
		ds_printf("DS_ERROR: You did not specify a required argument (file .opk).\n");
		return CMD_ERROR;
	}
	
	if(metadata) {
		return display_info(file);
	}

	if(extractf) {

		if(dest == NULL) {
			ds_printf("DS_ERROR: You did not specify a required argument (destination file).\n");
			return CMD_ERROR;
		}

		return extract_file_to(file, extractf, dest);
	}
	
	if(install) {
		return install_package(file);
	}
	
	if(uninstall) {
		return uninstall_package(file);
	}

	return CMD_OK;
}

