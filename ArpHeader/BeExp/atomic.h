#ifndef _ATOMIC_H
#define _ATOMIC_H

#include <OS.h>

#if __cplusplus
	extern "C" {
#endif

#if !NOINLINE && !_KERNEL_MODE && __INTEL__
	inline int32 compare_and_swap32(volatile int32 *location, int32 oldValue, int32 newValue)
	{
		int32 success;
		asm volatile("lock; cmpxchg %%ecx, (%%edx); sete %%al; andl $1, %%eax"
			: "=a" (success) : "a" (oldValue), "c" (newValue), "d" (location));
		return success;
	}
	
#else
	int32 compare_and_swap32(volatile int32 *location, int32 oldValue, int32 newValue);
#endif

#if 0
	inline int32 compare_and_swap64(volatile int64 *location, int64 oldValue, int64 newValue)
	{
		int32 success;
		int dummy;
		asm volatile("lock; cmpxchg8b (%%edi); sete %%al; andl $1, %%eax"
			: "=a" (success) : "a" ((unsigned) oldValue), "b" ((unsigned) newValue),
				"c" ((unsigned)(newValue >> 32)), "d" ((unsigned)(oldValue >> 32)),
				"D" (location));
		return success;
	}
#else
	int32 compare_and_swap64(volatile int64 *location, int64 oldValue, int64 newValue);
#endif

inline bool cmpxchg32(volatile int32 *atom, int32 *value, int32 newValue)
{
	int32 success = compare_and_swap32(atom, *value, newValue);
	if (!success)
		*value = *atom;

	return success;
};

inline bool cmpxchg64(volatile int64 *atom, int64 *value, int64 newValue)
{
	int32 success = compare_and_swap64(atom, *value, newValue);
	if (!success)
		*value = *atom;

	return success;
};

#if __cplusplus
	}
#endif


#endif
