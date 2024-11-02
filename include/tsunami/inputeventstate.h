/*
   Tsunami for KallistiOS ##version##

   animation.h

   Copyright (C) 2002 Megan Potter
   Copyright (C) 2024 Maniac Vera

*/

#ifndef __TSUNAMI_INPUTEVENTSTATE_H
#define __TSUNAMI_INPUTEVENTSTATE_H

#ifdef __cplusplus

class InputEventState {
private:
    int m_previous_state, m_control_state;
    int *m_change_state;

public:
	/// Constructor / Destructor
	InputEventState();
	virtual ~InputEventState();

	virtual void inputEvent(int even_type, int key) = 0;
    void setStates(int previous_state, int control_state, int *change_state);
    int getControlState();
    int getPreviousState();
    void returnState();
};

#else

typedef struct animation Animation;

#ifndef TYPEDEF_DRAWABLE
	typedef struct drawable Drawable;
#endif

#endif

#ifdef __cplusplus
extern "C"
{
#endif

#ifdef __cplusplus
};
#endif

#endif	/* __TSUNAMI_ANIMATION_H */
