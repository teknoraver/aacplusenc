/*
  SBR bit writing routines
*/
#include <stdlib.h>
#include <assert.h>
#include "sbr_def.h"
#include "FFR_bitbuffer.h"
#include "bit_sbr.h"
#include "sbr.h"
#include "code_env.h"
#include "cmondata.h"
#include "ps_bitenc.h"

#include "counters.h" /* the 3GPP instrumenting tool */



#ifndef min
#define min(a, b) ((a) < (b) ? (a) : (b))
#endif


typedef enum {
  SBR_ID_SCE = 1,
  SBR_ID_CPE
} SBR_ELEMENT_TYPE;


static int encodeSbrData (HANDLE_SBR_ENV_DATA   sbrEnvDataLeft,
                          HANDLE_SBR_ENV_DATA   sbrEnvDataRight,
                          HANDLE_COMMON_DATA    cmonData,
                          SBR_ELEMENT_TYPE      sbrElem,
                          int                   sampleRateMode,
                          int                   data_extra,
                          struct PS_ENC*     h_ps_e,
                          int                   bHeaderActive,
                          int                   coupling);

static int encodeSbrHeader (HANDLE_SBR_HEADER_DATA     sbrHeaderData,
                            HANDLE_SBR_BITSTREAM_DATA  sbrBitstreamData,
                            HANDLE_COMMON_DATA         cmonData,
                            SBR_ELEMENT_TYPE           sbrElem);


static int encodeSbrHeaderData (HANDLE_SBR_HEADER_DATA sbrHeaderData,
                                HANDLE_BIT_BUF         hBitStream,
                                SBR_ELEMENT_TYPE       sbrElem);

static int encodeSbrSingleChannelElement (HANDLE_SBR_ENV_DATA    sbrEnvData,
                                          HANDLE_BIT_BUF         hBitStream,
                                          int                    data_extra);


static int encodeSbrChannelPairElement (HANDLE_SBR_ENV_DATA  sbrEnvDataLeft,
                                        HANDLE_SBR_ENV_DATA  sbrEnvDataRight,
                                        HANDLE_BIT_BUF        hBitStream,
                                        int                   data_extra,
                                        int                   coupling);


static int encodeSbrGrid (HANDLE_SBR_ENV_DATA   sbrEnvData,
                          HANDLE_BIT_BUF        hBitStream);

static int encodeSbrDtdf (HANDLE_SBR_ENV_DATA   sbrEnvData,
                          HANDLE_BIT_BUF        hBitStream);

static int writeNoiseLevelData (HANDLE_SBR_ENV_DATA   sbrEnvData,
                                HANDLE_BIT_BUF        hBitStream,
                                int                   coupling);

static int writeEnvelopeData (HANDLE_SBR_ENV_DATA    sbrEnvData,
                              HANDLE_BIT_BUF         hBitStream,
                              int                    coupling);

static int writeSyntheticCodingData (HANDLE_SBR_ENV_DATA  sbrEnvData,
                                     HANDLE_BIT_BUF       hBitStream);


static void encodeExtendedData (HANDLE_SBR_ENV_DATA  sbrEnvDataLeft,
                                HANDLE_SBR_ENV_DATA  sbrEnvDataRight,
                                struct PS_ENC*    h_ps_e,
                                int                  bHeaderActive,
                                HANDLE_BIT_BUF       hBitStreamPrev,
                                int*                 sbrHdrBits,
                                HANDLE_BIT_BUF       hBitStream,
                                int* payloadBitsReturn);



static int
getSbrExtendedDataSize (HANDLE_SBR_ENV_DATA sbrEnvDataLeft,
                        HANDLE_SBR_ENV_DATA sbrEnvDataRight,
                        struct PS_ENC*      h_ps_e,
                        int                 bHeaderActive
);


/*****************************************************************************

    description:  writes SBR single channel data element
    returns:      number of bits written

*****************************************************************************/
int
WriteEnvSingleChannelElement(HANDLE_SBR_HEADER_DATA     sbrHeaderData,
                             HANDLE_SBR_BITSTREAM_DATA  sbrBitstreamData,
                             HANDLE_SBR_ENV_DATA        sbrEnvData,
                             struct PS_ENC*             h_ps_e,
                             HANDLE_COMMON_DATA         cmonData)

{
  int payloadBits = 0;

  COUNT_sub_start("WriteEnvSingleChannelElement");

  MOVE(1); /* counting previous operation */

  INDIRECT(3); MOVE(3);
  cmonData->sbrHdrBits  = 0;
  cmonData->sbrDataBits = 0;
  cmonData->sbrCrcLen   = 0;

  /* write pure sbr data */
  BRANCH(1);
  if (sbrEnvData != NULL) {

    /* write header */
    INDIRECT(1); FUNC(4); ADD(1);
    payloadBits += encodeSbrHeader (sbrHeaderData,
                                    sbrBitstreamData,
                                    cmonData,
                                    SBR_ID_SCE);


    /* write data */
    INDIRECT(4); FUNC(9); ADD(1);
    payloadBits += encodeSbrData (sbrEnvData,
                                  NULL,
                                  cmonData,
                                  SBR_ID_SCE,
                                  sbrHeaderData->sampleRateMode,
                                  sbrHeaderData->sbr_data_extra,
                                  h_ps_e,
                                  sbrBitstreamData->HeaderActive,
                                  0);


  }

  COUNT_sub_end();

  return payloadBits;
}

/*****************************************************************************

    description:  writes SBR channel pair data element
    returns:      number of bits written

*****************************************************************************/
int
WriteEnvChannelPairElement (HANDLE_SBR_HEADER_DATA     sbrHeaderData,
                            HANDLE_SBR_BITSTREAM_DATA  sbrBitstreamData,
                            HANDLE_SBR_ENV_DATA        sbrEnvDataLeft,
                            HANDLE_SBR_ENV_DATA        sbrEnvDataRight,
                            HANDLE_COMMON_DATA         cmonData)

