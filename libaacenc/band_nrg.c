/*
  calculation of band energies
*/
#include "band_nrg.h"

#include "counters.h" /* the 3GPP instrumenting tool */

void CalcBandEnergy(const float *mdctSpectrum,
                    const int   *bandOffset,
                    const int    numBands,
                    float      *bandEnergy,
                    float       *bandEnergySum) {
  int i, j;

  COUNT_sub_start("CalcBandEnergy");

  MOVE(2);
  j = 0;
  *bandEnergySum = 0.0f;
 
  PTR_INIT(3); /* pointers for bandEnergy[],
                               bandOffset[],
                               mdctSpectrum[]
                */
  LOOP(1);
  for(i=0; i<numBands; i++) {

    MOVE(1);
    bandEnergy[i] = 0.0f;

    LOOP(1);
    while (j < bandOffset[i+1]) {

      MAC(1); STORE(1);
      bandEnergy[i] += mdctSpectrum[j] * mdctSpectrum[j];

      j++;
    }
    ADD(1); STORE(1);
    *bandEnergySum += bandEnergy[i];
  }

  COUNT_sub_end();
}



void CalcBandEnergyMS(const float *mdctSpectrumLeft,
                      const float *mdctSpectrumRight,
                      const int   *bandOffset,
                      const int    numBands,
                      float      *bandEnergyMid,
                      float       *bandEnergyMidSum,
                      float      *bandEnergySide,
                      float       *bandEnergySideSum) {

  int i, j;

  COUNT_sub_start("CalcBandEnergyMS");

  MOVE(3);
  j = 0;
	*bandEnergyMidSum = 0.0f;
	*bandEnergySideSum = 0.0f;

  PTR_INIT(5); /* pointers for bandEnergyMid[],
                               bandEnergySide[],
                               bandOffset[],
                               mdctSpectrumLeft[],
                               mdctSpectrumRight[]
               */
  LOOP(1);
  for(i=0; i<numBands; i++) {

    MOVE(2);
    bandEnergyMid[i] = 0.0f;
    bandEnergySide[i] = 0.0f;

    LOOP(1);
    while (j < bandOffset[i+1]) {
        float specm, specs;

        ADD(2); MULT(2);
        specm = 0.5f * (mdctSpectrumLeft[j] + mdctSpectrumRight[j]);
        specs = 0.5f * (mdctSpectrumLeft[j] - mdctSpectrumRight[j]);

        MAC(2); STORE(2);
        bandEnergyMid[i]  += specm * specm;
        bandEnergySide[i] += specs * specs;

        j++;
    }
    ADD(2); STORE(2);
    *bandEnergyMidSum += bandEnergyMid[i];
		*bandEnergySideSum += bandEnergySide[i];

  }

  COUNT_sub_end();
}

