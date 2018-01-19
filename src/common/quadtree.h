/*
 * quadtree.h
 *		Quad tree structure -- for spatial quick searching
 *		cheungmine 
 *      Oct. 5, 2007.  All rights reserved.
 */
#ifndef QUADTREE_H_INCLUDED
#define QUADTREE_H_INCLUDED

#ifdef __cplusplus
extern "C" {
#endif

#include "unitype.h"

#include "list.h"

#define QUAD_SUBNODES		4

#define QBOX_OVERLAP_MAX	0.4
#define QBOX_OVERLAP_MIN	0.02

#define QTREE_DEPTH_MAX		8
#define QTREE_DEPTH_MIN		4

#define QUADRANT_BITS		3

#ifndef BOOL
  typedef int BOOL;
#endif

/* a quadrant defined below:
		  
		NW(1)   |	NE(0)
	 -----------|-----------
	    SW(2)   |	SE(3)
*/
typedef enum
{
	NE = 0,
	NW = 1,
	SW = 2,
	SE = 3
}QuadrantEnum;

/* a box defined below:
		   _____max
		  |__|__|
		  |  |  |
		 min-----
*/
typedef struct _quadbox_t
{	
	double	_xmin, 
			_ymin, 
			_xmax, 
			_ymax;
}quadbox_t;

/* quad node */
typedef struct _quadnode_t
{
	quadbox_t			 _box;						/* node bound box */
	list_t				*_lst;						/* node data list */
	struct _quadnode_t	*_sub[QUAD_SUBNODES];		/* pointer to subnodes of this node */
}quadnode_t;

/* quad tree */
typedef struct _quadtree_t
{
	quadnode_t		*_root;
	int				 _depth;				   /* max depth of tree: 0-based */
	float			 _overlap;				   /* overlapped ratio of quanbox */	
}quadtree_t;


/*=============================================================================
						Public Functions
=============================================================================*/
/* creates a quadtree and returns a pointer to it */
quadtree_t* quadtree_create (
				 quadbox_t	box, 
				 int		depth,  /* 4~8 */
				 float		overlap /* 0.02 ~ 0.4 */
				 );

/* destroys a quad tree and free all memory */
void quadtree_destroy (IN  quadtree_t		*qtree
				  );

/* inserts a node identified by node_key into a quadtree, returns the node quadtree encoding */
quadnode_t *quadtree_insert (
				 IN  quadtree_t			*qtree, 
			     IN  long				 node_key, 
				 IN  quadbox_t			*node_box
				 );
 
/* searches nodes inside search_box */
void quadtree_search (
				 IN  const quadtree_t	*qtree, 
 			     IN  quadbox_t			*search_box, 
			     OUT list_t				*results_list
				 );

#ifdef __cplusplus
}
#endif

#endif // QUADTREE_H_INCLUDED