{
  int payloadBits = 0;

  COUNT_sub_start("WriteEnvChannelPairElement");

  MOVE(1); /* counting previous operation */

  INDIRECT(3); MOVE(3);
  cmonData->sbrHdrBits  = 0;
  cmonData->sbrDataBits = 0;
  cmonData->sbrCrcLen   = 0;

  /* write pure sbr data */
  LOGIC(1); BRANCH(1);
  if ((sbrEnvDataLeft != NULL) && (sbrEnvDataRight != NULL)) {

    /* write header */
    FUNC(4); ADD(1);
    payloadBits += encodeSbrHeader (sbrHeaderData,
                                    sbrBitstreamData,
                                    cmonData,
                                    SBR_ID_CPE);


    /* write data */
    INDIRECT(3); FUNC(9); ADD(1);
    payloadBits += encodeSbrData (sbrEnvDataLeft,
                                  sbrEnvDataRight,
                                  cmonData,
                                  SBR_ID_CPE,
                                  sbrHeaderData->sampleRateMode,
                                  sbrHeaderData->sbr_data_extra,
                                  NULL,
                                  0,
                                  sbrHeaderData->coupling);


  }
  COUNT_sub_end();

  return payloadBits;
}

int
CountSbrChannelPairElement (HANDLE_SBR_HEADER_DATA     sbrHeaderData,
                            HANDLE_SBR_BITSTREAM_DATA  sbrBitstreamData,
                            HANDLE_SBR_ENV_DATA        sbrEnvDataLeft,
                            HANDLE_SBR_ENV_DATA        sbrEnvDataRight,
                            HANDLE_COMMON_DATA         cmonData)

{
  int payloadBits;
  struct BIT_BUF bitBufTmp = cmonData->sbrBitbuf;

  COUNT_sub_start("CountSbrChannelPairElement");

  INDIRECT(1); MOVE(1); /* counting previous operation */

  FUNC(5);
  payloadBits = WriteEnvChannelPairElement(sbrHeaderData,
                                           sbrBitstreamData,
                                           sbrEnvDataLeft,
                                           sbrEnvDataRight,
                                           cmonData);

  INDIRECT(1); MOVE(1);
  cmonData->sbrBitbuf = bitBufTmp;

  COUNT_sub_end();

  return payloadBits;
}

/*****************************************************************************

    description:  encodes SBR Header information
    returns:      number of bits written

*****************************************************************************/
static int
encodeSbrHeader (HANDLE_SBR_HEADER_DATA     sbrHeaderData,
                 HANDLE_SBR_BITSTREAM_DATA  sbrBitstreamData,
                 HANDLE_COMMON_DATA         cmonData,
                 SBR_ELEMENT_TYPE           sbrElem)

{
  int payloadBits = 0;

  COUNT_sub_start("encodeSbrHeader");

  MOVE(1); /* counting previous operation */

  INDIRECT(1); BRANCH(1);
  if (sbrBitstreamData->CRCActive) {

    INDIRECT(1); MOVE(1);
    cmonData->sbrCrcLen = 1;
  }
  else {

    INDIRECT(1); MOVE(1);
    cmonData->sbrCrcLen = 0;
  }


  BRANCH(1);
  if (sbrBitstreamData->HeaderActive) {

    INDIRECT(1); PTR_INIT(1); FUNC(3); ADD(1);
    payloadBits += WriteBits (&cmonData->sbrBitbuf, 1, 1);

    INDIRECT(1); PTR_INIT(1); FUNC(3); ADD(1);
    payloadBits += encodeSbrHeaderData (sbrHeaderData,
                                        &cmonData->sbrBitbuf,
                                        sbrElem);
  }
  else {

    INDIRECT(1); PTR_INIT(1); FUNC(3); ADD(1);
    payloadBits += WriteBits (&cmonData->sbrBitbuf, 0, 1);
  }


  INDIRECT(1); MOVE(1);
  cmonData->sbrHdrBits = payloadBits;

  COUNT_sub_end();

  return payloadBits;
}



/*****************************************************************************

    functionname: encodeSbrHeaderData
    description:  writes sbr_header()
    returns:      number of bits written

*****************************************************************************/
static int
encodeSbrHeaderData (HANDLE_SBR_HEADER_DATA sbrHeaderData,
                     HANDLE_BIT_BUF hBitStream,
                     SBR_ELEMENT_TYPE sbrElem)

