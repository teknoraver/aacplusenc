#include <stdio.h>
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include "FloatFR.h"
#include "au_channel.h"
#include "aacenc.h"
#include "iir32resample.h"
#include "resampler.h"

#include "sbr_main.h"
#include "aac_ram.h"
#include "aac_rom.h"
#include "adts.h"
#include "cfftn.h"
#include "config.h"

/* dynamic buffer of SBR that can be reused for resampling */
extern float sbr_envRBuffer[];

#define CORE_DELAY   (1600)
#define INPUT_DELAY  ((CORE_DELAY)*2 +6*64-2048+1)	/* ((1600 (core codec)*2 (multi rate) + 6*64 (sbr dec delay) - 2048 (sbr enc delay) + magic */
#define MAX_DS_FILTER_DELAY 16	/* the additional max resampler filter delay (source fs) */

#define CORE_INPUT_OFFSET_PS (0)	/* (96-64) makes AAC still some 64 core samples too early wrt SBR ... maybe -32 would be even more correct, but 1024-32 would need additional SBR bitstream delay by one frame */

#define bEncodeMono (inputInfo.nChannels == 1)

/*

      input buffer (1ch)

      |------------ 1537   -------------|-----|---------- 2048 -------------|
           (core2sbr delay     )          ds     (read, core and ds area)
*/
static float
    inputBuffer[(AACENC_BLOCKSIZE * 2 + MAX_DS_FILTER_DELAY +
		 INPUT_DELAY) * MAX_CHANNELS];
char outputBuffer[ADTS_HEADER_SIZE + (6144 / 8) * MAX_CHANNELS];

static IIR21_RESAMPLER IIR21_reSampler[MAX_CHANNELS];

