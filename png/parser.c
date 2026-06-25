#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

uint32_t read_be32(FILE *fp);

const unsigned char png_signature[8] = {
	0x89,
	'P',
	'N',
	'G',
	'\r',
	'\n',
	0x1A,
	'\n'
};

#pragma pack(push,1)

typedef struct {
	uint32_t len;
	char type[4];
} ChunkHeader;

#pragma pack(pop)

int main(void)
{
	//file read.
	FILE *fp = fopen("image.png", "rb");

	//read the img file.
	unsigned char sig[8];
	fread(sig, 1, 8, fp);

	//validate the signature.
	if(memcmp(sig, png_signature, 8) != 0)
	{
		printf("Invalid Signature\n");
		return 1;
	}

	//chunk reading.
	ChunkHeader chunk;

	while(1)
	{
		chunk.len = read_be32(fp);//convert the chunk length.
		fread(chunk.type, 1, 4, fp);//read the chunk type.
		printf("%.4s : %u\n", chunk.type, chunk.len);


		if(memcmp(chunk.type, "IEND", 4) == 0)
			break;
		else if(memcmp(chunk.type, "IHDR", 4) == 0)
		{
			printf("\n___IHDR_Chunk___\n");
			//read chunk data.
			uint8_t *data = malloc(chunk.len);
			if(data == NULL)
			{
				perror("malloc");
				fclose(fp);
				return 1;
			}
			if(fread(data, 1, chunk.len, fp) != chunk.len)
			{
				printf("Failed to read IHDR data part\n");
				free(data);
				fclose(fp);
				return 1;
			}

			//print chunk data.
			for(uint32_t i = 0 ; i < chunk.len ; i++)
			{
				printf("%02X", data[i]);
				if((i+1) % 16 == 0)
					printf("\n");
			}
			printf("\n");

			//read crc.
			uint32_t crc = read_be32(fp);
			printf("CRC : 0x%08X\n", crc);
			free(data);
		}
		else if(memcmp(chunk.type, "IDAT", 4) == 0)
		{
			printf("___IDAT_Chunk___\n");
			//read chunk data.
			uint8_t *data = malloc(chunk.len);
			if(data == NULL)
			{
				perror("malloc");
				fclose(fp);
				return 1;
			}
			if(fread(data, 1, chunk.len, fp) != chunk.len)
			{
				printf("Failed to read IDAT data part\n");
				free(data);
				fclose(fp);
				return 1;
			}

			//print chunk data.
			for(uint32_t i = 0 ; i < chunk.len ; i++)
			{
				printf("%02X", data[i]);
				if((i + 1) % 16 == 0)
					printf("\n");
			}
			printf("\n");

			//crc.
			uint32_t crc = read_be32(fp);
			printf("CRC : 0x%08X\n", crc);
			free(data);
		}
		else
		{
			fseek(fp, chunk.len, SEEK_CUR);
			read_be32(fp);
		}

	}

	fclose(fp);
	return 0;
}

uint32_t read_be32(FILE *fp)
{
	uint8_t buffer[4];
	size_t bytesRead = fread(buffer, sizeof(uint8_t), 4, fp);

	if(bytesRead == 4)
		printf("Read 4 bytes\n");
	else
		printf("Err or EOF %zu\n", bytesRead);

	//shift first, second, third bytes.
	uint32_t firstByte = buffer[0] << 24;
	uint32_t secByte = buffer[1] << 16;
	uint32_t thirdByte = buffer[2] << 8;

	uint32_t result = firstByte | secByte | thirdByte | buffer[3];

	//printf("result : %u", result);
	return result;
}
