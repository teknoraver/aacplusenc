#ifndef MINMAX__H
#define MINMAX__H
/* Makros to determine the smaller/bigger value of two integers or doubles or floats.
   Watch the expanding process: min(rst++, xyz) will cause problems. */
#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif
#ifndef max
#define max(a, b) ((a) > (b) ? (a) : (b))
#endif
#endif
