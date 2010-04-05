/*
  Sbr miscellaneous helper functions prototypes
*/

#ifndef _SBR_MISC_H
#define _SBR_MISC_H

/* Sorting routines */
void Shellsort_int   (int *in, int n);

void AddLeft (int *vector, int *length_vector, int value);
void AddRight (int *vector, int *length_vector, int value);
void AddVecLeft (int *dst, int *length_dst, int *src, int length_src);
void AddVecRight (int *dst, int *length_vector_dst, int *src, int length_src);
#endif
