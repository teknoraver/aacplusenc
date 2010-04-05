/*
  DPCM envelope coding
*/

#include <stdlib.h>
#include <string.h>
#include <assert.h>

#include "code_env.h"
#include "sbr.h"
#include "sbr_rom.h"

#include "counters.h" /* the 3GPP instrumenting tool */

#ifndef min
#define min(a,b) ( a < b ? a:b)
#endif

#ifndef max
#define max(a,b) ( a > b ? a:b)
#endif


/*****************************************************************************

 functionname: InitSbrHuffmanTables
 description:  initializes Huffman Tables dependent on chosen amp_res
 returns:      error handle

*****************************************************************************/
int
InitSbrHuffmanTables (HANDLE_SBR_ENV_DATA       sbrEnvData,
                      HANDLE_SBR_CODE_ENVELOPE  henv,
                      HANDLE_SBR_CODE_ENVELOPE  hnoise,
                      AMP_RES                   amp_res)
{
  COUNT_sub_start("InitSbrHuffmanTables");

  LOGIC(3); BRANCH(1);
  if ( (!henv)  ||  (!hnoise)  || (!sbrEnvData) ) {
    COUNT_sub_end();

    return (1);
  }

  INDIRECT(1); MOVE(1);
  sbrEnvData->init_sbr_amp_res = amp_res;

  BRANCH(2);
  switch (amp_res) {
  case  SBR_AMP_RES_3_0:



    INDIRECT(4); PTR_INIT(4);
    sbrEnvData->hufftableLevelTimeC   = v_Huff_envelopeLevelC11T;
    sbrEnvData->hufftableLevelTimeL   = v_Huff_envelopeLevelL11T;
    sbrEnvData->hufftableBalanceTimeC = bookSbrEnvBalanceC11T;
    sbrEnvData->hufftableBalanceTimeL = bookSbrEnvBalanceL11T;

    INDIRECT(4); PTR_INIT(4);
    sbrEnvData->hufftableLevelFreqC   = v_Huff_envelopeLevelC11F;
    sbrEnvData->hufftableLevelFreqL   = v_Huff_envelopeLevelL11F;
    sbrEnvData->hufftableBalanceFreqC = bookSbrEnvBalanceC11F;
    sbrEnvData->hufftableBalanceFreqL = bookSbrEnvBalanceL11F;


    INDIRECT(4); PTR_INIT(4);
    sbrEnvData->hufftableTimeC        = v_Huff_envelopeLevelC11T;
    sbrEnvData->hufftableTimeL        = v_Huff_envelopeLevelL11T;
    sbrEnvData->hufftableFreqC        = v_Huff_envelopeLevelC11F;
    sbrEnvData->hufftableFreqL        = v_Huff_envelopeLevelL11F;

    INDIRECT(2); MOVE(2);
    sbrEnvData->codeBookScfLavBalance  = CODE_BOOK_SCF_LAV_BALANCE11;
    sbrEnvData->codeBookScfLav         = CODE_BOOK_SCF_LAV11;

    INDIRECT(2); MOVE(2);
    sbrEnvData->si_sbr_start_env_bits           = SI_SBR_START_ENV_BITS_AMP_RES_3_0;
    sbrEnvData->si_sbr_start_env_bits_balance   = SI_SBR_START_ENV_BITS_BALANCE_AMP_RES_3_0;
    break;

  case SBR_AMP_RES_1_5:


    INDIRECT(4); PTR_INIT(4);
    sbrEnvData->hufftableLevelTimeC   = v_Huff_envelopeLevelC10T;
    sbrEnvData->hufftableLevelTimeL   = v_Huff_envelopeLevelL10T;
    sbrEnvData->hufftableBalanceTimeC = bookSbrEnvBalanceC10T;
    sbrEnvData->hufftableBalanceTimeL = bookSbrEnvBalanceL10T;

    INDIRECT(4); PTR_INIT(4);
    sbrEnvData->hufftableLevelFreqC   = v_Huff_envelopeLevelC10F;
    sbrEnvData->hufftableLevelFreqL   = v_Huff_envelopeLevelL10F;
    sbrEnvData->hufftableBalanceFreqC = bookSbrEnvBalanceC10F;
    sbrEnvData->hufftableBalanceFreqL = bookSbrEnvBalanceL10F;


    INDIRECT(4); PTR_INIT(4);
    sbrEnvData->hufftableTimeC        = v_Huff_envelopeLevelC10T;
    sbrEnvData->hufftableTimeL        = v_Huff_envelopeLevelL10T;
    sbrEnvData->hufftableFreqC        = v_Huff_envelopeLevelC10F;
    sbrEnvData->hufftableFreqL        = v_Huff_envelopeLevelL10F;

    INDIRECT(2); MOVE(2);
    sbrEnvData->codeBookScfLavBalance = CODE_BOOK_SCF_LAV_BALANCE10;
    sbrEnvData->codeBookScfLav = CODE_BOOK_SCF_LAV10;

    INDIRECT(2); MOVE(2);
    sbrEnvData->si_sbr_start_env_bits           = SI_SBR_START_ENV_BITS_AMP_RES_1_5;
    sbrEnvData->si_sbr_start_env_bits_balance   = SI_SBR_START_ENV_BITS_BALANCE_AMP_RES_1_5;
    break;

  default:
    COUNT_sub_end();
    return (1);
    break;
  }


  INDIRECT(4); PTR_INIT(4);
  sbrEnvData->hufftableNoiseLevelTimeC   = v_Huff_NoiseLevelC11T;
  sbrEnvData->hufftableNoiseLevelTimeL   = v_Huff_NoiseLevelL11T;
  sbrEnvData->hufftableNoiseBalanceTimeC = bookSbrNoiseBalanceC11T;
  sbrEnvData->hufftableNoiseBalanceTimeL = bookSbrNoiseBalanceL11T;

  INDIRECT(4); PTR_INIT(4);
  sbrEnvData->hufftableNoiseLevelFreqC   = v_Huff_envelopeLevelC11F;
  sbrEnvData->hufftableNoiseLevelFreqL   = v_Huff_envelopeLevelL11F;
  sbrEnvData->hufftableNoiseBalanceFreqC = bookSbrEnvBalanceC11F;
  sbrEnvData->hufftableNoiseBalanceFreqL = bookSbrEnvBalanceL11F;


  INDIRECT(4); PTR_INIT(4);
  sbrEnvData->hufftableNoiseTimeC        = v_Huff_NoiseLevelC11T;
  sbrEnvData->hufftableNoiseTimeL        = v_Huff_NoiseLevelL11T;
  sbrEnvData->hufftableNoiseFreqC        = v_Huff_envelopeLevelC11F;
  sbrEnvData->hufftableNoiseFreqL        = v_Huff_envelopeLevelL11F;

  INDIRECT(2); MOVE(2);
  sbrEnvData->si_sbr_start_noise_bits         = SI_SBR_START_NOISE_BITS_AMP_RES_3_0;
  sbrEnvData->si_sbr_start_noise_bits_balance = SI_SBR_START_NOISE_BITS_BALANCE_AMP_RES_3_0;


  INDIRECT(9); MOVE(6);
  henv->codeBookScfLavBalanceTime = sbrEnvData->codeBookScfLavBalance;
  henv->codeBookScfLavBalanceFreq = sbrEnvData->codeBookScfLavBalance;
  henv->codeBookScfLavLevelTime = sbrEnvData->codeBookScfLav;
  henv->codeBookScfLavLevelFreq = sbrEnvData->codeBookScfLav;
  henv->codeBookScfLavTime = sbrEnvData->codeBookScfLav;
  henv->codeBookScfLavFreq = sbrEnvData->codeBookScfLav;

  INDIRECT(12); PTR_INIT(6);
  henv->hufftableLevelTimeL = sbrEnvData->hufftableLevelTimeL;
  henv->hufftableBalanceTimeL = sbrEnvData->hufftableBalanceTimeL;
  henv->hufftableTimeL = sbrEnvData->hufftableTimeL;
  henv->hufftableLevelFreqL = sbrEnvData->hufftableLevelFreqL;
  henv->hufftableBalanceFreqL = sbrEnvData->hufftableBalanceFreqL;
  henv->hufftableFreqL = sbrEnvData->hufftableFreqL;

  INDIRECT(2); MOVE(2);
  henv->codeBookScfLavFreq = sbrEnvData->codeBookScfLav;
  henv->codeBookScfLavTime = sbrEnvData->codeBookScfLav;

  INDIRECT(2); MOVE(2);
  henv->start_bits = sbrEnvData->si_sbr_start_env_bits;
  henv->start_bits_balance = sbrEnvData->si_sbr_start_env_bits_balance;

  INDIRECT(6); MOVE(6);
  hnoise->codeBookScfLavBalanceTime = CODE_BOOK_SCF_LAV_BALANCE11;
  hnoise->codeBookScfLavBalanceFreq = CODE_BOOK_SCF_LAV_BALANCE11;
  hnoise->codeBookScfLavLevelTime = CODE_BOOK_SCF_LAV11;
  hnoise->codeBookScfLavLevelFreq = CODE_BOOK_SCF_LAV11;
  hnoise->codeBookScfLavTime = CODE_BOOK_SCF_LAV11;
  hnoise->codeBookScfLavFreq = CODE_BOOK_SCF_LAV11;

  INDIRECT(6); PTR_INIT(6);
  hnoise->hufftableLevelTimeL = sbrEnvData->hufftableNoiseLevelTimeL;
  hnoise->hufftableBalanceTimeL = sbrEnvData->hufftableNoiseBalanceTimeL;
  hnoise->hufftableTimeL = sbrEnvData->hufftableNoiseTimeL;
  hnoise->hufftableLevelFreqL = sbrEnvData->hufftableNoiseLevelFreqL;
  hnoise->hufftableBalanceFreqL = sbrEnvData->hufftableNoiseBalanceFreqL;
  hnoise->hufftableFreqL = sbrEnvData->hufftableNoiseFreqL;


  INDIRECT(2); MOVE(2);
  hnoise->start_bits = sbrEnvData->si_sbr_start_noise_bits;
  hnoise->start_bits_balance = sbrEnvData->si_sbr_start_noise_bits_balance;

  INDIRECT(2); MOVE(2);
  henv->upDate = 0;
  hnoise->upDate = 0;

  COUNT_sub_end();

  return  (0);
}

