/*
  Scalefactor estimation
*/
#include <memory.h>
#include <limits.h>
#include "float.h"
#include "sf_estim.h"
#include "minmax.h"
#include "quantize.h"
#include "bit_cnt.h"
#include "aac_ram.h"

#include "counters.h" /* the 3GPP instrumenting tool */

#define MAX_SCF_DELTA  60

#define C1   -69.33295f     /* -16/3*log(MAX_QUANT+0.5-logCon)/log(2) */
#define C2     5.77078f     /* 4/log(2) */

#define LOG2_1 1.442695041f /* 1/log(2) */

#define PE_C1  3.0f         /* log(8.0)/log(2) */
#define PE_C2  1.3219281f   /* log(2.5)/log(2) */
#define PE_C3  0.5593573f   /* 1-C2/C1 */


static void
CalcFormFactorChannel(float *sfbFormFactor,
                      float *sfbNRelevantLines,
                      PSY_OUT_CHANNEL *psyOutChan)
{
  int i, j;
  int sfbOffs, sfb,sfbWidth;

  COUNT_sub_start("CalcFormFactorChannel");

  FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(psyOutChan->sfbCnt);
  memset(sfbNRelevantLines,0,sizeof(float)*psyOutChan->sfbCnt);

 
  INDIRECT(2); LOOP(1);
  for (sfbOffs=0; sfbOffs<psyOutChan->sfbCnt; sfbOffs+=psyOutChan->sfbPerGroup){

    PTR_INIT(5); /* pointer for sfbFormFactor[],
                                psyOutChan->sfbEnergy[]
                                psyOutChan->sfbThreshold[]
                                psyOutChan->sfbOffsets[]
                                sfbNRelevantLines[]
                 */
    INDIRECT(1); LOOP(1);
    for (sfb=0; sfb<psyOutChan->maxSfbPerGroup; sfb++) {
      i = sfbOffs+sfb;

      MOVE(1);
      sfbFormFactor[i] = FLT_MIN;

      ADD(1); BRANCH(1);
      if (psyOutChan->sfbEnergy[i] > psyOutChan->sfbThreshold[i]) {
        float avgFormFactor;

        /* calc sum of sqrt(spec) */
        PTR_INIT(1); /* pointer for psyOutChan->mdctSpectrum[j] */
        LOOP(1);
        for (j=psyOutChan->sfbOffsets[i]; j<psyOutChan->sfbOffsets[i+1]; j++) {

          MISC(1); TRANS(1); ADD(1);
          sfbFormFactor[i] += (float) sqrt(fabs(psyOutChan->mdctSpectrum[j]));
        }
        STORE(1); /* sfbFormFactor[i] */

        /* estimate number of relevant lines in this sfb */
        ADD(1);
        sfbWidth = psyOutChan->sfbOffsets[i+1]-psyOutChan->sfbOffsets[i];

        DIV(1); TRANS(1);
        avgFormFactor = (float) pow(psyOutChan->sfbEnergy[i]/(float)sfbWidth, 0.25);

        DIV(1); STORE(1);
        sfbNRelevantLines[i] = sfbFormFactor[i] / avgFormFactor;
      }
    }
  }

  COUNT_sub_end();
}


