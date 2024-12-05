/*
   Tsunami for KallistiOS ##version##

   genmenu.cpp

   Copyright (C) 2003 Megan Potter
   Copyright (C) 2024 Maniac Vera
   
*/

/*
  This module implements a generic menu system. There are some pieces here
  that were more integrated into Feet of Fury which have been commented out
  for the moment until they can be more properly sorted out.

  The basic idea is that you derive a C++ class from this and implement the
  functionality that differs from the norm. If you have a very simple menu,
  then for example you can simply override the constructor (to add elements
  to the scene object) and the inputEvent method to receive keypresses
  from the user. If you wanted to do a custom exit sequence, you can also
  override startExit(). There is a lot more to override of course. Take a look
  through the module and the headers for some more ideas.

  GenericMenu is also, unfortunately, a bit DC specific right now but
  we're hoping to rectify this over time.

  This module was donated from Feet of Fury by Cryptic Allusion, LLC.

 */

#include "genmenu.h"
#include <plx/list.h>
#include <kos/thread.h>
#include <arch/timer.h>
#include <dc/maple.h>
#include <dc/maple/controller.h>
#include <dc/maple/keyboard.h>

#include <cstdio>

GenericMenu::GenericMenu() {
	m_usebgm = false;
	m_cachebgm = false;

	m_bg[0] = m_bg[1] = m_bg[2] = 0.0f;

	m_exiting = false;
	m_exitCount = m_exitSpeed = 0.0f;

	m_totime = 0;
	m_timeout = 45;

	m_scene = new Scene();

	m_contTypes[0] = m_contTypes[1] = m_contTypes[2] = m_contTypes[3] = 0;
	m_contBtns[0] = m_contBtns[1] = m_contBtns[2] = m_contBtns[3] = 0;

	m_autoRep = 0;
	m_postDelay = 0;

	kbd_set_queue(0);
}

GenericMenu::~GenericMenu() {
	if (m_scene) {
		delete m_scene;
		m_scene = nullptr;
	}
}

void GenericMenu::doMenu() {
	// Start background music if necessary
	/* if (m_usebgm) {
		oggStart(m_bgmFn, true, m_cachebgm, false);
	} */

	// Reset our timeout
	resetTimeout();

	// Enter the main loop
	m_exiting = false;
	while (!m_exiting) {
		controlPerFrame();
		visualPerFrame();
	}

	// Ok, we're exiting -- do the same as before, but we'll exit out
	// entirely when the scene is finished (and music faded).
	while (!m_scene->isFinished() && m_exitCount > 0.0f) {
		m_exitCount -= m_exitSpeed;
		if (m_exitCount < 0.0f)
			m_exitCount = 0.0f;
		/* if (m_usebgm) {
			oggVolume(m_exitCount);
		} */
		visualPerFrame();
	}

	// Did we get a quit-now request?
	if (!m_exitSpeed)
		return;

	// We should be faded out now -- do three more frames to finish
	// clearing out the PVR buffered scenes.
	m_exitCount = 0;
	visualPerFrame();
	visualPerFrame();
	visualPerFrame();

	// Stop music if necessary
	/* if (m_usebgm) {
		oggStop(m_cachebgm);
	} */

	if (m_postDelay)
		thd_sleep(m_postDelay);

	// Stop any sound effects
	/* snd_sfx_stop_all(); */
}

void GenericMenu::setPostDelay(int d) {
	m_postDelay = d;
}

void GenericMenu::visualPerFrame() {
	pvr_wait_ready();

	if (m_exiting)
		pvr_set_bg_color(m_bg[0]*m_exitCount, m_bg[1]*m_exitCount, m_bg[2]*m_exitCount);
	else
		pvr_set_bg_color(m_bg[0], m_bg[1], m_bg[2]);

	pvr_scene_begin();

	pvr_list_begin(PLX_LIST_OP_POLY);
	visualOpaqueList();

	pvr_list_begin(PLX_LIST_TR_POLY);
	visualTransList();

	pvr_scene_finish();

	m_scene->nextFrame();
}

void GenericMenu::visualOpaqueList() {
	m_scene->draw(PLX_LIST_OP_POLY);
}

void GenericMenu::visualTransList() {
	m_scene->draw(PLX_LIST_TR_POLY);
}

