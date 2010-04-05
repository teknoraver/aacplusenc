/*
  calculation of band energies
*/
#ifndef _BAND_NRG_H
#define _BAND_NRG_H

void CalcBandEnergy(const float *mdctSpectrum,
                    const int   *bandOffset,
                    const int    numBands,
                    float      *bandEnergy,
                    float       *bandEnergySum);


void CalcBandEnergyMS(const float *mdctSpectrumLeft,
                      const float *mdctSpectrumRight,
                      const int   *bandOffset,
                      const int    numBands,
                      float       *bandEnergyMid,
                      float       *bandEnergyMidSum,
                      float       *bandEnergySide,
                      float       *bandEnergySideSum
                      );

#endif