static int improveScf(float *spec, 
                      float *expSpec,
                      short *quantSpec,
                      short *quantSpecTmp,
                      int    sfbWidth, 
                      float thresh, 
                      short    scf,
                      short    minScf,
                      float *dist, 
                      short    *minScfCalculated)
{
   float sfbDist;
   short scfBest = scf;
   int j;
   
   COUNT_sub_start("improveScf");

   MOVE(1); /* counting previous operations */

   /* calc real distortion */
   FUNC(5);
   sfbDist = calcSfbDist(spec, expSpec, quantSpec, sfbWidth, scf);

   MOVE(1);
   *minScfCalculated = scf;

   /* nmr > 1.25 -> try to improve nmr */
   MULT(1); ADD(1); BRANCH(1);
   if (sfbDist > (1.25 * thresh)) {
      short scfEstimated = scf;
      float sfbDistBest = sfbDist;
      int cnt;

      MOVE(2); /* counting previous operations */

      /* improve by bigger scf ? */
      MOVE(1);
      cnt = 0;

      LOOP(1);
      while ((sfbDist > (1.25 * thresh)) && (cnt++ < 3)) {
         MULT(1); ADD(3); LOGIC(1); /* while() condition */

         ADD(1);
         scf++;

         FUNC(5);
         sfbDist = calcSfbDist(spec, expSpec, quantSpecTmp, sfbWidth, scf);

         ADD(1); BRANCH(1);
         if (sfbDist < sfbDistBest) {

            MOVE(2);
            scfBest = scf;
            sfbDistBest = sfbDist;

            PTR_INIT(2); /* pointers for quantSpec[],
                                         quantSpecTmp[]
                         */
            LOOP(1);
            for (j=0; j<sfbWidth; j++) {

              MOVE(1);
              quantSpec[j] = quantSpecTmp[j];
            }
         }
      }

      /* improve by smaller scf ? */
      MOVE(3);
      cnt = 0;
      scf = scfEstimated;
      sfbDist = sfbDistBest;

      LOOP(1);
      while ((sfbDist > (1.25 * thresh)) && (cnt++ < 1) && (scf > minScf)) {
         MULT(1); ADD(4); LOGIC(2); /* while() condition */

         ADD(1);
         scf--;

         FUNC(5);
         sfbDist = calcSfbDist(spec, expSpec, quantSpecTmp, sfbWidth, scf);

         ADD(1); BRANCH(1);
         if (sfbDist < sfbDistBest) {

            MOVE(2);
            scfBest = scf;
            sfbDistBest = sfbDist;

            PTR_INIT(2); /* pointers for quantSpec[],
                                         quantSpecTmp[]
                         */
            LOOP(1);
            for (j=0; j<sfbWidth; j++) {

              MOVE(1);
              quantSpec[j] = quantSpecTmp[j];
            }
         }
         MOVE(1);
         *minScfCalculated = scf;
      }
      MOVE(1);
      *dist = sfbDistBest;
   }
   else { /* nmr <= 1.25 -> try to find bigger scf to use less bits */
      float sfbDistBest = sfbDist; 
      float sfbDistAllowed = min(sfbDist * 1.25f,thresh);
      int cnt;

      MULT(1); ADD(1); BRANCH(1); MOVE(1); /* min() */ MOVE(1); /* counting previous operations */

      LOOP(1);
      for (cnt=0; cnt<3; cnt++) {

         ADD(1);
         scf++;

         FUNC(5);
         sfbDist = calcSfbDist(spec, expSpec, quantSpecTmp, sfbWidth, scf);

         ADD(1); BRANCH(1);
         if (sfbDist < sfbDistAllowed) {

           ADD(1); STORE(1);
           *minScfCalculated = scfBest+1;

           MOVE(2);
           scfBest = scf;
           sfbDistBest = sfbDist;

           PTR_INIT(2); /* pointers for quantSpec[],
                                        quantSpecTmp[]
                        */
           LOOP(1);
           for (j=0; j<sfbWidth; j++) {

             MOVE(1);
             quantSpec[j] = quantSpecTmp[j];
           }
         }
      }
      MOVE(1);
      *dist = sfbDistBest;
   }

   /* sign for quantSpec */
   PTR_INIT(2); /* pointers for spec[],
                                quantSpec[]
                */
   LOOP(1);
   for (j=0; j<sfbWidth; j++) {

      BRANCH(1);
      if (spec[j] < 0) {

         MULT(1); STORE(1);
         quantSpec[j] = -quantSpec[j];
      }
   }

   COUNT_sub_end();

   /* return best scalefactor */
   return scfBest;
}

static int countSingleScfBits(int scf, int scfLeft, int scfRight)
{
  int scfBits;

  COUNT_sub_start("countSingleScfBits");

  ADD(3); FUNC(1); FUNC(1);
  scfBits = bitCountScalefactorDelta(scfLeft-scf) +
            bitCountScalefactorDelta(scf-scfRight);

  COUNT_sub_end();

  return scfBits;
}

static float calcSingleSpecPe(int scf, float sfbConstPePart, float nLines)
{
  float specPe = 0.0f;
  float ldRatio;

  COUNT_sub_start("calcSingleSpecPe");

  MULT(1); ADD(1);
  ldRatio = sfbConstPePart - (float)0.375f * (float)scf;

  ADD(1); BRANCH(1);
  if (ldRatio >= PE_C1) {

    MULT(2);
    specPe = (float)0.7f * nLines * ldRatio;
  }
  else {

    MULT(3); ADD(1);
    specPe = (float)0.7f * nLines * (PE_C2 + PE_C3*ldRatio);
  }

  COUNT_sub_end();
 
  return specPe;
}



static int countScfBitsDiff(short *scfOld, short *scfNew, 
                            int sfbCnt, int startSfb, int stopSfb)
{
  int scfBitsDiff = 0;
  int sfb = 0, sfbLast;
  int sfbPrev, sfbNext;

  COUNT_sub_start("countScfBitsDiff");

  /* search for first relevant sfb */
  MOVE(1);
  sfbLast = startSfb;

  PTR_INIT(2); /* pointers for scfOld[sfbLast],
                               scfNew[sfbLast]
               */
  LOOP(1);
  while ((sfbLast<stopSfb) && (scfOld[sfbLast]==SHRT_MIN)) {
    ADD(2); LOGIC(1); /* while() condition */

    sfbLast++;
  }

  /* search for previous relevant sfb and count diff */
  ADD(1);
  sfbPrev = startSfb - 1;

  PTR_INIT(2); /* pointers for scfOld[sfbPrev],
                               scfNew[sfbPrev]
               */
  LOOP(1);
  while ((sfbPrev>=0) && (scfOld[sfbPrev]==SHRT_MIN)) {
    ADD(2); LOGIC(1); /* while() condition */

    sfbPrev--;
  }

  BRANCH(1);
  if (sfbPrev>=0) {

    ADD(4); FUNC(1); FUNC(1);
    scfBitsDiff += bitCountScalefactorDelta(scfNew[sfbPrev]-scfNew[sfbLast]) -
                   bitCountScalefactorDelta(scfOld[sfbPrev]-scfOld[sfbLast]);
  }

  /* now loop through all sfbs and count diffs of relevant sfbs */
  PTR_INIT(2); /* pointers for scfOld[sfb],
                               scfNew[sfb]
               */
  LOOP(1);
  for (sfb=sfbLast+1; sfb<stopSfb; sfb++) {

    ADD(1); BRANCH(1);
    if (scfOld[sfb]!=SHRT_MIN) {

      ADD(4); FUNC(1); FUNC(1);
      scfBitsDiff += bitCountScalefactorDelta(scfNew[sfbLast]-scfNew[sfb]) - 
                     bitCountScalefactorDelta(scfOld[sfbLast]-scfOld[sfb]);

      MOVE(1);
      sfbLast = sfb;
    }
  }

  /* search for next relevant sfb and count diff */
  MOVE(1);
  sfbNext = stopSfb;

  LOOP(1);
  while ((sfbNext<sfbCnt) && (scfOld[sfbNext]==SHRT_MIN)) {
    ADD(2); LOGIC(1); /* while() condition */

    sfbNext++;
  }

  ADD(1); BRANCH(1);
  if (sfbNext<sfbCnt) {

    ADD(4); FUNC(1); FUNC(1);
    scfBitsDiff += bitCountScalefactorDelta(scfNew[sfbLast]-scfNew[sfbNext]) -
                   bitCountScalefactorDelta(scfOld[sfbLast]-scfOld[sfbNext]);
  }

  COUNT_sub_end();

  return scfBitsDiff;
}