void GenericMenu::controlPerFrame() {
	// Go through and look for attach/detach messages
	for (int p=0; p<4; p++) {
		maple_device_t * dev = maple_enum_dev(p, 0);
		if (!dev) {
			if (m_contTypes[p]) {
				triggerDetach(p);
			}
		} else {
			bool initial = false;
			if (!m_contTypes[p]) {
				m_contTypes[p] = dev->info.functions;
				triggerAttach(p);
				initial = true;
			}

			// What kind of device is it?
			if (m_contTypes[p] & MAPLE_FUNC_CONTROLLER)
				scanController(p, initial);
			else if (m_contTypes[p] & MAPLE_FUNC_KEYBOARD)
				scanKeyboard(p, initial);
		}
	}

	// Check for timeouts and send a message if we have one. In case the
	// subclass isn't paying attention, reset the counter as well.
	uint32 s;
	timer_ms_gettime(&s, NULL);
	if ((s - m_totime) > m_timeout) {
		resetTimeout();
		inputEvent(Event(Event::EvtTimeout));
	}
}

void GenericMenu::triggerAttach(int p) {
	Event evt(Event::EvtAttach);
	evt.port = p;
	if (m_contTypes[p] & MAPLE_FUNC_KEYBOARD)
		evt.ptype = Event::TypeKeyboard;
	else if (m_contTypes[p] & MAPLE_FUNC_CONTROLLER)
		evt.ptype = Event::TypeController;
	else
		evt.ptype = Event::TypeOther;

	inputEvent(evt);
}

void GenericMenu::triggerDetach(int p) {
	Event evt(Event::EvtDetach);
	evt.port = p;
	evt.ptype = getType(p);

	m_contBtns[p] = 0;
	m_contTypes[p] = 0;

	inputEvent(evt);
}

void GenericMenu::scanController(int p, bool initial) {
	// Get the status of the controller
	maple_device_t * dev = maple_enum_dev(p, 0);
	cont_state_t * state = (cont_state_t *)maple_dev_status(dev);
	uint32 btns = state->buttons;

	// Convert the buttons to key bits
	uint32 keys = 0;
	if ((btns & (CONT_A | CONT_B | CONT_X | CONT_Y | CONT_START)) ==
		(CONT_A | CONT_B | CONT_X | CONT_Y | CONT_START))
	{
		keys |= 1 << Event::KeyReset;
	}
	if (btns & CONT_DPAD_LEFT)
		keys |= 1 << Event::KeyLeft;
	if (btns & CONT_DPAD_UP)
		keys |= 1 << Event::KeyUp;
	if (btns & CONT_DPAD_RIGHT)
		keys |= 1 << Event::KeyRight;
	if (btns & CONT_DPAD_DOWN)
		keys |= 1 << Event::KeyDown;
	if (btns & CONT_A)
		keys |= 1 << Event::KeySelect;
	if (btns & CONT_B)
		keys |= 1 << Event::KeyCancel;
	if (btns & CONT_X)
		keys |= 1 << Event::KeyMiscX;
	if (btns & CONT_Y)
		keys |= 1 << Event::KeyMiscY;
	if (btns & CONT_START)
		keys |= 1 << Event::KeyStart;
	if (state->ltrig)
		keys |= 1 << Event::KeyPgup;
	if (state->rtrig)
		keys |= 1 << Event::KeyPgdn;

	if (initial)
		m_contBtns[p] = keys;
	else
		scanCommon(p, keys, state);
}

void GenericMenu::scanKeyboard(int p, bool initial) {
	// Get the status of the keyboard
	maple_device_t * dev = maple_enum_dev(p, 0);
	kbd_state_t * state = (kbd_state_t *)maple_dev_status(dev);
	uint8 * kb = state->matrix;

	// Convert the buttons to key bits
	uint32 keys = 0;
	if (kb[KBD_KEY_LEFT])
		keys |= 1 << Event::KeyLeft;
	if (kb[KBD_KEY_UP])
		keys |= 1 << Event::KeyUp;
	if (kb[KBD_KEY_RIGHT])
		keys |= 1 << Event::KeyRight;
	if (kb[KBD_KEY_DOWN])
		keys |= 1 << Event::KeyDown;
	if (kb[KBD_KEY_ENTER])
		keys |= 1 << Event::KeySelect;
	if (kb[KBD_KEY_ESCAPE])
		keys |= 1 << Event::KeyCancel;
	if (kb[KBD_KEY_PGUP])
		keys |= 1 << Event::KeyPgup;
	if (kb[KBD_KEY_PGDOWN])
		keys |= 1 << Event::KeyPgdn;
	if (kb[KBD_KEY_SPACE])
		keys |= 1 << Event::KeyStart;

	// Heheh
	if ( kb[KBD_KEY_DEL] &&
		(state->shift_keys & (KBD_MOD_LCTRL | KBD_MOD_RCTRL)) &&
		(state->shift_keys & (KBD_MOD_LALT | KBD_MOD_RALT)) )
	{
		keys |= 1 << Event::KeyReset;
	}

	if (initial)
		m_contBtns[p] = keys;
	else
		scanCommon(p, keys, state);
}

