/*
  Hybrid Filter Bank header file
*/

#ifndef _HYBRID_H
#define _HYBRID_H

#define HYBRID_FILTER_LENGTH  13
#define QMF_BUFFER_MOVE       (HYBRID_FILTER_LENGTH - 1)
#define HYBRID_FILTER_DELAY    6
#define NO_QMF_BANDS_IN_HYBRID 3
#define NO_HYBRID_BANDS 16

typedef enum {

  HYBRID_2_REAL = 2,
  HYBRID_4_CPLX = 4,
  HYBRID_8_CPLX = 8

} HYBRID_RES;

typedef struct
{
  float *pWorkReal;
  float *pWorkImag;

  float **mQmfBufferReal;
  float **mQmfBufferImag;
} HYBRID;

typedef HYBRID *HANDLE_HYBRID;

void
HybridAnalysis ( const float **mQmfReal,
                 const float **mQmfImag,
                 float **mHybridReal,
                 float **mHybridImag,
                 HANDLE_HYBRID hHybrid );
void
HybridSynthesis ( const float **mHybridReal,
                  const float **mHybridImag,
                  float **mQmfReal,
                  float **mQmfImag,
                  HANDLE_HYBRID hHybrid );

int
CreateHybridFilterBank ( HANDLE_HYBRID hHybrid,
                         float **pPtr);

void
DeleteHybridFilterBank ( HANDLE_HYBRID *phHybrid );



#endif /* _HYBRID_H */

