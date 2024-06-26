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


Event_t *AddEvent(const char *name, int type, int prio, Event_func *event, void *param) {

	Event_t *e;
	Item_t *i;

	if(!event || !name || GetEventByName(name)) {
		return NULL;
	}

	e = (Event_t *)calloc(1, sizeof(Event_t)); 
	if(e == NULL) return NULL;

	e->name = name;
	e->event = event;
	e->active = 1;
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


int SetEventActive(Event_t *e, int is_active) {
	if(e->type == EVENT_TYPE_VIDEO) {
		LockVideo();
	}

	e->active = is_active;

	if(e->type == EVENT_TYPE_VIDEO) {
		UnlockVideo();
	}
	return is_active;
}


void ProcessInputEvents(SDL_Event *event) {

	Event_t *e;
	Item_t *c, *n;

	c = listGetItemFirst(events);

	while(c != NULL) {
		n = listGetItemNext(c);
		e = (Event_t *)c->data;

		if(e->type == EVENT_TYPE_INPUT && e->active) {
			e->event(e, event, EVENT_ACTION_UPDATE);
		}

		c = n;
	}
}


void ProcessVideoEventsRender() {

	Event_t *e;
	Item_t *i;

	SLIST_FOREACH(i, events, list) {

		e = (Event_t *) i->data;

		if(e->type == EVENT_TYPE_VIDEO && e->active && e->prio == EVENT_PRIO_DEFAULT) {
			e->event(e, e->param, EVENT_ACTION_RENDER);
		}
	}

	SLIST_FOREACH(i, events, list) {

		e = (Event_t *) i->data;

		if(e->type == EVENT_TYPE_VIDEO && e->active && e->prio == EVENT_PRIO_OVERLAY
		) {
			e->event(e, e->param, EVENT_ACTION_RENDER);
		}
	}
}

void ProcessVideoEventsRenderPost() {

	Event_t *e;
	Item_t *i;

	SLIST_FOREACH(i, events, list) {

		e = (Event_t *) i->data;

		if(e->type == EVENT_TYPE_VIDEO && e->active && e->prio == EVENT_PRIO_DEFAULT) {
			e->event(e, e->param, EVENT_ACTION_RENDER_POST);
		}
	}

	SLIST_FOREACH(i, events, list) {

		e = (Event_t *) i->data;

		if(e->type == EVENT_TYPE_VIDEO && e->active && e->prio == EVENT_PRIO_OVERLAY) {
			e->event(e, e->param, EVENT_ACTION_RENDER_POST);
		}
	}
}


void ProcessVideoEventsUpdate(VideoEventUpdate_t *area) {

	Event_t *e;
	Item_t *i;

	LockVideo();

	SLIST_FOREACH(i, events, list) {

		e = (Event_t *) i->data;

		if(e->type == EVENT_TYPE_VIDEO && e->active && e->prio == EVENT_PRIO_DEFAULT) {
			e->event(e, e->param, EVENT_ACTION_UPDATE);
		}
	}

	SLIST_FOREACH(i, events, list) {

		e = (Event_t *) i->data;

		if(e->type == EVENT_TYPE_VIDEO && e->active && e->prio == EVENT_PRIO_OVERLAY) {
			e->event(e, e->param, EVENT_ACTION_UPDATE);
		}
	}

	UnlockVideo();
}
