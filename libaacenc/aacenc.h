/*
  fast aac coder interface library functions
*/

#ifndef _aacenc_h_
#define _aacenc_h_

/* here we distinguish between stereo and the mono only encoder */
#ifdef MONO_ONLY
#define MAX_CHANNELS        1
#else
#define MAX_CHANNELS        2
#endif

#define AACENC_BLOCKSIZE    1024   /*! encoder only takes BLOCKSIZE samples at a time */
#define AACENC_TRANS_FAC    8      /*! encoder short long ratio */
#define AACENC_PCM_LEVEL    1.0    /*! encoder pcm 0db refernence */


/*-------------------------- defines --------------------------------------*/

#define BUFFERSIZE 1024     /* anc data */

/*-------------------- structure definitions ------------------------------*/

typedef  struct {
  int   sampleRate;            /* audio file sample rate */
  int   bitRate;               /* encoder bit rate in bits/sec */
  int   nChannelsIn;           /* number of channels on input (1,2) */
  int   nChannelsOut;          /* number of channels on output (1,2) */
  int   bandWidth;             /* core coder audio bandwidth in Hz */
} AACENC_CONFIG;

struct AAC_ENCODER;

/*
 * p u b l i c   a n c i l l a r y
 *
 */


/*-----------------------------------------------------------------------------

     functionname: AacInitDefaultConfig
     description:  gives reasonable default configuration
     returns:      ---

 ------------------------------------------------------------------------------*/
void AacInitDefaultConfig(AACENC_CONFIG *config);

/*---------------------------------------------------------------------------

    functionname:AacEncOpen
    description: allocate and initialize a new encoder instance
    returns:     AACENC_OK if success

  ---------------------------------------------------------------------------*/

int  AacEncOpen
(  struct AAC_ENCODER**     phAacEnc,       /* pointer to an encoder handle, initialized on return */
   const  AACENC_CONFIG     config          /* pre-initialized config struct */
);

int AacEncEncode(struct AAC_ENCODER  *hAacEnc,
                 float               *timeSignal,
                 unsigned int        timeInStride,
                 const unsigned char *ancBytes,      /*!< pointer to ancillary data bytes */
                 unsigned int        *numAncBytes,   /*!< number of ancillary Data Bytes, send as fill element  */
                 unsigned int        *outBytes,      /*!< pointer to output buffer            */
                 int                 *numOutBytes    /*!< number of bytes in output buffer */
                 );


/*---------------------------------------------------------------------------

    functionname:AacEncClose
    description: deallocate an encoder instance

  ---------------------------------------------------------------------------*/

void AacEncClose (struct AAC_ENCODER* hAacEnc); /* an encoder handle */

#endif /* _aacenc_h_ */
