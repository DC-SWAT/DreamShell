/****************************
 * DreamShell ##version##   *
 * events.c                 *
 * DreamShell events        *
 * Created by SWAT          *
 * http://www.dc-swat.ru    *
 ***************************/


#include "ds.h"
#include "video.h"
#include "events.h"
#include "console.h"

static Item_list_t *events;
static mutex_t event_mutex = MUTEX_INITIALIZER;


static void FreeEvent(void *event) {
    free(event);
}


int InitEvents() {

	if((events = listMake()) == NULL) {
		return -1;
	}

	mutex_init((mutex_t *)&event_mutex, MUTEX_TYPE_NORMAL);
	return 0;
}

void ShutdownEvents() {
	mutex_lock(&event_mutex);
	listDestroy(events, (listFreeItemFunc *) FreeEvent);
	mutex_unlock(&event_mutex);
	mutex_destroy(&event_mutex);
}


Item_list_t *GetEventList() {
	return events;
}


Event_t *GetEventById(uint32 id) {
    
	mutex_lock(&event_mutex);
	Item_t *i = listGetItemById(events, id);
	mutex_unlock(&event_mutex);

	if(i != NULL) {
		return (Event_t *) i->data;
	} else {
		return NULL;
	}
}


Event_t *GetEventByName(const char *name) {
    
	mutex_lock(&event_mutex);
	Item_t *i = listGetItemByName(events, name);
	mutex_unlock(&event_mutex);

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

	mutex_lock(&event_mutex);

	if((i = listAddItem(events, LIST_ITEM_EVENT, e->name, e, sizeof(Event_t))) == NULL) {
		FreeEvent(e);
		e = NULL;
	} else {
		e->id = i->id;
	}
	
	mutex_unlock(&event_mutex);
	return e;
}


int RemoveEvent(Event_t *e) {

	int rv = 0;
	
	mutex_lock(&event_mutex);
	Item_t *i = listGetItemById(events, e->id);
	
	if(!i) 
		rv = -1;
	else
		listRemoveItem(events, i, (listFreeItemFunc *) FreeEvent);
	
	mutex_unlock(&event_mutex);
	return rv;
}


int SetEventState(Event_t *e, uint16 state) {

//	mutex_lock(&event_mutex);
	e->state = state;
//	mutex_unlock(&event_mutex);
	
	return state;
}


void ProcessInputEvents(SDL_Event *event) {
	
	Event_t *e;
	Item_t *i;
	
	mutex_lock(&event_mutex);
	SLIST_FOREACH(i, events, list) {
		e = (Event_t *) i->data;

		if(e->type == EVENT_TYPE_INPUT && e->state == EVENT_STATE_ACTIVE && e->event != NULL) {
			e->event(e, event, EVENT_ACTION_UPDATE);
		}
	}
	mutex_unlock(&event_mutex);
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
	mutex_unlock(&event_mutex);
}

void ProcessVideoEventsUpdate(VideoEventUpdate_t *area) {
    
	Event_t *e;
	Item_t *i;
	
	mutex_lock(&event_mutex);
	SLIST_FOREACH(i, events, list) {

		e = (Event_t *) i->data;

		if(e->type == EVENT_TYPE_VIDEO && e->state == EVENT_STATE_ACTIVE) {
			e->event(e, area, EVENT_ACTION_UPDATE);
		}
	}
	mutex_unlock(&event_mutex);
}