static float calcSpecPeDiff(PSY_OUT_CHANNEL *psyOutChan,
                            short *scfOld,
                            short *scfNew,
                            float *sfbConstPePart,
                            float *sfbFormFactor,
                            float *sfbNRelevantLines,
                            int startSfb, 
                            int stopSfb)
{
  float specPeDiff = 0.0f;
  int sfb;
  int sfbWidth;

  COUNT_sub_start("calcSpecPeDiff");

  MOVE(1); /* counting previous operation */

  /* loop through all sfbs and count pe difference */
  PTR_INIT(6); /* pointers for scfOld[sfb],
                               scfNew[sfb],
                               sfbConstPePart[sfb],
                               sfbFormFactor[sfb],
                               psyOutChan->sfbOffsets[sfb+1],
                               psyOutChan->sfbEnergy[sfb]
               */
  LOOP(1);
  for (sfb=startSfb; sfb<stopSfb; sfb++) {

    ADD(1); BRANCH(1);
    if (scfOld[sfb]!=SHRT_MIN) {
      float ldRatioOld, ldRatioNew, pOld, pNew;

      ADD(1);
      sfbWidth = psyOutChan->sfbOffsets[sfb+1] - psyOutChan->sfbOffsets[sfb];

      ADD(1); BRANCH(1);
      if (sfbConstPePart[sfb]==FLT_MIN) {

        MULT(2); DIV(1); TRANS(1); STORE(1);
        sfbConstPePart[sfb] = (float) log(psyOutChan->sfbEnergy[sfb] * (float)6.75f /
                                          sfbFormFactor[sfb]) * LOG2_1 ;
      }

      MULT(2); ADD(2);
      ldRatioOld = sfbConstPePart[sfb] - 0.375f * scfOld[sfb];
      ldRatioNew = sfbConstPePart[sfb] - 0.375f * scfNew[sfb];

      ADD(1); BRANCH(1);
      if (ldRatioOld >= PE_C1) {

        MOVE(1);
        pOld = ldRatioOld;
      }
      else {

        MULT(1); ADD(1);
        pOld = PE_C2 + PE_C3 * ldRatioOld;
      }

      ADD(1); BRANCH(1);
      if (ldRatioNew >= PE_C1) {

        MOVE(1);
        pNew = ldRatioNew;
      }
      else {

        MULT(1); ADD(1);
        pNew = PE_C2 + PE_C3 * ldRatioNew;
      }

      ADD(1); MULT(1); MAC(1);
      specPeDiff += (float)0.7f * sfbNRelevantLines[sfb] * (pNew - pOld);
    }
  }

  COUNT_sub_end();

  return specPeDiff;
}



