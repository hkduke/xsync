/**
 * spinlock.h
 *
 * for MSVC and GNUC
 * 
 * created by cheungmine
 * 2013-4-24
 */
#ifndef _SPIN_LOCK_H_INCLUDED
#define _SPIN_LOCK_H_INCLUDED

typedef struct
{
  volatile long  flag_;
  volatile long *spin_;
} spin_lock_t;

void spin_init(spin_lock_t *lock, long *flag);
void spin_lock(spin_lock_t *lock);
int spin_trylock(spin_lock_t *lock);
void spin_unlock(spin_lock_t *lock);
int spin_is_lock(spin_lock_t *lock);

#endif /* _SPIN_LOCK_H_INCLUDED */
