#include <stdlib.h>
#include <string.h>
#include <sys/queue.h>

#include "aica_common.h"

struct Handler {
	aica_funcp_t handler;
	const char *funcname;
	unsigned int id;
	SLIST_ENTRY(Handler) next;
};

static SLIST_HEAD(Head, Handler) head = SLIST_HEAD_INITIALIZER(head);

void __aica_share(aica_funcp_t func,
			const char *funcname, size_t sz_in, size_t sz_out)
{
	unsigned int id;
	struct Handler *hdl = malloc(sizeof(*hdl));

	if (SLIST_FIRST(&head) == NULL)
		id = 0;
	else
		id = SLIST_FIRST(&head)->id + 1;

	struct function_params fparams = {
		{ sz_in, sz_in ? malloc(sz_in) : NULL, },
		{ sz_out, sz_out ? malloc(sz_out) : NULL, },
		FUNCTION_CALL_AVAIL,
	};

	aica_update_fparams_table(id, &fparams);

	hdl->id = id;
	hdl->handler = func;
	hdl->funcname = funcname;

	SLIST_INSERT_HEAD(&head, hdl, next);
}


int aica_clear_handler(unsigned int id)
{
	struct Handler *hdl;

	SLIST_FOREACH(hdl, &head, next) {
		if (hdl->id == id) {
			SLIST_REMOVE(&head, hdl, Handler, next);
			free(hdl);
			return 0;
		}
	}

	return -1;
}

void aica_clear_handler_table(void)
{
	struct Handler *hdl;

	while (1) {
		hdl = SLIST_FIRST(&head);
		if (!hdl)
			return;

		SLIST_REMOVE_HEAD(&head, next);
		free(hdl);
	}
}

int aica_find_id(unsigned int *id, char *funcname)
{
	struct Handler *hdl;

	SLIST_FOREACH(hdl, &head, next) {
		if (strcmp(hdl->funcname, funcname) == 0) {
			*id = hdl->id;
			return 0;
		}
	}

	return -EAGAIN;
}

aica_funcp_t aica_get_func_from_id(unsigned int id)
{
	struct Handler *hdl;

	SLIST_FOREACH(hdl, &head, next) {
		if (hdl->id == id)
			return hdl->handler;
	}

	return NULL;
}

const char * aica_get_funcname_from_id(unsigned int id)
{
	struct Handler *hdl;

	SLIST_FOREACH(hdl, &head, next) {
		if (hdl->id == id)
		  return hdl->funcname;
	}

	return NULL;
}