static void assimilateSingleScf(PSY_OUT_CHANNEL *psyOutChan,
                                float *expSpec,
                                short *quantSpec,
                                short *quantSpecTmp,
                                short *scf, 
                                short *minScf,
                                float *sfbDist, 
                                float *sfbConstPePart,
                                float *sfbFormFactor,
                                float *sfbNRelevantLines,
                                short *minScfCalculated,
                                int restartOnSuccess)
{
  int sfbLast, sfbAct, sfbNext;
  short scfAct, *scfLast, *scfNext, scfMin, scfMax;
  int sfbWidth, sfbOffs;
  float en;
  float sfbPeOld, sfbPeNew;
  float sfbDistNew;
  int j;
  int success = 0;
  float deltaPe = 0.0f, deltaPeNew, deltaPeTmp;
  short prevScfLast[MAX_GROUPED_SFB], prevScfNext[MAX_GROUPED_SFB];
  float deltaPeLast[MAX_GROUPED_SFB];
  int updateMinScfCalculated;

  COUNT_sub_start("assimilateSingleScf");

  MOVE(2); /* counting previous operations */

  PTR_INIT(3); /* pointers for prevScfLast[],
                               prevScfNext[],
                               deltaPeLast[]
               */
  INDIRECT(1); LOOP(1);
  for(j=0;j<psyOutChan->sfbCnt;j++){

    MOVE(3);
    prevScfLast[j]=SHRT_MAX;
    prevScfNext[j]=SHRT_MAX;
    deltaPeLast[j]=FLT_MAX;
  }

    
 
 
  MOVE(7);
  sfbLast = -1;
  sfbAct  = -1;
  sfbNext = -1;
  scfLast = 0;
  scfNext = 0;
  scfMin  = SHRT_MAX;
  scfMax  = SHRT_MAX;

  PTR_INIT(13); /* pointers for scf[sfbNext],
                                scf[sfbAct],
                                minScf[sfbAct],
                                minScfCalculated[sfbAct],
                                prevScfLast[sfbAct],
                                prevScfNext[sfbAct],
                                deltaPeLast[sfbAct],
                                sfbDist[sfbAct],
                                sfbConstPePart[sfbAct],
                                sfbFormFactor[sfbAct],
                                sfbNRelevantLines[sfbAct],
                                psyOutChan->sfbOffsets[sfbAct],
                                psyOutChan->sfbEnergy[sfbAct]
                */
  INDIRECT(1); LOOP(1);
  do {
    /* search for new relevant sfb */
    ADD(1);
    sfbNext++;

    INDIRECT(1); LOOP(1);
    while ((sfbNext < psyOutChan->sfbCnt) && (scf[sfbNext] == SHRT_MIN)) {
      ADD(2); LOGIC(1); /* while() condition */

      sfbNext++;
    }

    INDIRECT(1); LOGIC(2); ADD(1); BRANCH(1);
    if ((sfbLast>=0) && (sfbAct>=0) && (sfbNext<psyOutChan->sfbCnt)) {
      /* relevant scfs to the left and to the right */

      MOVE(1);
      scfAct  = scf[sfbAct];

      ADD(2);
      scfLast = scf + sfbLast;
      scfNext = scf + sfbNext;

      ADD(1); BRANCH(1); MOVE(1);
      scfMin  = min(*scfLast, *scfNext);

      ADD(1); BRANCH(1); MOVE(1);
      scfMax  = max(*scfLast, *scfNext); 
    }
    else {

      INDIRECT(1); ADD(2); LOGIC(2); BRANCH(1);
      if ((sfbLast==-1) && (sfbAct>=0) && (sfbNext<psyOutChan->sfbCnt)) {
      /* first relevant scf */

      MOVE(1);
      scfAct  = scf[sfbAct];

      PTR_INIT(1);
      scfLast = &scfAct;

      ADD(1);
      scfNext = scf + sfbNext;

      MOVE(1);
      scfMin  = *scfNext;

      MOVE(1);
      scfMax  = *scfNext;
    }
    else {

      INDIRECT(1); ADD(1); LOGIC(2); BRANCH(1);
      if ((sfbLast>=0) && (sfbAct>=0) && (sfbNext==psyOutChan->sfbCnt)) {
      /* last relevant scf */

      MOVE(1);
      scfAct  = scf[sfbAct];

      ADD(1);
      scfLast = scf + sfbLast;

      PTR_INIT(1);
      scfNext = &scfAct;

      MOVE(1);
      scfMin  = *scfLast;

      MOVE(1);
      scfMax  = *scfLast;
    }
    }
    }

    BRANCH(1); 
    if (sfbAct>=0) {
     
      ADD(1); BRANCH(1); MOVE(1);
      scfMin = max(scfMin, minScf[sfbAct]);
    }

    INDIRECT(1); ADD(5); LOGIC(8); BRANCH(1);
    if ((sfbAct >= 0) && 
        (sfbLast>=0 || sfbNext<psyOutChan->sfbCnt) && 
        (scfAct > scfMin) && 
        (scfAct <= scfMin+MAX_SCF_DELTA) &&
        (scfAct >= scfMax-MAX_SCF_DELTA) &&
        (*scfLast != prevScfLast[sfbAct] || 
         *scfNext != prevScfNext[sfbAct] || 
         deltaPe < deltaPeLast[sfbAct])) {
      /* bigger than neighbouring scf found, try to use smaller scf */
      MOVE(1);
      success = 0;

      ADD(1);
      sfbWidth = psyOutChan->sfbOffsets[sfbAct+1] - 
                 psyOutChan->sfbOffsets[sfbAct];

      MOVE(1);
      sfbOffs = psyOutChan->sfbOffsets[sfbAct];

      /* estimate required bits for actual scf */
      MOVE(1);
      en = psyOutChan->sfbEnergy[sfbAct];

      ADD(1); BRANCH(1);
      if (sfbConstPePart[sfbAct] == FLT_MIN) {

        MULT(2); DIV(1); TRANS(1); STORE(1);
        sfbConstPePart[sfbAct] = (float)log(en*(float)6.75f/sfbFormFactor[sfbAct]) * LOG2_1 ;
      }
                                  
      FUNC(3); FUNC(3); ADD(1);
      sfbPeOld = calcSingleSpecPe(scfAct, sfbConstPePart[sfbAct], 
                                  sfbNRelevantLines[sfbAct]) +
                 countSingleScfBits(scfAct, *scfLast, *scfNext);

      MOVE(2);
      deltaPeNew = deltaPe;
      updateMinScfCalculated = 1;

      LOOP(1);
      do {
        /* estimate required bits for smaller scf */

        ADD(1);
        scfAct--;

        /* check only if the same check was not done before */
        ADD(1); BRANCH(1); LOGIC(1);
        if (scfAct < minScfCalculated[sfbAct] && scfAct>=scfMax-MAX_SCF_DELTA){
          /* estimate required bits for new scf */

          FUNC(3); FUNC(3); ADD(1);
          sfbPeNew = calcSingleSpecPe(scfAct, sfbConstPePart[sfbAct], 
                                      sfbNRelevantLines[sfbAct]) +
                     (float)countSingleScfBits(scfAct, *scfLast, *scfNext);

          /* use new scf if no increase in pe and 
             quantization error is smaller */
          ADD(2);
          deltaPeTmp = deltaPe + sfbPeNew - sfbPeOld;

          ADD(1); BRANCH(1);
          if (deltaPeTmp < (float)10.0f) {
            /* distortion of new scf */

            INDIRECT(1); ADD(3); FUNC(5);
            sfbDistNew = calcSfbDist(psyOutChan->mdctSpectrum+sfbOffs,
                                     expSpec+sfbOffs,
                                     quantSpecTmp+sfbOffs,
                                     sfbWidth,
                                     scfAct);

            ADD(1); BRANCH(1);
            if (sfbDistNew < sfbDist[sfbAct]) {
              /* success, replace scf by new one */

              MOVE(2);
              scf[sfbAct] = scfAct;
              sfbDist[sfbAct] = sfbDistNew;

              PTR_INIT(3); /* pointers for psyOutChan->mdctSpectrum[],
                                           quantSpec[],
                                           quantSpecTmp[]
                           */
              LOOP(1);
              for (j=sfbOffs; j<sfbOffs+sfbWidth; j++) {

                MOVE(1);
                quantSpec[j] = quantSpecTmp[j];

                /* sign for quant spec */
                BRANCH(1);
                if (psyOutChan->mdctSpectrum[j] < 0.0f) {
                  MULT(1); STORE(1);
                  quantSpec[j] = -quantSpec[j];
                }
              }
              MOVE(2);
              deltaPeNew = deltaPeTmp;
              success = 1;
            }

            /* mark as already checked */
            BRANCH(1);
            if (updateMinScfCalculated) {
              MOVE(1);
              minScfCalculated[sfbAct] = scfAct;
            }
          }
          else {
            /* from this scf value on not all new values have been checked */
            MOVE(1);
            updateMinScfCalculated = 0;
          }
        }
      } while (scfAct > scfMin);

      MOVE(1);
      deltaPe = deltaPeNew;

      /* save parameters to avoid multiple computations of the same sfb */
      MOVE(3);
      prevScfLast[sfbAct] = *scfLast;
      prevScfNext[sfbAct] = *scfNext;
      deltaPeLast[sfbAct] = deltaPe;
    }

    LOGIC(1); BRANCH(1);
    if (success && restartOnSuccess) {
      /* start again at first sfb */

      MOVE(8);
      sfbLast = -1;
      sfbAct  = -1;
      sfbNext = -1;
      scfLast = 0;
      scfNext = 0;
      scfMin  = SHRT_MAX;
      scfMax  = SHRT_MAX;
      success = 0;
    }
    else {
      /* shift sfbs for next band */

      MOVE(2);
      sfbLast = sfbAct;
      sfbAct  = sfbNext;
    }
  } while (sfbNext < psyOutChan->sfbCnt);

  COUNT_sub_end();
}


