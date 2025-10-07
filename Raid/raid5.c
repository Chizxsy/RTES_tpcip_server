#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>

#define buffer_size 1028

int store_image(char *input_file, size_t chunk_size, char *output_dir){
    int chunk_num = 0;

    // open source image
    FILE *fp_input = fopen(input_file, "rb");
    if (fp_input == NULL){
        syslog(LOG_ERR, "Failed to open input file");
	perror("fopen error"); 
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
        size_t bytes_to_read = (bytes_rem < (long)chunk_size) ? (size_t)bytes_rem : chunk_size;
        // read remaining bytes
        size_t bytes_read = fread(buffer, 1, bytes_to_read, fp_input);

        if (bytes_read != bytes_to_read) {
            // More detailed logging
            syslog(LOG_ERR, "File read error: Expected %zu bytes, but only got %zu.", bytes_to_read, bytes_read);
            
            // Check if a specific I/O error occurred
            if (ferror(fp_input)) {
                syslog(LOG_ERR, "Stream error reported by ferror().");
                perror("fread error reason");
            }
            
            // Check if we hit the end-of-file prematurely
            if (feof(fp_input)) {
                syslog(LOG_ERR, "End-of-file was reached unexpectedly.");
            }

            free(buffer);
            fclose(fp_input);
            return -1;
        }
	
	char output_filename[buffer_size];
	snprintf(output_filename, sizeof(output_filename), "%s/chunk_%d.bin", output_dir, chunk_num);
        
	FILE *fp_output = fopen(output_filename, "wb");
        if (fp_output == NULL){
            syslog(LOG_ERR, "Failed to open output file");
            free(buffer);
            fclose(fp_input);
            return -1;
        }

        size_t bytes_written = fwrite(buffer, 1, bytes_rem, fp_output);
	fclose(fp_output);

        if (bytes_written != bytes_rem){
            syslog(LOG_ERR, "Failed to write to output file");
            free(buffer);
            fclose(fp_input);
            return -1;
        }

        bytes_rem -= bytes_read;
        chunk_num++;
    }

    // clean up
    syslog(LOG_INFO, "File splitting complete. Total chunks created: %d\n", chunk_num);
    free(buffer);
    fclose(fp_input);
    return 0;
}


int main(int argc, char *argv[]){
    openlog("Raid5", LOG_PID, LOG_USER);

    if (argc != 4){
        syslog(LOG_ERR, "Incorrect number of arguments");
        return -1;
    }

    char *input_file = argv[1];
    size_t chunk_size = strtoul(argv[2], NULL, 10);
    char *output_dir = argv[3];

    int split_file = store_image(input_file, chunk_size, output_dir);
    if (split_file > 0){
        syslog(LOG_ERR, "Failed to store image");
	return -1;
    }


}
