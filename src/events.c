/****************************
 * DreamShell ##version##   *
 * events.c                 *
 * DreamShell events        *
 * (c)2007-2023 SWAT        *
 * http://www.dc-swat.ru    *
 ***************************/


#include "ds.h"
#include "video.h"
#include "events.h"
#include "console.h"

static Item_list_t *events;

static void FreeEvent(void *event) {
    free(event);
}


int InitEvents() {

	if((events = listMake()) == NULL) {
		return -1;
	}

	return 0;
}

void ShutdownEvents() {
	listDestroy(events, (listFreeItemFunc *) FreeEvent);
}


Item_list_t *GetEventList() {
	return events;
}


Event_t *GetEventById(uint32 id) {

	Item_t *i = listGetItemById(events, id);

	if(i != NULL) {
		return (Event_t *) i->data;
	} else {
		return NULL;
	}
}


Event_t *GetEventByName(const char *name) {

	Item_t *i = listGetItemByName(events, name);

	if(i != NULL) {
		return (Event_t *) i->data;
	} else {
		return NULL;
	}
}


Event_t *AddEvent(const char *name, uint16 type, Event_func *event, void *param) {

	Event_t *e;
	Item_t *i;

	if(name && GetEventByName(name)) return NULL;

	e = (Event_t *)calloc(1, sizeof(Event_t)); 
	if(e == NULL) return NULL;

	e->name = name;
	e->event = event;
	e->state = EVENT_STATE_ACTIVE;
	e->type = type;
	e->param = param;

	if(type == EVENT_TYPE_VIDEO) {
		LockVideo();
	}

	if((i = listAddItem(events, LIST_ITEM_EVENT, e->name, e, sizeof(Event_t))) == NULL) {
		FreeEvent(e);
		e = NULL;
	} else {
		e->id = i->id;
	}

	if(type == EVENT_TYPE_VIDEO) {
		UnlockVideo();
	}
	return e;
}


int RemoveEvent(Event_t *e) {

	int rv = 0;

	if(e->type == EVENT_TYPE_VIDEO) {
		LockVideo();
	}
	Item_t *i = listGetItemById(events, e->id);
	
	if(!i) 
		rv = -1;
	else
		listRemoveItem(events, i, (listFreeItemFunc *) FreeEvent);

	if(e->type == EVENT_TYPE_VIDEO) {
		UnlockVideo();
	}
	return rv;
}


int SetEventState(Event_t *e, uint16 state) {
	if(e->type == EVENT_TYPE_VIDEO) {
		LockVideo();
	}

	e->state = state;

	if(e->type == EVENT_TYPE_VIDEO) {
		UnlockVideo();
	}
	return state;
}


void ProcessInputEvents(SDL_Event *event) {
	
	Event_t *e;
	Item_t *i;

	SLIST_FOREACH(i, events, list) {
		e = (Event_t *) i->data;

		if(e->type == EVENT_TYPE_INPUT && e->state == EVENT_STATE_ACTIVE && e->event != NULL) {
			e->event(e, event, EVENT_ACTION_UPDATE);
		}
	}
}


void ProcessVideoEventsRender() {

	Event_t *e;
	Item_t *i;

	SLIST_FOREACH(i, events, list) {

		e = (Event_t *) i->data;

		if(e->type == EVENT_TYPE_VIDEO && e->state == EVENT_STATE_ACTIVE) {
			e->event(e, e->param, EVENT_ACTION_RENDER);
		}
	}
}

void ProcessVideoEventsUpdate(VideoEventUpdate_t *area) {

	Event_t *e;
	Item_t *i;

	SLIST_FOREACH(i, events, list) {

		e = (Event_t *) i->data;

		if(e->type == EVENT_TYPE_VIDEO && e->state == EVENT_STATE_ACTIVE) {
			e->event(e, area, EVENT_ACTION_UPDATE);
		}
	}
}