/*******************************************************************************
 Functionname:  indexLow2High
 *******************************************************************************

 Description:   patch-functions in order to cope with non-factor-2
                ratios between high-res and low-res

 Arguments:     int offset, int index, FREQ_RES res

 Return:        int

*******************************************************************************/
static int indexLow2High(int offset, int index, FREQ_RES res)
{
  COUNT_sub_start("indexLow2High");

  ADD(1); BRANCH(1);
  if(res == FREQ_RES_LOW)
  {
    BRANCH(1);
    if (offset >= 0)
    {
        ADD(1); BRANCH(1);
        if (index < offset) {
          COUNT_sub_end();
          return(index);
        }
        else {
          MULT(1); ADD(1); /* counting post-operation */
          COUNT_sub_end();
          return(2*index - offset);
        }
    }
    else
    {
        MULT(1);
        offset = -offset;

        ADD(1); BRANCH(1);
        if (index < offset) {
          MULT(1); ADD(1); /* counting post-operation */
          COUNT_sub_end();
          return(2*index+index);
        }
        else {
          MULT(1); ADD(1); /* counting post-operation */
          COUNT_sub_end();
          return(2*index + offset);
        }
    }
  }
  else {
    COUNT_sub_end();
    return(index);
  }
}



/*******************************************************************************
 Functionname:  mapLowResEnergyVal
 *******************************************************************************


 Arguments:     int currVal,int* prevData, int offset, int index, FREQ_RES res

 Return:        none

*******************************************************************************/
static void mapLowResEnergyVal(int currVal,int* prevData, int offset, int index, FREQ_RES res)
{

  COUNT_sub_start("mapLowResEnergyVal");

  ADD(1); BRANCH(1);
  if(res == FREQ_RES_LOW)
  {
    BRANCH(1);
    if (offset >= 0)
    {
        ADD(1); BRANCH(1);
        if(index < offset) {
            INDIRECT(1); MOVE(1);

            prevData[index] = currVal;
        }
        else
        {
            MULT(1); ADD(1); INDIRECT(2); MOVE(2);

            prevData[2*index - offset] = currVal;
            prevData[2*index+1 - offset] = currVal;
        }
    }
    else
    {
        MULT(1);
        offset = -offset;

        ADD(1); BRANCH(1);
        if (index < offset)
        {
            MULT(1); INDIRECT(3); MOVE(3);

            prevData[3*index] = currVal;
            prevData[3*index+1] = currVal;
            prevData[3*index+2] = currVal;
        }
        else
        {
            MULT(1); INDIRECT(2); MOVE(2);

            prevData[2*index + offset] = currVal;
            prevData[2*index + 1 + offset] = currVal;
        }
    }
  }
  else {
    INDIRECT(1); MOVE(1);

    prevData[index] = currVal;
  }

  COUNT_sub_end();
}



