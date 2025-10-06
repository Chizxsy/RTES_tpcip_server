#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define buffer_size 1028

int store_image(char *input_file, size_t chunk_size, char *output_file){
    int chunk_num = 0;

    // open source image
    FILE *fp_input = open(input_file, "rb");
    if (fp_input == NULL){
        syslog(LOG_ERR, "Failed to open input file");
        return 1;
    }

    // determine file size
    fseek(fp_input, 0, SEEK_END);
    long file_size = ftell(fp_input);
    fseek(fp_input, 0, SEEK_SET);

    // print file and chunk size
    syslog(LOG_INFO, "File size: %ld bytes\n", file_size);
    syslog(LOG_INFO, "Chunk size: %zu bytes\n", chunk_size);

    // allocate memory to hold 1 chunk of image data
    char *buffer = malloc(chunk_size);
    if (buffer == NULL){
        syslog(LOG_INFO, "Failed to allocate memory");
        fclose(fp);
    }

    // loop through all bytes and separate into chunks
    while (file_size > 0){
        char output_filename[buffer_size];
        snprintf(output_filename, sizeof(output_filename), "chunk_%s.bin", output_file, chunk_num);

        FILE *fp_output = open(output_file, "wb");
        if (fp_output == NULL){
            syslog(LOG_ERR, "Failed to open output file");
            return 1;
        }




        chunk_num++;
    }

    // clean up
    syslog(LOG_INFO, "File splitting complete. Total chunks created: %d\n", chunk_number);
    free(buffer);
    fclose(f_in);
}


int main(int argc, char argv[]){
    openlog("Raid5", LOG_PID, LOG_USER);
}