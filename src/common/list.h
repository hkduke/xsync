/*******************************************************************************
 * list.h                                                                      *
 *                                                                             *
 * Copyright (C)1996-2009 by cheungmine <cheungmine@gmail.com>.                *
 * All rights reserved.                                                        *
 *                                                                             *
 * This program is free  software: you can redistribute it and/or modify       *
 *   it under the terms of the GNU Lesser General Public License as published  *
 *   by the Free Software Foundation, either version 2.1 of the License, or    *
 *   (at your option) any later version.                                       *
 *                                                                             *
 * This program  is  distributed in the hope that it will be useful,           *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of            *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the              *
 *   GNU Lesser General Public License for more details.                       *
 *                                                                             *
 * You should  have received a copy of the GNU Lesser General Public License   *
 *   along with  this program.                                                 *
 ******************************************************************************/
#ifndef LIST_H__
#define LIST_H__

#include <stdio.h>
#include <malloc.h>
#include <assert.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct _listnode_t
{
	struct _listnode_t	*next;

	union{
		void            *data;
		struct _list_t	*list;
		char		    *str;
		int              key;
	};
}listnode_t;

typedef struct _list_t
{
    size_t      size;	/* count of nodes */
	
    listnode_t *head;
	listnode_t *tail;

	listnode_t *curr;	/* cursor ptr for traverse */
}list_t, *list_p;

/**
 * A prototype of callbacked function called by:
 * 	- list_destroy()
 *	- list_node_free()
 *	0 for no use 
 */
typedef void(*pfcb_list_free_node)(listnode_t *node, void *param);

/**
 * A prototype of callback function called by list_traverse:
 * 	Returns:
 *    1 - continue to next node
 *	  0 - successfuly returned
 *   < 0 - bad occurred, return user defined error code
 */
typedef int(*pfcb_list_traverse_node)(listnode_t *node, void *param);	

/**
 * An prototype example of free node data function implemented by caller:
 */
static void my_list_free_node_data(listnode_t *node, void *param)
{
 	free(node->data);
}

/**
 * A prototype example of traverse implemented by caller:
 */
static int my_list_traverse_node_key(listnode_t *node, void *param)
{
    printf("    key=%ld\n", node->key);
    return 1;   /* goto next */
}

/**
 * Traverses a list, applied callback functionn for each node
 */
int list_traverse(list_t *in_list, pfcb_list_traverse_node pfcbTraverseNode, void *param);

/**
 * Allocates a empty list from heap, this creates a new list 
 */
list_t* list_create();

/** 
 * Clears a list and free memory, the list cannot be used later
 */
void list_destroy(list_t *in_list, pfcb_list_free_node  pfcbFreeNode, void *param);

/** 
 * Creates a new node assigned with data, not allocates for data
 */
listnode_t* list_node_create(void* data);

/** 
 * Free a list node and it's associated nodes, the freed node cannot be used later
 */
void list_node_free(listnode_t* node, pfcb_list_free_node  pfcbFreeNode, void *param);

/**
 * Creates a new node assigned with a key, not allocates for key
 */
listnode_t* list_key_create(long key);

/**
 * Finds prev node of given node 
 */
listnode_t* list_find_prev(const list_t *in_list, const listnode_t *node);

/** 
 * Appends a node to a list at back
 */
void list_push_back(list_t *in_list, listnode_t *node);

/** 
 * Inserts a node in front of head into a list 
 */
void list_push_front(list_t *in_list, listnode_t *in_node);

/**
 * Inserts a node after pos node into a list 
 */
void list_insert_after(list_t *in_list, listnode_t *pos_node, listnode_t *in_node);

/**
 * Inserts a node before pos node into a list 
 */
void list_insert_before(list_t *in_list, listnode_t *pos_node, listnode_t *in_node);

/** 
 * Removes the first node from a list and returns it 
 */
listnode_t* list_pop_front(list_t *in_list);

/**
 * Removes the last node from a list and returns it 
 */
listnode_t* list_pop_back(list_t *in_list);

/**
 * Removes all nodes but for list itself 
 */
void list_clear(list_t *in_list, pfcb_list_free_node  pfcbFreeNode, void *param);

/** 
 * Returns a copy of a list_t from heap 
 */
list_t* list_copy(list_t list);

/**
 * Concatenates two lists into first list 
 */
void list_concat(list_t *first, list_t *second);

/** 
 * Gets count of nodes in the list 
 */
size_t list_size(const list_t* in_list);

/** 
 * Gets node by index: 0-based. 0 is head 
 */
listnode_t* list_node_at(const list_t* in_list, size_t index);

/** 
 * Erases node after prev node and returns erased node
 */
listnode_t* list_node_erase(list_t* in_list, listnode_t* prev);

/** 
 * Slices list off from begin to end, returns begin node
 * Caller should free returned list nodes
 * begin and end can be same one
 */
listnode_t* list_slice(list_t* in_list, listnode_t* begin, listnode_t* end);

/**
 * Reverse list
 */
void list_reverse(list_t* in_list);

/**
 * Rewind list cursor ptr
 */
void list_rewind(list_t* in_list);

/**
 * Get next node ptr by cursor
 */
listnode_t* list_next_node(list_t* in_list);

#ifdef __cplusplus
}
#endif

#endif  /* LIST_H__ */
