/*
   Tsunami for KallistiOS ##version##

   sound.cpp

   Copyright (C) 2002 Megan Potter
*/

#include "sound.h"

#include <cassert>
#include <kos/dbglog.h>

int Sound::m_default_vol = 240;

Sound::Sound(const std::filesystem::path &fn) {
	m_index = SFXHND_INVALID;
	if (!loadFromFile(fn))
		assert( false );
}

Sound::Sound() {
	m_index = SFXHND_INVALID;
}

Sound::~Sound() {
	if (m_index != SFXHND_INVALID)
		snd_sfx_unload(m_index);
}

void Sound::play() {
	play(m_default_vol, 0x80);
}

void Sound::play(int vol) {
	play(vol, 0x80);
}

void Sound::play(int vol, int pan) {
	snd_sfx_play(m_index, vol, pan);
}

bool Sound::loadFromFile(const std::filesystem::path &fn) {
	if (m_index != SFXHND_INVALID)
		snd_sfx_unload(m_index);
	m_index = snd_sfx_load(fn.c_str());
	if (m_index == SFXHND_INVALID) {
		dbglog(DBG_WARNING, "Sound::loadFromFile: Can't load '%s'\n", fn.c_str());
		return false;
	} else
		return true;
}

// Set the default volume value
void Sound::setDefaultVolume(int vol) {
	m_default_vol = vol;
}
