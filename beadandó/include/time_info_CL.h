#ifndef TIME_INFO_H
#define TIME_INFO_H
// nem lehet include-olni az openCL-es és nem-openCL-es változatot is

#include <time.h>

typedef struct TimeInfo{
    cl_ulong maskStart;
    cl_ulong maskEnd;
    cl_ulong combineStart;
    cl_ulong combineEnd;
} TimeInfo;

#endif