/****************************************************************************************
 * dlist.c                                                                               *
 *		Generic sequential linked list node structure -- can hold any type data.        *
 *		cheungmine                                                                      *
 *      May. 22, 2008.  All rights reserved.                                            *
 ****************************************************************************************/
#include "dlist.h"

#include <assert.h>

#define DLIST_USE_TAIL_NODE(lst)     (lst->_head->_prev)
#define DLIST_IS_HEAD_NODE(lst, node)     (node==lst->_head)
#define DLIST_IS_TAIL_NODE(lst, node)     (node->_next==lst->_head)


/*===============================================================================
 *                                                                              *
 *                           dlist public functions                             *
 *                                                                              *
 *==============================================================================*/
dlist_t* dlist_create()
{
    dlist_t	*dl = (dlist_t*) malloc (sizeof(dlist_t));
    assert(dl);
	dl->_size = 0;
	dl->_head = 0;
	return dl;
}


void dlist_destroy(dlist_t *lst, pfcb_dlist_free_node  pfcbFreeNodeData, void *param)
{
    dlist_clear(lst, pfcbFreeNodeData, param);	
    free(lst);
}


void dlist_clear(dlist_t *lst, pfcb_dlist_free_node pfcbFreeNodeData, void *param)
{
    dlistnode_t	*node;

    if (pfcbFreeNodeData) {
        while((node = dlist_pop_front(lst)) != 0){
            (*pfcbFreeNodeData)(node, param);
            free(node);
        }
    }
    else {
        while((node = dlist_pop_front(lst)) != 0){
            free(node);
        }
    }
    assert (lst->_head==0 && lst->_size==0);
}

int dlist_traverse(dlist_t *lst, pfcb_dlist_traverse_node  pfcbTraverseNode, void *param, bool_t tail_to_head)
{
    int ret = 0;
    dlistnode_t* node;
    assert(pfcbTraverseNode);

    if (!lst->_head)
        return -1;      /* bad list */

    if (tail_to_head) {
		node = DLIST_USE_TAIL_NODE(lst);
		while (node) {
            ret = (*pfcbTraverseNode)(node, param);            
            if (ret != 1)
                return ret;
            if (DLIST_IS_HEAD_NODE(lst, node))
                break;
            node = node->_prev;            
		}
	}
    else {
        node = lst->_head;
		while (node) {
			ret = (*pfcbTraverseNode)(node, param);
            if (ret != 1)
                return ret;
            if (DLIST_IS_TAIL_NODE(lst, node))
                break;
            node = node->_next;           
		}
    }

    return 0;
}

dlistnode_t* dlist_pop_front(dlist_t *lst)
{
    dlistnode_t	*pop_node = 0;
	
    if (lst->_head)
	{
        if (lst->_size==1){
            pop_node = lst->_head;
            pop_node->_next = pop_node->_prev = 0;
            lst->_size = 0;
            lst->_head = 0;
        }
        else{
            assert(lst->_size>1);
            pop_node = lst->_head;
            lst->_head = pop_node->_next;
            DLIST_USE_TAIL_NODE(lst) = pop_node->_prev;
            DLIST_USE_TAIL_NODE(lst)->_next = lst->_head;

            pop_node->_next = pop_node->_prev = 0;
            lst->_size--;
        }
    }

    assert(lst->_size >= 0);
	return pop_node;
}


dlistnode_t* dlist_pop_back(dlist_t *lst)
{
    dlistnode_t	*pop_node = 0;
	
    if (lst->_head)
	{
        if (lst->_size==1){
            pop_node = lst->_head;
            pop_node->_next = pop_node->_prev = 0;
            lst->_size = 0;
            lst->_head = 0;
        }
        else{
            assert(lst->_size>1);
            pop_node = DLIST_USE_TAIL_NODE(lst);
            DLIST_USE_TAIL_NODE(lst) = pop_node->_prev;
            assert(pop_node->_next==lst->_head);
            pop_node->_prev->_next = pop_node->_next;

            pop_node->_next = pop_node->_prev = 0;
            lst->_size--;
        }
    }
    assert(lst->_size >= 0);
	return pop_node;
}


dlistnode_t* dlist_node_create(void* data)
{
    dlistnode_t	*node = (dlistnode_t*) malloc (sizeof(dlistnode_t));
    assert(node);
	node->_next = node->_prev = 0;
	node->data = data;
	return node;
}

dlistnode_t* dlist_node_create_key(int key)
{
    dlistnode_t	*node = (dlistnode_t*) malloc (sizeof(dlistnode_t));
    assert(node);
	node->_next = node->_prev = 0;
    node->val = key;
	return node;
}

void dlist_push_back(dlist_t *lst, dlistnode_t *back_node)
{
    dlistnode_t *tail;

    if (lst->_head) {
        assert(lst->_size>0);

        tail = DLIST_USE_TAIL_NODE(lst);
        back_node->_prev = tail;
        back_node->_next = tail->_next;
        tail->_next = back_node;
        lst->_head->_prev = back_node;

        lst->_size++;
    }
    else {
        assert(lst->_size==0);
        lst->_head = back_node->_next = back_node->_prev = back_node;
        lst->_size = 1;
    }
}


