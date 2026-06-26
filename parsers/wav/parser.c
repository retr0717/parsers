#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>


#pragma pack(push,1)

typedef struct {
	char riff[4];
	uint32_t fileSize;
	char wave[4];
} RIFFHeader;

typedef struct {
	char id[4];
	uint32_t size;
} ChunkHeader;

typedef struct {
	uint16_t audioFormat;
	uint16_t channels;
	uint32_t sampleRate;
	uint32_t byteRate;
	uint16_t blockAlign;
	uint16_t bitsPerSample;
} FormatChunk;

#pragma pack(pop)

int main(void)
{
	//file pointer;
	FILE *fp = fopen("output.wav", "rb");

	//read from the file
	RIFFHeader hdr;
	fread(&hdr, sizeof(hdr), 1, fp);

	//validate the headers.
	printf("\n___FILE_HEADER___\n");
	if(memcmp(hdr.riff, "RIFF", 4) != 0)
	{
		printf("Not RIFF\n");
		return 1;
	}
	printf("%.4s : %u\n", hdr.riff, hdr.fileSize);

	if(memcmp(hdr.wave, "WAVE", 4) != 0)
	{
		printf("Not WAV\n");
		return 1;
	}
	printf("%.4s\n", hdr.wave);

	//generic chunk header
	ChunkHeader chunk;
	FormatChunk fmt;
	uint32_t byteRate;
	uint32_t dataSize;
	while(fread(&chunk, sizeof(chunk), 1, fp) == 1)
	{
		//printf("%.4s : %u bytes\n", chunk.id, chunk.size);

		//fmt
		if(memcmp(chunk.id, "fmt", 3) == 0)
		{
			printf("\n___FORMAT_HEADER___\n");
			fread(&fmt, sizeof(fmt), 1, fp);
        		printf("Channels : %u\n", fmt.channels);
        		printf("Sample Rate : %u\n", fmt.sampleRate);
			printf("Byte Rate : %u\n", fmt.byteRate);
			byteRate = fmt.byteRate;
        		printf("Bits/Sample: %u\n", fmt.bitsPerSample);

			if(fmt.audioFormat != 1)
				printf("Compressed WAV not supported\n");
		}
		else if(memcmp(chunk.id, "data", 4) == 0) //read samples.
		{
			printf("\n___DATA_HEADER___\n");
			dataSize = chunk.size;
			int16_t sample;
			while(fread(&sample, sizeof(sample), 1, fp) == 1)
			{
				printf("Sample : %d\n", sample);
			}
		}
		else
		{
			fseek(fp, chunk.size, SEEK_CUR);
		}


	}

	printf("Audio Size : %u bytes\n", chunk.size);
	printf("Duration : %u seconds\n", (chunk.size/byteRate));

	return 0;
}
