/*
  Framing generator
*/

#include <stdlib.h>
#include <string.h>
#include <math.h>
#include <assert.h>
#include "fram_gen.h"
#include "sbr_misc.h"
#include "FloatFR.h"

#include "counters.h" /* the 3GPP instrumenting tool */

static SBR_FRAME_INFO
frameInfo1_2048 = {1,
                   {0, 16},
                   {FREQ_RES_HIGH},
                   0,
                   1,
                   {0, 16}};

static SBR_FRAME_INFO
frameInfo2_2048 = {2,
                   {0, 8, 16},
                   {FREQ_RES_HIGH, FREQ_RES_HIGH},
                   0,
                   2,
                   {0, 8, 16}};

static SBR_FRAME_INFO
frameInfo4_2048 = {4,
                   {0, 4, 8, 12, 16},
                   {FREQ_RES_HIGH,
                    FREQ_RES_HIGH,
                    FREQ_RES_HIGH,
                    FREQ_RES_HIGH},
                   0,
                   2,
                   {0, 8, 16}};



static void
addFreqLeft (FREQ_RES *vector, int *length_vector, FREQ_RES value)
{
  int i;

  COUNT_sub_start("addFreqLeft");

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


static void
addFreqVecLeft (FREQ_RES *dst, int *length_dst, FREQ_RES *src, int length_src)
{
  int i;

  COUNT_sub_start("addFreqVecLeft");

  PTR_INIT(1); /* src[] */
  LOOP(1);
  for (i = length_src - 1; i >= 0; i--)
  {
    FUNC(3);
    addFreqLeft (dst, length_dst, src[i]);
  }

  COUNT_sub_end();
}



static void
addFreqRight (FREQ_RES *vector, int *length_vector, FREQ_RES value)
{
  COUNT_sub_start("addFreqRight");

  INDIRECT(1); MOVE(1);
  vector[*length_vector] = value;

  ADD(1);
  (*length_vector)++;

  COUNT_sub_end();
}


/***************************************************************************/
/*!
  \brief     Add mandatory borders

  \return    void

****************************************************************************/
static void
fillFrameTran (int *v_bord, int *length_v_bord,
               FREQ_RES *v_freq, int *length_v_freq,
               int *bmin, int *bmax,
               int tran,
               int *v_tuningSegm, FREQ_RES *v_tuningFreq)
{
  int bord, i;

  COUNT_sub_start("fillFrameTran");

  MOVE(2);
  *length_v_bord = 0;
  *length_v_freq = 0;

  BRANCH(1);
  if (v_tuningSegm[0]) {

    ADD(1); FUNC(3);
    AddRight (v_bord, length_v_bord, (tran - v_tuningSegm[0]));

    FUNC(3);
    addFreqRight (v_freq, length_v_freq, v_tuningFreq[0]);
  }

  MOVE(1);
  bord = tran;

  FUNC(3);
  AddRight (v_bord, length_v_bord, tran);

  BRANCH(1);
  if (v_tuningSegm[1]) {

    ADD(1);
    bord += v_tuningSegm[1];

    FUNC(3);
    AddRight (v_bord, length_v_bord, bord);

    FUNC(3);
    addFreqRight (v_freq, length_v_freq, v_tuningFreq[1]);
  }


  BRANCH(1);
  if (v_tuningSegm[2] != 0) {

    ADD(1);
    bord += v_tuningSegm[2];


    FUNC(3);
    AddRight (v_bord, length_v_bord, bord);

    FUNC(3);
    addFreqRight (v_freq, length_v_freq, v_tuningFreq[2]);
  }

  FUNC(3);
  addFreqRight (v_freq, length_v_freq, FREQ_RES_HIGH);


  MOVE(1);
  *bmin = v_bord[0];

  PTR_INIT(1); /* v_bord[] */
  LOOP(1);
  for (i = 0; i < *length_v_bord; i++)
  {
    ADD(1); BRANCH(1);
    if (v_bord[i] < *bmin)
    {
      MOVE(1);
      *bmin = v_bord[i];
    }
  }

  MOVE(1);
  *bmax = v_bord[0];

  PTR_INIT(1); /* v_bord[] */
  LOOP(1);
  for (i = 0; i < *length_v_bord; i++)
  {
    ADD(1); BRANCH(1);
    if (v_bord[i] > *bmax)
    {
      MOVE(1);
      *bmax = v_bord[i];
    }
  }

  COUNT_sub_end();
}


/***************************************************************************/
/*!
  \brief     Add borders before mandatory borders, if needed

  \return    void

****************************************************************************/
static void
fillFramePre (int dmax,
              int *v_bord, int *length_v_bord,
              FREQ_RES *v_freq, int *length_v_freq,
              int bmin, int rest)
{

  int parts, d, j, S, s = 0, segm, bord;

  COUNT_sub_start("fillFramePre");

  MOVE(1); /* counting previous operation */


  MOVE(2);
  parts = 1;
  d = rest;

  LOOP(1);
  while (d > dmax) {
    ADD(1);
    parts++;

    DIV(1);
    segm = rest / parts;

    ADD(1); MULT(1); MISC(1);
    S = (int) floor ((segm - 2) * 0.5);

    MULT(1); ADD(2); BRANCH(1); MOVE(1);
    s = min (8, 2 * S + 2);

    ADD(2); MULT(1);
    d = rest - (parts - 1) * s;
  }


  MOVE(1);
  bord = bmin;

  LOOP(1);
  for (j = 0; j <= parts - 2; j++) {

    ADD(1);
    bord = bord - s;

    FUNC(3);
    AddLeft (v_bord, length_v_bord, bord);

    FUNC(3);
    addFreqLeft (v_freq, length_v_freq, FREQ_RES_HIGH);
  }

  COUNT_sub_end();
}

/***************************************************************************/
/*!
  \brief Add borders after mandatory borders

  \return    void

****************************************************************************/
static void
fillFramePost (int *parts, int *d, int dmax, int *v_bord, int *length_v_bord,
               FREQ_RES *v_freq, int *length_v_freq, int bmax,
               int fmax)
{
  int j, rest, segm, S, s = 0, bord;

  COUNT_sub_start("fillFramePost");

  MOVE(1); /* counting previous operation */

  ADD(1);
  rest = 32 - bmax;

  MOVE(1);
  *d = rest;

  BRANCH(1);
  if (*d > 0) {

    MOVE(1);
    *parts = 1;           /* start with one envelope */

    LOOP(1);
    while (*d > dmax) {

      ADD(1); STORE(1);
      *parts = *parts + 1;

      DIV(1);
      segm = rest / (*parts);

      ADD(1); MULT(1); MISC(1);
      S = (int) floor ((segm - 2) * 0.5);

      MULT(1); ADD(2); BRANCH(1); MOVE(1);
      s = min (fmax, 2 * S + 2);

      ADD(2); MULT(1); STORE(1);
      *d = rest - (*parts - 1) * s;
    }

    MOVE(1);
    bord = bmax;

    ADD(1); LOOP(1);
    for (j = 0; j <= *parts - 2; j++) {

      ADD(1);
      bord += s;

      FUNC(3);
      AddRight (v_bord, length_v_bord, bord);

      FUNC(3);
      addFreqRight (v_freq, length_v_freq, FREQ_RES_HIGH);
    }
  }
  else {
    MOVE(1);
    *parts = 1;

    ADD(2); STORE(2);
    *length_v_bord = *length_v_bord - 1;
    *length_v_freq = *length_v_freq - 1;

  }

  COUNT_sub_end();
}


/***************************************************************************/
/*!
  \brief    Fills in the envelopes between transients

  \return   void

****************************************************************************/
static void
fillFrameInter (int *nL,
                int *v_tuningSegm,
                int *v_bord,
                int *length_v_bord,
                int bmin,
                FREQ_RES *v_freq,
                int *length_v_freq,
                int *v_bordFollow,
                int *length_v_bordFollow,
                FREQ_RES *v_freqFollow,
                int *length_v_freqFollow,
                int i_fillFollow,
                int dmin,
                int dmax)
{
  int middle, b_new, numBordFollow, bordMaxFollow, i;

  COUNT_sub_start("fillFrameInter");


    ADD(1); BRANCH(1);
    if (i_fillFollow >= 1) {

      MOVE(2);
      *length_v_bordFollow = i_fillFollow;
      *length_v_freqFollow = i_fillFollow;
    }

    MOVE(1);
    numBordFollow = *length_v_bordFollow;

    INDIRECT(1); MOVE(1);
    bordMaxFollow = v_bordFollow[numBordFollow - 1];

    ADD(1);
    middle = bmin - bordMaxFollow;

    PTR_INIT(1); /* v_bordFollow[] */
    LOOP(1);
    while (middle < 0) {

      ADD(1);
      numBordFollow--;

      MOVE(1);
      bordMaxFollow = v_bordFollow[numBordFollow - 1];

      ADD(1);
      middle = bmin - bordMaxFollow;
    }

    MOVE(2);
    *length_v_bordFollow = numBordFollow;
    *length_v_freqFollow = numBordFollow;

    ADD(1); STORE(1);
    *nL = numBordFollow - 1;

    MOVE(1);
    b_new = *length_v_bord;


    ADD(1); BRANCH(1);
    if (middle <= dmax) {

      ADD(1); BRANCH(1);
      if (middle >= dmin) {

        FUNC(4);
        AddVecLeft (v_bord, length_v_bord, v_bordFollow, *length_v_bordFollow);

        FUNC(4);
        addFreqVecLeft (v_freq, length_v_freq, v_freqFollow, *length_v_freqFollow);
      }

      else {

        BRANCH(1);
        if (v_tuningSegm[0] != 0) {

          ADD(1); STORE(1);
          *length_v_bord = b_new - 1;

          FUNC(4);
          AddVecLeft (v_bord, length_v_bord, v_bordFollow,
              *length_v_bordFollow);

          ADD(1); STORE(1);
          *length_v_freq = b_new - 1;

          PTR_INIT(1); FUNC(4);
          addFreqVecLeft (v_freq + 1, length_v_freq, v_freqFollow,
                          *length_v_freqFollow);

        }
        else {

          ADD(1); BRANCH(1);
          if (*length_v_bordFollow > 1) {

            PTR_INIT(1); FUNC(4);
            AddVecLeft (v_bord, length_v_bord, v_bordFollow,
                        *length_v_bordFollow - 1);

            PTR_INIT(1); FUNC(4);
            addFreqVecLeft (v_freq, length_v_freq, v_freqFollow,
                            *length_v_bordFollow - 1);

            ADD(1); STORE(1);
            *nL = *nL - 1;
          }
          else {


            PTR_INIT(1); /* v_bord[] */
            LOOP(1);
            for (i = 0; i < *length_v_bord - 1; i++)
            {
              MOVE(1);
              v_bord[i] = v_bord[i + 1];
            }

            PTR_INIT(1); /* v_freq[] */
            LOOP(1);
            for (i = 0; i < *length_v_freq - 1; i++)
            {
              MOVE(1);
              v_freq[i] = v_freq[i + 1];
            }

            ADD(2); STORE(2);
            *length_v_bord = b_new - 1;
            *length_v_freq = b_new - 1;

            FUNC(4);
            AddVecLeft (v_bord, length_v_bord, v_bordFollow,
                        *length_v_bordFollow);

            FUNC(4);
            addFreqVecLeft (v_freq, length_v_freq, v_freqFollow,
                            *length_v_freqFollow);
          }
        }
      }
    }
    else {

      FUNC(7);
      fillFramePre (dmax, v_bord, length_v_bord, v_freq, length_v_freq, bmin,
                    middle);

      FUNC(4);
      AddVecLeft (v_bord, length_v_bord, v_bordFollow, *length_v_bordFollow);

      FUNC(4);
      addFreqVecLeft (v_freq, length_v_freq, v_freqFollow, *length_v_freqFollow);
    }

  COUNT_sub_end();
}



/***************************************************************************/
/*!
  \brief    Calculates frame class for current frame

  \return   void

****************************************************************************/
static void
calcFrameClass (FRAME_CLASS *frameClass,
                FRAME_CLASS *frameClassOld,
                int tranFlag,
                int *pSpreadFlag
                )
{
  COUNT_sub_start("calcFrameClass");

  BRANCH(2);
  switch (*frameClassOld) {

  case FIXFIX:

    BRANCH(1); MOVE(1);
    if (tranFlag)
      *frameClass = FIXVAR;
    else
      *frameClass = FIXFIX;
    break;

  case FIXVAR:

    BRANCH(1);
    if (tranFlag) {

      MOVE(2);
      *frameClass = VARVAR;
      *pSpreadFlag = 0;
    }
    else {

      BRANCH(1); MOVE(1);
      if (*pSpreadFlag)
        *frameClass = VARVAR;
      else
        *frameClass = VARFIX;
    }
    break;

  case VARFIX:

    BRANCH(1); MOVE(1);
    if (tranFlag)
      *frameClass = FIXVAR;
    else
      *frameClass = FIXFIX;
    break;

  case VARVAR:

    BRANCH(1);
    if (tranFlag) {

      MOVE(2);
      *frameClass = VARVAR;
      *pSpreadFlag = 0;
    }
    else {

      BRANCH(1); MOVE(1);
      if (*pSpreadFlag)
        *frameClass = VARVAR;
      else
        *frameClass = VARFIX;
    }
    break;

  default:
    assert(0);
  }

  MOVE(1);
  *frameClassOld = *frameClass;

  COUNT_sub_end();
}


/***************************************************************************/
/*!
  \brief    Handles special case for the framing generator

  \return   void

****************************************************************************/
static void
specialCase (int *pSpreadFlag,
             int allowSpread,
             int *aBorders,
             int *pBorderVecLen,
             FREQ_RES *aFreqRes,
             int *pFreqResVecLen,
             int *pNumParts,
             int d
             )
{
  int L;

  COUNT_sub_start("specialCase");

  MOVE(1);
  L = *pBorderVecLen;

  BRANCH(1);
  if (allowSpread) {

    MOVE(1);
    *pSpreadFlag = 1;

    INDIRECT(1); ADD(1); FUNC(3);
    AddRight (aBorders, pBorderVecLen, aBorders[L - 1] + 8);

    FUNC(3);
    addFreqRight (aFreqRes, pFreqResVecLen, FREQ_RES_HIGH);

    ADD(1); STORE(1);
    (*pNumParts)++;
  }
  else {

    ADD(1); BRANCH(1);
    if (d == 1) {

      ADD(2); STORE(2);
      *pBorderVecLen = L - 1;
      *pFreqResVecLen = L - 1;
    }
    else {

      INDIRECT(2); ADD(2); BRANCH(1);
      if ((aBorders[L - 1] - aBorders[L - 2]) > 2) {

        ADD(1); STORE(1);
        aBorders[L - 1] = aBorders[L - 1] - 2;

        INDIRECT(1); MOVE(1);
        aFreqRes[*pFreqResVecLen - 1] = FREQ_RES_LOW;
      }
    }
  }

  COUNT_sub_end();
}



/***************************************************************************/
/*!
  \brief    Calculates a common border

  \return   void

****************************************************************************/
static void
calcCmonBorder (int *pCmonBorderIdx,  /*!< */
                int *pTranIdx,        /*!< */
                int *aBorders,        /*!< */
                int *pBorderVecLen,   /*!< */
                int tran             /*!< */


                )
{
  int i;

  COUNT_sub_start("calcCmonBorder");

  PTR_INIT(1); /* aBorders[] */
  LOOP(1);
  for (i = 0; i < *pBorderVecLen; i++)
  {
    ADD(1); BRANCH(1);
    if (aBorders[i] >= 16) {

      MOVE(1);
      *pCmonBorderIdx = i;
      break;
    }
  }

  PTR_INIT(1); /* aBorders[] */
  LOOP(1);
  for (i = 0; i < *pBorderVecLen; i++)
  {
    ADD(1); BRANCH(1);
    if (aBorders[i] >= tran) {

      MOVE(1);
      *pTranIdx = i;
      break;
    }
    else
    {
      MOVE(1);
      *pTranIdx = EMPTY;
    }
  }

  COUNT_sub_end();
}



/***************************************************************************/
/*!
  \brief    Stores values for following frame

  \return   void

****************************************************************************/
static void
keepForFollowUp (int *aBordersFollow,
                 int *pBorderVecLenFollow,
                 FREQ_RES *aFreqResFollow,
                 int *pFreqResVecLenFollow,
                 int *pTranIdxFollow,
                 int *pFillIdxFollow,
                 int *aBorders,
                 int *pBorderVecLen,
                 FREQ_RES *aFreqRes,
                 int cmonBorderIdx,
                 int tranIdx,
                 int numParts

                 )
{
  int L, i, j;

  COUNT_sub_start("keepForFollowUp");

  MOVE(1);
  L = *pBorderVecLen;

  MOVE(2);
  (*pBorderVecLenFollow) = 0;
  (*pFreqResVecLenFollow) = 0;

  PTR_INIT(4); /* aBordersFollow[]
                  aBorders[]
                  aFreqResFollow[]
                  aFreqRes[];
               */
  LOOP(1);
  for (j = 0, i = cmonBorderIdx; i < L; i++, j++) {

    ADD(1); STORE(1);
    aBordersFollow[j] = aBorders[i] - 16;

    MOVE(1);
    aFreqResFollow[j] = aFreqRes[i];

    ADD(2); STORE(2);
    (*pBorderVecLenFollow)++;
    (*pFreqResVecLenFollow)++;
  }

  ADD(1); BRANCH(1);
  if (tranIdx != EMPTY)
  {
    ADD(1); STORE(1);
    *pTranIdxFollow = tranIdx - cmonBorderIdx;
  }
  else
  {
    MOVE(1);
    *pTranIdxFollow = EMPTY;
  }

  ADD(3); STORE(1);
  *pFillIdxFollow = L - (numParts - 1) - cmonBorderIdx;

  COUNT_sub_end();
}


/***************************************************************************/
/*!
  \brief    Calculates the control signal

  \return   void

****************************************************************************/
static void
calcCtrlSignal (HANDLE_SBR_GRID hSbrGrid,
                FRAME_CLASS frameClass, int *v_bord, int length_v_bord, FREQ_RES *v_freq,
                int length_v_freq, int i_cmon, int i_tran, int spreadFlag,
                int nL)
{


  int i, r, a, n, p, b, aL, aR, ntot, nmax, nR;

  FREQ_RES *v_f = hSbrGrid->v_f;
  FREQ_RES *v_fLR = hSbrGrid->v_fLR;
  int *v_r = hSbrGrid->bs_rel_bord;
  int *v_rL = hSbrGrid->bs_rel_bord_0;
  int *v_rR = hSbrGrid->bs_rel_bord_1;

  int length_v_r = 0;
  int length_v_rR = 0;
  int length_v_rL = 0;

  COUNT_sub_start("calcCtrlSignal");

  INDIRECT(5); PTR_INIT(5); MOVE(3); /* counting previous operations */

  BRANCH(2);
  switch (frameClass) {
  case FIXVAR:

    MOVE(1);
    a = v_bord[i_cmon];

    MOVE(2);
    length_v_r = 0;
    i = i_cmon;

    PTR_INIT(1); /* v_bord[i] */
    LOOP(1);
    while (i >= 1) {

      ADD(1);
      r = v_bord[i] - v_bord[i - 1];

      PTR_INIT(1); FUNC(3);
      AddRight (v_r, &length_v_r, r);

      i--;
    }


    MOVE(1);
    n = length_v_r;

    PTR_INIT(2); /* v_f[i]
                    v_freq[i_cmon - 1 - i]
                 */
    LOOP(1);
    for (i = 0; i < i_cmon; i++)
    {
      MOVE(1);
      v_f[i] = v_freq[i_cmon - 1 - i];
    }

    INDIRECT(1); MOVE(1);
    v_f[i_cmon] = FREQ_RES_HIGH;

    /* pointer: */
    ADD(2); LOGIC(1); BRANCH(1);
    if (i_cmon >= i_tran && i_tran != EMPTY)
    {
      ADD(2);
      p = i_cmon - i_tran + 1;
    }
    else
    {
      MOVE(1);
      p = 0;
    }

    INDIRECT(4); MOVE(4);
    hSbrGrid->frameClass = frameClass;
    hSbrGrid->bs_abs_bord = a;
    hSbrGrid->n = n;
    hSbrGrid->p = p;

    break;
  case VARFIX:

    MOVE(1);
    a = v_bord[0];

    MOVE(1);
    length_v_r = 0;

    PTR_INIT(1); /* v_bord[] */
    LOOP(1);
    for (i = 1; i < length_v_bord; i++) {

      ADD(1);
      r = v_bord[i] - v_bord[i - 1];

      PTR_INIT(1); FUNC(3);
      AddRight (v_r, &length_v_r, r);
    }


    MOVE(1);
    n = length_v_r;

    FUNC(2); LOOP(1); PTR_INIT(2); MOVE(1); STORE(length_v_freq);
    memcpy (v_f, v_freq, length_v_freq * sizeof (int));

    /* pointer: */
    ADD(1); LOGIC(1); BRANCH(1);
    if (i_tran >= 0 && i_tran != EMPTY)
    {
      ADD(1);
      p = i_tran + 1;
    }
    else
    {
      MOVE(1);
      p = 0;
    }

    INDIRECT(4); MOVE(4);
    hSbrGrid->frameClass = frameClass;
    hSbrGrid->bs_abs_bord = a;
    hSbrGrid->n = n;
    hSbrGrid->p = p;

    break;
  case VARVAR:

    BRANCH(1);
    if (spreadFlag) {

      MOVE(1);
      b = length_v_bord;

      MOVE(1);
      aL = v_bord[0];

      INDIRECT(1); MOVE(1);
      aR = v_bord[b - 1];


      ADD(1);
      ntot = b - 2;

      MOVE(1);
      nmax = 2;

      ADD(1); BRANCH(1);
      if (ntot > nmax) {

        MOVE(2);
        nL = nmax;
        nR = ntot - nmax;
      }
      else {

        MOVE(2);
        nL = ntot;
        nR = 0;
      }

      MOVE(1);
      length_v_rL = 0;

      PTR_INIT(1); /* v_bord[] */
      LOOP(1);
      for (i = 1; i <= nL; i++) {

        ADD(1);
        r = v_bord[i] - v_bord[i - 1];

        PTR_INIT(1); FUNC(3);
        AddRight (v_rL, &length_v_rL, r);
      }

      MOVE(1);
      length_v_rR = 0;

      ADD(1);
      i = b - 1;

      PTR_INIT(1); /* v_bord[] */
      ADD(1); LOOP(1);
      while (i >= b - nR) {

        ADD(1);
        r = v_bord[i] - v_bord[i - 1];

        PTR_INIT(1); FUNC(3);
        AddRight (v_rR, &length_v_rR, r);

        i--;
      }


      ADD(1); LOGIC(1); BRANCH(1);
      if (i_tran > 0 && i_tran != EMPTY)
      {
        ADD(1);
        p = b - i_tran;
      }
      else
      {
        MOVE(1);
        p = 0;
      }


      PTR_INIT(2); /* v_fLR[i]
                      v_freq[i]
                   */
      LOOP(1);
      for (i = 0; i < b - 1; i++)
      {
        MOVE(1);
        v_fLR[i] = v_freq[i];
      }
    }
    else {

      ADD(2);
      length_v_bord = i_cmon + 1;
      length_v_freq = i_cmon + 1;


      MOVE(1);
      b = length_v_bord;

      MOVE(1);
      aL = v_bord[0];

      INDIRECT(1); MOVE(1);
      aR = v_bord[b - 1];

      ADD(2);
      ntot = b - 2;
      nR = ntot - nL;


      MOVE(1);
      length_v_rL = 0;

      PTR_INIT(1); /* v_bord[i] */
      LOOP(1);
      for (i = 1; i <= nL; i++) {

        ADD(1);
        r = v_bord[i] - v_bord[i - 1];

        PTR_INIT(1); FUNC(3);
        AddRight (v_rL, &length_v_rL, r);
      }



      MOVE(1);
      length_v_rR = 0;

      ADD(1);
      i = b - 1;

      PTR_INIT(1); /* v_bord[i] */
      ADD(1); LOOP(1);
      while (i >= b - nR) {

        ADD(1);
        r = v_bord[i] - v_bord[i - 1];

        PTR_INIT(1); FUNC(3);
        AddRight (v_rR, &length_v_rR, r);

        i--;
      }



      ADD(2); LOGIC(1); BRANCH(1);
      if (i_cmon >= i_tran && i_tran != EMPTY)
      {
        ADD(2);
        p = i_cmon - i_tran + 1;
      }
      else
      {
        MOVE(1);
        p = 0;
      }




      PTR_INIT(2); /* v_fLR[i]
                      v_freq[i]
                   */
      LOOP(1);
      for (i = 0; i < b - 1; i++)
      {
        MOVE(1);
        v_fLR[i] = v_freq[i];
      }

    }


    INDIRECT(6); MOVE(6);
    hSbrGrid->frameClass = frameClass;
    hSbrGrid->bs_abs_bord_0 = aL;
    hSbrGrid->bs_abs_bord_1 = aR;
    hSbrGrid->bs_num_rel_0 = nL;
    hSbrGrid->bs_num_rel_1 = nR;
    hSbrGrid->p = p;

    break;

  default:
    /* do nothing */
    break;
  }

  COUNT_sub_end();
}


/***************************************************************************/
/*!
  \brief    Creates a frame info default

  \return   void

****************************************************************************/
static void
createDefFrameInfo(HANDLE_SBR_FRAME_INFO hSbrFrameInfo,
                   int nEnv

                   )
{
  COUNT_sub_start("createDefFrameInfo");

  BRANCH(2);
  switch (nEnv) {
  case 1:



      FUNC(2); LOOP(1); PTR_INIT(2); MOVE(1); STORE(sizeof (SBR_FRAME_INFO));
      memcpy (hSbrFrameInfo, &frameInfo1_2048, sizeof (SBR_FRAME_INFO));


    break;

  case 2:



      FUNC(2); LOOP(1); PTR_INIT(2); MOVE(1); STORE(sizeof (SBR_FRAME_INFO));
      memcpy (hSbrFrameInfo, &frameInfo2_2048, sizeof (SBR_FRAME_INFO));


    break;

  case 4:



      FUNC(2); LOOP(1); PTR_INIT(2); MOVE(1); STORE(sizeof (SBR_FRAME_INFO));
      memcpy (hSbrFrameInfo, &frameInfo4_2048, sizeof (SBR_FRAME_INFO));


    break;

  default:
    assert(0);
  }

  COUNT_sub_end();
}

/***************************************************************************/
/*!
  \brief    Translates frame_info struct to control signal

  \return   void

****************************************************************************/
static void
ctrlSignal2FrameInfo (HANDLE_SBR_GRID hSbrGrid,
                      HANDLE_SBR_FRAME_INFO hSbrFrameInfo,
                      FREQ_RES freq_res_fixfix
                      )
{
  int nEnv = 0, border = 0, i, k, p;
  int *v_r = hSbrGrid->bs_rel_bord;
  FREQ_RES *v_f = hSbrGrid->v_f;

  FRAME_CLASS frameClass = hSbrGrid->frameClass;



  COUNT_sub_start("ctrlSignal2FrameInfo");

  INDIRECT(3); MOVE(3); PTR_INIT(2); /* counting previous operations */

  BRANCH(2);
  switch (frameClass) {
  case FIXFIX:
    INDIRECT(1); FUNC(2);
    createDefFrameInfo(hSbrFrameInfo, hSbrGrid->bs_num_env);

    ADD(1); BRANCH(1);
    if (freq_res_fixfix == FREQ_RES_LOW) {

      PTR_INIT(1); /* hSbrFrameInfo->freqRes[] */
      INDIRECT(1); LOOP(1);
      for (i = 0; i < hSbrFrameInfo->nEnvelopes; i++) {
        MOVE(1);
        hSbrFrameInfo->freqRes[i] = FREQ_RES_LOW;
      }
    }
    break;

  case FIXVAR:
  case VARFIX:

    INDIRECT(1); ADD(1);
    nEnv = hSbrGrid->n + 1;

    assert(nEnv <= MAX_ENVELOPES_FIXVAR_VARFIX);

    INDIRECT(1); MOVE(1);
    hSbrFrameInfo->nEnvelopes = nEnv;

    INDIRECT(1); MOVE(1);
    border = hSbrGrid->bs_abs_bord;

    ADD(1); BRANCH(1); INDIRECT(1); MOVE(1);
    if (nEnv == 1)
      hSbrFrameInfo->nNoiseEnvelopes = 1;
    else
      hSbrFrameInfo->nNoiseEnvelopes = 2;

    break;

  default:
    /* do nothing */
    break;
  }

  BRANCH(2);
  switch (frameClass) {
  case FIXVAR:

    INDIRECT(1); MOVE(1);
    hSbrFrameInfo->borders[0] = 0;

    INDIRECT(1); MOVE(1);
    hSbrFrameInfo->borders[nEnv] = border;

    PTR_INIT(2); /* v_r[k];
                    hSbrFrameInfo->borders[i]
                 */
    ADD(1); LOOP(1);
    for (k = 0, i = nEnv - 1; k < nEnv - 1; k++, i--) {
      ADD(1);
      border -= v_r[k];

      MOVE(1);
      hSbrFrameInfo->borders[i] = border;
    }

    INDIRECT(1); MOVE(1);
    p = hSbrGrid->p;

    BRANCH(1);
    if (p == 0) {

      INDIRECT(1); MOVE(1);
      hSbrFrameInfo->shortEnv = 0;
    } else {

      INDIRECT(1); ADD(2); STORE(1);
      hSbrFrameInfo->shortEnv = nEnv + 1 - p;
    }

    PTR_INIT(2); /* hSbrFrameInfo->freqRes[i]
                    v_f[k]
                 */
    ADD(1); LOOP(1);
    for (k = 0, i = nEnv - 1; k < nEnv; k++, i--) {
      MOVE(1);
      hSbrFrameInfo->freqRes[i] = v_f[k];
    }

    ADD(1); LOGIC(1); BRANCH(1);
    if (p == 0 || p == 1) {
      INDIRECT(2); MOVE(1);
      hSbrFrameInfo->bordersNoise[1] = hSbrFrameInfo->borders[nEnv - 1];
    } else {
      INDIRECT(2); MOVE(1);
      hSbrFrameInfo->bordersNoise[1] = hSbrFrameInfo->borders[hSbrFrameInfo->shortEnv];
    }

    break;

  case VARFIX:
    INDIRECT(1); MOVE(1);
    hSbrFrameInfo->borders[0] = border;

    PTR_INIT(2); /* v_r[]
                    hSbrFrameInfo->borders[]
                 */
    ADD(1); LOOP(1);
    for (k = 0; k < nEnv - 1; k++) {
      ADD(1);
      border += v_r[k];

      MOVE(1);
      hSbrFrameInfo->borders[k + 1] = border;
    }

    INDIRECT(1); STORE(1);
    hSbrFrameInfo->borders[nEnv] = 16;

    INDIRECT(1); MOVE(1);
    p = hSbrGrid->p;

    ADD(1); LOGIC(1); BRANCH(1);
    if (p == 0 || p == 1) {
      INDIRECT(1); MOVE(1);
      hSbrFrameInfo->shortEnv = 0;
    } else {
      INDIRECT(1); ADD(1); STORE(1);
      hSbrFrameInfo->shortEnv = p - 1;
    }

    PTR_INIT(2); /* hSbrFrameInfo->freqRes[k]
                    v_f[k]
                 */
    LOOP(1);
    for (k = 0; k < nEnv; k++) {
      MOVE(1);
      hSbrFrameInfo->freqRes[k] = v_f[k];
    }

    BRANCH(2);
    switch (p) {
    case 0:
      INDIRECT(2); MOVE(1);
      hSbrFrameInfo->bordersNoise[1] = hSbrFrameInfo->borders[1];
      break;
    case 1:
      INDIRECT(2); MOVE(1);
      hSbrFrameInfo->bordersNoise[1] = hSbrFrameInfo->borders[nEnv - 1];
      break;
    default:
      INDIRECT(2); MOVE(1);
      hSbrFrameInfo->bordersNoise[1] = hSbrFrameInfo->borders[hSbrFrameInfo->shortEnv];
      break;
    }
    break;

  case VARVAR:
    INDIRECT(2); ADD(2);
    nEnv = hSbrGrid->bs_num_rel_0 + hSbrGrid->bs_num_rel_1 + 1;

    assert(nEnv <= MAX_ENVELOPES_VARVAR); /* just to be sure */

    INDIRECT(1); MOVE(1);
    hSbrFrameInfo->nEnvelopes = nEnv;

    INDIRECT(2); ADD(1); STORE(1);
    hSbrFrameInfo->borders[0] = border = hSbrGrid->bs_abs_bord_0;

    PTR_INIT(2); /* hSbrGrid->bs_rel_bord_0[k]
                    hSbrFrameInfo->borders[i]
                 */
    INDIRECT(1); LOOP(1);
    for (k = 0, i = 1; k < hSbrGrid->bs_num_rel_0; k++, i++) {
      ADD(1);
      border += hSbrGrid->bs_rel_bord_0[k];

      MOVE(1);
      hSbrFrameInfo->borders[i] = border;
    }

    INDIRECT(1); MOVE(1);
    border = hSbrGrid->bs_abs_bord_1;

    INDIRECT(1); MOVE(1);
    hSbrFrameInfo->borders[nEnv] = border;

    PTR_INIT(2); /* hSbrGrid->bs_rel_bord_1[k]
                    hSbrFrameInfo->borders[i]
                 */
    INDIRECT(1); ADD(1); LOOP(1);
    for (k = 0, i = nEnv - 1; k < hSbrGrid->bs_num_rel_1; k++, i--) {
      ADD(1);
      border -= hSbrGrid->bs_rel_bord_1[k];

      MOVE(1);
      hSbrFrameInfo->borders[i] = border;
    }

    INDIRECT(1); MOVE(1);
    p = hSbrGrid->p;

    BRANCH(1);
    if (p == 0) {

      INDIRECT(1); MOVE(1);
      hSbrFrameInfo->shortEnv = 0;
    } else {

      INDIRECT(1); ADD(2); STORE(1);
      hSbrFrameInfo->shortEnv = nEnv + 1 - p;
    }

    PTR_INIT(2); /* hSbrFrameInfo->freqRes[k]
                    hSbrGrid->v_fLR[k]
                 */
    LOOP(1);
    for (k = 0; k < nEnv; k++) {
      MOVE(1);
      hSbrFrameInfo->freqRes[k] = hSbrGrid->v_fLR[k];
    }

    ADD(1); BRANCH(1);
    if (nEnv == 1) {

      INDIRECT(5); MOVE(3);
      hSbrFrameInfo->nNoiseEnvelopes = 1;
      hSbrFrameInfo->bordersNoise[0] = hSbrGrid->bs_abs_bord_0;
      hSbrFrameInfo->bordersNoise[1] = hSbrGrid->bs_abs_bord_1;
    } else {

      INDIRECT(3); MOVE(2);
      hSbrFrameInfo->nNoiseEnvelopes = 2;
      hSbrFrameInfo->bordersNoise[0] = hSbrGrid->bs_abs_bord_0;

      ADD(1); LOGIC(1); BRANCH(1);
      if (p == 0 || p == 1) {

        INDIRECT(2); MOVE(1);
        hSbrFrameInfo->bordersNoise[1] = hSbrFrameInfo->borders[nEnv - 1];
      } else {

        INDIRECT(2); MOVE(1);
        hSbrFrameInfo->bordersNoise[1] = hSbrFrameInfo->borders[hSbrFrameInfo->shortEnv];
      }

      INDIRECT(2); MOVE(1);
      hSbrFrameInfo->bordersNoise[2] = hSbrGrid->bs_abs_bord_1;
    }
    break;

  default:
    /* do nothing */
    break;
  }

  ADD(2); LOGIC(1); BRANCH(1);
  if (frameClass == VARFIX || frameClass == FIXVAR) {

    INDIRECT(2); MOVE(1);
    hSbrFrameInfo->bordersNoise[0] = hSbrFrameInfo->borders[0];

    ADD(1); BRANCH(1);
    if (nEnv == 1) {

      INDIRECT(2); MOVE(1);
      hSbrFrameInfo->bordersNoise[1] = hSbrFrameInfo->borders[nEnv];
    } else {

      INDIRECT(2); MOVE(1);
      hSbrFrameInfo->bordersNoise[2] = hSbrFrameInfo->borders[nEnv];
    }
  }

  COUNT_sub_end();
}


/***************************************************************************/
/*!
  \brief    Produces the FRAME_INFO struct for the current frame

  \return   The frame info handle for the current frame

****************************************************************************/
HANDLE_SBR_FRAME_INFO
frameInfoGenerator (HANDLE_SBR_ENVELOPE_FRAME hSbrEnvFrame,
                    int *v_pre_transient_info,
                    int *v_transient_info,
                    int *v_tuning)
{
  int numEnv, tran=0, bmin=0, bmax=0, parts, d, i_cmon, i_tran, nL;
  int fmax = 0;

  int *v_bord = hSbrEnvFrame->v_bord;
  FREQ_RES *v_freq = hSbrEnvFrame->v_freq;
  int *v_bordFollow = hSbrEnvFrame->v_bordFollow;
  FREQ_RES *v_freqFollow = hSbrEnvFrame->v_freqFollow;


  int *length_v_bordFollow = &hSbrEnvFrame->length_v_bordFollow;
  int *length_v_freqFollow = &hSbrEnvFrame->length_v_freqFollow;
  int *length_v_bord = &hSbrEnvFrame->length_v_bord;
  int *length_v_freq = &hSbrEnvFrame->length_v_freq;
  int *spreadFlag = &hSbrEnvFrame->spreadFlag;
  int *i_tranFollow = &hSbrEnvFrame->i_tranFollow;
  int *i_fillFollow = &hSbrEnvFrame->i_fillFollow;
  FRAME_CLASS *frameClassOld = &hSbrEnvFrame->frameClassOld;
  FRAME_CLASS frameClass;


  int allowSpread = hSbrEnvFrame->allowSpread;
  int numEnvStatic = hSbrEnvFrame->numEnvStatic;
  int staticFraming = hSbrEnvFrame->staticFraming;
  int dmin = hSbrEnvFrame->dmin;
  int dmax = hSbrEnvFrame->dmax;





  int tranPos = v_transient_info[0];
  int tranFlag = v_transient_info[1];

  int *v_tuningSegm = v_tuning;
  FREQ_RES *v_tuningFreq = (FREQ_RES*) (v_tuning + 3);

  FREQ_RES freq_res_fixfix = hSbrEnvFrame->freq_res_fixfix;

  COUNT_sub_start("frameInfoGenerator");

  INDIRECT(18); MOVE(12); PTR_INIT(14); /* counting previous operation */


  BRANCH(1);
  if (staticFraming) {

    INDIRECT(2); MOVE(5);
    frameClass = FIXFIX;
    numEnv = numEnvStatic;
    *frameClassOld = FIXFIX;
    hSbrEnvFrame->SbrGrid.bs_num_env = numEnv;
    hSbrEnvFrame->SbrGrid.frameClass = frameClass;
  }
  else {
    PTR_INIT(1); FUNC(4);
    calcFrameClass (&frameClass, frameClassOld, tranFlag, spreadFlag);

    BRANCH(1);
    if (tranFlag) {

      ADD(1); BRANCH(1);
      if (tranPos < 4)
      {
        MOVE(1);
        fmax = 6;
      }
      else {
      ADD(2); LOGIC(1); BRANCH(1); MOVE(1);
      if (tranPos == 4 || tranPos == 5)
        fmax = 4;
      else
        fmax = 8;
      }

      ADD(1);
      tran = tranPos + 4;

      PTR_INIT(2); FUNC(9);
      fillFrameTran (v_bord, length_v_bord, v_freq, length_v_freq, &bmin,
                     &bmax, tran, v_tuningSegm, v_tuningFreq);
    }

    BRANCH(2);
    switch (frameClass) {
    case FIXVAR:

      FUNC(7);
      fillFramePre (dmax, v_bord, length_v_bord, v_freq, length_v_freq,
                    bmin, bmin);


      PTR_INIT(2); FUNC(9);
      fillFramePost (&parts, &d, dmax, v_bord, length_v_bord, v_freq,
                     length_v_freq, bmax, fmax);

      ADD(2); LOGIC(1); BRANCH(1);
      if (parts == 1 && d < dmin)
      {
        PTR_INIT(1); FUNC(8);
        specialCase (spreadFlag, allowSpread, v_bord, length_v_bord,
                     v_freq, length_v_freq, &parts, d);
      }

      PTR_INIT(2); FUNC(5);
      calcCmonBorder (&i_cmon, &i_tran, v_bord, length_v_bord, tran
                      );
      FUNC(12);
      keepForFollowUp (v_bordFollow, length_v_bordFollow, v_freqFollow,
                       length_v_freqFollow, i_tranFollow, i_fillFollow,
                       v_bord, length_v_bord, v_freq, i_cmon, i_tran, parts);  /* FH 00-06-26 */

      INDIRECT(1); PTR_INIT(1); FUNC(10);
      calcCtrlSignal (&hSbrEnvFrame->SbrGrid, frameClass,
                      v_bord, *length_v_bord, v_freq, *length_v_freq,
                      i_cmon, i_tran, *spreadFlag, DC);
      break;
    case VARFIX:
      INDIRECT(1); PTR_INIT(1); FUNC(10);
      calcCtrlSignal (&hSbrEnvFrame->SbrGrid, frameClass,
                      v_bordFollow, *length_v_bordFollow, v_freqFollow,
                      *length_v_freqFollow, DC, *i_tranFollow,
                      *spreadFlag, DC);
      break;
    case VARVAR:

      BRANCH(1);
      if (*spreadFlag) {
        INDIRECT(1); PTR_INIT(1); FUNC(10);
        calcCtrlSignal (&hSbrEnvFrame->SbrGrid,
                        frameClass, v_bordFollow, *length_v_bordFollow,
                        v_freqFollow, *length_v_freqFollow, DC,
                        *i_tranFollow, *spreadFlag, DC);

        MOVE(1);
        *spreadFlag = 0;

        INDIRECT(1); ADD(1); STORE(1);
        v_bordFollow[0] = hSbrEnvFrame->SbrGrid.bs_abs_bord_1 - 16;

        MOVE(3);
        v_freqFollow[0] = FREQ_RES_HIGH;
        *length_v_bordFollow = 1;
        *length_v_freqFollow = 1;

        MULT(2); STORE(2);
        *i_tranFollow = -DC;
        *i_fillFollow = -DC;
      }
      else {
        PTR_INIT(1); FUNC(14);
        fillFrameInter (&nL, v_tuningSegm, v_bord, length_v_bord, bmin,
                        v_freq, length_v_freq, v_bordFollow,
                        length_v_bordFollow, v_freqFollow,
                        length_v_freqFollow, *i_fillFollow, dmin, dmax
                        );

        PTR_INIT(2); FUNC(9);
        fillFramePost (&parts, &d, dmax, v_bord, length_v_bord, v_freq,
                       length_v_freq, bmax, fmax);

        ADD(2); LOGIC(1); BRANCH(1);
        if (parts == 1 && d < dmin)
        {
          PTR_INIT(1); FUNC(8);
          specialCase (spreadFlag, allowSpread, v_bord, length_v_bord,
                       v_freq, length_v_freq, &parts, d);
        }

        PTR_INIT(2); FUNC(5);
        calcCmonBorder (&i_cmon, &i_tran, v_bord, length_v_bord, tran
                        );

        FUNC(12);
        keepForFollowUp (v_bordFollow, length_v_bordFollow,
                         v_freqFollow, length_v_freqFollow,
                         i_tranFollow, i_fillFollow, v_bord,
                         length_v_bord, v_freq, i_cmon, i_tran, parts);

        INDIRECT(1); PTR_INIT(1); FUNC(10);
        calcCtrlSignal (&hSbrEnvFrame->SbrGrid,
                        frameClass, v_bord, *length_v_bord, v_freq,
                        *length_v_freq, i_cmon, i_tran, 0, nL);
      }
      break;
    case FIXFIX:
      BRANCH(1); MOVE(1);
      if (tranPos == 0)
        numEnv = 1;
      else
        numEnv = 2;

      INDIRECT(2); MOVE(2);
      hSbrEnvFrame->SbrGrid.bs_num_env = numEnv;
      hSbrEnvFrame->SbrGrid.frameClass = frameClass;

      break;
    default:
      assert(0);
    }
  }


  INDIRECT(2); PTR_INIT(2); FUNC(3);
  ctrlSignal2FrameInfo (&hSbrEnvFrame->SbrGrid,
                        &hSbrEnvFrame->SbrFrameInfo,
                        freq_res_fixfix);

  INDIRECT(1); PTR_INIT(1);

  COUNT_sub_end();

  return &hSbrEnvFrame->SbrFrameInfo;
}

/***************************************************************************/
/*!
  \brief    Creates an instance of the SBR framing handle

  \return  void

****************************************************************************/
void
CreateFrameInfoGenerator (HANDLE_SBR_ENVELOPE_FRAME  hSbrEnvFrame,
                          int allowSpread,
                          int numEnvStatic,
                          int staticFraming,

                          FREQ_RES freq_res_fixfix)

{

  COUNT_sub_start("CreateFrameInfoGenerator");

  INDIRECT(4); MOVE(4);
  hSbrEnvFrame->allowSpread = allowSpread;
  hSbrEnvFrame->numEnvStatic = numEnvStatic;
  hSbrEnvFrame->staticFraming = staticFraming;
  hSbrEnvFrame->freq_res_fixfix = freq_res_fixfix;

  INDIRECT(2); MOVE(2);
  hSbrEnvFrame->dmin = 4;
  hSbrEnvFrame->dmax = 12;

  COUNT_sub_end();
}

/***************************************************************************/
/*!
  \brief    Deletes an instance of the SBR framing handle

  \return   void

****************************************************************************/
void
deleteFrameInfoGenerator (HANDLE_SBR_ENVELOPE_FRAME hSbrEnvFrame)
{
  COUNT_sub_start("deleteFrameInfoGenerator");

  /*
    nothing to do
  */

  COUNT_sub_end();
}


