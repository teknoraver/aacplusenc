/*
  Floating Point Reference Library
*/

#ifndef __FLOATFR_H
#define __FLOATFR_H

#include <stdarg.h>


#define LOG_DUALIS_TABLE_SIZE 65

#define INV_TABLE_BITS 8
#define INV_TABLE_SIZE (1<<INV_TABLE_BITS)

#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif

#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif


void   FloatFR_Init ( void );
float  FloatFR_logDualis(int a);
float  FloatFR_getNumOctaves(int a, int b);
#endif
