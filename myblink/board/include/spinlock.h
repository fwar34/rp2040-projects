#ifndef __LOCK_H_
#define __LOCK_H_

#include <inttypes.h>
#include <stdbool.h>

#define LOCK_INIT_VALUE 0
typedef int32_t SpinLock;

int cas(volatile int32_t *ptr, int32_t old, int32_t new);
void init_lock(volatile SpinLock *lock);
bool try_accquire_lock(volatile SpinLock *lock);
bool accquire_spinlock(volatile SpinLock *lock, uint32_t ms);
void release_spinlock(volatile SpinLock *lock);

#endif
