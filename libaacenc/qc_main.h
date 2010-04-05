/*
  Quantizing & coding main module
*/
#ifndef _QC_MAIN_H
#define _QC_MAIN_H

#include "qc_data.h"
#include "interface.h"


int  QCOutNew(QC_OUT *hQC, int nChannels);

void QCOutDelete(QC_OUT *hQC);

int  QCNew(QC_STATE *hQC);

int  QCInit(QC_STATE *hQC, struct QC_INIT *init);
void QCDelete(QC_STATE *hQC);


int QCMain(QC_STATE *hQC,
           int nChannels,
           ELEMENT_BITS* elBits,
           ATS_ELEMENT* adjThrStateElement,
           PSY_OUT_CHANNEL psyOutChannel[MAX_CHANNELS],
           PSY_OUT_ELEMENT* psyOutElement,
           QC_OUT_CHANNEL  qcOutChannel[MAX_CHANNELS],
           QC_OUT_ELEMENT* qcOutElement,
           int ancillaryDataBytes);

void UpdateBitres(QC_STATE* qcKernel,
                  QC_OUT* qcOut);

int FinalizeBitConsumption(QC_STATE *hQC,
                           QC_OUT* qcOut);

int AdjustBitrate(QC_STATE *hQC,
                  int bitRate,
                  int sampleRate);

#endif /* _QC_MAIN_H */