/*******************************************************************************
 Functionname:  computeBits
 *******************************************************************************

 Description:

 Arguments:     int delta,
                int codeBookScfLavLevel,
                int codeBookScfLavBalance,
                const unsigned char * hufftableLevel,
                const unsigned char * hufftableBalance, int coupling, int channel)

 Return:        int

*******************************************************************************/
static int
computeBits (int delta,
             int codeBookScfLavLevel,
             int codeBookScfLavBalance,
             const unsigned char * hufftableLevel,
             const unsigned char * hufftableBalance, int coupling, int channel)
{
  int index;
  int delta_bits = 0;

  COUNT_sub_start("computeBits");

  MOVE(1); /* counting previous operations */

  BRANCH(1);
  if (coupling) {

    ADD(1); BRANCH(1);
    if (channel == 1)
      {
        BRANCH(1); ADD(1); BRANCH(1); MOVE(1);
        index =
          (delta < 0) ? max (delta, -codeBookScfLavBalance) : min (delta,
                                                                   codeBookScfLavBalance);

        ADD(1); BRANCH(1);
        if (index != delta) {
          assert(0);

          COUNT_sub_end();
          return (10000);
        }

        INDIRECT(1); MOVE(1);
        delta_bits = hufftableBalance[index + codeBookScfLavBalance];
      }
    else {

      BRANCH(1); ADD(1); BRANCH(1); MOVE(1);
      index =
        (delta < 0) ? max (delta, -codeBookScfLavLevel) : min (delta,
                                                               codeBookScfLavLevel);

      ADD(1); BRANCH(1);
      if (index != delta) {
        assert(0);

        COUNT_sub_end();
        return (10000);
      }

      INDIRECT(1); MOVE(1);
      delta_bits = hufftableLevel[index + codeBookScfLavLevel];
    }
  }
  else {

    BRANCH(1); ADD(1); BRANCH(1); MOVE(1);
    index =
      (delta < 0) ? max (delta, -codeBookScfLavLevel) : min (delta,
                                                             codeBookScfLavLevel);

    ADD(1); BRANCH(1);
    if (index != delta) {
      assert(0);

      COUNT_sub_end();
      return (10000);
    }

    INDIRECT(1); MOVE(1);
    delta_bits = hufftableLevel[index + codeBookScfLavLevel];
  }

  COUNT_sub_end();

  return (delta_bits);
}