{
  int payloadBits = 0;

  COUNT_sub_start("encodeSbrHeaderData");

  MOVE(1); /* counting previous operation */

  BRANCH(1);
  if (sbrHeaderData != NULL) {

    INDIRECT(1); FUNC(3); ADD(1);
    payloadBits += WriteBits (hBitStream, sbrHeaderData->sbr_amp_res,
                              SI_SBR_AMP_RES_BITS);

    INDIRECT(1); FUNC(3); ADD(1);
    payloadBits += WriteBits (hBitStream, sbrHeaderData->sbr_start_frequency,
                              SI_SBR_START_FREQ_BITS);

    INDIRECT(1); FUNC(3); ADD(1);
    payloadBits += WriteBits (hBitStream, sbrHeaderData->sbr_stop_frequency,
                              SI_SBR_STOP_FREQ_BITS);

    INDIRECT(1); FUNC(3); ADD(1);
    payloadBits += WriteBits (hBitStream, sbrHeaderData->sbr_xover_band,
                              SI_SBR_XOVER_BAND_BITS);

    FUNC(3); ADD(1);
    payloadBits += WriteBits (hBitStream, 0,
                              SI_SBR_RESERVED_BITS);



    INDIRECT(1); FUNC(3); ADD(1);
    payloadBits += WriteBits (hBitStream, sbrHeaderData->header_extra_1,
                              SI_SBR_HEADER_EXTRA_1_BITS);

    INDIRECT(1); FUNC(3); ADD(1);
    payloadBits += WriteBits (hBitStream, sbrHeaderData->header_extra_2,
                              SI_SBR_HEADER_EXTRA_2_BITS);


    INDIRECT(1); BRANCH(1);
    if (sbrHeaderData->header_extra_1) {

      INDIRECT(1); FUNC(3); ADD(1);
      payloadBits += WriteBits (hBitStream, sbrHeaderData->freqScale,
                                SI_SBR_FREQ_SCALE_BITS);

      INDIRECT(1); FUNC(3); ADD(1);
      payloadBits += WriteBits (hBitStream, sbrHeaderData->alterScale,
                                SI_SBR_ALTER_SCALE_BITS);

      INDIRECT(1); FUNC(3); ADD(1);
      payloadBits += WriteBits (hBitStream, sbrHeaderData->sbr_noise_bands,
                                SI_SBR_NOISE_BANDS_BITS);
    } /* sbrHeaderData->header_extra_1 */

    INDIRECT(1); BRANCH(1);
    if (sbrHeaderData->header_extra_2) {

      INDIRECT(1); FUNC(3); ADD(1);
      payloadBits += WriteBits (hBitStream, sbrHeaderData->sbr_limiter_bands,
                                SI_SBR_LIMITER_BANDS_BITS);

      INDIRECT(1); FUNC(3); ADD(1);
      payloadBits += WriteBits (hBitStream, sbrHeaderData->sbr_limiter_gains,
                                SI_SBR_LIMITER_GAINS_BITS);

      INDIRECT(1); FUNC(3); ADD(1);
      payloadBits += WriteBits (hBitStream, sbrHeaderData->sbr_interpol_freq,
                                SI_SBR_INTERPOL_FREQ_BITS);

      INDIRECT(1); FUNC(3); ADD(1);
      payloadBits += WriteBits (hBitStream, sbrHeaderData->sbr_smoothing_length,
                                SI_SBR_SMOOTHING_LENGTH_BITS);

    } /* sbrHeaderData->header_extra_2 */
  } /* sbrHeaderData != NULL */

  COUNT_sub_end();

  return payloadBits;
}


/*****************************************************************************

    description:  encodes sbr Data information
    returns:      number of bits written

*****************************************************************************/
static int
encodeSbrData (HANDLE_SBR_ENV_DATA   sbrEnvDataLeft,
               HANDLE_SBR_ENV_DATA   sbrEnvDataRight,
               HANDLE_COMMON_DATA    cmonData,
               SBR_ELEMENT_TYPE      sbrElem,
               int                   sampleRateMode,
               int                   data_extra,
               struct PS_ENC*        h_ps_e,
               int                   bHeaderActive,
               int                   coupling)
{
  int payloadBits = 0;

  COUNT_sub_start("encodeSbrData");

  MOVE(1); /* counting previous operation */

  BRANCH(2);
  switch (sbrElem) {
  case SBR_ID_SCE:

    INDIRECT(1); PTR_INIT(1); FUNC(3); ADD(1);
    payloadBits += encodeSbrSingleChannelElement (sbrEnvDataLeft,
                                                  &cmonData->sbrBitbuf,
                                                  data_extra);

    INDIRECT(3); PTR_INIT(4); FUNC(8);
    encodeExtendedData(sbrEnvDataLeft,
                       NULL,
                       h_ps_e,
                       bHeaderActive,
                       &cmonData->sbrBitbufPrev,
                       &cmonData->sbrHdrBits,
                       &cmonData->sbrBitbuf,
                       &payloadBits);


    break;
  case SBR_ID_CPE:

    INDIRECT(1); PTR_INIT(1); FUNC(5); ADD(1);
    payloadBits += encodeSbrChannelPairElement (sbrEnvDataLeft, sbrEnvDataRight, &cmonData->sbrBitbuf,
                                                data_extra, coupling);


    INDIRECT(1); PTR_INIT(2); FUNC(8);
    encodeExtendedData(sbrEnvDataLeft,
                       sbrEnvDataRight,
                       NULL,
                       0,
                       NULL,
                       0,
                       &cmonData->sbrBitbuf,
                       &payloadBits);

    break;
  default:
    /* we never should apply SBR to any other element type */
    assert (0);
  }


  INDIRECT(1); MOVE(1);
  cmonData->sbrDataBits = payloadBits;

  COUNT_sub_end();

  return payloadBits;
}



/*****************************************************************************

    description:  encodes sbr SCE information
    returns:      number of bits written

*****************************************************************************/
static int
encodeSbrSingleChannelElement (HANDLE_SBR_ENV_DATA   sbrEnvData,
                               HANDLE_BIT_BUF        hBitStream,
                               int                   data_extra)

{
  int payloadBits = 0;

  COUNT_sub_start("encodeSbrSingleChannelElement");

  MOVE(1); /* counting previous operation */

    FUNC(3); ADD(1);
    payloadBits += WriteBits (hBitStream, 0, 1); /* no reserved bits */



  FUNC(2); ADD(1);
  payloadBits += encodeSbrGrid (sbrEnvData, hBitStream);

  FUNC(2); ADD(1);
  payloadBits += encodeSbrDtdf (sbrEnvData, hBitStream);

  {
    int i;

    PTR_INIT(1); /* sbrEnvData->sbr_invf_mode_vec[] */
    INDIRECT(1); LOOP(1);
    for (i = 0; i < sbrEnvData->noOfnoisebands; i++) {

      FUNC(3); ADD(1);
      payloadBits += WriteBits (hBitStream, sbrEnvData->sbr_invf_mode_vec[i], SI_SBR_INVF_MODE_BITS);
    }
  }


  FUNC(3); ADD(1);
  payloadBits += writeEnvelopeData (sbrEnvData, hBitStream, 0);

  FUNC(3); ADD(1);
  payloadBits += writeNoiseLevelData (sbrEnvData, hBitStream, 0);

  FUNC(2); ADD(1);
  payloadBits += writeSyntheticCodingData (sbrEnvData,hBitStream);


  COUNT_sub_end();

  return payloadBits;
}


