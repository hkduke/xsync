/*******************************************************************************
 * dlist.h                                                                     *
 *		Generic double circular linked list - can hold any type data.          *
 *		cheungmine                                                             *
 *      Mar. 22, 2008.  All Rights Reserved.                                   *
 *                                                                             *
 *                ...........................................                  *
 *     dlist     |   ______    ______    ______    ______    |                 *
 *   _________   +->| node |  | node |  | node |  | node |   |                 *
 *  |size/head|---->|  next+->|  next+->|  next+->|  next+----                 *
 *   ---------    --+prev  |<-+prev  |<-+prev  |<-+prev  |<--                  *
 *               |   ------    ------    ------    ------    |                 *
 *               | front/head                     back/tail  |                 *
 *               |             begin      end                |                 *
 *               |...........................................|                 *
 *                                                                             *
 ******************************************************************************/
#ifndef DLIST_H_INCLUDED__
#define DLIST_H_INCLUDED__

#ifdef __cplusplus
extern "C" {
#endif

#include "unitype.h"

#include <malloc.h>

/**
 * dlist node type
 */
typedef struct _dlistnode_t
{
	  struct _dlistnode_t	*_next;
    struct _dlistnode_t	*_prev;

	  union{
		    void            *data;
		    struct _dlist_t	*list;
		    char		        *str;
		    int              val;
	  };
}dlistnode_t, *dlistnode_p;


/**
 * dlist type
 */
typedef struct _dlist_t
{
	size_t		  _size;	 /* count of all nodes */
	dlistnode_t	 *_head; /* point to the front node of list */
}dlist_t, *dlist_p;


/**
 * A prototype of callback function called by dlist_traverse:
 * 	Returns:
 *    1 - continue to next node
 *	  0 - successfuly returned
 *   < 0 - bad occurred, return user defined error code
 *   others - stop and returned
 */
typedef int(*pfcb_dlist_traverse_node)(dlistnode_t* node, void *param);

/**
 * Free node callback function prototype
 */
typedef void(*pfcb_dlist_free_node)(dlistnode_t* node, void *param);

/**
 * Compare 2 nodes callback function prototype
 * Returns:
 *    0: a = b
 *    1: a > b
 *   -1: a < b
 */
typedef int(*pfcb_dlist_compare_nodes)(dlistnode_t* node_a, dlistnode_t* node_b, void *param);

/**
 * A prototype example of free node data function implemented by caller:
 */
static void my_dlist_node_free(dlistnode_t *node, void *param)
{
#ifdef _DEBUG
    printf("    free node: %d\n", node->val);
#endif

    /* free(node->data); */
}

/**
 * A prototype of callback function called by dlist_traverse:
 */
static int my_dlist_node_printf(dlistnode_t* node, void *param)
{
    printf("%d\n", node->val);
    return 1;
}

static int my_dlist_sort_nodes_descend(dlistnode_t* node_a, dlistnode_t* node_b, void *param)
{
    if (node_a->val > node_b->val)
        return -1;
    else if(node_a->val < node_b->val)
        return 1;
    else
        return 0;
}

static int my_dlist_sort_nodes_ascend(dlistnode_t* node_a, dlistnode_t* node_b, void *param)
{
    if (node_a->val > node_b->val)
        return 1;
    else if(node_a->val < node_b->val)
        return -1;
    else
        return 0;
}
/*===============================================================================
 *                                                                              *
 *                           dlist public functions                             *
 *                                                                              *
 *==============================================================================*/
/**
 * Allocates a empty list from heap, this creates a new list 
 */
dlist_t* dlist_create();


/** 
 * Clears a list and free all memory, the list cannot be used later
 * Since the nodes data are allocated by caller, so caller MUST provide a callback 
 * function to free the node.
 */
void dlist_destroy(dlist_t *lst, pfcb_dlist_free_node  pfcbFreeNodeData, void *param);


/**
 * Removes all nodes except for list itself 
 */
void dlist_clear(dlist_t *lst, pfcb_dlist_free_node pfcbFreeNodeData, void *param);


/**
 * Traverses a list, applied callback functionn for each node. So pfcbTraverseNode MUST be valid.
 * If tail_to_head is bool_true, traverse is make from tail to head otherwise is from head to tail
 */
int dlist_traverse(dlist_t *lst, pfcb_dlist_traverse_node pfcbTraverseNode, void *param, bool_t tail_to_head);


/** 
 * Creates a new node assigned with data, not allocates for data
 */
dlistnode_t* dlist_node_create(void* data);

dlistnode_t* dlist_node_create_key(int key);

/** 
 * Free the single node.
 */
void dlist_node_free(dlistnode_t* node, pfcb_dlist_free_node  pfcbFreeNodeData, void *param);


/** 
 * Free the nodes from begin to end. If end node is 0, only free the begin node
 */
void dlist_nodes_free(dlistnode_t* begin_node, dlistnode_t* end_node, pfcb_dlist_free_node  pfcbFreeNodeData, void *param);


/** 
 * Calculate size of nodes from begin to end
 */
size_t dlist_nodes_size(dlistnode_t* begin_node, dlistnode_t* end_node);


/** 
 * Get head node of a list
 */
dlistnode_t* dlist_head_node(dlist_t *lst);


/** 
 * Get tail node of a list
 */
dlistnode_t* dlist_tail_node(dlist_t *lst);


/** 
 * Gets node by index: 0-based. 0 is head, -1 is tail.
 * If index isnot a valid index (==-1 || >=0 && <size), return 0
 */
dlistnode_t* dlist_node_at(const dlist_t* lst, size_t index);


/** 
 * Appends a node to the back of list
 */
void dlist_push_back(dlist_t *lst, dlistnode_t *back_node);


/** 
 * Inserts a node as head of list 
 */
void dlist_push_front(dlist_t *lst, dlistnode_t *front_node);


/** 
 * Removes the front node from a list and returns the node removed
 */
dlistnode_t* dlist_pop_front(dlist_t *lst);


/** 
 * Removes the back node from a list and returns the node removed
 */
dlistnode_t* dlist_pop_back(dlist_t *lst);


/**
 * Inserts a node after the node into a list 
 */
bool_t dlist_insert_after(dlist_t *lst, dlistnode_t *prev_node, dlistnode_t *ins_node);


/**
 * Inserts a node before the node into a list 
 */
bool_t dlist_insert_before(dlist_t *lst, dlistnode_t *back_node, dlistnode_t *ins_node);


/** 
 * Cut nodes from begin node to end node off the list, returns the node prior to 
 * the begin node cut off.
 * Caller should free the nodes lead by begin node.
 * When the begin and end nodes are the same one, it will only cut off that.
 * When return 0 indicates list is empty (size==0)
 * It can be explained that why not providing the way to cut off a single node here.
 * NOTE: begin node must be prior to end node in sequence. 
 *
 *    dlistnode_t  *begin = dlist_node_at(lst, 2);
 *    dlistnode_t  *end   = dlist_node_at(lst, 5);
 *    dlistnode_t  *node = dlist_slice(lst, begin, end);
 *    dlist_nodes_free(begin, end, 0, 0);
 *
 * YOU CAN NOT APPLY dlist_nodes_free TO NODE WHICH IS NOT SLICED FROM LIST
 * The following snippet is ILLEGITIMATE:
 *
 *    dlistnode_t  *begin = dlist_node_at(lst, 2);
 *    dlist_nodes_free(begin, begin, 0, 0);
 *
 * While the following snippet is correct:
 *    dlistnode_t  *begin = dlist_node_at(lst, 2);
 *    dlist_slice(lst, begin, 0);
 *    dlist_nodes_free(begin, begin, 0, 0);
 */
dlistnode_t* dlist_slice(dlist_t* lst, dlistnode_t* begin, dlistnode_t* end);


/** 
 * Gets count of nodes in the list 
 */
size_t dlist_size(const dlist_t* lst);


/**
 * Appends the second list to the first list
 */
void dlist_concat(dlist_t *first, dlist_t *second);


/**
 * Reverse list. For example the initial nodes are:0,1,2,3, 
 * after dlist_reverse will be: 3,2,1,0
 */
void dlist_reverse(dlist_t* lst);


/**
 * Swap two nodes in list: front node must preceding the back node, only the situations below can be accepted:
 *   1) a==b
 *   2) a->b
 *   3) a->...->b
 * NOTE:
 *   b->a or b->...->a is unaccepted, that will cause an exception
 */
void dlist_swap_nodes(dlist_t* lst, dlistnode_t *ahead, dlistnode_t *back);


/**
 * Bubble sort list nodes by ascend: 0,1,2,3,...,9
 */
void dlist_bubble_sort(dlist_t* lst, pfcb_dlist_compare_nodes  pfcbCompareNodes, void *param);


/**
 * Merge sort src list to dst list. Caller can free src after merging.
 * dst and src lists MUST be sorted by the same dlist_compare_callback as dlist_merge_sort
 */
void dlist_merge_sort(dlist_t* dst, dlist_t* src, pfcb_dlist_compare_nodes  pfcbCompareNodes, void *param);

#ifdef __cplusplus
}
#endif

#endif  /* DLIST_H_INCLUDED__ */
