#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#pragma pack(push,1)
//file header 14bytes.
typedef struct {
	uint16_t bfType;
	uint32_t bfSize;
	uint16_t bfReserved1;
	uint16_t bfReserved2;
	uint32_t bfOffBits;
} BMPFileHeader;


//info header.40 Bytes (width, height, bits per pixel, compression, image size).
typedef struct {
	uint32_t biSize;
	int32_t biWidth;
	int32_t biHeight;
	uint16_t biPlanes;
	uint16_t biBitCount;
	uint32_t biCompression;
	uint32_t biSizeImage;
	int32_t biXPelsPerMeter;
	int32_t biYPelsPerMeter;
	uint32_t biClrUsed;
	uint32_t biClrImportant;
} BMPInfoHeader;

#pragma pack(pop)

int main(void)
{
	printf("FileHeader : %zu\n", sizeof(BMPFileHeader));
	printf("InfoHeader : %zu\n", sizeof(BMPInfoHeader));

	//open bmp in read binary mode.
	FILE *fp = fopen("image.bmp", "rb");

	//read headers.
	BMPFileHeader filehdr;

	fread(&filehdr, sizeof(filehdr), 1, fp);

	//validate file headers.
	if(filehdr.bfType != 0x4D42)
	{
		printf("Not a BMP\n");
		return 1;
	}

	//read info headers.
	BMPInfoHeader infohdr;

	fread(&infohdr, sizeof(infohdr), 1, fp);
	printf("Width : %d\n", infohdr.biWidth);
	printf("Height : %d\n", infohdr.biHeight);
	printf("BPP : %u\n", infohdr.biBitCount);

	//jumping to pixel data.
	fseek(fp, filehdr.bfOffBits, SEEK_SET);

	//read the first pixel.
	uint8_t pixel[3];
	fread(pixel, 3, 1, fp);
	printf("B=%u G=%u R=%u\n", pixel[0], pixel[1], pixel[2]);

	// print all pixels.
	for(int y = 0; y < 4/*infohdr.biHeight*/; y++)
	{
		for(int x = 0; x < 4/*infohdr.biWidth*/; x++)
		{
			uint8_t pixel[3];
			fread(pixel, 3, 1, fp);
			printf("(%3u, %3u, %3u)", pixel[2], pixel[1], pixel[0]);
		}
		printf("\n");
	}

	return 0;
}