static void assimilateMultipleScf(PSY_OUT_CHANNEL *psyOutChan,
                                  float *expSpec,
                                  short *quantSpec,
                                  short *quantSpecTmp,
                                  short *scf, 
                                  short *minScf,
                                  float *sfbDist, 
                                  float *sfbConstPePart,
                                  float *sfbFormFactor,
                                  float *sfbNRelevantLines)
{
  int sfb, startSfb, stopSfb;
  short scfTmp[MAX_GROUPED_SFB], scfMin, scfMax, scfAct;
  int possibleRegionFound;
  int sfbWidth, sfbOffs, j;
  float sfbDistNew[MAX_GROUPED_SFB], distOldSum, distNewSum;
  int deltaScfBits;
  float deltaSpecPe;
  float deltaPe = 0.0f, deltaPeNew;
  int sfbCnt = psyOutChan->sfbCnt;

  COUNT_sub_start("assimilateMultipleScf");

  MOVE(2); /* counting previous operations */

  /* calc min and max scalfactors */
  MOVE(2);
  scfMin = SHRT_MAX;
  scfMax = SHRT_MIN;

  PTR_INIT(1); /* pointer for scf[sfb] */
  LOOP(1);
  for (sfb=0; sfb<sfbCnt; sfb++) {

    ADD(1); BRANCH(1);
    if (scf[sfb]!=SHRT_MIN) {

      ADD(1); BRANCH(1); MOVE(1);
      scfMin = min(scfMin, scf[sfb]);

      ADD(1); BRANCH(1); MOVE(1);
      scfMax = max(scfMax, scf[sfb]);
    }
  }

  ADD(1); BRANCH(1); LOGIC(1);
  if (scfMax != SHRT_MIN && scfMax <= scfMin+MAX_SCF_DELTA) {

    MOVE(1);
    scfAct = scfMax;

    PTR_INIT(6); /* pointers for scf[sfb],
                                 minScf[sfb],
                                 scfTmp[sfb],
                                 psyOutChan->sfbOffsets[sfb],
                                 sfbDistNew[sfb],
                                 psyOutChan->sfbThreshold[sfb]
                 */
    LOOP(1);
    do {
      /* try smaller scf */

      ADD(1);
      scfAct--;

      FUNC(2); LOOP(1); PTR_INIT(2); MOVE(1); STORE(MAX_GROUPED_SFB);
      memcpy(scfTmp,scf,MAX_GROUPED_SFB*sizeof(short));

      MOVE(1);
      stopSfb = 0;

      LOOP(1);
      do {
        /* search for region where all scfs are bigger than scfAct */

        MOVE(1);
        sfb = stopSfb;

        LOOP(1);
        while (sfb<sfbCnt && (scf[sfb]==SHRT_MIN || scf[sfb] <= scfAct)) {
          ADD(3); LOGIC(2); /* while() condition */

          ADD(1);
          sfb++;
        }

        MOVE(1);
        startSfb = sfb;

        ADD(1);
        sfb++;

        LOOP(1);
        while (sfb<sfbCnt && (scf[sfb]==SHRT_MIN || scf[sfb] > scfAct)) {
          ADD(3); LOGIC(2); /* while() condition */

          ADD(1);
          sfb++;
        }

        MOVE(1);
        stopSfb = sfb;

        /* check if in all sfb of a valid region scfAct >= minScf[sfb] */
        MOVE(1);
        possibleRegionFound = 0;

        ADD(1); BRANCH(1);
        if (startSfb < sfbCnt) {

          MOVE(1);
          possibleRegionFound = 1;

          LOOP(1);
          for (sfb=startSfb; sfb<stopSfb; sfb++) {

            ADD(1); BRANCH(1);
            if (scf[sfb] != SHRT_MIN) {

              ADD(1); BRANCH(1);
              if (scfAct < minScf[sfb]) {

                MOVE(1);
                possibleRegionFound = 0;
                break;
              }
            }

          }
        }

        BRANCH(1);
        if (possibleRegionFound) { /* region found */

          /* replace scfs in region by scfAct */
          LOOP(1);
          for (sfb=startSfb; sfb<stopSfb; sfb++) {

            ADD(1); BRANCH(1);
            if (scfTmp[sfb] != SHRT_MIN) {

              MOVE(1);
              scfTmp[sfb] = scfAct;
            }
          }

          /* estimate change in bit demand for new scfs */
          FUNC(5);
          deltaScfBits = countScfBitsDiff(scf,scfTmp,sfbCnt,startSfb,stopSfb);

          FUNC(8);
          deltaSpecPe = calcSpecPeDiff(psyOutChan, scf, scfTmp, sfbConstPePart,
                                       sfbFormFactor, sfbNRelevantLines, 
                                       startSfb, stopSfb);

          ADD(2);
          deltaPeNew = deltaPe + (float)deltaScfBits + deltaSpecPe;

          /* new bit demand small enough ? */
          ADD(1); BRANCH(1);
          if (deltaPeNew < (float)10.0f) {

            /* quantize and calc sum of new distortion */
            MOVE(2);
            distOldSum = distNewSum = 0.0f;

            LOOP(1);
            for (sfb=startSfb; sfb<stopSfb; sfb++) {

              ADD(1); BRANCH(1);
              if (scfTmp[sfb] != SHRT_MIN) {

                ADD(1);
                distOldSum += sfbDist[sfb];

                ADD(1);
                sfbWidth = psyOutChan->sfbOffsets[sfb+1] - 
                           psyOutChan->sfbOffsets[sfb];

                MOVE(1);
                sfbOffs = psyOutChan->sfbOffsets[sfb];

                INDIRECT(1); ADD(3); FUNC(5); STORE(1);
                sfbDistNew[sfb] = calcSfbDist(psyOutChan->mdctSpectrum+sfbOffs, 
                                              expSpec+sfbOffs,
                                              quantSpecTmp+sfbOffs,
                                              sfbWidth, 
                                              scfAct);

                ADD(1); BRANCH(1);
                if (sfbDistNew[sfb] > psyOutChan->sfbThreshold[sfb]) {
                  /* no improvement, skip further dist. calculations */

                  MULT(1);
                  distNewSum = (float)2.0f * distOldSum;
                  break;
                }

                ADD(1);
                distNewSum += sfbDistNew[sfb];
              }
            }

            /* distortion smaller ? -> use new scalefactors */
            ADD(1); BRANCH(1);
            if (distNewSum < distOldSum) {

              MOVE(1);
              deltaPe = deltaPeNew;

              LOOP(1);
              for (sfb=startSfb; sfb<stopSfb; sfb++) {

                ADD(1); BRANCH(1);
                if (scf[sfb] != SHRT_MIN) {

                  ADD(1);
                  sfbWidth = psyOutChan->sfbOffsets[sfb+1] - 
                             psyOutChan->sfbOffsets[sfb];

                  MOVE(3);
                  sfbOffs = psyOutChan->sfbOffsets[sfb];
                  scf[sfb] = scfAct;
                  sfbDist[sfb] = sfbDistNew[sfb];

                  PTR_INIT(3); /* pointers for psyOutChan->mdctSpectrum[],
                                               quantSpec[],
                                               quantSpecTmp[]
                               */
                  LOOP(1);
                  for (j=sfbOffs; j<sfbOffs+sfbWidth; j++) {

                    MOVE(1);
                    quantSpec[j] = quantSpecTmp[j];

                    /* sign for quant spec */
                    BRANCH(1);
                    if (psyOutChan->mdctSpectrum[j] < 0.0f) {

                      MULT(1); STORE(1);
                      quantSpec[j] = -quantSpec[j];
                    }
                  }
                }
              }
            }

          }
        }


      } while (stopSfb <= sfbCnt);

    } while (scfAct > scfMin);
  }

  COUNT_sub_end();
}