int main(int argc, char *argv[])
{
	AACENC_CONFIG config;

	WavInfo inputInfo;
	FILE *inputFile;

	FILE *hADTSFile;

	int error;

	int bitrate;
	int nChannelsAAC, nChannelsSBR;
	unsigned int sampleRateAAC;
	int frmCnt;
	int bandwidth = 0;

	unsigned int numAncDataBytes = 0;
	unsigned char ancDataBytes[MAX_PAYLOAD_SIZE];
	unsigned int ancDataLength = 0;

	/*!< required only for interfacing with audio output library, thus not counted into RAM requirements */
	short TimeDataPcm[AACENC_BLOCKSIZE * 2 * MAX_CHANNELS];

	int numSamplesRead;
	int bDoIIR2Downsample = 0;
	int bDingleRate = 0;
	int useParametricStereo = 0;
	int coreWriteOffset = 0;
	int coreReadOffset = 0;
	int envWriteOffset = 0;
	int envReadOffset = 0;
	int writeOffset = INPUT_DELAY * MAX_CHANNELS;
	int percent = -1;

	struct AAC_ENCODER *aacEnc = 0;

	int bDoUpsample = 0;
	int upsampleReadOffset = 0;

	int inSamples;
	int bDoIIR32Resample = 0;
	int nSamplesPerChannel;
	const int nRuns = 4;
	float *resamplerScratch = sbr_envRBuffer;

	HANDLE_SBR_ENCODER hEnvEnc = NULL;

	/* initialize the 3GPP instrumenting tool */

	fprintf(stderr,
		"\n"
		"*************************************************************\n"
		"* Enhanced aacPlus Encoder\n"
		"* Build %s, %s\n"
		"* Matteo Croce <rootkit85@yahoo.it>\n"
		"*************************************************************\n\n",
		__DATE__, __TIME__);

	/*
	 * parse command line arguments
	 */
	if (argc != 4) {
		fprintf(stderr,
			"\nUsage:   %s <source.wav> <destination.aac> <bitrate>\n",
			argv[0]);
		fprintf(stderr,
			"\nUse - as filename for stdin and/or stdout.\n");
		fprintf(stderr, "\nExample: %s song.wav song.aac 32\n",
			argv[0]);
		return 0;
	}

	bitrate = atoi(argv[3]) * 1000;;

	fflush(stderr);

	/*
	   open audio input file
	 */

	inputFile = AuChannelOpen(argv[1], &inputInfo);

	if (!inputFile) {
		fprintf(stderr, "could not open %s\n", argv[1]);
		exit(10);
	}

	/*
	   set up basic parameters for aacPlus codec
	 */

	AacInitDefaultConfig(&config);

	nChannelsAAC = nChannelsSBR = inputInfo.nChannels;

	if ((inputInfo.nChannels == 2) && (bitrate >= 16000)
	    && (bitrate < 44001))
		useParametricStereo = 1;

	if (useParametricStereo) {
		nChannelsAAC = 1;
		nChannelsSBR = 2;
	}

	if (((inputInfo.sampleRate == 48000) && (nChannelsAAC == 2)
	     && (bitrate < 24000))
	    || ((inputInfo.sampleRate == 48000) && (nChannelsAAC == 1)
		&& (bitrate < 12000))
	    )
		bDoIIR32Resample = 1;

	if (inputInfo.sampleRate == 16000) {

		bDoUpsample = 1;
		inputInfo.sampleRate = 32000;
		bDingleRate = 1;
	}

	sampleRateAAC = inputInfo.sampleRate;

	if (bDoIIR32Resample)
		sampleRateAAC = 32000;

	config.bitRate = bitrate;
	config.nChannelsIn = inputInfo.nChannels;
	config.nChannelsOut = nChannelsAAC;

	config.bandWidth = bandwidth;

	/*
	   set up SBR configuration
	 */

	if (!IsSbrSettingAvail
	    (bitrate, nChannelsAAC, sampleRateAAC, &sampleRateAAC)) {
		fprintf(stderr,
			"No valid SBR configuration found for:\n\t\t\tbitrate=%d\n\t\t\tchannels=%d\n\t\t\tsamplerate=%d\n",
			bitrate, nChannelsAAC, sampleRateAAC);
		exit(10);
	}

	{
		sbrConfiguration sbrConfig;

		envReadOffset = 0;

		coreWriteOffset = 0;

		if (useParametricStereo) {
			envReadOffset =
			    (MAX_DS_FILTER_DELAY + INPUT_DELAY) * MAX_CHANNELS;
			coreWriteOffset = CORE_INPUT_OFFSET_PS;
			writeOffset = envReadOffset;
		}

		InitializeSbrDefaults(&sbrConfig);

		sbrConfig.usePs = useParametricStereo;

		AdjustSbrSettings(&sbrConfig,
				  bitrate,
				  nChannelsAAC,
				  sampleRateAAC, AACENC_TRANS_FAC, 24000);

		EnvOpen(&hEnvEnc,
			inputBuffer + coreWriteOffset,
			&sbrConfig, &config.bandWidth);

		/* set IIR 2:1 downsampling */

		bDoIIR2Downsample = (bDoUpsample) ? 0 : 1;

		if (useParametricStereo)
			bDoIIR2Downsample = 0;

	}

	/*
	   set up mode-dependant 1:2 upsampling
	 */

	if (bDoUpsample) {

		if (inputInfo.nChannels > 1) {
			fprintf(stderr,
				"\n Stereo @ 16kHz input sample rate is not supported\n");
			return -1;
		}

		InitIIR21_Resampler(IIR21_reSampler);
		InitIIR21_Resampler(IIR21_reSampler + 1);

		assert(IIR21_reSampler[0].delay <= MAX_DS_FILTER_DELAY);

		if (useParametricStereo) {
			writeOffset += AACENC_BLOCKSIZE * MAX_CHANNELS;
			upsampleReadOffset = writeOffset;
			envWriteOffset = envReadOffset;
		} else {
			writeOffset += AACENC_BLOCKSIZE * MAX_CHANNELS;
			coreReadOffset = writeOffset;
			upsampleReadOffset = writeOffset - (((INPUT_DELAY -  IIR21_reSampler[0].delay) >> 1) * MAX_CHANNELS);
			envWriteOffset = ((INPUT_DELAY - IIR21_reSampler[0].delay) & 0x1) * MAX_CHANNELS;
			envReadOffset = 0;
		}
	} else if (bDoIIR2Downsample) {

		/*
		   set up 2:1 downsampling
		 */

			InitIIR21_Resampler(IIR21_reSampler);
			InitIIR21_Resampler(IIR21_reSampler + 1);

			assert(IIR21_reSampler[0].delay <= MAX_DS_FILTER_DELAY);

			writeOffset += IIR21_reSampler[0].delay * MAX_CHANNELS;
		}

	if (bDoIIR32Resample)
		IIR32Init();

	/*
	   set up AAC encoder, now that samling rate is known
	 */

	config.sampleRate = sampleRateAAC;

	error = AacEncOpen(&aacEnc, config);

	if (error) {
		fprintf(stderr, "\n Initialisation of AAC failed !\n");

		AacEncClose(aacEnc);

		AuChannelClose(inputFile);

		return 1;
	}

	if (strcmp(argv[2], "-") == 0)
		hADTSFile = stdout;
	else
		hADTSFile = fopen(argv[2], "wb");

	if (!hADTSFile) {
		fprintf(stderr, "\nFailed to create ADTS file\n");
		exit(10);
	}

	/*
	   Be verbose
	 */

	fprintf(stderr, "input file %s: \nsr = %d, nc = %d\n\n", argv[1],
		inputInfo.sampleRate, inputInfo.nChannels);
	fprintf(stderr, "output file %s: \nbr = %d sr-OUT = %d  nc-OUT = %d\n\n",
		argv[2], bitrate, sampleRateAAC * 2, nChannelsSBR);
	fflush(stderr);

	init_plans();

	frmCnt = 0;

	memset(TimeDataPcm, 0, sizeof(TimeDataPcm));

	/*
	   set up input samples block size feed
	 */

	if (bDoIIR32Resample) {
		inSamples = IIR32GetResamplerFeed(AACENC_BLOCKSIZE *  inputInfo.nChannels * 2) / nRuns;
		assert(inSamples <= AACENC_BLOCKSIZE * 2);
	} else {
		inSamples = AACENC_BLOCKSIZE * inputInfo.nChannels * 2;
		if (bDoUpsample)
			inSamples = inSamples >> 1;
	}

	/* create the ADTS header */
	adts_hdr(outputBuffer, &config);

	/*
	   The frame loop
	 */
	while (1) {
		int i, ch, outSamples, numOutBytes, newpercent = -1;

		/*
		   File input read, resample and downmix
		 */

		if (bDoIIR32Resample) {

			/* resampling from 48 kHz to 32 kHz prior to encoding */

			int stopLoop = 0;
			int nSamplesProcessed = 0;
			int r;

			const int nDSOutBlockSize = AACENC_BLOCKSIZE * 2 / nRuns;
			int stride = inputInfo.nChannels;
			if (inputInfo.nChannels == 1)
				stride = 2;

			/* counting previous operations */

			for (r = 0; r < nRuns; r++) {
				if (AuChannelReadShort(inputFile, TimeDataPcm,  inSamples, &numSamplesRead)) {
					stopLoop = 1;
					break;
				}

				/* copy from short to float input buffer */
				/* resamplerScratch[]
				   TimeDataPcm[]
				 */
				for (i = 0; i < numSamplesRead; i++)
					resamplerScratch[i] = (float)TimeDataPcm[i];

				switch (inputInfo.nChannels) {
				case 1:
					nSamplesPerChannel = numSamplesRead;
					break;
				case 2:
					nSamplesPerChannel = numSamplesRead >> 1;
					break;
				default:
					nSamplesPerChannel = numSamplesRead / inputInfo.nChannels;
				}

				/* supposed to return exact number samples to encode one block of audio */

				nSamplesProcessed +=
				    IIR32Resample(resamplerScratch,
						  &inputBuffer[writeOffset +  r * nDSOutBlockSize * stride],
						  nSamplesPerChannel,
						  nDSOutBlockSize,
						  inputInfo.nChannels);

				if (inputInfo.nChannels == 1 && stride == 2)
					/* reordering necessary since the encoder takes interleaved data */
					for (i = nDSOutBlockSize - 1; i >= 0;  i--)
						inputBuffer[writeOffset + r * nDSOutBlockSize * 2 + 2 * i] =
						    inputBuffer[writeOffset + r * nDSOutBlockSize * 2 + i];
			}

			if (stopLoop)
				break;

			numSamplesRead = nSamplesProcessed;

		} else {
			/* no resampling prior to encoding required */
			/* read from file */
			if (AuChannelReadShort(inputFile, TimeDataPcm,
					       inSamples, &numSamplesRead))
				break;

			/* copy from short to float input buffer */

			/* inputBuffer[i+writeOffset]
			   TimeDataPcm[i]
			 */
			if (inputInfo.nChannels == nChannelsSBR)
				for (i = 0; i < numSamplesRead; i++)
					inputBuffer[i + writeOffset] = (float)TimeDataPcm[i];

			/* copy from short to float input buffer, reordering necessary since the encoder takes interleaved data */
			if (inputInfo.nChannels == 1)
				/* inputBuffer[writeOffset+MAX_CHANNELS*i]
				   inputBuffer[writeOffset+i]
				 */
				for (i = 0; i < numSamplesRead; i++)
					inputBuffer[writeOffset + 2 * i] = (float)TimeDataPcm[i];

			/* copy from short to float input buffer, downmix stereo input signal to mono, reordering necessary since the encoder takes interleaved data */

			if ((inputInfo.nChannels == 2) && bEncodeMono)
				/*  inputBuffer[writeOffset+2*i]
				   inputBuffer[writeOffset+2*i+1]
				 */
				for (i = 0; i < numSamplesRead / 2; i++)
					inputBuffer[writeOffset + 2 * i] = ((float)TimeDataPcm[2 * i] + (float)TimeDataPcm[2 * i + 1]) * 0.5f;
		}		/*end if (bDoIIR32Resample) */

		if (bDoUpsample)
			for (ch = 0; ch < inputInfo.nChannels; ch++)
				IIR21_Upsample(&(IIR21_reSampler[ch]),
					       inputBuffer +
					       upsampleReadOffset + ch,
					       numSamplesRead /
					       inputInfo.nChannels,
					       MAX_CHANNELS,
					       inputBuffer + envWriteOffset +
					       ch, &outSamples, MAX_CHANNELS);

		/*
		   encode one SBR frame
		 */
		EnvEncodeFrame(hEnvEnc,
			       inputBuffer + envReadOffset,
			       inputBuffer + coreWriteOffset,
			       MAX_CHANNELS, &numAncDataBytes, ancDataBytes);

		/*
		   2:1 downsampling for AAC core
		 */
		if (bDoIIR2Downsample)
			for (ch = 0; ch < nChannelsAAC; ch++)
				IIR21_Downsample(&(IIR21_reSampler[ch]),
						 inputBuffer + writeOffset + ch,
						 numSamplesRead /
						 inputInfo.nChannels,
						 MAX_CHANNELS, inputBuffer + ch,
						 &outSamples, MAX_CHANNELS);

		/* SBR side info data is passed as ancillary data */
		if (numAncDataBytes == 0)
			numAncDataBytes = ancDataLength;

		/*
		   encode one AAC frame
		 */
		if (hEnvEnc && useParametricStereo) {
			AacEncEncode(aacEnc, inputBuffer, 1,	/* stride */
				     ancDataBytes,
				     &numAncDataBytes,
				     (unsigned *)(outputBuffer + ADTS_HEADER_SIZE),
				     &numOutBytes);

			if (hEnvEnc)
				memcpy(inputBuffer,
				       inputBuffer + AACENC_BLOCKSIZE,
				       CORE_INPUT_OFFSET_PS * sizeof(float));
		} else {
			AacEncEncode(aacEnc,
				     inputBuffer + coreReadOffset,
				     MAX_CHANNELS,
				     ancDataBytes,
				     &numAncDataBytes,
				     (unsigned *)(outputBuffer + ADTS_HEADER_SIZE),
				     &numOutBytes);

			if (hEnvEnc) {
				if (bDoUpsample) {
					memmove(&inputBuffer[envReadOffset],
						&inputBuffer[envReadOffset +  AACENC_BLOCKSIZE * MAX_CHANNELS * 2],
						(envWriteOffset - envReadOffset) *
						sizeof(float));

					memmove(&inputBuffer
						[upsampleReadOffset],
						&inputBuffer[upsampleReadOffset + AACENC_BLOCKSIZE * MAX_CHANNELS],
						(writeOffset - upsampleReadOffset) *
						sizeof(float));
				} else
					memmove(inputBuffer,
						inputBuffer +
						AACENC_BLOCKSIZE * 2 *
						MAX_CHANNELS,
						writeOffset * sizeof(float));
			}
		}

		/*
		   Write one frame of encoded audio to file
		 */

		if (numOutBytes) {
			adts_hdr_up(outputBuffer, numOutBytes);
			fwrite(outputBuffer, 1, numOutBytes + ADTS_HEADER_SIZE, hADTSFile);
		}

		frmCnt++;

		/* 3GPP instrumenting tool: measure worst case work load at end of each decoding loop */

		if (inputInfo.nSamples > 0) {
			newpercent = frmCnt * AACENC_BLOCKSIZE * inputInfo.nChannels / (inputInfo.nSamples / (4 * 100));
		}
		if (newpercent != percent) {
			percent = newpercent;
			fprintf(stderr, "[%d%%]\r", newpercent);
			fflush(stderr);
		}
	}
	fprintf(stderr, "\n");
	fflush(stderr);

	/*
	   Close encoder
	 */

	if (numSamplesRead > 0) {
		AacEncClose(aacEnc);
	}

	AuChannelClose(inputFile);

	destroy_plans();

	if (hEnvEnc)
		EnvClose(hEnvEnc);

	/* 3GPP instrumenting tool: print output data */

	fprintf(stderr, "\nencoding finished\n");
	fflush(stderr);

	return 0;
}