/*****************************************************************************

    description:  encodes sbr CPE information

*****************************************************************************/
static int
encodeSbrChannelPairElement (HANDLE_SBR_ENV_DATA   sbrEnvDataLeft,
                             HANDLE_SBR_ENV_DATA   sbrEnvDataRight,
                             HANDLE_BIT_BUF        hBitStream,
                             int                   data_extra,
                             int                   coupling)
{
  int payloadBits = 0;
  int i = 0;

  COUNT_sub_start("encodeSbrChannelPairElement");

  MOVE(2); /* counting previous operation */

    FUNC(3); ADD(1);
    payloadBits += WriteBits (hBitStream, 0, 1); /* no reserved bits */

  FUNC(3); ADD(1);
  payloadBits += WriteBits (hBitStream, coupling, SI_SBR_COUPLING_BITS);

  BRANCH(1);
  if (coupling) {

    FUNC(2); ADD(1);
    payloadBits += encodeSbrGrid (sbrEnvDataLeft, hBitStream);

    FUNC(2); ADD(1);
    payloadBits += encodeSbrDtdf (sbrEnvDataLeft, hBitStream);

    FUNC(2); ADD(1);
    payloadBits += encodeSbrDtdf (sbrEnvDataRight, hBitStream);

    PTR_INIT(1); /* sbrEnvDataLeft->sbr_invf_mode_vec[] */
    INDIRECT(1); LOOP(1);
    for (i = 0; i < sbrEnvDataLeft->noOfnoisebands; i++) {

      FUNC(3); ADD(1);
      payloadBits += WriteBits (hBitStream, sbrEnvDataLeft->sbr_invf_mode_vec[i], SI_SBR_INVF_MODE_BITS);
    }




    FUNC(3); ADD(1);
    payloadBits += writeEnvelopeData  (sbrEnvDataLeft,  hBitStream,1);

    FUNC(3); ADD(1);
    payloadBits += writeNoiseLevelData (sbrEnvDataLeft,  hBitStream,1);

    FUNC(3); ADD(1);
    payloadBits += writeEnvelopeData  (sbrEnvDataRight, hBitStream,1);

    FUNC(3); ADD(1);
    payloadBits += writeNoiseLevelData (sbrEnvDataRight, hBitStream,1);




    FUNC(2); ADD(1);
    payloadBits += writeSyntheticCodingData (sbrEnvDataLeft,hBitStream);

    FUNC(2); ADD(1);
    payloadBits += writeSyntheticCodingData (sbrEnvDataRight,hBitStream);


  } else { /* no coupling */

    FUNC(2); ADD(1);
    payloadBits += encodeSbrGrid (sbrEnvDataLeft,  hBitStream);

    FUNC(2); ADD(1);
    payloadBits += encodeSbrGrid (sbrEnvDataRight, hBitStream);

    FUNC(2); ADD(1);
    payloadBits += encodeSbrDtdf (sbrEnvDataLeft,  hBitStream);

    FUNC(2); ADD(1);
    payloadBits += encodeSbrDtdf (sbrEnvDataRight, hBitStream);

    PTR_INIT(1); /* sbrEnvDataLeft->sbr_invf_mode_vec[] */
    INDIRECT(1); LOOP(1);
    for (i = 0; i < sbrEnvDataLeft->noOfnoisebands; i++) {

      FUNC(3); ADD(1);
      payloadBits += WriteBits (hBitStream, sbrEnvDataLeft->sbr_invf_mode_vec[i],
                                SI_SBR_INVF_MODE_BITS);
    }

    PTR_INIT(1); /* sbrEnvDataRight->sbr_invf_mode_vec[] */
    INDIRECT(1); LOOP(1);
    for (i = 0; i < sbrEnvDataRight->noOfnoisebands; i++) {

      FUNC(3); ADD(1);
      payloadBits += WriteBits (hBitStream, sbrEnvDataRight->sbr_invf_mode_vec[i],
                                SI_SBR_INVF_MODE_BITS);
    }



    FUNC(3); ADD(1);
    payloadBits += writeEnvelopeData  (sbrEnvDataLeft,  hBitStream,0);

    FUNC(3); ADD(1);
    payloadBits += writeEnvelopeData  (sbrEnvDataRight, hBitStream,0);

    FUNC(3); ADD(1);
    payloadBits += writeNoiseLevelData (sbrEnvDataLeft,  hBitStream,0);

    FUNC(3); ADD(1);
    payloadBits += writeNoiseLevelData (sbrEnvDataRight, hBitStream,0);


    FUNC(2); ADD(1);
    payloadBits += writeSyntheticCodingData (sbrEnvDataLeft,hBitStream);

    FUNC(2); ADD(1);
    payloadBits += writeSyntheticCodingData (sbrEnvDataRight,hBitStream);

  } /* coupling */



  COUNT_sub_end();

  return payloadBits;
}

static int ceil_ln2(int x)
{
  int tmp=-1;

  COUNT_sub_start("ceil_ln2");

  MOVE(1); /* counting previous operation */

  LOOP(1);
  while((1<<++tmp) < x);
  SHIFT(tmp+1);

  COUNT_sub_end();

  return(tmp);
}


