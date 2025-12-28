#include "spinlock.h"
#include "pico/time.h"
#include "hardware/sync.h"

/*
 * ptr：指向需要进行CAS操作的变量的指针。
 * old：期望的旧值。
 * new：如果ptr指向的值等于old，则将ptr指向的值更新为new。
 * 返回值：如果CAS操作成功（即ptr指向的值等于old并被更新为new），返回1；否则返回0。
 */
int cas(volatile int32_t *ptr, int32_t old, int32_t new)
{
    // 使用Pico SDK提供的原子比较交换函数
    return __atomic_compare_exchange_n(ptr, &old, new, false, __ATOMIC_SEQ_CST, __ATOMIC_SEQ_CST);
}

bool try_accquire_lock(volatile SpinLock *lock)
{
	return cas(lock, 0, 1);
}
bool accquire_spinlock(volatile SpinLock *lock, uint32_t ms)
{
	uint32_t beginTick = to_ms_since_boot(time_us_32());
	while (!try_accquire_lock(lock)) {
//		HAL_Delay(1);
		if (ms != 0 && to_ms_since_boot(time_us_32()) - beginTick >= ms * 1000) {
			return false;
		}
	}
	return true;
}
void release_spinlock(volatile SpinLock *lock)
{
	cas(lock, 1, 0);
}

void init_lock(volatile SpinLock *lock)
{
    *lock = 0;
}
