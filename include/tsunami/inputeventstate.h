/*
   Tsunami for KallistiOS ##version##

   inputeventstate.h

   Copyright (C) 2002 Megan Potter
   Copyright (C) 2024-2026 Maniac Vera

*/

#ifndef __TSUNAMI_INPUTEVENTSTATE_H
#define __TSUNAMI_INPUTEVENTSTATE_H

enum WindowStateEnum
{
    WSE_DEFAULT = 0
};

#ifdef __cplusplus

class InputEventState {
private:
    int m_window_state, m_control_state, m_previous_state;
    static int m_global_window_state;

public:
	/// Constructor / Destructor
	InputEventState();
	virtual ~InputEventState();

	virtual void inputEvent(int event_type, int key) = 0;
    void setStates(int control_state, int previous_state);
    void setControlState(int control_state);
    virtual void setWindowState(int window_state);
    int getPreviuosState();
    int getControlState();
    virtual int getWindowState();
    void returnState();
    void ToggleToPreviousState();
    void ToggleToControlState();

    static void setGlobalWindowState(int global_window_state) {
        m_global_window_state = global_window_state;
    }

    static int getGlobalWindowState() {
        return m_global_window_state;
    }
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

    int TSU_InputEventStateGetGlobalWindowState();
	void TSU_InputEventStateSetGlobalWindowState(int window_state);

#ifdef __cplusplus
};
#endif

#endif	/* __TSUNAMI_ANIMATION_H */