/*****************************************************************************

    description:  Encode SBR grid information

*****************************************************************************/
static int
encodeSbrGrid (HANDLE_SBR_ENV_DATA sbrEnvData, HANDLE_BIT_BUF hBitStream)
{
  int payloadBits = 0;
  int i, temp;



  COUNT_sub_start("encodeSbrGrid");

  MOVE(1); /* counting previous operations */

  INDIRECT(1); FUNC(3); ADD(1);
  payloadBits += WriteBits (hBitStream, sbrEnvData->hSbrBSGrid->frameClass,
                            SBR_CLA_BITS);

  INDIRECT(1); BRANCH(2);
  switch (sbrEnvData->hSbrBSGrid->frameClass) {
  case FIXFIX:

    INDIRECT(1); FUNC(1);
    temp = ceil_ln2(sbrEnvData->hSbrBSGrid->bs_num_env);

    FUNC(3); ADD(1);
    payloadBits += WriteBits (hBitStream, temp, SBR_ENV_BITS);

    INDIRECT(1); FUNC(3); ADD(1);
    payloadBits += WriteBits (hBitStream, sbrEnvData->freq_res_fixfix, SBR_RES_BITS);
    break;

  case FIXVAR:
  case VARFIX:

    INDIRECT(1); ADD(1); BRANCH(1);
    if (sbrEnvData->hSbrBSGrid->frameClass == FIXVAR)
    {
      INDIRECT(1); ADD(1);
      temp = sbrEnvData->hSbrBSGrid->bs_abs_bord - 16;
    }
    else
    {
      INDIRECT(1);
      temp = sbrEnvData->hSbrBSGrid->bs_abs_bord;
    }

    FUNC(3); ADD(1);
    payloadBits += WriteBits (hBitStream, temp, SBR_ABS_BITS);

    INDIRECT(1); FUNC(3); ADD(1);
    payloadBits += WriteBits (hBitStream, sbrEnvData->hSbrBSGrid->n, SBR_NUM_BITS);

    PTR_INIT(1); /* sbrEnvData->hSbrBSGrid->bs_rel_bord[] */
    INDIRECT(1); LOOP(1);
    for (i = 0; i < sbrEnvData->hSbrBSGrid->n; i++) {

      ADD(1); SHIFT(1);
      temp = (sbrEnvData->hSbrBSGrid->bs_rel_bord[i] - 2) >> 1;

      FUNC(3); ADD(1);
      payloadBits += WriteBits (hBitStream, temp, SBR_REL_BITS);
    }

    INDIRECT(1); ADD(1); FUNC(1);
    temp = ceil_ln2(sbrEnvData->hSbrBSGrid->n + 2);

    INDIRECT(1); FUNC(3); ADD(1);
    payloadBits += WriteBits (hBitStream, sbrEnvData->hSbrBSGrid->p, temp);

    PTR_INIT(1); /* sbrEnvData->hSbrBSGrid->v_f[] */
    INDIRECT(1); ADD(1); LOOP(1);
    for (i = 0; i < sbrEnvData->hSbrBSGrid->n + 1; i++) {

      FUNC(3); ADD(1);
      payloadBits += WriteBits (hBitStream, sbrEnvData->hSbrBSGrid->v_f[i],
                                SBR_RES_BITS);
    }
    break;

  case VARVAR:

    INDIRECT(1);
    temp = sbrEnvData->hSbrBSGrid->bs_abs_bord_0;

    FUNC(3); ADD(1);
    payloadBits += WriteBits (hBitStream, temp, SBR_ABS_BITS);

    INDIRECT(1); ADD(1);
    temp = sbrEnvData->hSbrBSGrid->bs_abs_bord_1 - 16;

    FUNC(3); ADD(1);
    payloadBits += WriteBits (hBitStream, temp, SBR_ABS_BITS);

    INDIRECT(1); FUNC(3); ADD(1);
    payloadBits += WriteBits (hBitStream, sbrEnvData->hSbrBSGrid->bs_num_rel_0, SBR_NUM_BITS);

    INDIRECT(1); FUNC(3); ADD(1);
    payloadBits += WriteBits (hBitStream, sbrEnvData->hSbrBSGrid->bs_num_rel_1, SBR_NUM_BITS);

    PTR_INIT(1); /* sbrEnvData->hSbrBSGrid->bs_rel_bord_0[] */
    INDIRECT(1); LOOP(1);
    for (i = 0; i < sbrEnvData->hSbrBSGrid->bs_num_rel_0; i++) {

      ADD(1); SHIFT(1);
      temp = (sbrEnvData->hSbrBSGrid->bs_rel_bord_0[i] - 2) >> 1;

      FUNC(3); ADD(1);
      payloadBits += WriteBits (hBitStream, temp, SBR_REL_BITS);
    }

    PTR_INIT(1); /* sbrEnvData->hSbrBSGrid->bs_rel_bord_1[] */
    INDIRECT(1); LOOP(1);
    for (i = 0; i < sbrEnvData->hSbrBSGrid->bs_num_rel_1; i++) {

      ADD(1); SHIFT(1);
      temp = (sbrEnvData->hSbrBSGrid->bs_rel_bord_1[i] - 2) >> 1;

      FUNC(3); ADD(1);
      payloadBits += WriteBits (hBitStream, temp, SBR_REL_BITS);
    }

    INDIRECT(2); ADD(2); FUNC(1);
    temp = ceil_ln2(sbrEnvData->hSbrBSGrid->bs_num_rel_0 +
                             sbrEnvData->hSbrBSGrid->bs_num_rel_1 + 2);

    INDIRECT(1); FUNC(3); ADD(1);
    payloadBits +=  WriteBits (hBitStream, sbrEnvData->hSbrBSGrid->p, temp);

    INDIRECT(2); ADD(2);
    temp = sbrEnvData->hSbrBSGrid->bs_num_rel_0 +
           sbrEnvData->hSbrBSGrid->bs_num_rel_1 + 1;

    PTR_INIT(1); /* sbrEnvData->hSbrBSGrid->v_fLR[] */
    INDIRECT(1); LOOP(1);
    for (i = 0; i < temp; i++) {

      FUNC(3); ADD(1);
      payloadBits += WriteBits (hBitStream, sbrEnvData->hSbrBSGrid->v_fLR[i],
                                SBR_RES_BITS);
    }
    break;
  }

  COUNT_sub_end();

  return payloadBits;
}