void dlist_push_front(dlist_t *lst, dlistnode_t *front_node)
{
    if (lst->_head) {
        assert(lst->_size>0);

        front_node->_next = lst->_head;
        DLIST_USE_TAIL_NODE(lst)->_next = front_node;
        front_node->_prev = DLIST_USE_TAIL_NODE(lst);
        lst->_head->_prev = front_node;
        lst->_head = front_node;

        lst->_size++;
    }
    else {
        assert(lst->_size==0);

        lst->_head = front_node->_next = front_node->_prev = front_node;
        lst->_size = 1;
    }
}


void dlist_nodes_free(dlistnode_t* node, dlistnode_t* end_node, pfcb_dlist_free_node  pfcbFreeNodeData, void *param)
{
    dlistnode_t* free_node;

    while (node) {
        free_node = node;
        node = node->_next;
        
        if (pfcbFreeNodeData)
            (*pfcbFreeNodeData) (free_node, param);
        free(free_node);

        if (!end_node)  /* only free node */
            break;

        if (node==end_node) {
            if (pfcbFreeNodeData)
                (*pfcbFreeNodeData) (node, param);
            free(node);            
            break;
        }
    }
}

void dlist_node_free(dlistnode_t* node, pfcb_dlist_free_node  pfcbFreeNodeData, void *param)
{
    if (node) {
        if (pfcbFreeNodeData)
            (*pfcbFreeNodeData) (node, param);

        free(node);
    }
}

size_t dlist_size(const dlist_t* lst)
{
    return lst->_size;
}


dlistnode_t* dlist_node_at(const dlist_t* lst, size_t index)
{
    size_t       i;
	dlistnode_t	*node;
    
    assert(index==-1 || index==0 || index < lst->_size);
    if (!lst->_head || index==0)
        return lst->_head;

    assert(lst->_head);
    if (index==-1)
        return lst->_head->_prev;

    /* bad index */
    if (index >= lst->_size)
        return 0;

    i = 0;

    if (index <= lst->_size/2) {
        /* To search from the head on */
        node = lst->_head;
	    while (i < index) {
		    node = node->_next;
		    i++;
	    }
    }
    else {
        /* It would be much quickly to search from the end on than the head */
        node = DLIST_USE_TAIL_NODE(lst);  /* tail node */
        index = lst->_size - index - 1;
        while (i < index) {
            node = node->_prev;
            i++;
        }
    }

	return node;
}


void dlist_reverse(dlist_t* lst)
{
    dlistnode_t *node;
    dlistnode_t *next;
    dlistnode_t *prev;

    node = lst->_head;

    while (node) {
        next = node->_next;
        prev = node->_prev;

        node->_next = prev;
        node->_prev = next; 

        prev = node;
        node = next;

        if (node==lst->_head) {
            lst->_head = prev;
            break;
        }
    }
}


size_t dlist_nodes_size(dlistnode_t* node, dlistnode_t* end_node)
{
    size_t       n = 0;
    if (node==end_node || !end_node)
        return 1;

    while (node) {
        n++;
        node = node->_next;

        if (node && node==end_node){
            n++;
            break;
        }
    }

    return n;
}


dlistnode_t* dlist_slice(dlist_t* lst, dlistnode_t* begin, dlistnode_t* end)
{
    dlistnode_t  *n1, *n2;   /* ...n1->begin->...->end->n2...*/
    size_t        c = dlist_nodes_size(begin, end);
    if (c==0)
        return lst->_head;

    assert(lst->_head);
    n1 = begin->_prev;
    n2 = end->_next;
    assert(n1 && n2);

    n1->_next = n2;
    n2->_prev = n1;

    /* cut nodes off list */
    begin->_prev = 0;
    end->_next = 0;

    if (lst->_head != begin) {
        lst->_size -= c;
        return n1;
    }
    else {
        assert(lst->_head==begin);
        if (DLIST_USE_TAIL_NODE(lst)==end) {
            lst->_head = 0;
            lst->_size = 0;
            return 0;
        }
        else {
            lst->_size -= c;
            lst->_head = n1;
            return n1;
        }        
    }

    return n1;
}


dlistnode_t* dlist_head_node(dlist_t *lst)
{
    return lst->_head;
}


dlistnode_t* dlist_tail_node(dlist_t *lst)
{
    return DLIST_USE_TAIL_NODE(lst);
}

bool_t dlist_insert_after(dlist_t *lst, dlistnode_t *prev_node, dlistnode_t *ins_node)
{
    assert(lst);

    if (!lst->_head) {
        if (prev_node)
            return bool_false;

        dlist_push_front(lst, ins_node);
        return bool_true;
    }
    else {
        assert(lst->_head);
        if (!prev_node)
            return bool_false;

        lst->_size++;

        ins_node->_prev = prev_node;
        ins_node->_next = prev_node->_next;

        prev_node->_next->_prev = ins_node;
        prev_node->_next = ins_node;

        return bool_true;
    }
}


