/*
  Sbr miscellaneous helper functions
*/
#include "sbr_misc.h"

#include "counters.h" /* the 3GPP instrumenting tool */


/* Sorting routine */
void Shellsort_int (int *in, int n)
{

  int i, j, v;
  int inc = 1;

  COUNT_sub_start("Shellsort_int");

  MOVE(1); /* counting previous operation */

  LOOP(1);
  do
  {
    MULT(1); ADD(1);
    inc = 3 * inc + 1;
  }
  while (inc <= n);

  LOOP(1);
  do {

    DIV(1);
    inc = inc / 3;

    PTR_INIT(1); /* pointers for in[] */
    ADD(1); LOOP(1);
    for (i = inc + 1; i <= n; i++) {

      MOVE(2);
      v = in[i-1];
      j = i;

      PTR_INIT(2); /* pointers for in[j-inc-1],
                                   in[j-1]
                   */
      LOOP(1);
      while (in[j-inc-1] > v) {

        MOVE(1);
        in[j-1] = in[j-inc-1];

        ADD(1);
        j -= inc;

        ADD(1); BRANCH(1);
        if (j <= inc)
          break;
      }

      MOVE(1);
      in[j-1] = v;
    }
  } while (inc > 1);

  COUNT_sub_end();
}



/*******************************************************************************
 Functionname:  AddVecLeft
 *******************************************************************************

 Arguments:   int* dst, int* length_dst, int* src, int length_src
 Return:      none
*******************************************************************************/
void
AddVecLeft (int *dst, int *length_dst, int *src, int length_src)
{
  int i;

  COUNT_sub_start("AddVecLeft");

  PTR_INIT(1); /* src[i] */
  ADD(1); LOOP(1);
  for (i = length_src - 1; i >= 0; i--)
  {
    FUNC(3);
    AddLeft (dst, length_dst, src[i]);
  }

  COUNT_sub_end();
}


/*******************************************************************************
 Functionname:  AddLeft
 *******************************************************************************

 Arguments:   int* vector, int* length_vector, int value
 Return:      none
*******************************************************************************/
void
AddLeft (int *vector, int *length_vector, int value)
{
  int i;

  COUNT_sub_start("AddLeft");

  PTR_INIT(1); /* vector[] */
  LOOP(1);
  for (i = *length_vector; i > 0; i--)
  {
    MOVE(1);
    vector[i] = vector[i - 1];
  }

  MOVE(1);
  vector[0] = value;

  ADD(1);
  (*length_vector)++;

  COUNT_sub_end();
}


/*******************************************************************************
 Functionname:  AddRight
 *******************************************************************************

 Arguments:   int* vector, int* length_vector, int value
 Return:      none
*******************************************************************************/
void
AddRight (int *vector, int *length_vector, int value)
{
  COUNT_sub_start("AddRight");

  INDIRECT(1); MOVE(1);
  vector[*length_vector] = value;

  ADD(1);
  (*length_vector)++;

  COUNT_sub_end();
}



/*******************************************************************************
 Functionname:  AddVecRight
 *******************************************************************************

 Arguments:   int* dst, int* length_dst, int* src, int length_src)
 Return:      none
*******************************************************************************/
void
AddVecRight (int *dst, int *length_dst, int *src, int length_src)
{
  int i;

  COUNT_sub_start("AddVecRight");

  PTR_INIT(1); /* src[] */
  LOOP(1);
  for (i = 0; i < length_src; i++)
  {
    FUNC(3);
    AddRight (dst, length_dst, src[i]);
  }

  COUNT_sub_end();
}