/*****************************************************************************

    description:  writes bits that describes the direction of the envelopes of a frame
    returns:      number of bits written

*****************************************************************************/
static int
encodeSbrDtdf (HANDLE_SBR_ENV_DATA sbrEnvData, HANDLE_BIT_BUF hBitStream)
{
  int i, payloadBits = 0, noOfNoiseEnvelopes;

  COUNT_sub_start("encodeSbrDtdf");

  MOVE(1); /* counting previous operations */

  INDIRECT(1); ADD(1); BRANCH(1); MOVE(1);
  noOfNoiseEnvelopes = (sbrEnvData->noOfEnvelopes > 1) ? 2 : 1;

  PTR_INIT(1); /* sbrEnvData->domain_vec[] */
  INDIRECT(1); LOOP(1);
  for (i = 0; i < sbrEnvData->noOfEnvelopes; ++i) {

    FUNC(3); ADD(1);
    payloadBits += WriteBits (hBitStream, sbrEnvData->domain_vec[i], SBR_DIR_BITS);
  }

  PTR_INIT(1); /* sbrEnvData->domain_vec_noise[] */
  LOOP(1);
  for (i = 0; i < noOfNoiseEnvelopes; ++i) {

    FUNC(3); ADD(1);
    payloadBits +=  WriteBits (hBitStream, sbrEnvData->domain_vec_noise[i], SBR_DIR_BITS);
  }

  COUNT_sub_end();

  return payloadBits;
}


/*****************************************************************************

    description:  writes bits corresponding to the noise-floor-level
    returns:      number of bits written

*****************************************************************************/
static int
writeNoiseLevelData (HANDLE_SBR_ENV_DATA sbrEnvData, HANDLE_BIT_BUF hBitStream, int coupling)
{
  int j, i, payloadBits = 0;
  int nNoiseEnvelopes = ((sbrEnvData->noOfEnvelopes > 1) ? 2 : 1);

  COUNT_sub_start("writeNoiseLevelData");

  INDIRECT(1); ADD(1); BRANCH(1); MOVE(1); /* .. > .. ? */ MOVE(1);

  PTR_INIT(2); /* sbrEnvData->domain_vec_noise[]
                  sbrEnvData->sbr_noise_levels[i * sbrEnvData->noOfnoisebands]
               */
  LOOP(1);
  for (i = 0; i < nNoiseEnvelopes; i++) {

    BRANCH(2);
    switch (sbrEnvData->domain_vec_noise[i]) {
    case FREQ:

      INDIRECT(1); LOGIC(1); BRANCH(1);
      if (coupling && sbrEnvData->balance) {

        INDIRECT(1); FUNC(3); ADD(1);
        payloadBits += WriteBits (hBitStream,
                                  sbrEnvData->sbr_noise_levels[i * sbrEnvData->noOfnoisebands],
                                  sbrEnvData->si_sbr_start_noise_bits_balance);
      } else {

        INDIRECT(1); FUNC(3); ADD(1);
        payloadBits += WriteBits (hBitStream,
                                  sbrEnvData->sbr_noise_levels[i * sbrEnvData->noOfnoisebands],
                                  sbrEnvData->si_sbr_start_noise_bits);
      }

      PTR_INIT(1); /* sbrEnvData->sbr_noise_levels[] */
      INDIRECT(1); MULT(2); ADD(2); LOOP(1);
      for (j = 1 + i * sbrEnvData->noOfnoisebands; j < (sbrEnvData->noOfnoisebands * (1 + i)); j++) {
        BRANCH(1);
        if (coupling) {

          INDIRECT(1); BRANCH(1);
          if (sbrEnvData->balance) {

            INDIRECT(2); FUNC(3); ADD(1);
            payloadBits += WriteBits (hBitStream,
                                      sbrEnvData->hufftableNoiseBalanceFreqC[sbrEnvData->sbr_noise_levels[j] +
                                                                            CODE_BOOK_SCF_LAV_BALANCE11],
                                      sbrEnvData->hufftableNoiseBalanceFreqL[sbrEnvData->sbr_noise_levels[j] +
                                                                            CODE_BOOK_SCF_LAV_BALANCE11]);
          } else {

            INDIRECT(2); FUNC(3); ADD(1);
            payloadBits += WriteBits (hBitStream,
                                      sbrEnvData->hufftableNoiseLevelFreqC[sbrEnvData->sbr_noise_levels[j] +
                                                                          CODE_BOOK_SCF_LAV11],
                                      sbrEnvData->hufftableNoiseLevelFreqL[sbrEnvData->sbr_noise_levels[j] +
                                                                          CODE_BOOK_SCF_LAV11]);
          }
        } else {

          INDIRECT(2); FUNC(3); ADD(1);
          payloadBits += WriteBits (hBitStream,
                                    sbrEnvData->hufftableNoiseFreqC[sbrEnvData->sbr_noise_levels[j] +
                                                                   CODE_BOOK_SCF_LAV11],
                                    sbrEnvData->hufftableNoiseFreqL[sbrEnvData->sbr_noise_levels[j] +
                                                                   CODE_BOOK_SCF_LAV11]);
        }
      }
      break;

    case TIME:

      PTR_INIT(1); /* sbrEnvData->sbr_noise_levels[] */
      INDIRECT(1); MULT(2); ADD(1); LOOP(1);
      for (j = i * sbrEnvData->noOfnoisebands; j < (sbrEnvData->noOfnoisebands * (1 + i)); j++) {

        BRANCH(1);
        if (coupling) {

          INDIRECT(1); BRANCH(1);
          if (sbrEnvData->balance) {

            INDIRECT(2); FUNC(3); ADD(1);
            payloadBits += WriteBits (hBitStream,
                                      sbrEnvData->hufftableNoiseBalanceTimeC[sbrEnvData->sbr_noise_levels[j] +
                                                                            CODE_BOOK_SCF_LAV_BALANCE11],
                                      sbrEnvData->hufftableNoiseBalanceTimeL[sbrEnvData->sbr_noise_levels[j] +
                                                                            CODE_BOOK_SCF_LAV_BALANCE11]);
          } else {

            INDIRECT(2); FUNC(3); ADD(1);
            payloadBits += WriteBits (hBitStream,
                                      sbrEnvData->hufftableNoiseLevelTimeC[sbrEnvData->sbr_noise_levels[j] +
                                                                          CODE_BOOK_SCF_LAV11],
                                      sbrEnvData->hufftableNoiseLevelTimeL[sbrEnvData->sbr_noise_levels[j] +
                                                                          CODE_BOOK_SCF_LAV11]);
          }
        } else {

          INDIRECT(2); FUNC(3); ADD(1);
          payloadBits += WriteBits (hBitStream,
                                    sbrEnvData->hufftableNoiseLevelTimeC[sbrEnvData->sbr_noise_levels[j] +
                                                                        CODE_BOOK_SCF_LAV11],
                                    sbrEnvData->hufftableNoiseLevelTimeL[sbrEnvData->sbr_noise_levels[j] +
                                                                        CODE_BOOK_SCF_LAV11]);
        }
      }
      break;
    }
  }

  COUNT_sub_end();

  return payloadBits;
}


