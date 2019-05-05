#pragma once // `std' posix headers
#include <stdint.h>
#include <math.h>
#include <stdlib.h>
#include <stdio.h>
#include <memory.h>

#ifdef __cplusplus
#define BEGIN_C extern "C" {
#define END_C }
#define null nullptr // https://stackoverflow.com/questions/1282295/what-exactly-is-nullptr/1283623#1283623
#else
#define BEGIN_C
#define END_C
#define true 1
#define false 0
#define null ((void*)0)
#endif

