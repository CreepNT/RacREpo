/**
 * mass_pif_convert.c
 * 
 * Looks for "2FIP" magic in files, and tries converting the potential images to PNG.
 * Basically, an evolved version of pifconverter.py that can do batch convertion.
 *
 * Released under Public Domain.
 */


#include <stdio.h>
#include <stdint.h>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "../_external/stb_image_write.h"

#define EXEC_NAME argv[0]
#define INPUT_FILE_NAME argv[1]

#define WARNING_THRESHOLD 1024 //Warn before converting any PIF with dimension > threshold

struct _PIF2_palette_entry{
	uint8_t red;
	uint8_t green;
	uint8_t blue;
	uint8_t alpha; //Max is 0x80.	
};

typedef struct _PIF2_palette_entry PIF2_palette_entry;

struct _PIF2_header{
	//char magic[4]; //We read past the magic, so it doesn't matter.
	uint32_t unk;
	uint32_t width;
	uint32_t height;
	uint32_t unk2;
	uint32_t unk3;
	uint32_t unk4; //Maybe Texture type
	uint32_t unk5;
	PIF2_palette_entry palette[256];
};
typedef struct _PIF2_header PIF2_header;

//Taken from https://github.com/chaoticgd/wrench/blob/master/docs/formats/2fip_textures.md
uint8_t decode_palette_index(uint8_t index) {
	// Swap middle two bits
	//  e.g. 00010000 becomes 00001000.
	int a = index & 8;
	int b = index & 16;
	if(a && !b) {
		index += 8;
	} else if(!a && b) {
		index -= 8;
	}
	return index;
}


int main(int argc, char** argv){
    if (argc != 2){
        printf("%s : Invalid arguments count.\n", EXEC_NAME);
        printf("Syntax :\n%s <file containing PIFs>", EXEC_NAME);
        exit(0);
    }
    FILE* inFH = fopen(INPUT_FILE_NAME, "rb");
    if (inFH == NULL){
        printf("Error opening %s.\n", INPUT_FILE_NAME);
        exit(0);
    }
    long fileSize = 0;
    fseek(inFH, 0, SEEK_END);
    fileSize = ftell(inFH);
    fseek(inFH, 0, SEEK_SET);

    char magic[4];
    int PIF_count = 0;
    for (long offset = 0; offset < fileSize; offset = ftell(inFH)){
        int ret = fread(&magic, 4, 1, inFH);
        if (ret == 0){
            fprintf(stderr, "fread() returned 0, reached end of file ?\n");
            break;
        }

        if (strncmp(magic, "2FIP", 4) == 0 /* 2FIP */){
            printf("Found 2FIP magic @ offset 0x%lX\n", offset);
            PIF2_header header = {0};
            fread(&header, sizeof(header), 1, inFH);
            printf("Width : %u | Height : %u\n", header.width, header.height);
            if (header.width > WARNING_THRESHOLD || header.height > WARNING_THRESHOLD){
                fprintf(stderr, "WARNING : dimension(s) higher than threshold (%d).\nExtract anyway ? [y/N] ", WARNING_THRESHOLD);
                int ch = getchar();
                if (ch != 'y' && ch != 'Y') 
                    continue; //Skip this
            }

            uint8_t* imgbuf = (uint8_t*)malloc(header.width * header.height * 4 /* 4 channels, 1 byte per channel*/);
            if (imgbuf == NULL){
                fprintf(stderr, "Couldn't allocate memory for image buffer.\nSkipping...\n");
                continue;
            }
            for (int i = 0; (i < header.width * header.height) ; i++){
                uint8_t ch, index;
                ret = fread(&ch, 1, 1, inFH);
                if (ret == 0){
                    fprintf(stderr, "fread() returned 0, reached end of file ?\n");
                    break;
                }
                index = decode_palette_index(ch);
                imgbuf[4 * i] = header.palette[index].red;
                imgbuf[4 * i + 1] = header.palette[index].green;
                imgbuf[4 * i + 2] = header.palette[index].blue;
                imgbuf[4 * i + 3] = (header.palette[index].alpha == 0x80) ? 0xFF : (header.palette[index].alpha * 2);
            }
            if (ret == 0) continue; //Catch the break
            char outName[256];
            snprintf(outName, sizeof(outName), "%s-0x%lX-%u.png", INPUT_FILE_NAME, offset, PIF_count);
            ret = stbi_write_png(outName, header.width, header.height, 4, imgbuf, header.width * 4);
            if (ret == 0){
                fprintf(stderr, "Error when writing output PNG.\n");
            }
            else {
                fprintf(stderr, "File exported as %s !\n", outName);
                PIF_count++;
            }
        }
    }
    fprintf(stderr, "Finished !\nTotal number of files exported : %u\n", PIF_count);
    fclose(inFH);
    return 0;
}