/*****************************************************************************

    description:  writes bits corresponding to the envelope data
    returns:      number of bits written

*****************************************************************************/
static int
writeEnvelopeData (HANDLE_SBR_ENV_DATA sbrEnvData, HANDLE_BIT_BUF hBitStream, int coupling)
{
  int payloadBits = 0, j, i, delta;

  COUNT_sub_start("writeEnvelopeData");

  PTR_INIT(3); /* sbrEnvData->domain_vec[]
                  sbrEnvData->ienvelope[][]
                  sbrEnvData->noScfBands[]
                */
  INDIRECT(1); LOOP(1);
  for (j = 0; j < sbrEnvData->noOfEnvelopes; j++) { /* loop over all envelopes */

    ADD(1); BRANCH(1);
    if (sbrEnvData->domain_vec[j] == FREQ) {

      INDIRECT(1); LOGIC(1); BRANCH(1);
      if (coupling && sbrEnvData->balance) {

        INDIRECT(1); FUNC(3); ADD(1);
        payloadBits += WriteBits (hBitStream, sbrEnvData->ienvelope[j][0], sbrEnvData->si_sbr_start_env_bits_balance);
      } else {

        INDIRECT(1); FUNC(3); ADD(1);
        payloadBits += WriteBits (hBitStream, sbrEnvData->ienvelope[j][0], sbrEnvData->si_sbr_start_env_bits);
      }
    }

    LOOP(1);
    for (i = 1 - sbrEnvData->domain_vec[j]; i < sbrEnvData->noScfBands[j]; i++) {

      MOVE(1);
      delta = sbrEnvData->ienvelope[j][i];

      if (coupling && sbrEnvData->balance) {
        assert (abs (delta) <= sbrEnvData->codeBookScfLavBalance);
      } else {
        assert (abs (delta) <= sbrEnvData->codeBookScfLav);
      }

      BRANCH(1);
      if (coupling) {

        INDIRECT(1); BRANCH(1);
        if (sbrEnvData->balance) {

          BRANCH(1);
          if (sbrEnvData->domain_vec[j]) {

            INDIRECT(2); FUNC(3); ADD(1);
            payloadBits += WriteBits (hBitStream,
                                      sbrEnvData->hufftableBalanceTimeC[delta + sbrEnvData->codeBookScfLavBalance],
                                      sbrEnvData->hufftableBalanceTimeL[delta + sbrEnvData->codeBookScfLavBalance]);
          } else {

            INDIRECT(2); FUNC(3); ADD(1);
            payloadBits += WriteBits (hBitStream,
                                      sbrEnvData->hufftableBalanceFreqC[delta + sbrEnvData->codeBookScfLavBalance],
                                      sbrEnvData->hufftableBalanceFreqL[delta + sbrEnvData->codeBookScfLavBalance]);
          }
        } else {

          BRANCH(1);
          if (sbrEnvData->domain_vec[j]) {

            INDIRECT(2); FUNC(3); ADD(1);
            payloadBits += WriteBits (hBitStream,
                                      sbrEnvData->hufftableLevelTimeC[delta + sbrEnvData->codeBookScfLav],
                                      sbrEnvData->hufftableLevelTimeL[delta + sbrEnvData->codeBookScfLav]);
          } else {

            INDIRECT(2); FUNC(3); ADD(1);
            payloadBits += WriteBits (hBitStream,
                                      sbrEnvData->hufftableLevelFreqC[delta + sbrEnvData->codeBookScfLav],
                                      sbrEnvData->hufftableLevelFreqL[delta + sbrEnvData->codeBookScfLav]);
          }
        }
      } else {

        BRANCH(1);
        if (sbrEnvData->domain_vec[j]) {

          INDIRECT(2); FUNC(3); ADD(1);
          payloadBits += WriteBits (hBitStream,
                                    sbrEnvData->hufftableTimeC[delta + sbrEnvData->codeBookScfLav],
                                    sbrEnvData->hufftableTimeL[delta + sbrEnvData->codeBookScfLav]);
        } else {

          INDIRECT(2); FUNC(3); ADD(1);
          payloadBits += WriteBits (hBitStream,
                                    sbrEnvData->hufftableFreqC[delta + sbrEnvData->codeBookScfLav],
                                    sbrEnvData->hufftableFreqL[delta + sbrEnvData->codeBookScfLav]);
        }
      }
    }
  }

  COUNT_sub_end();

  return payloadBits;
}


