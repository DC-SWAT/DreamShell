/*
   Tsunami for KallistiOS ##version##

   inputeventstate.cpp

   Copyright (C) 2024 Maniac Vera
   
*/

#include "inputeventstate.h"
#include <algorithm>

InputEventState::InputEventState() {
}

InputEventState::~InputEventState() {
	m_change_state = nullptr;
}

void InputEventState::setStates(int previous_state, int control_state, int *change_state) {
	m_previous_state = previous_state;
	m_control_state = control_state;
	m_change_state = change_state;
}

void InputEventState::returnState() {
	if (m_change_state != nullptr) {
		*m_change_state = m_previous_state;
	}
}

int InputEventState::getControlState() {
	return m_control_state;
}

int InputEventState::getPreviousState() {
	return m_previous_state;
}

extern "C"
{
	
}