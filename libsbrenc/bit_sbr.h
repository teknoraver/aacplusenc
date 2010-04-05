/*
  SBR bit writing
*/
#ifndef __BIT_SBR_H
#define __BIT_SBR_H


struct SBR_HEADER_DATA;
struct SBR_BITSTREAM_DATA;
struct SBR_ENV_DATA;
struct COMMON_DATA;
struct PS_ENC;


int WriteEnvSingleChannelElement(struct SBR_HEADER_DATA    *sbrHeaderData,
                                 struct SBR_BITSTREAM_DATA *sbrBitstreamData,
                                 struct SBR_ENV_DATA       *sbrEnvData,
                                 struct PS_ENC             *h_ps_e,
                                 struct COMMON_DATA        *cmonData);


int WriteEnvChannelPairElement(struct SBR_HEADER_DATA    *sbrHeaderData,
                               struct SBR_BITSTREAM_DATA *sbrBitstreamData,
                               struct SBR_ENV_DATA       *sbrEnvDataLeft,
                               struct SBR_ENV_DATA       *sbrEnvDataRight,
                               struct COMMON_DATA        *cmonData);



int CountSbrChannelPairElement (struct SBR_HEADER_DATA     *sbrHeaderData,
                                struct SBR_BITSTREAM_DATA  *sbrBitstreamData,
                                struct SBR_ENV_DATA        *sbrEnvDataLeft,
                                struct SBR_ENV_DATA        *sbrEnvDataRight,
                                struct COMMON_DATA         *cmonData);




#endif