/*****************************************************************************

    description:  writes bits corresponding to the extended data
    returns:      number of bits written

*****************************************************************************/
static void
encodeExtendedData (HANDLE_SBR_ENV_DATA  sbrEnvDataLeft,
                    HANDLE_SBR_ENV_DATA  sbrEnvDataRight,
                    struct PS_ENC*    h_ps_e,
                    int                  bHeaderActive,
                    HANDLE_BIT_BUF       hBitStreamPrev,
                    int                  *sbrHdrBits,
                    HANDLE_BIT_BUF       hBitStream,
                    int* payloadBitsReturn)
{
  int extDataSize;
  int payloadBitsIn = *payloadBitsReturn;
  int payloadBits = 0;

  COUNT_sub_start("encodeExtendedData");

  MOVE(2); /* counting previous operations */

  FUNC(4);
  extDataSize = getSbrExtendedDataSize(sbrEnvDataLeft,
                                       sbrEnvDataRight,
                                       h_ps_e,
                                       bHeaderActive
                                       );



  BRANCH(1);
  if (extDataSize != 0) {

#ifndef MONO_ONLY

    FUNC(4); LOGIC(1); BRANCH(1);
    if (h_ps_e && AppendPsBS (h_ps_e, NULL, NULL, 0)) {

      FUNC(4); STORE(1);
      *payloadBitsReturn = AppendPsBS (h_ps_e, hBitStream, hBitStreamPrev, sbrHdrBits);
    }
    else {

#endif /*#ifndef MONO_ONLY */

      int maxExtSize = (1<<SI_SBR_EXTENSION_SIZE_BITS) - 1;

      SHIFT(1); ADD(1); /* counting previous operation */

      FUNC(3); ADD(1);
      payloadBits += WriteBits (hBitStream, 1, SI_SBR_EXTENDED_DATA_BITS);

      assert(extDataSize <= SBR_EXTENDED_DATA_MAX_CNT);

      ADD(1); BRANCH(1);
      if (extDataSize < maxExtSize) {

        FUNC(3); ADD(1);
        payloadBits += WriteBits (hBitStream, extDataSize, SI_SBR_EXTENSION_SIZE_BITS);
      } else {

        FUNC(3); ADD(1);
        payloadBits += WriteBits (hBitStream, maxExtSize, SI_SBR_EXTENSION_SIZE_BITS);

        FUNC(3); ADD(2);
        payloadBits += WriteBits (hBitStream, extDataSize - maxExtSize, SI_SBR_EXTENSION_ESC_COUNT_BITS);
      }

      ADD(1); STORE(1);
      *payloadBitsReturn = payloadBits + payloadBitsIn;

#ifndef MONO_ONLY
    }
#endif /*#ifndef MONO_ONLY */

  }
  else {

    FUNC(3); ADD(1);
    payloadBits += WriteBits (hBitStream, 0, SI_SBR_EXTENDED_DATA_BITS);

    ADD(1); STORE(1);
    *payloadBitsReturn = payloadBits + payloadBitsIn;
  }

  COUNT_sub_end();
}


/*****************************************************************************

    description:  writes bits corresponding to the "synthetic-coding"-extension
    returns:      number of bits written

*****************************************************************************/
static int writeSyntheticCodingData (HANDLE_SBR_ENV_DATA  sbrEnvData,
                                     HANDLE_BIT_BUF       hBitStream)

{
  int i;
  int payloadBits = 0;

  COUNT_sub_start("writeSyntheticCodingData");

  MOVE(1); /* counting previous operation */

    INDIRECT(1); FUNC(3); ADD(1);
    payloadBits += WriteBits (hBitStream, sbrEnvData->addHarmonicFlag, 1);

  BRANCH(1);
  if (sbrEnvData->addHarmonicFlag) {

    PTR_INIT(1); /* sbrEnvData->addHarmonic[] */
    LOOP(1);
    for (i = 0; i < sbrEnvData->noHarmonics; i++) {

      FUNC(3); ADD(1);
      payloadBits += WriteBits (hBitStream, sbrEnvData->addHarmonic[i], 1);
    }
  }

  COUNT_sub_end();

  return payloadBits;
}


/*****************************************************************************

    description:  counts the number of bits needed

    returns:      number of bits needed for the extended data

*****************************************************************************/
static int
getSbrExtendedDataSize (HANDLE_SBR_ENV_DATA sbrEnvDataLeft,
                        HANDLE_SBR_ENV_DATA sbrEnvDataRight,
                        struct PS_ENC*      h_ps_e,
                        int                 bHeaderActive)

{
  int extDataBits = 0;

  COUNT_sub_start("getSbrExtendedDataSize");

  MOVE(1); /* counting previous operation */

#ifndef MONO_ONLY

  BRANCH(1);
  if (h_ps_e) {

    FUNC(2); ADD(1);
    extDataBits += WritePsData(h_ps_e, bHeaderActive);
  }

#endif /*#ifndef MONO_ONLY */

  /*
    no extended data
  */

  BRANCH(1);
  if (extDataBits != 0)
  {
    ADD(1);
    extDataBits += SI_SBR_EXTENSION_ID_BITS;
  }

  ADD(1); SHIFT(1); /* counting post-operation */

  COUNT_sub_end();

  return (extDataBits+7) >> 3;
}


