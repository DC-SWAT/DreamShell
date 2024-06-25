/*
   Tsunami for KallistiOS ##version##

   genmenu.h

   Copyright (C) 2003 Megan Potter
*/

#ifndef __TSUNAMI_GENMENU_H
#define __TSUNAMI_GENMENU_H

#include "drawables/scene.h"

#include <filesystem>

/* This defines a fully generic menu system. Basically what you do is
   derive from this class and then implement the constructor (which adds
   things to the internal scene object) and the inputEvent method (which
   is triggered any time a key change is detected). Pretty much everything
   else is taken care of by this base class, including background music
   if you want it (set in constructor). */

class GenericMenu {
public:
	// Constructor / destructor
	GenericMenu();
	virtual ~GenericMenu();

	// Called to actually "do" the menu -- this method will only return
	// when the menu has been completed and removed from the screen or
	// whatnot. The object can then be queried for any result data.
	virtual void doMenu();

	// Event class. Contains data about an input event that occured.
	struct Event {
		// Event types.
		enum EventType {
			EvtKeypress = 0,	// A key was pressed
			EvtAttach,		// A peripheral was plugged in
			EvtDetach,		// A peripheral was removed
			EvtTimeout		// The user didn't do anything for too long
		};

		// Key constants.
		enum KeyConstant {
			KeyLeft = 0,
			KeyRight,
			KeyUp,
			KeyDown,
			KeySelect,
			KeyCancel,
			KeyPgup,
			KeyPgdn,
			KeyReset,
			KeyStart,
			KeyMiscX,
			KeyMiscY,
			KeyUnknown,
			KeySentinel
		};

		// Type constants.
		enum TypeConstant {
			TypeAny = 0,
			TypeController,
			TypeKeyboard,
			TypeOther,
			TypeNone
		};

		Event() { }
		Event(EventType t) : type(t) { }

		EventType	type;		// For all events
		int		port;		// For all events except Timeout
		TypeConstant	ptype;		// For Attach/Detach/Keypress
		KeyConstant	key;		// For Keypress
		void		* rawState;	// For Keypress: raw device state struct
	};

	// Set a post-doMenu delay in case sounds may still be playing
	void setPostDelay(int ms);

protected:
	// Called once per frame to update the screen. Generally no need
	// to override this method.
	virtual void visualPerFrame();
	virtual void visualOpaqueList();
	virtual void visualTransList();

	// Called once per frame to handle any control input events. Generally
	// no need to override this method.
	virtual void controlPerFrame();

	// Called by controlPerFrame to process attach/detach events.
	void triggerAttach(int p);
	void triggerDetach(int p);

	// Called by controlPerFrame to process each type of device we support
	void scanController(int p, bool initial = false);
	void scanKeyboard(int p, bool initial = false);
	void scanCommon(int p, uint32 newKeys, void *rawState);

	// Called any time an "input event" is detected. You should override
	// this to handle debounced key-style inputs and peripheral add/remove.
	virtual void inputEvent(const Event & evt);

	// Override this method to provide your own "start shutdown" code. This
	// method should also be called by you on an appropriate inputEvent.
	// If you override this method, make sure you call this parent impl.
	virtual void startExit();

	// Call this method to force the menu to quit instantly. This is helpful
	// if you want to transition to another menu, etc.
	virtual void quitNow();

	// Call this method to setup a background song to be played during the
	// menu. You should do this before calling doMenu(). The song will
	// be started with the menu and faded out on exit.
	virtual void setBgm(const std::filesystem::path &fn, bool cache = false);

	// This method should be called any time the user does something that
	// would cancel the menu's timeout.
	virtual void resetTimeout();
	virtual void setTimeout(uint32 v);

	// Set background colors
	virtual void setBg(float r, float g, float b);

	// What type of controller is in port N?
	Event::TypeConstant getType(int port);

	// Set auto-repeat on or off. This isn't like standard auto-repeat which
	// has a delay and such, it just goes full blast as long as the user
	// is holding down the buttons (once per frame per button, actually).
	// This is good if you want to allow the user to hold down buttons to
	// do stuff. Note that repeat is only enabled for the given key.
	void setAutoRepeat(Event::KeyConstant key, bool enabled);

	// Name of the song we'll use for background music (if any). If this
	// is an empty string, we'll not use a song.
	std::filesystem::path	m_bgmFn;
	bool		m_usebgm, m_cachebgm;

	// Background plane color
	float		m_bg[3];

	// Are we exiting? If so, we'll be fading music and backplane...
	bool		m_exiting;
	float		m_exitCount;
	float		m_exitSpeed;

	// When was the last time the user did something?
	uint32		m_totime;

	// How many seconds should we allow before triggering a timeout?
	uint32		m_timeout;

	// Our scene object
	std::shared_ptr<Scene>	m_scene;

	// Allow one "main" controller in each port. We'll track what's in
	// each port and what buttons are currently held.
	uint32		m_contTypes[4];
	uint32		m_contBtns[4];

	// Key autorepeat mask
	uint32		m_autoRep;

	// Post-doMenu delay in case sounds are still playing
	int		m_postDelay;
};

#endif	/* __GENMENU_H */
