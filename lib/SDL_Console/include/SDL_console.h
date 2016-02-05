/*
	SDL_console: An easy to use drop-down console based on the SDL library
	Copyright (C) 1999, 2000, 2001, 2002, 2003, 2004 Clemens Wacha
	
	This library is free software; you can redistribute it and/or
	modify it under the terms of the GNU Library General Public
	License as published by the Free Software Foundation; either
	version 2 of the License, or (at your option) any later version.
	
	This library is distributed in the hope that it will be useful,
	but WHITOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
	Library General Public License for more details.
	
	You should have received a copy of the GNU Library Generla Public
	License along with this library; if not, write to the Free
	Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
	
	Clemens Wacha
	reflex-2000@gmx.net
*/

#ifndef _CONSOLE_H_
#define _CONSOLE_H_

#ifdef HAVE_CONFIG_H
  #include <config.h>
#endif


#include "SDL_events.h"
#include "SDL_video.h"
#include "begin_code.h"

/*! Number of visible characters in a line. Lines in the history, the commandline, or CON_Out strings cannot be longer
	than this. Remark that this number does NOT include the '/0' character at the end of a string. So if we create
	a string we do this char* mystring[CON_CHARS_PER_LINE + 1];
	*/
#define CON_CHARS_PER_LINE   127
/*! Cursor blink frequency in ms */
#define CON_BLINK_RATE       500
/*! Border in pixels from the left margin to the first letter */
#define CON_CHAR_BORDER      4
/*! Default prompt used at the commandline */
#define CON_DEFAULT_PROMPT	"]"
/*! Scroll this many lines at a time (when pressing PGUP or PGDOWN) */
#define CON_LINE_SCROLL	2
/*! Indicator showing that you scrolled up the history */
#define CON_SCROLL_INDICATOR "^"
/*! Cursor shown if we are in insert mode */
#define CON_INS_CURSOR "_"
/*! Cursor shown if we are in overwrite mode */
#define CON_OVR_CURSOR "|"
/*! Defines the default hide key (that Hide()'s the console if pressed) */
#define CON_DEFAULT_HIDEKEY	SDLK_ESCAPE
/*! Defines the opening/closing speed when the console switches from CON_CLOSED to CON_OPEN */
#define CON_OPENCLOSE_SPEED 25

