/*
  Declaration of constant tables
*/
#ifndef __SBR_ROM_H
#define __SBR_ROM_H

extern const int   aHybridResolution[];
extern const int   hiResBandBorders[];
extern const int   groupBordersMix[];
extern const int   bins2groupMap[];
extern const float panClass[];
extern const float saClass[];
extern const float p4_13[];
extern const float p8_13[];
extern const float sbr_cos_twiddle[];
extern const float sbr_sin_twiddle[];
extern const float sbr_alt_sin_twiddle[];
extern const float p_64_640_qmf[];
extern const int   aBookPsIidTimeCode[];
extern const char  aBookPsIidTimeLength[];
extern const int   aBookPsIidFreqCode[];
extern const char  aBookPsIidFreqLength[];
extern const short aBookPsIccTimeCode[];
extern const char  aBookPsIccTimeLength[];
extern const short aBookPsIccFreqCode[];
extern const char  aBookPsIccFreqLength[];

extern const float trigData_fct4_32[];
extern const float trigData_fct4_16[];
extern const float trigData_fct4_8[];

extern const float sbr_qmf_64_640[325];

extern const int           v_Huff_envelopeLevelC10T[121];
extern const unsigned char v_Huff_envelopeLevelL10T[121];
extern const int           v_Huff_envelopeLevelC10F[121];
extern const unsigned char v_Huff_envelopeLevelL10F[121];
extern const int           bookSbrEnvBalanceC10T[49];
extern const unsigned char bookSbrEnvBalanceL10T[49];
extern const int           bookSbrEnvBalanceC10F[49];
extern const unsigned char bookSbrEnvBalanceL10F[49];
extern const int           v_Huff_envelopeLevelC11T[63];
extern const unsigned char v_Huff_envelopeLevelL11T[63];
extern const int           v_Huff_envelopeLevelC11F[63];
extern const unsigned char v_Huff_envelopeLevelL11F[63];
extern const int           bookSbrEnvBalanceC11T[25];
extern const unsigned char bookSbrEnvBalanceL11T[25];
extern const int           bookSbrEnvBalanceC11F[25];
extern const unsigned char bookSbrEnvBalanceL11F[25];
extern const int           v_Huff_NoiseLevelC11T[63];
extern const unsigned char v_Huff_NoiseLevelL11T[63];
extern const int           bookSbrNoiseBalanceC11T[25];
extern const unsigned char bookSbrNoiseBalanceL11T[25];

#endif