static void
estimateScaleFactorsChannel(PSY_OUT_CHANNEL *psyOutChan,
                            short *scf,
                            int *globalGain,
                            float *sfbFormFactor,
                            float *sfbNRelevantLines,
                            short *quantSpec)
{
  int   i, j;
  float thresh, energy, energyPart, thresholdPart;
  float scfFloat;
  short scfInt, minScf, maxScf;
  float maxSpec;
  float sfbDist[MAX_GROUPED_SFB];
  short minSfMaxQuant[MAX_GROUPED_SFB];
  short minScfCalculated[MAX_GROUPED_SFB];

  COUNT_sub_start("EstimateScaleFactorsChannel");



  PTR_INIT(2); /* pointer for quantSpec[],
                  expSpec[]
               */
  LOOP(1);
  for (i=0; i<FRAME_LEN_LONG; i++) {
    
    MOVE(2);
    expSpec[i] = 0.0f;
    quantSpec[i] = 0;
  }

  PTR_INIT(7); /* pointer for psyOutChan->sfbThreshold[i],
                              psyOutChan->sfbEnergy[i],
                              psyOutChan->sfbOffsets[i],
                              scf[i],
                              minSfMaxQuant[i],
                              sfbFormFactor[i],
                              sfbDist[i]
               */
  INDIRECT(1); LOOP(1);
  for(i=0; i<psyOutChan->sfbCnt; i++) {

    MOVE(2);
    thresh = psyOutChan->sfbThreshold[i];
    energy = psyOutChan->sfbEnergy[i];

    MOVE(1);
    maxSpec = 0.0f;

    /* maximum of spectrum */
    PTR_INIT(1); /* pointer for psyOutChan->mdctSpectrum[j] */
    LOOP(1);
    for(j=psyOutChan->sfbOffsets[i]; j<psyOutChan->sfbOffsets[i+1]; j++ ){

      ADD(1); BRANCH(1); /* max() */ BRANCH(1); MULT(1); /* for -mdctSpectrum[j] */
      maxSpec = max(maxSpec, psyOutChan->mdctSpectrum[j] > 0.0 ? psyOutChan->mdctSpectrum[j]:-psyOutChan->mdctSpectrum[j] );
    }

    /* scfs without energy or with thresh>energy are marked with SHRT_MIN */
    MOVE(2);
    scf[i] = SHRT_MIN;
    minSfMaxQuant[i]=SHRT_MIN; 

    ADD(1); LOGIC(1); BRANCH(1);
    if( (maxSpec > 0.0) && (energy > thresh) ){

        TRANS(1); 
        energyPart = (float) log10(sfbFormFactor[i]);

        /* influence of allowed distortion */
        MULT(1); ADD(1); TRANS(1);
        thresholdPart = (float) log10(6.75*thresh+FLT_MIN);

        ADD(1); MULT(1);
        scfFloat = 8.8585f *(thresholdPart - energyPart);    /* scf calc */

        MISC(1);
        scfInt = (int)floor(scfFloat);           /* integer scalefactor */

        /* avoid quantized values bigger than MAX_QUANT */
        TRANS(1); MULT(1); ADD(1); MISC(1); STORE(1);
        minSfMaxQuant[i] = (int)ceil(C1 + C2*log(maxSpec));

        ADD(1); BRANCH(1); MOVE(1);
        scfInt = max(scfInt, minSfMaxQuant[i]);


        /* find better scalefactor with analysis by synthesis */
        ADD(3); FUNC(3);
        calcExpSpec(expSpec+psyOutChan->sfbOffsets[i],
                    psyOutChan->mdctSpectrum+psyOutChan->sfbOffsets[i],
                    psyOutChan->sfbOffsets[i+1]-psyOutChan->sfbOffsets[i]);
        
        ADD(5); FUNC(10);
        scfInt = improveScf(psyOutChan->mdctSpectrum+psyOutChan->sfbOffsets[i],
                            expSpec+psyOutChan->sfbOffsets[i],
                            quantSpec+psyOutChan->sfbOffsets[i],
                            quantSpecTmp+psyOutChan->sfbOffsets[i],
                            psyOutChan->sfbOffsets[i+1]-psyOutChan->sfbOffsets[i],
                            thresh, scfInt, minSfMaxQuant[i], 
                            &sfbDist[i], &minScfCalculated[i]);
        
        MOVE(1);
        scf[i] = scfInt;
    }
  }

  
  {
     /* try to decrease scf differences */
    float sfbConstPePart[MAX_GROUPED_SFB];

    PTR_INIT(1); /* pointer for sfbConstPePart[] */
    INDIRECT(1); LOOP(1);
    for(i=0;i<psyOutChan->sfbCnt;i++) {
      MOVE(1);
      sfbConstPePart[i]=FLT_MIN;
    }
     
    FUNC(12);
    assimilateSingleScf(psyOutChan, expSpec, quantSpec, quantSpecTmp, scf, 
                         minSfMaxQuant, sfbDist, sfbConstPePart,
                         sfbFormFactor, sfbNRelevantLines, minScfCalculated, 1);

    FUNC(10);
    assimilateMultipleScf(psyOutChan, expSpec, quantSpec, quantSpecTmp, scf, 
                          minSfMaxQuant, sfbDist, sfbConstPePart,
                          sfbFormFactor, sfbNRelevantLines);
  }

  /* get max scalefac for global gain */
  MOVE(2);
  maxScf = SHRT_MIN;
  minScf = SHRT_MAX;

  PTR_INIT(1); /* pointer for scf[] */
  INDIRECT(1); LOOP(1);
  for(i=0; i<psyOutChan->sfbCnt; i++) {

    ADD(1); BRANCH(1);
    if( maxScf < scf[i] ) {

      MOVE(1);
      maxScf = scf[i];
    }

    ADD(2); LOGIC(1); BRANCH(1);
    if ((scf[i] != SHRT_MIN) && (minScf > scf[i])) {

      MOVE(1);
      minScf = scf[i];
    }
  }

  /* limit scf delta */
  PTR_INIT(1); /* pointer for scf[] */
  INDIRECT(1); LOOP(1);
  for(i=0; i<psyOutChan->sfbCnt; i++) {

    ADD(3); LOGIC(1);
    if ((scf[i] != SHRT_MIN) && (minScf+MAX_SCF_DELTA) < scf[i]) {

      ADD(1); STORE(1);
      scf[i] = minScf + MAX_SCF_DELTA;

      /* changed bands need to be quantized again */
      FUNC(5); ADD(4); /* ??? */
      sfbDist[i] =
        calcSfbDist(psyOutChan->mdctSpectrum+psyOutChan->sfbOffsets[i],
                    expSpec+psyOutChan->sfbOffsets[i],
                    quantSpec+psyOutChan->sfbOffsets[i],
                    psyOutChan->sfbOffsets[i+1]-psyOutChan->sfbOffsets[i],
                    scf[i]);
    }
  }

  /* new maxScf if any scf has been limited */
  ADD(2); BRANCH(1); MOVE(1);
  maxScf = min( (minScf+MAX_SCF_DELTA), maxScf);

  /* calc loop scalefactors, if spec is not all zero (i.e. maxScf == -99) */
  ADD(1); BRANCH(1);
  if( maxScf > SHRT_MIN ) {

    MOVE(2);
    *globalGain = maxScf;

    PTR_INIT(2); /* pointer for scf[],
                                psyOutChan->sfbOffsets[i]
                 */
    INDIRECT(1); LOOP(1);
    for(i=0; i<psyOutChan->sfbCnt; i++) {

      /* intermediate bands with SHRT_MIN get the value of previous band */
      ADD(1); BRANCH(1);
      if( scf[i] == SHRT_MIN ) {

        MOVE(1);
        scf[i] = 0;

        /* set band explicitely to zero */
        PTR_INIT(1); /* pointer for psyOutChan->mdctSpectrum[j] */
        LOOP(1);
        for (j=psyOutChan->sfbOffsets[i]; j<psyOutChan->sfbOffsets[i+1]; j++) {

          MOVE(1);
          psyOutChan->mdctSpectrum[j] = 0.0f;
        }
      }
      else {
        ADD(1); STORE(1);
        scf[i] = maxScf - scf[i];
      }
    }
  }
  else{

    MOVE(1);
    *globalGain = 0;

    /* set spectrum explicitely to zero */
    PTR_INIT(2); /* pointer for scf[],
                                 psyOutChan->sfbOffsets[i]
                 */
    INDIRECT(1); LOOP(1);
    for(i=0; i<psyOutChan->sfbCnt; i++) {

      MOVE(1);
      scf[i]=0;

      PTR_INIT(1); /* pointer for psyOutChan->mdctSpectrum[j] */
      LOOP(1);
      for (j=psyOutChan->sfbOffsets[i]; j<psyOutChan->sfbOffsets[i+1]; j++) {

        MOVE(1);
        psyOutChan->mdctSpectrum[j] = 0.0f;
      }
    }
  }

  COUNT_sub_end();
}


