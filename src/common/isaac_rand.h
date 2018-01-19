/**
* isaac_rand.h
*   definitions for a random number generator
* -----------------------------------------------------------------------------
* By Bob Jenkins, 1996, Public Domain
* MODIFIED:
*   960327: Creation (addition of randinit, really)
*   970719: use context, not global variables, for internal state
*   980324: renamed seed to flag
*   980605: recommend RANDSIZL=4 for noncryptography.
*   010626: note this is public domain
* -----------------------------------------------------------------------------
*
* http://burtleburtle.net/bob/rand/isaacafa.html
*
* 2015-01-21: revised by cheungmine
*/
#ifndef ISAAC_RAND_H
#define ISAAC_RAND_H

#ifdef    __cplusplus
extern "C" {
#endif

#ifndef STANDARD
#  include "standard.h"
#endif

#define RANDSIZL   (8)
#define RANDSIZ    (1<<RANDSIZL)

/**
* context of random number generator
*/
struct randctx_ub4
{
    ub4 randcnt;
    ub4 seed[RANDSIZ];
    ub4 mm[RANDSIZ];
    ub4 aa;
    ub4 bb;
    ub4 cc;
};
typedef  struct randctx_ub4  randctx;


/**
* context of random number generator for 64-bits int
*/
struct randctx_ub8
{
    ub8 randcnt;
    ub8 seed[RANDSIZ];
    ub8 mm[RANDSIZ];
    ub8 aa;
    ub8 bb;
    ub8 cc;
};
typedef  struct randctx_ub8  randctx64;


/**
* randinit
*   init rand seed
*/
extern void randctx_init(randctx *r, word time_as_seed);

extern void randctx64_init(randctx64 *r, word time_as_seed);

/**
* rand
*   Call rand(randctx *) to retrieve a single 32-bit random value.
*/
extern ub4 isaac_rand(randctx *r);

extern ub4 isaac_randint(randctx *r, ub4 rmin, ub4 rmax);

extern ub8 isaac_rand64(randctx64 *r);

extern ub8 isaac_randint64(randctx64 *r, ub8 rmin, ub8 rmax);


#ifdef    __cplusplus
}
#endif

#endif  /* ISAAC_RAND_H */
