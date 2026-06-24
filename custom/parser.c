#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>

#define MAGIC 0xDEADBEEF

#pragma pack(push, 1)

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t count;
} Header;

typedef struct {
    uint32_t id;
    uint16_t len;
} RecordHeader;

#pragma pack(pop)

int main(void)
{
    FILE *fp = fopen("sample.bin", "rb");

    if (!fp) {
        perror("fopen");
        return 1;
    }

    Header hdr;

    if (fread(&hdr, sizeof(hdr), 1, fp) != 1) {
        printf("Failed to read header\n");
        fclose(fp);
        return 1;
    }

    if (hdr.magic != MAGIC) {
        printf("Invalid magic number\n");
        fclose(fp);
        return 1;
    }

    printf("=== HEADER ===\n");
    printf("Magic   : 0x%X\n", hdr.magic);
    printf("Version : %u\n", hdr.version);
    printf("Records : %u\n\n", hdr.count);

    for (uint16_t i = 0; i < hdr.count; i++) {

        RecordHeader rec;

        if (fread(&rec, sizeof(rec), 1, fp) != 1) {
            printf("Failed to read record header\n");
            break;
        }

        if (rec.len > 4096) {
            printf("Suspicious record length\n");
            break;
        }

        char *buffer = malloc(rec.len + 1);

        if (!buffer) {
            perror("malloc");
            fclose(fp);
            return 1;
        }

        if (fread(buffer, rec.len, 1, fp) != 1) {
            printf("Failed to read record data\n");
            free(buffer);
            break;
        }

        buffer[rec.len] = '\0';

        printf("Record %u\n", i + 1);
        printf("ID   : %u\n", rec.id);
        printf("LEN  : %u\n", rec.len);
        printf("DATA : %s\n\n", buffer);

        free(buffer);
    }

    fclose(fp);

    return 0;
}
