#pragma once // `std' posix headers
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>
#ifdef __APPLE__
#include <pthread/pthread.h>
#endif

#ifdef __cplusplus
#define BEGIN_C extern "C" {
#define END_C }
#define null nullptr
#else
#define BEGIN_C
#define END_C
#define true 1
#define false 0
#define null ((void*)0)
#endif

// because min(), max() are tainted by a meaningless war
#define minimum(a,b) ((a) < (b) ? (a) : (b))
#define maximum(a,b) ((a) > (b) ? (a) : (b))

#ifdef __APPLE__
#define gettid() pthread_mach_thread_np(pthread_self())
#endif