void
CalcFormFactor(float sfbFormFactor[MAX_CHANNELS][MAX_GROUPED_SFB],
               float sfbNRelevantLines[MAX_CHANNELS][MAX_GROUPED_SFB],
               PSY_OUT_CHANNEL psyOutChannel[MAX_CHANNELS],
               const int nChannels)
{
   int j;

   COUNT_sub_start("CalcFormFactor");

   LOOP(1);
   for (j=0; j<nChannels; j++) {

      FUNC(3);
      CalcFormFactorChannel(sfbFormFactor[j], sfbNRelevantLines[j],&psyOutChannel[j]);
   }

   COUNT_sub_end();
}


void
EstimateScaleFactors(PSY_OUT_CHANNEL psyOutChannel[MAX_CHANNELS],
                     QC_OUT_CHANNEL  qcOutChannel[MAX_CHANNELS],
                     float  sfbFormFactor[MAX_CHANNELS][MAX_GROUPED_SFB],
                     float sfbNRelevantLines[MAX_CHANNELS][MAX_GROUPED_SFB],
                     const    int nChannels)
{
   int j;

   COUNT_sub_start("EstimateScaleFactors");

   LOOP(1);
   for (j=0; j<nChannels; j++) {

      FUNC(6);
      estimateScaleFactorsChannel(&psyOutChannel[j],
                                  qcOutChannel[j].scf,
                                  &(qcOutChannel[j].globalGain),
                                  sfbFormFactor[j],
                                  sfbNRelevantLines[j],
                                  qcOutChannel[j].quantSpec);
   }

  COUNT_sub_end();
}