bool_t dlist_insert_before(dlist_t *lst, dlistnode_t *back_node, dlistnode_t *ins_node)
{
    assert(lst);

    if (!lst->_head) {
        if (back_node)
            return bool_false;

        dlist_push_back(lst, ins_node);
        return bool_true;
    }
    else {
        assert(lst->_head);
        if (!back_node)
            return bool_false;

        if (lst->_head==back_node) {
            dlist_push_front(lst, ins_node);
        }
        else {
            lst->_size++;

            ins_node->_next = back_node;
            ins_node->_prev = back_node->_prev;

            back_node->_prev->_next = ins_node;
            back_node->_prev = ins_node;
        }

        return bool_true;
    }
}


void dlist_concat(dlist_t *first, dlist_t *second)
{
    assert(first && second);

    if (first->_head) {
        if (second->_head) {
            dlistnode_t *second_tail = DLIST_USE_TAIL_NODE(second);

            DLIST_USE_TAIL_NODE(first)->_next = second->_head;
            DLIST_USE_TAIL_NODE(second)->_next = first->_head;
            DLIST_USE_TAIL_NODE(second) = DLIST_USE_TAIL_NODE(first);
            DLIST_USE_TAIL_NODE(first) = second_tail;
            
            first->_size += second->_size;
            second->_head = 0;
            second->_size = 0;
        }
    }
    else {
        assert(!first->_head);
        if (second->_head) {
            first->_head = second->_head;
            first->_size = second->_size;
            second->_head = 0;
            second->_size = 0;
        }
    }
}


void dlist_swap_nodes(dlist_t* lst, dlistnode_t *a, dlistnode_t *b)
{
    dlistnode_t *p, *n;
    assert(a && b);
    
    if (a==b) {
        /* [p]->[a|b]->[n] */
        return;
    }
    else if (a->_next==b && b->_next==a) {
        if (lst->_head==a)
            lst->_head = b;
        else
            lst->_head = a;
    }
    else if (a->_next==b) {
        /* [p]->[a]->[b]->[n] */
        p = a->_prev;
        n = b->_next;

        p->_next = b;
        b->_next = a;
        a->_next = n;

        n->_prev = a;
        a->_prev = b;
        b->_prev = p;

        if (lst->_head==a) {
            lst->_head = b;
        }
    }
    else if (b->_next==a) {
        /* [head:a]->...->[tail:b] */
        p = b->_prev;
        n = a->_next;

        p->_next = a;
        a->_next = b;
        b->_next = n;

        n->_prev = b;
        b->_prev = a;
        a->_prev = p;

        if (lst->_head==a) {
            lst->_head = b;
        }
    }
    else {
        /* [p]->[a]->[a1...b1]->[b]->[n] */
        dlistnode_t *a1, *b1;

        p = a->_prev;
        n = b->_next;
        a1 = a->_next;
        b1 = b->_prev;

        p->_next = b;
        b->_next = a1;
        b1->_next = a;
        a->_next = n;

        a1->_prev = b;
        b->_prev = p;
        n->_prev = a;
        a->_prev = b1;
    }
}

void dlist_bubble_sort(dlist_t* lst, pfcb_dlist_compare_nodes  pfcbCompareNodes, void *param)
{
    int           flag   = 1;
    
    dlistnode_t  *first  = 0;
    dlistnode_t  *second = 0;
    dlistnode_t  *last_sink = 0;

    while (flag==1) {
        flag = 0;

        first = lst->_head;

        while(first) {
            second = first->_next;
            assert(second);

            if (second==lst->_head)
                break;

            if (second==last_sink)
                break;

            if (pfcbCompareNodes) {
                if ((*pfcbCompareNodes)(first, second, param) > 0) {
                    flag = 1;
                    dlist_swap_nodes(lst, first, second);
                    last_sink = first;
                }
                else
                    first = second;
            }
            else {
                if (first->val > second->val) {
                    flag = 1;
                    dlist_swap_nodes(lst, first, second);
                    last_sink = first;
                }
                else
                    first = second;
            }
        }
    }
}


void dlist_merge_sort(dlist_t* dst, dlist_t* src, pfcb_dlist_compare_nodes  pfcbCompareNodes, void *param)
{
    dlistnode_t *dst_node;
    dlistnode_t *src_node;

    assert(dst && src);

    if (!dst->_head) {
        dlist_concat(dst, src);
        return;
    }

    dst_node = dst->_head;
    src_node = src->_head;
        
    while (src_node) {
        if (pfcbCompareNodes) {
            if ((*pfcbCompareNodes)(dst_node, src_node, param) > 0) {
                dlist_insert_before(dst, dst_node, dlist_pop_front(src));
                src_node = src->_head;
            }
            else {
                dst_node = dst_node->_next;

                if (dst_node==dst->_head) {
                    dlist_concat(dst, src);
                    return;
                }
            }
        }
        else {
            if (dst_node->val > src_node->val) {
                dlist_insert_before(dst, dst_node, dlist_pop_front(src));
                src_node = src->_head;
            }
            else {
                dst_node = dst_node->_next;
                
                if (dst_node==dst->_head) {
                    dlist_concat(dst, src);
                    return;
                }
            }
        }            
    }
}
