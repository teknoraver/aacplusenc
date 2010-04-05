/*
  Implementation of math functions
*/
   
#include <math.h>
#include <assert.h>
#include "FloatFR.h"

#include "counters.h" /* the 3GPP instrumenting tool */

static float logDualisTable[LOG_DUALIS_TABLE_SIZE];

/*
  Creates lookup tables for some arithmetic functions
*/
void FloatFR_Init(void)
{
  int i;

  COUNT_sub_start("FloatFR_Init");

  MULT(1); STORE(1);
  logDualisTable[0] = -1.0f; /* actually, ld 0 is not defined */

  PTR_INIT(1); /* logDualisTable[] */
  LOOP(1);
  for (i=1; i<LOG_DUALIS_TABLE_SIZE; i++) {
    TRANS(1); MULT(1); /* xxx * (1 / 0.30103) */ STORE(1);
    logDualisTable[i] = (float) ( log(i)/log(2.0f) );
  }
  
  COUNT_sub_end();
}


/*
  The table must have been created before by FloatFR_Init().
  The valid range for a is 1 to LOG_DUALIS_TABLE_SIZE.
  For a=0, the result will be -1 (should be -inf).

  returns ld(a)
*/
float FloatFR_logDualis(int a)  /* Index for logarithm table */
{
  assert( a>=0 && a<LOG_DUALIS_TABLE_SIZE );

  COUNT_sub_start("FloatFR_logDualis");
  INDIRECT(1);
  COUNT_sub_end();

  return logDualisTable[a];
}


/*
  The function FloatFR_Init() must have been called before use.
  The valid range for a and b is 1 to LOG_DUALIS_TABLE_SIZE.

  returns ld(a/b)
*/
float FloatFR_getNumOctaves(int a, /* lower band */
                            int b) /* upper band */
{
  COUNT_sub_start("FloatFR_getNumOctaves");
  FUNC(1); FUNC(1); ADD(1);
  COUNT_sub_end();

  return (FloatFR_logDualis(b) - FloatFR_logDualis(a));
}

