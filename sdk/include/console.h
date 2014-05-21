/** 
 * \file    console.h
 * \brief   DreamShell console
 * \date    2004-2014
 * \author  SWAT www.dc-swat.ru
 */


#ifndef _DS_CONSOLE_H
#define _DS_CONSOLE_H

#ifndef _NO_VIDEO_H
#include "video.h"
#else
#include "list.h"
#endif



typedef enum {
	CMD_OK = 0,
	CMD_NO_ARG,
	CMD_ERROR,
	CMD_NOT_EXISTS
} CMD_RESULT;


typedef enum {
	CMD_TYPE_INTERNAL = 0,
	CMD_TYPE_ELF,
	CMD_TYPE_LUA,
	CMD_TYPE_DSC,
	CMD_TYPE_BIN,
	CMD_TYPE_UKNOWN
} CMD_TYPES;


typedef int CmdHandler(int, char *[]);

typedef struct Cmd {

	const char *command;
	const char *desc;
	CmdHandler *handler;
	
} Cmd_t;


#define CMD_DEFAULT_ARGS_PARSER(options)						\
	do {															\
		CFG_CONTEXT con = cfg_get_context(options);			\
		cfg_set_context_flag(con, CFG_IGNORE_UNKNOWN);		\
																	\
		if (con == NULL) {										\
			ds_printf("DS_ERROR: Not enough memory\n");		\
			return CMD_ERROR;										\
		}															\
																	\
		cfg_set_cmdline_context(con, 1, -1, argv);				\
																	\
		if (cfg_parse(con) != CFG_OK) {							\
			ds_printf("DS_ERROR: Parsing command line: %s\n",	\
						cfg_get_error_str(con));					\
			return CMD_ERROR;										\
		}															\
																	\
		cfg_free_context(con);									\
	} while(0)


int CallCmd(int argc, char *argv[]);
int CallExtCmd(int argc, char *argv[]);
int CallCmdFile(const char *fn, int argc, char *argv[]);
int CheckExtCmdType(const char *fn);

Cmd_t *AddCmd(const char *cmd, const char *helpmsg, CmdHandler *handler);
void RemoveCmd(Cmd_t *cmd);

Item_list_t *GetCmdList();
Cmd_t *GetCmdByName(const char *name);

int InitCmd(); 
void ShutdownCmd();


void SetConsoleDebug(int mode);


int ds_printf(const char *fmt, ...); 
int ds_uprintf(const char *fmt, ...);
int dbg_printf(const char *fmt, ...);


int dsystem(const char *buff); 
int dsystemf(const char *fmt, ...);
int dsystem_script(const char *fn);
int dsystem_buff(const char *buff);

int InitConsole(const char *font, const char *background, int lines, int x, int y, int w, int h, int alpha);
void ShutdownConsole();

int CreateConsolePTY();
int DestroyConsolePTY();

void DrawConsole();
int ToggleConsole();
void ShowConsole();
void HideConsole();
int ConsoleIsVisible();

#ifndef _NO_VIDEO_H
ConsoleInformation *GetConsole();
#endif

extern dbgio_handler_t dbgio_ds;
extern dbgio_handler_t dbgio_sd;


#endif