/*******************************************************************************
 Functionname:  codeEnvelope
 *******************************************************************************

 Arguments:     int *sfb_nrg,
                const FREQ_RES *freq_res,
                SBR_CODE_ENVELOPE * h_sbrCodeEnvelope,
                int *directionVec, int scalable, int nEnvelopes, int channel,
                int headerActive)

*******************************************************************************/
void
codeEnvelope (int               *sfb_nrg,
              const FREQ_RES    *freq_res,
              SBR_CODE_ENVELOPE * h_sbrCodeEnvelope,
              int *directionVec,
              int coupling,
              int nEnvelopes,
              int channel,
              int headerActive)
{
  int i, no_of_bands, band, last_nrg, curr_nrg;
  int *ptr_nrg;

  int codeBookScfLavLevelTime;
  int codeBookScfLavLevelFreq;
  int codeBookScfLavBalanceTime;
  int codeBookScfLavBalanceFreq;
  const unsigned char *hufftableLevelTimeL;
  const unsigned char *hufftableBalanceTimeL;
  const unsigned char *hufftableLevelFreqL;
  const unsigned char *hufftableBalanceFreqL;

  int offset = h_sbrCodeEnvelope->offset;
  int envDataTableCompFactor;

  int delta_F_bits = 0, delta_T_bits = 0;
  int use_dT;

  int delta_F[MAX_FREQ_COEFFS];
  int delta_T[MAX_FREQ_COEFFS];

  float dF_edge_1stEnv = h_sbrCodeEnvelope->dF_edge_1stEnv +
    h_sbrCodeEnvelope->dF_edge_incr * h_sbrCodeEnvelope->dF_edge_incr_fac;

  COUNT_sub_start("codeEnvelope");

  INDIRECT(3); ADD(1); MULT(1); MOVE(2); /* counting previous operations */

  BRANCH(1);
  if (coupling) {

    INDIRECT(8); MOVE(8);
    codeBookScfLavLevelTime = h_sbrCodeEnvelope->codeBookScfLavLevelTime;
    codeBookScfLavLevelFreq = h_sbrCodeEnvelope->codeBookScfLavLevelFreq;
    codeBookScfLavBalanceTime = h_sbrCodeEnvelope->codeBookScfLavBalanceTime;
    codeBookScfLavBalanceFreq = h_sbrCodeEnvelope->codeBookScfLavBalanceFreq;
    hufftableLevelTimeL = h_sbrCodeEnvelope->hufftableLevelTimeL;
    hufftableBalanceTimeL = h_sbrCodeEnvelope->hufftableBalanceTimeL;
    hufftableLevelFreqL = h_sbrCodeEnvelope->hufftableLevelFreqL;
    hufftableBalanceFreqL = h_sbrCodeEnvelope->hufftableBalanceFreqL;
  }
  else {

    INDIRECT(8); MOVE(8);
    codeBookScfLavLevelTime = h_sbrCodeEnvelope->codeBookScfLavTime;
    codeBookScfLavLevelFreq = h_sbrCodeEnvelope->codeBookScfLavFreq;
    codeBookScfLavBalanceTime = h_sbrCodeEnvelope->codeBookScfLavTime;
    codeBookScfLavBalanceFreq = h_sbrCodeEnvelope->codeBookScfLavFreq;
    hufftableLevelTimeL = h_sbrCodeEnvelope->hufftableTimeL;
    hufftableBalanceTimeL = h_sbrCodeEnvelope->hufftableTimeL;
    hufftableLevelFreqL = h_sbrCodeEnvelope->hufftableFreqL;
    hufftableBalanceFreqL = h_sbrCodeEnvelope->hufftableFreqL;
  }

  ADD(2); LOGIC(1); BRANCH(1); MOVE(1);
  if(coupling == 1 && channel == 1)
    envDataTableCompFactor = 1;
  else
    envDataTableCompFactor = 0;


  INDIRECT(1); BRANCH(1);
  if (h_sbrCodeEnvelope->deltaTAcrossFrames == 0) {

    INDIRECT(1); MOVE(1);
    h_sbrCodeEnvelope->upDate = 0;
  }

  BRANCH(1);
  if (headerActive) {

    INDIRECT(1); MOVE(1);
    h_sbrCodeEnvelope->upDate = 0;
  }


  PTR_INIT(2); /* pointers for freq_res[],
                               directionVec[]
               */
  LOOP(1);
  for (i = 0; i < nEnvelopes; i++)
  {
    ADD(1); BRANCH(1); INDIRECT(1); MOVE(1);
    if (freq_res[i] == FREQ_RES_HIGH)
      no_of_bands = h_sbrCodeEnvelope->nSfb[FREQ_RES_HIGH];
    else
      no_of_bands = h_sbrCodeEnvelope->nSfb[FREQ_RES_LOW];


    MOVE(2);
    ptr_nrg = sfb_nrg;
    curr_nrg = *ptr_nrg;

    SHIFT(1); STORE(1);
    delta_F[0] = curr_nrg >> envDataTableCompFactor;

    ADD(1); LOGIC(1); BRANCH(1); INDIRECT(1); MOVE(1);
    if (coupling && channel == 1)
      delta_F_bits = h_sbrCodeEnvelope->start_bits_balance;
    else
      delta_F_bits = h_sbrCodeEnvelope->start_bits;


    INDIRECT(1); BRANCH(1);
    if(h_sbrCodeEnvelope->upDate != 0)
    {
      SHIFT(1); ADD(1); INDIRECT(1); STORE(1);
      delta_T[0] = (curr_nrg - h_sbrCodeEnvelope->sfb_nrg_prev[0]) >> envDataTableCompFactor;

      FUNC(7);
      delta_T_bits = computeBits (delta_T[0],
                                  codeBookScfLavLevelTime,
                                  codeBookScfLavBalanceTime,
                                  hufftableLevelTimeL,
                                  hufftableBalanceTimeL, coupling, channel);
    }


    INDIRECT(1); FUNC(5);
    mapLowResEnergyVal(curr_nrg, h_sbrCodeEnvelope->sfb_nrg_prev, offset, 0, freq_res[i]);



    /* ensure that nrg difference is not higher than codeBookScfLavXXXFreq */
    ADD(1); LOGIC(1); BRANCH(1);
    if ( coupling && channel == 1 ) {
      LOOP(1); PTR_INIT(1);
      for (band = no_of_bands - 1; band > 0; band--) {
        ADD(1); LOGIC(1); BRANCH(1);
        if ( ptr_nrg[band] - ptr_nrg[band-1] > codeBookScfLavBalanceFreq ) {
          ADD(1); MOVE(1);
          ptr_nrg[band-1] = ptr_nrg[band] - codeBookScfLavBalanceFreq;
        }
      }
      LOOP(1); PTR_INIT(1);
      for (band = 1; band < no_of_bands; band++) {
        ADD(1); LOGIC(1); BRANCH(1);
        if ( ptr_nrg[band-1] - ptr_nrg[band] > codeBookScfLavBalanceFreq ) {
          ADD(1); MOVE(1);
          ptr_nrg[band] = ptr_nrg[band-1] - codeBookScfLavBalanceFreq;
        }
      }
    }
    else {
      LOOP(1); PTR_INIT(1);
      for (band = no_of_bands - 1; band > 0; band--) {
        ADD(1); LOGIC(1); BRANCH(1);
        if ( ptr_nrg[band] - ptr_nrg[band-1] > codeBookScfLavLevelFreq ) {
          ADD(1); MOVE(1);
          ptr_nrg[band-1] = ptr_nrg[band] - codeBookScfLavLevelFreq;
        }
      }
      LOOP(1); PTR_INIT(1);
      for (band = 1; band < no_of_bands; band++) {
        ADD(1); LOGIC(1); BRANCH(1);
        if ( ptr_nrg[band-1] - ptr_nrg[band] > codeBookScfLavLevelFreq ) {
          ADD(1); MOVE(1);
          ptr_nrg[band] = ptr_nrg[band-1] - codeBookScfLavLevelFreq;
        }
      }
    }



    PTR_INIT(2); /* pointers for delta_F[band],
                                 delta_T[band]
                 */
    LOOP(1);
    for (band = 1; band < no_of_bands; band++)
    {
      MOVE(1);
      last_nrg = (*ptr_nrg);

      ADD(1);
      ptr_nrg++;

      MOVE(1);
      curr_nrg = (*ptr_nrg);


      ADD(1); SHIFT(1); STORE(1);
      delta_F[band] = (curr_nrg - last_nrg) >> envDataTableCompFactor;

      FUNC(7); ADD(1);
      delta_F_bits += computeBits (delta_F[band],
                                   codeBookScfLavLevelFreq,
                                   codeBookScfLavBalanceFreq,
                                   hufftableLevelFreqL,
                                   hufftableBalanceFreqL, coupling, channel);

      INDIRECT(1); BRANCH(1);
      if(h_sbrCodeEnvelope->upDate != 0)
      {
        FUNC(3); INDIRECT(1); ADD(1); STORE(1);
        delta_T[band] = curr_nrg - h_sbrCodeEnvelope->sfb_nrg_prev[indexLow2High(offset, band, freq_res[i])];

        SHIFT(1); STORE(1);
        delta_T[band] = delta_T[band] >> envDataTableCompFactor;
      }

      INDIRECT(1); FUNC(5);
      mapLowResEnergyVal(curr_nrg, h_sbrCodeEnvelope->sfb_nrg_prev, offset, band, freq_res[i]);

      INDIRECT(1); BRANCH(1);
      if(h_sbrCodeEnvelope->upDate != 0)
      {

        FUNC(7); ADD(1);
        delta_T_bits += computeBits (delta_T[band],
                                     codeBookScfLavLevelTime,
                                     codeBookScfLavBalanceTime,
                                     hufftableLevelTimeL,
                                     hufftableBalanceTimeL, coupling, channel);
      }
    }

    BRANCH(1);
    if (i == 0) {

      INDIRECT(1); ADD(2); MULT(1); LOGIC(1);
      use_dT = (h_sbrCodeEnvelope->upDate != 0 && (delta_F_bits > delta_T_bits * (1 + dF_edge_1stEnv)));
    }
    else {

      ADD(1);
      use_dT = (delta_F_bits > delta_T_bits);
    }

    BRANCH(1);
    if (use_dT) {

      MOVE(1);
      directionVec[i] = TIME;

      FUNC(2); LOOP(1); PTR_INIT(2); MOVE(1); STORE(no_of_bands);
      memcpy (sfb_nrg, delta_T, no_of_bands * sizeof (int));
    }
    else {

      MOVE(1);
      directionVec[i] = FREQ;

      FUNC(2); LOOP(1); PTR_INIT(2); MOVE(1); STORE(no_of_bands);
      memcpy (sfb_nrg, delta_F, no_of_bands * sizeof (int));
    }

    ADD(1);
    sfb_nrg += no_of_bands;

    INDIRECT(1); MOVE(1);
    h_sbrCodeEnvelope->upDate = 1;
  }

  COUNT_sub_end();
}




