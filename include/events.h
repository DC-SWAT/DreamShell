/** 
 * \file    events.h
 * \brief   DreamShell event system
 * \date    2007-2023
 * \author  SWAT www.dc-swat.ru
 */
  
#ifndef _DS_EVENTS_H
#define _DS_EVENTS_H

#include "video.h"

#define EVENT_STATE_ACTIVE 0
#define EVENT_STATE_SLEEP 1

#define EVENT_ACTION_RENDER 0
#define EVENT_ACTION_UPDATE 1

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
	uint32 id;
	Event_func *event;
	void *param;
	uint16 state;
	uint16 type;
	uint16 prio;

} Event_t;

#ifdef SDL_Rect

#define VideoEventUpdate_t SDL_Rect;

#else

typedef struct VideoEventUpdate {
	int16 x;
	int16 y;
	uint16 w;
	uint16 h;
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
Event_t *AddEvent(const char *name, uint16 type, uint16 prio, Event_func *event, void *param);

/**
 * Remove event from list
 */
int RemoveEvent(Event_t *e);

/**
 * Setup event state
 */
int SetEventState(Event_t *e, uint16 state);

/**
 * Events list utils
 */
Item_list_t *GetEventList();
Event_t *GetEventById(uint32 id);
Event_t *GetEventByName(const char *name);

/**
 * Processing input events in main thread
 */
void ProcessInputEvents(SDL_Event *event);

/**
 * Processing video rendering events in video thread
 */
void ProcessVideoEventsRender();

/**
 * Manual processing for forcing screen update in another video events
 */
void ProcessVideoEventsUpdate(VideoEventUpdate_t *area);

#endif
