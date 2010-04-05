#include <stdio.h>
#include <string.h>

#include "config.h"

#define b2l(A)  ((((unsigned short)(A) & 0xff00) >> 8) | (((unsigned short)(A) & 0x00ff) << 8))

#define WAV_HEADER_SIZE 44

typedef struct {
	int	sampleRate;
	int	nChannels;
	long	nSamples;
} WavInfo;

FILE* AuChannelOpen (const char* filename, WavInfo* info)
{
	unsigned char header[WAV_HEADER_SIZE];
	FILE *handle;
	
	if (!strcmp(filename,"-"))
		handle = stdin;
	else
		handle = fopen(filename, "rb");

	if(!handle)
		return NULL;

	if(fread(header, 1, WAV_HEADER_SIZE, handle) != WAV_HEADER_SIZE)
		return 0;

	info->nSamples		= (header[4] | (header[5] << 8) | (header[6] << 16) | (header[7] << 24)) + 8;
	info->nChannels		= header[22] | header[23] << 8;
	info->sampleRate	= header[24] | header[25] << 8 | header[26] << 12 | header[27] << 16;
	return handle;
}

void AuChannelClose (FILE *audioChannel)
{
	fclose(audioChannel);
}

size_t AuChannelReadShort(FILE *audioChannel, short *samples, int nSamples, int *readed)
{
	*readed = fread(samples, 2, nSamples, audioChannel);
#ifdef _BE_ARCH
	{
		int i;
		for(i = 0; i < *readed; i++)
			samples[i] = b2l(samples[i]);
	}
#endif
	return *readed <= 0;
}
