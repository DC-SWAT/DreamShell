/** 
 * \file    events.h
 * \brief   DreamShell event system
 * \date    2007-2024
 * \author  SWAT www.dc-swat.ru
 */
  
#ifndef _DS_EVENTS_H
#define _DS_EVENTS_H

#include "video.h"

/**
 * SDL render (sofware) event
 */
#define EVENT_ACTION_RENDER 0
/**
 * Event update (SDL or input)
 */
#define EVENT_ACTION_UPDATE 1
/**
 * PVR render (hardware) event
 */
#define EVENT_ACTION_RENDER_HW 2

#define EVENT_TYPE_INPUT 0
#define EVENT_TYPE_VIDEO 1

#define EVENT_PRIO_DEFAULT 0
#define EVENT_PRIO_OVERLAY 1

/**
 * Event_t, data, action
 */
typedef void Event_func(void *, void *, int);

typedef struct Event {

	const char *name;
	uint32_t id;
	Event_func *event;
	void *param;
	int active;
	int type;
	int prio;

} Event_t;

#ifdef SDL_Rect

#define VideoEventUpdate_t SDL_Rect;

#else

typedef struct VideoEventUpdate {
	int16_t x;
	int16_t y;
	uint16_t w;
	uint16_t h;
} VideoEventUpdate_t;

#endif

/**
 * Initialize and shutdown event system
 */
int InitEvents();
void ShutdownEvents();

/**
 * Add event to list
 * 
 * return NULL on error
 */
Event_t *AddEvent(const char *name, int type, int prio, Event_func *event, void *param);

/**
 * Remove event from list
 */
int RemoveEvent(Event_t *e);

/**
 * Set event active state
 */
int SetEventActive(Event_t *e, int is_active);

/**
 * Events list utils
 */
Item_list_t *GetEventList();
Event_t *GetEventById(uint32_t id);
Event_t *GetEventByName(const char *name);

/**
 * Processing input events in main thread
 */
void ProcessInputEvents(SDL_Event *event);

/**
 * Processing software (SDL) video rendering events in video thread
 */
void ProcessVideoEventsSDL();

/**
 * Processing hardware (PVR) video rendering events in video thread
 */
void ProcessVideoEventsPVR();

/**
 * Forcing SDL screen update
 */
void ProcessVideoEventsUpdate(VideoEventUpdate_t *area);

#endif
