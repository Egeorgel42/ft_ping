#ifndef COMPILER_H
#define COMPILER_H
#include "stdbool.h"

#if defined(__has_builtin)
	#if __has_builtin(__builtin_expect)
		#define BRANCH_LIKELY(x) __builtin_expect(!!(x), true)
		#define BRANCH_UNLIKELY(x) __builtin_expect(!!(x), false)
	#else
		#define BRANCH_LIKELY(x) (x)
		#define BRANCH_UNLIKELY(x) (x)
	#endif
#elif defined(__GNUC__) && (__GNUC__ >= 3)
	#define BRANCH_LIKELY(x) __builtin_expect(!!(x), true)
	#define BRANCH_UNLIKELY(x) __builtin_expect(!!(x), false)
#else
	#define BRANCH_LIKELY(x) (x)
	#define BRANCH_UNLIKELY(x) (x)
#endif

#endif