#ifndef TIME_INFO_H
#define TIME_INFO_H

#include <time.h>

typedef struct TimeInfo{
    clock_t maskStart;
    clock_t maskEnd;
    clock_t combineStart;
    clock_t combineEnd;
} TimeInfo;

#endif