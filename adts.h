/*
  adts

  Copyright (C) Matteo Croce

 */

#define ADTS_HEADER_SIZE 7

#include "aacenc.h"

const int aac_sampling_freq[16] = {96000, 88200, 64000, 48000, 44100, 32000,
                                   24000, 22050, 16000, 12000, 11025,  8000,
                                   0, 0, 0, 0}; /* filling */
const int id = 0, profile = 1;

void adts_hdr(char *adts, AACENC_CONFIG *config) {
	int srate_idx = 15, i;

	/* sync word, 12 bits */
	adts[0] = (char)0xff;
	adts[1] = (char)0xf0;

	/* ID, 1 bit */
	adts[1] |= id << 3;
	/* layer: 2 bits = 00 */

	/* protection absent: 1 bit = 1 (ASSUMPTION!) */
	adts[1] |= 1;

	/* profile, 2 bits */
	adts[2] = profile << 6;

	for (i = 0; i < 16; i++)
		if (config->sampleRate >= (aac_sampling_freq[i] - 1000)) {
			srate_idx = i;
			break;
		}

	/* sampling frequency index, 4 bits */
	adts[2] |= srate_idx << 2;

	/* private, 1 bit = 0 (ASSUMPTION!) */

	/* channels, 3 bits */
	adts[2] |= (config->nChannelsOut & 4) >> 2;
	adts[3] = (config->nChannelsOut & 3) << 6;

	/* adts buffer fullness, 11 bits, 0x7ff = VBR (ASSUMPTION!) */
	adts[6] = (char)0xfc;
}

void adts_hdr_up(char *adts, int size)
{
	/* frame length, 13 bits */
	int len = size + 7;
	adts[3] = (adts[3] & 0xc0) | (len >> 11);
	adts[4] = (len >> 3) & 0xff;
	adts[5] = (len & 7) << 5 | (char)0x1f;
}