/*******************************************************************************
 Functionname:  CreateSbrCodeEnvelope
 *******************************************************************************

 Description:

 Arguments:

 Return:

*******************************************************************************/
int
CreateSbrCodeEnvelope (HANDLE_SBR_CODE_ENVELOPE  h_sbrCodeEnvelope,
                       int *nSfb,
                       int deltaTAcrossFrames,
                       float dF_edge_1stEnv,
                       float dF_edge_incr)
{
  COUNT_sub_start("CreateSbrCodeEnvelope");

  FUNC(2); LOOP(1); PTR_INIT(1); MOVE(1); STORE(sizeof(SBR_CODE_ENVELOPE));
  memset(h_sbrCodeEnvelope,0,sizeof(SBR_CODE_ENVELOPE));

  INDIRECT(7); MOVE(7);
  h_sbrCodeEnvelope->deltaTAcrossFrames = deltaTAcrossFrames;
  h_sbrCodeEnvelope->dF_edge_1stEnv = dF_edge_1stEnv;
  h_sbrCodeEnvelope->dF_edge_incr = dF_edge_incr;
  h_sbrCodeEnvelope->dF_edge_incr_fac = 0;
  h_sbrCodeEnvelope->upDate = 0;
  h_sbrCodeEnvelope->nSfb[FREQ_RES_LOW] = nSfb[FREQ_RES_LOW];
  h_sbrCodeEnvelope->nSfb[FREQ_RES_HIGH] = nSfb[FREQ_RES_HIGH];

  INDIRECT(1); MULT(1); ADD(1); STORE(1);
  h_sbrCodeEnvelope->offset = 2*h_sbrCodeEnvelope->nSfb[FREQ_RES_LOW] - h_sbrCodeEnvelope->nSfb[FREQ_RES_HIGH];

  COUNT_sub_end();

  return (0);
}





/*******************************************************************************
 Functionname:  deleteSbrCodeEnvelope
 *******************************************************************************

 Description:

 Arguments:

 Return:

*******************************************************************************/
void
deleteSbrCodeEnvelope (HANDLE_SBR_CODE_ENVELOPE h_sbrCodeEnvelope)
{

  COUNT_sub_start("deleteSbrCodeEnvelope");
  /*
    nothing to do
  */

  COUNT_sub_end();
}
