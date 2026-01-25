/*
   Tsunami for KallistiOS ##version##

   inputeventstate.cpp

   Copyright (C) 2024-2026 Maniac Vera
   
*/

#include "inputeventstate.h"
#include <algorithm>

int InputEventState::m_global_window_state = WindowStateEnum::WSE_DEFAULT;

InputEventState::InputEventState() {
	m_window_state = InputEventState::getGlobalWindowState();
}

InputEventState::~InputEventState() {
}

void InputEventState::setStates(int control_state, int previous_state) {
	m_control_state = control_state;
	m_previous_state = previous_state;
}

void InputEventState::returnState() {
	m_global_window_state = m_previous_state;
}

int InputEventState::getPreviuosState() {
	return m_previous_state;
}

int InputEventState::getControlState() {
	return m_control_state;
}

void InputEventState::setControlState(int control_state) {
	m_control_state = control_state;
}

int InputEventState::getWindowState() {
	return m_window_state;
}

void InputEventState::setWindowState(int window_state) {
	m_window_state = window_state;
}

void InputEventState::ToggleToPreviousState() {
	m_global_window_state = m_previous_state;
}

void InputEventState::ToggleToControlState() {
	m_global_window_state = m_control_state;
}

extern "C"
{
	int TSU_InputEventStateGetGlobalWindowState() {
		return InputEventState::getGlobalWindowState();
	}
	void TSU_InputEventStateSetGlobalWindowState(int window_state) {
		InputEventState::setGlobalWindowState(window_state);
	}
}