void GenericMenu::scanCommon(int p, uint32 keys, void * rawState) {
	bool reset = false;
	uint32 called = 0;

	// Anything we're doing autorepeat for, call automatically
	if (m_autoRep & m_contBtns[p]) {
		// Find the pressed buttons
		uint32 btns = m_contBtns[p] & m_autoRep;
		for (int i=0; i<Event::KeySentinel; i++) {
			if (btns & (1 << i)) {
				Event evt(Event::EvtKeypress);
				evt.port = p;
				evt.ptype = getType(p);
				evt.key = (Event::KeyConstant)i;
				evt.rawState = rawState;
				inputEvent(evt);

				reset = true;

				called |= 1 << i;
			}
		}
	}

	// Any buttons just get pressed/released?
	if (keys ^ m_contBtns[p]) {
		// Find the pressed buttons
		for (int i=0; i<Event::KeySentinel; i++) {
			uint32 mask = 1 << i;
			if ((keys & mask) && !(m_contBtns[p] & mask) && !(called & mask)) {
				Event evt(Event::EvtKeypress);
				evt.port = p;
				evt.ptype = getType(p);
				evt.key = (Event::KeyConstant)i;
				evt.rawState = rawState;
				inputEvent(evt);

				reset = true;
			}
		}

		m_contBtns[p] = keys;
	}

	if (reset)
		resetTimeout();
}

// The default inputEvent just gives you simple debugging output so
// you know it works.
void GenericMenu::inputEvent(const Event & evt) {
	static const char * keyNames[] = {
		"KeyLeft",
		"KeyRight",
		"KeyUp",
		"KeyDown",
		"KeySelect",
		"KeyCancel",
		"KeyPgup",
		"KeyPgdn",
		"KeyReset",
		"KeyStart",
		"KeyMiscX",
		"KeyMiscY",
		"KeyUnknown"
	};
	static const char * devNames[] = {
		"TypeAll",
		"TypeController",
		"TypeKeyboard",
		"TypeOther",
		"TypeNone"
	};

	switch (evt.type) {

	case Event::EvtKeypress:
		printf("inputEvent: key %s pressed on port %d\n",
			keyNames[evt.key], evt.port);
		break;

	case Event::EvtAttach:
		printf("inputEvent: device %s attached on port %d\n",
			devNames[evt.ptype], evt.port);
		break;

	case Event::EvtDetach:
		printf("inputEvent: device detached on port %d\n",
			evt.port);
		break;

	case Event::EvtTimeout:
		printf("inputEvent: timeout detected\n");
		break;
	}
}

void GenericMenu::startExit() {
	m_exiting = true;
	m_exitCount = 1.0f;
	m_exitSpeed = 1.0f/60.0f;
	// m_exitSpeed = 1.0f/hz;	// hz is 50 or 60
}

void GenericMenu::quitNow() {
	m_exiting = true;
	m_exitCount = 0.0f;
	m_exitSpeed = 0.0f;
}

void GenericMenu::setBgm(const std::filesystem::path &fn, bool cache) {
	if (!fn.empty()) {
		m_bgmFn = fn;
		m_usebgm = true;
	} else
		m_usebgm = false;
	m_cachebgm = cache;
}

void GenericMenu::resetTimeout() {
	timer_ms_gettime(&m_totime, NULL);
}

void GenericMenu::setTimeout(uint32 v) {
	m_timeout = v;
}

void GenericMenu::setBg(float r, float g, float b) {
	m_bg[0] = r;
	m_bg[1] = g;
	m_bg[2] = b;
}

GenericMenu::Event::TypeConstant GenericMenu::getType(int port) {
	assert( port >=0 && port <= 3 );
	if (m_contTypes[port] == 0)
		return Event::TypeNone;
	if (m_contTypes[port] & MAPLE_FUNC_CONTROLLER)
		return Event::TypeController;
	if (m_contTypes[port] & MAPLE_FUNC_KEYBOARD)
		return Event::TypeKeyboard;
	return Event::TypeOther;
}

void GenericMenu::setAutoRepeat(GenericMenu::Event::KeyConstant key, bool enabled) {
	if (enabled)
		m_autoRep |= 1 << key;
	else
		m_autoRep &= ~(1 << key);
}