/* Set up for C function definitions, even when using C++ */
#ifdef __cplusplus
extern "C" {
#endif

	enum {
	    CON_CLOSED,	/*! The console is closed (and not shown) */
	    CON_CLOSING,	/*! The console is still open and visible but closing. After it has completely disappeared it changes to CON_CLOSED */
	    CON_OPENING,	/*! The console is visible and opening but not yet fully open. Changes to CON_OPEN when done */
	    CON_OPEN	/*! The console is open and visible */
	};

	/*! This is a struct for each consoles data */
	typedef struct console_information_td {
		int Visible;						/*! enum that tells which visible state we are in CON_CLOSED, CON_OPEN, CON_CLOSING, CON_OPENING */
		int WasUnicode;						/*! stores the UNICODE value before the console was shown. On Hide() the UNICODE value is restored. */
		int RaiseOffset;					/*! Offset used in show/hide animation */
		int HideKey;						/*! the key that can hide the console */
		char **ConsoleLines;				/*! List of all the past lines */
		char **CommandLines;				/*! List of all the past commands */
		int TotalConsoleLines;				/*! Total number of lines in the console */
		int ConsoleScrollBack;				/*! How much the user scrolled back in the console */
		int TotalCommands;					/*! Number of commands that were typed in before (which are now in the CommandLines array) */
		int FontNumber;						/*! This is the number of the font for the console (DT_* specific; will hopefully disappear in future releases) */
		int LineBuffer;						/*! The number of visible lines in the console (autocalculated on CON_UpdateConsole()) */
		int VChars;							/*! The number of visible characters in one console line (autocalculated on CON_Init() and recalc. on CON_Resize()) */
		int BackX, BackY;					/*! Background image x and y coords */
		char* Prompt;						/*! Prompt displayed in command line */
		char Command[CON_CHARS_PER_LINE+1];	/*! current command in command line = lcommand + rcommand (Get's updated in AssembleCommand())*/
		char RCommand[CON_CHARS_PER_LINE+1];	/*! left hand side of cursor */
		char LCommand[CON_CHARS_PER_LINE+1];	/*! right hand side of cursor */
		char VCommand[CON_CHARS_PER_LINE+1];	/*! current visible command line */
		int CursorPos;						/*! Current cursor position relative to the currently typed in command */
		int Offset;							/*! First visible character relative to the currently typed in command (used if command is too long to fit into console) */
		int InsMode;						/*! Boolean that tells us whether we are in Insert- or Overwrite-Mode */
		SDL_Surface *ConsoleSurface;		/*! THE Surface of the console */
		SDL_Surface *OutputScreen;			/*! This is the screen to draw the console to (normally you VideoSurface)*/
		SDL_Surface *BackgroundImage;		/*! Background image for the console */
		SDL_Surface *InputBackground;		/*! Dirty rectangle that holds the part of the background image that is behind the commandline */
		int DispX, DispY;					/*! The top left x and y coords of the console on the display screen */
		unsigned char ConsoleAlpha;			/*! The consoles alpha level */
		int CommandScrollBack;				/*! How much the users scrolled back in the command lines */
		void(*CmdFunction)(struct console_information_td *console, char* command);	/*! The Function that is executed if you press 'Return' in the console */
		char*(*TabFunction)(char* command);	/*! The Function that is executed if you press 'Tab' in the console */

		int FontHeight;						/*! The height of the font used in the console */
		int FontWidth;						/*! The width of the font used in the console (Remark that the console needs FIXED width fonts!!) */
	} ConsoleInformation;

	/*! Takes keys from the keyboard and inputs them to the console if the console isVisible().
		If the event was not handled (i.e. WM events or unknown ctrl- or alt-sequences) 
		the function returns the event for further processing. ***The prototype of this function will change in the next major release to
		int CON_Events(ConsoleInformation* console, SDL_Event *event) ***/
	extern DECLSPEC SDL_Event* SDLCALL CON_Events(SDL_Event *event);
	/*! Makes the console visible */
	extern DECLSPEC void SDLCALL CON_Show(ConsoleInformation *console);
	/*! Hides the console */
	extern DECLSPEC void SDLCALL CON_Hide(ConsoleInformation *console);
	/*! Returns 1 if the console is opening or open, 0 else */
	extern DECLSPEC int SDLCALL CON_isVisible(ConsoleInformation *console);
	/*! Internal: Updates visible state. This function is responsible for the opening/closing animation. Only used in CON_DrawConsole() */
	extern DECLSPEC void SDLCALL CON_UpdateOffset(ConsoleInformation* console);
	/*! Draws the console to the screen if it is visible (NOT if it isVisible()). It get's drawn if it is REALLY visible ;-) */
	extern DECLSPEC void SDLCALL CON_DrawConsole(ConsoleInformation *console);
	/*! Initializes a new console.
		@param FontName A filename of an image containing the font. Look at the example code for the image contents
		@param DisplayScreen The VideoSurface we are blitting to. ***This was not a very intelligent move. I will change this in the next major release.
		CON_DrawConsole will then no more blit the console to this surface but give you a pointer to ConsoleSurface when all updates are done***
		@param lines The total number of lines in the history
		@param rect Position and size of the new console */
	extern DECLSPEC ConsoleInformation* SDLCALL CON_Init(const char *FontName, SDL_Surface *DisplayScreen, int lines, SDL_Rect rect);
	/*! Frees DT_DrawText and calls CON_Free */
	extern DECLSPEC void SDLCALL CON_Destroy(ConsoleInformation *console);
	/*! Frees all the memory loaded by the console */
	extern DECLSPEC void SDLCALL CON_Free(ConsoleInformation *console);
	/*! Function to send text to the console. Works exactly like printf and supports the same format */
	extern DECLSPEC void SDLCALL CON_Out(ConsoleInformation *console, const char *str, ...);
	/*! Sets the alpha level of the console to the specified value (0 - transparent,
		255 - opaque). Use this function also for OpenGL. */
	extern DECLSPEC void SDLCALL CON_Alpha(ConsoleInformation *console, unsigned char alpha);
	/*! Internal: Sets the alpha channel of an SDL_Surface to the specified value.
		Preconditions: the surface in question is RGBA. 0 <= a <= 255, where 0 is transparent and 255 opaque */
	extern DECLSPEC void SDLCALL CON_AlphaGL(SDL_Surface *s, int alpha);
	/*! Sets a background image for the console */
	extern DECLSPEC int SDLCALL CON_Background(ConsoleInformation *console, const char *image, int x, int y);
	/*! Changes current position of the console to the new given coordinates */
	extern DECLSPEC void SDLCALL CON_Position(ConsoleInformation *console, int x, int y);
	/*! Changes the size of the console */
	extern DECLSPEC int SDLCALL CON_Resize(ConsoleInformation *console, SDL_Rect rect);
	/*! Beams a console to another screen surface. Needed if you want to make a Video restart in your program. This
		function first changes the OutputScreen Pointer then calls CON_Resize to adjust the new size. ***Will disappear in the next major release. Instead
		i will introduce a new function called CON_ReInit or something that adjusts the internal parameters etc *** */
	extern DECLSPEC int SDLCALL CON_Transfer(ConsoleInformation* console, SDL_Surface* new_outputscreen, SDL_Rect rect);
	/*! Give focus to a console. Make it the "topmost" console. This console will receive events
		sent with CON_Events() ***Will disappear in the next major release. There is no need for such a focus model *** */
	extern DECLSPEC void SDLCALL CON_Topmost(ConsoleInformation *console);
	/*! Modify the prompt of the console. If you want a backslash you will have to escape it. */
	extern DECLSPEC void SDLCALL CON_SetPrompt(ConsoleInformation *console, char* newprompt);
	/*! Set the key, that invokes a CON_Hide() after press. default is ESCAPE and you can always hide using
		ESCAPE and the HideKey (2 keys for hiding). compared against event->key.keysym.sym !! */
	extern DECLSPEC void SDLCALL CON_SetHideKey(ConsoleInformation *console, int key);
	/*! Internal: executes the command typed in at the console (called if you press 'Return')*/
	extern DECLSPEC void SDLCALL CON_Execute(ConsoleInformation *console, char* command);
	/*! Sets the callback function that is called if a command was typed in. The function you would like to use as the callback will have to
		look like this: <br>
		<b> void my_command_handler(ConsoleInformation* console, char* command)</b> <br><br>
		You will then call the function like this:<br><b> 
		CON_SetExecuteFunction(console, my_command_handler)</b><br><br>
		If this is not clear look at the example program */
	extern DECLSPEC void SDLCALL CON_SetExecuteFunction(ConsoleInformation *console, void(*CmdFunction)(ConsoleInformation *console2, char* command));
	/*! Sets the callback function that is called if you press the 'Tab' key. The function has to look like this:<br><b> 
		char* my_tabcompletion(char* command)</b><br><br>
		The commandline on the left side of the cursor gets passed over to your function. You will then have to make your
		own tab-complete and return the completed string as return value. If you have nothing to complete you can return
		NULL or the string you got. ***Will change in the next major release to char* mytabfunction(ConsoleInformation* console, char* command) *** */
	extern DECLSPEC void SDLCALL CON_SetTabCompletion(ConsoleInformation *console, char*(*TabFunction)(char* command));
	/*! Internal: Gets called when TAB was pressed and executes the function you have earlier registered with CON_SetTabCompletion() */
	extern DECLSPEC void SDLCALL CON_TabCompletion(ConsoleInformation *console);
	/*! Internal: makes a newline (same as printf("\n") or CON_Out(console, "\n") ) */
	extern DECLSPEC void SDLCALL CON_NewLineConsole(ConsoleInformation *console);
	/*! Internal: shift command history (the one you can switch with the up/down keys) */
	extern DECLSPEC void SDLCALL CON_NewLineCommand(ConsoleInformation *console);
	/*! Internal: updates console after resize, background image change, CON_Out() etc. This function draws the upper part of the console (that holds the history) */
	extern DECLSPEC void SDLCALL CON_UpdateConsole(ConsoleInformation *console);


	/*! Internal: Default Execute callback */
	extern DECLSPEC void SDLCALL Default_CmdFunction(ConsoleInformation *console, char* command);
	/*! Internal: Default TabCompletion callback */
	extern DECLSPEC char* SDLCALL Default_TabFunction(char* command);

	/*! Internal: draws the commandline the user is typing in to the screen. Called from within CON_DrawConsole() *** Will change in the next major release to
		void DrawCommandLine(ConsoleInformation* console) *** */
	extern DECLSPEC void SDLCALL DrawCommandLine();

	/*! Internal: Gets called if you press the LEFT key (move cursor left) */
	extern DECLSPEC void SDLCALL Cursor_Left(ConsoleInformation *console);
	/*! Internal: Gets called if you press the RIGHT key (move cursor right) */
	extern DECLSPEC void SDLCALL Cursor_Right(ConsoleInformation *console);
	/*! Internal: Gets called if you press the HOME key (move cursor to the beginning
	of the line */
	extern DECLSPEC void SDLCALL Cursor_Home(ConsoleInformation *console);
	/*! Internal: Gets called if you press the END key (move cursor to the end of the line*/
	extern DECLSPEC void SDLCALL Cursor_End(ConsoleInformation *console);
	/*! Internal: Called if you press DELETE (deletes character under the cursor) */
	extern DECLSPEC void SDLCALL Cursor_Del(ConsoleInformation *console);
	/*! Internal: Called if you press BACKSPACE (deletes character left of cursor) */
	extern DECLSPEC void SDLCALL Cursor_BSpace(ConsoleInformation *console);
	/*! Internal: Called if you type in a character (add the char to the command) */
	extern DECLSPEC void SDLCALL Cursor_Add(ConsoleInformation *console, SDL_Event *event);

	/*! Internal: Called if you press Ctrl-C (deletes the commandline) */
	extern DECLSPEC void SDLCALL Clear_Command(ConsoleInformation *console);
	/*! Internal: Called if the command line has changed (assemles console->Command from LCommand and RCommand */
	extern DECLSPEC void SDLCALL Assemble_Command(ConsoleInformation *console);
	/*! Internal: Called if you press Ctrl-L (deletes the History) */
	extern DECLSPEC void SDLCALL Clear_History(ConsoleInformation *console);

	/*! Internal: Called if you press UP key (switches through recent typed in commands */
	extern DECLSPEC void SDLCALL Command_Up(ConsoleInformation *console);
	/*! Internal: Called if you press DOWN key (switches through recent typed in commands */
	extern DECLSPEC void SDLCALL Command_Down(ConsoleInformation *console);

/* Ends C function definitions when using C++ */
#ifdef __cplusplus
};
#endif
#include "close_code.h"

#endif /* _CONSOLE_H_ */

/* end of SDL_console.h ... */

