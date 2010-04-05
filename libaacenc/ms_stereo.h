/*
  MS stereo processing
*/
#ifndef __MS_STEREO_H__
#define __MS_STEREO_H__

void MsStereoProcessing(float       *sfbEnergyLeft,
                        float       *sfbEnergyRight,
                        const float *sfbEnergyMid,
                        const float *sfbEnergySide,
                        float       *mdctSpectrumLeft,
                        float       *mdctSpectrumRight,
                        float       *sfbThresholdLeft,
                        float       *sfbThresholdRight,
                        float       *sfbSpreadedEnLeft,
                        float       *sfbSpreadedEnRight,
                        int         *msDigest,
                        int         *msMask,
                        const int    sfbCnt,
                        const int    sfbPerGroup,
                        const int   maxSfbPerGroup,
                        const int   *sfbOffset,
                        float       *weightMsLrPeRatio);


#endif
