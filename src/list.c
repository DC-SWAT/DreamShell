/****************************
 * DreamShell ##version##   *
 * list.c                   *
 * DreamShell list manager  *
 * Created by SWAT          *
 * http://www.dc-swat.ru    *
 ***************************/

#include <kos.h>
#include <stdlib.h>
#include "list.h"


Item_list_t *listMake() {
    
    Item_list_t *l;
    
    l = (Item_list_t *) calloc(1, sizeof(Item_list_t)); 
	
    if(l == NULL) 
		return NULL;
    
    SLIST_INIT(l);
    return l;
}


void listDestroy(Item_list_t *lst, listFreeItemFunc *ifree) {
 
    Item_t *c, *n;
    
    c = SLIST_FIRST(lst); 
    
    while(c) {
        n = SLIST_NEXT(c, list); 
		
        if(ifree != NULL) 
			ifree(c->data); 
			
        free(c);
        c = n; 
    }
              
    SLIST_INIT(lst); 
    free(lst);
}

static uint32 listLastId = 0;


uint32 listGetLastId(Item_list_t *lst) {
	return listLastId; 
}


Item_t *listAddItem(Item_list_t *lst, ListItemType type, const char *name, void *data, uint32 size) {

    Item_t *i = NULL;
    
    i = (Item_t *) calloc(1, sizeof(Item_t)); 
	
    if(i == NULL) 
		return NULL;
    
	i->name = name;
	i->type = type;
	i->id = ++listLastId;
	i->data = data;
	i->size = size;
    
    //printf("List added item with id=%d\n", i->id);
    
    SLIST_INSERT_HEAD(lst, i, list); 

    return i; 
}


void listRemoveItem(Item_list_t *lst, Item_t *i, listFreeItemFunc *ifree) {

    SLIST_REMOVE(lst, i, Item, list); 
	
    if(ifree != NULL) 
		ifree(i->data);
		
    free(i);
}


Item_t *listGetItemByName(Item_list_t *lst, const char *name) {

    Item_t *i;
    
    SLIST_FOREACH(i, lst, list) {
       if(!strcasecmp(name, i->name)) 
		   return i;
    } 
    
    return NULL; 
}

Item_t *listGetItemByNameAndType(Item_list_t *lst, const char *name, ListItemType type) {

    Item_t *i;
    
    SLIST_FOREACH(i, lst, list) {
       if(i->type == type && !strcmp(name, i->name)) 
		   return i;
    } 
    
    return NULL; 
}

Item_t *listGetItemByType(Item_list_t *lst, ListItemType type) {

    Item_t *i;
    
    SLIST_FOREACH(i, lst, list) {
       if(i->type == type) 
		   return i;
    } 
    
    return NULL; 
}


Item_t *listGetItemById(Item_list_t *lst, uint32 id) {

    Item_t *i;
    
    SLIST_FOREACH(i, lst, list) {
       if(id == i->id) 
		   return i;
    } 
    
    return NULL; 
}


Item_t *listGetItemFirst(Item_list_t *lst) {
    return SLIST_FIRST(lst); 
}


Item_t *listGetItemNext(Item_t *i) {
    return SLIST_NEXT(i, list); 
}

