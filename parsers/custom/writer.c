#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

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
    FILE *fp = fopen("sample.bin", "wb");

    if (!fp) {
        perror("fopen");
        return 1;
    }

    Header hdr;

    hdr.magic = MAGIC;
    hdr.version = 1;

    printf("Number of records: ");
    scanf("%hu", &hdr.count);
    getchar();

    fwrite(&hdr, sizeof(hdr), 1, fp);

    for (uint16_t i = 0; i < hdr.count; i++) {

        RecordHeader rec;

        char buffer[1024];

        printf("\nRecord %u\n", i + 1);

        printf("ID: ");
        scanf("%u", &rec.id);
        getchar();

        printf("Data: ");
        fgets(buffer, sizeof(buffer), stdin);

        buffer[strcspn(buffer, "\n")] = '\0';

        rec.len = strlen(buffer);

        fwrite(&rec, sizeof(rec), 1, fp);
        fwrite(buffer, rec.len, 1, fp);
    }

    fclose(fp);

    printf("\nFile written successfully.\n");

    return 0;
}
