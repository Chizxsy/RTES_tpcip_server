#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#define buffer_size 1028

int store_image(char *input_file, size_t chunk_size, char *output_file){
    int chunk_num = 0;

    // open source image
    FILE *fp_input = fopen(input_file, "rb");
    if (fp_input == NULL){
        syslog(LOG_ERR, "Failed to open input file");
        return -1;
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
        fclose(fp_input);
    }

    // loop through all bytes and separate into chunks
    long bytes_rem = file_size;
    while (bytes_rem > 0){
        // check number of remainging bytes
        size_t num_bytes = (bytes_rem < (long)chunk_size) ? (size_t)bytes_rem : chunk_size;
        // read remaining bytes
        size_t bytes_rem = fread(buffer, 1, num_bytes, fp_input);

        if (num_bytes != bytes_rem){
            syslog(LOG_ERR, "Failed to read byte");
            fclose(fp_input);
            free(buffer);
        }

        char output_filename[buffer_size];
        snprintf(output_filename, sizeof(output_filename), "chunk_%s.bin", output_file, chunk_num);

        FILE *fp_output = fopen(output_file, "wb");
        if (fp_output == NULL){
            syslog(LOG_ERR, "Failed to open output file");
            free(buffer);
            fclose(fp_input);
            return -1;
        }

        size_t bytes_written = fwrite(buffer, 1, bytes_rem, fp_output);

        if (bytes_written != bytes_rem){
            syslog(LOG_ERR, "Failed to write to output file");
            free(buffer);
            fclose(fp_output);
            fclose(fp_input);
            return -1;
        }

        fclose(fp_output);  
        num_bytes -= bytes_rem;
        chunk_num++;
    }

    // clean up
    syslog(LOG_INFO, "File splitting complete. Total chunks created: %d\n", chunk_num);
    free(buffer);
    fclose(fp_input);
}


int main(int argc, char *argv[]){
    openlog("Raid5", LOG_PID, LOG_USER);

    if (argc != 4){
        syslog(LOG_ERR, "Incorrect number of arguments");
        return 1;
    }

    char *input_file = &argv[1];
    size_t chunk_size = (size_t)argv[2];
    char *output_dir = &argv[3];

    int split_file = store_image(input_file, chunk_size, output_dir);
    if (split_file > 0){
        syslog(LOG_ERR, "Failed to store image");
    }


}
