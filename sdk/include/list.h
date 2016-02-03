/** 
 * \file    list.h
 * \brief   DreamShell lists
 * \date    2007-2014
 * \author  SWAT www.dc-swat.ru
 */

#ifndef _DS_LIST_H
#define _DS_LIST_H


#include <kos.h>
#include <stdlib.h>


/** 
 * Определенные типы данных элемента, которые есть в DreamShell. Вы можете добавлять свои. 
 * Если вы используете список, где данные элемента не подходят не под один существующий тип, 
 * то можно использовать универсальный тип LIST_ITEM_USERDATA.
 */

typedef enum {
	
	LIST_ITEM_USERDATA = 0,
	LIST_ITEM_SDL_SURFACE,
	LIST_ITEM_SDL_RWOPS,

	LIST_ITEM_GUI_SURFACE,
	LIST_ITEM_GUI_FONT,
	LIST_ITEM_GUI_WIDGET,

	LIST_ITEM_MODULE,
	LIST_ITEM_APP,
	LIST_ITEM_EVENT,
	LIST_ITEM_THREAD,
	LIST_ITEM_CMD,
	LIST_ITEM_LUA_LIB,
	
	LIST_ITEM_XML_NODE
	
} ListItemType;


/* Структура элемента списка */

typedef struct Item {

        /* Точка входа SLIST, только для внутреннего использования. */
        SLIST_ENTRY(Item) list; 
        
        /* Имя элемента списка */
        const char *name;
        
        /* ID элемента списка */
        uint32 id;
        
        /* Тип элемента списка */
        ListItemType type;
        
        /* Данные элемента списка */
        void *data;
		
		/* Размер данных (не обязателен) */
		uint32 size;
   
} Item_t;


/**
 * Определение типа для списка. Используется SLIST из библиотеки newlib
 */
typedef SLIST_HEAD(ItemList, Item) Item_list_t;

/**
 * Определение типа функции, освобождающей память, занимаемую данными элемента.
 */
typedef void listFreeItemFunc(void *);
	

/**
 * Создает список и возвращает указатель на него. Возвращает NULL в случае не удачи.
 */
Item_list_t *listMake();
#define listNew listMake;

/**
 * Удаляет все элементы списка и сам список, аргумент ifree является функцией, для освобождения памяти, которую занимают данные элемента. 
 * В основном подходит функция free, она же используется там по умолчанию, если указать вместо аргумента ifree - NULL
 */
void listDestroy(Item_list_t *lst, listFreeItemFunc *ifree);

/**
 * Получает ID последнего элемента (существующего или уже нет) из списка. 
 */
uint32 listGetLastId(Item_list_t *lst);

/**
 * Добавляет элемент в список и возвращает ссылку на него при успехе, а при ошибке возвращает NULL.
 */
Item_t *listAddItem(Item_list_t *lst, ListItemType type, const char *name, void *data, uint32 size);

/**
 * Удаляет элемент из списка.
 */
void listRemoveItem(Item_list_t *lst, Item_t *i, listFreeItemFunc *ifree);

/**
 * Ищет элемент в списке по его имени. 
 * Если элемент найден, возвращается ссылка на него, если нет, то возвращает NULL.
 */
Item_t *listGetItemByName(Item_list_t *lst, const char *name);

/**
 * Ищет элемент в списке по типу. 
 * Если элемент найден, возвращается ссылка на него, если нет, то возвращает NULL.
 */
Item_t *listGetItemByType(Item_list_t *lst, ListItemType type);

/**
 * Ищет элемент в списке по его имени и типу. 
 * Если элемент найден, возвращается ссылка на него, если нет, то возвращает NULL.
 */
Item_t *listGetItemByNameAndType(Item_list_t *lst, const char *name, ListItemType type);


/**
 * Ищет элемент в списке по его ID. 
 * Если элемент найден, возвращается ссылка на него, если же нет, то возвращает NULL.
 */
Item_t *listGetItemById(Item_list_t *lst, uint32 id);

/**
 * Возвращает первый элемент списка.
 */
Item_t *listGetItemFirst(Item_list_t *lst);

/**
 * Возвращает следующий элемент списка, находящийся после элемента переданного аргументом i.
 */
Item_t *listGetItemNext(Item_t *i);

/**
 * Примечание:
 * Функции listGetItemFirst и listGetItemNext служат для создания циклической обработки списков.
 */

#endif
