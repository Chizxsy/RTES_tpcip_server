#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <syslog.h>
#include <stdbool.h>

#define BUFFER_SIZE 2048

int rebuild_image(char *output_filename, char *input_dir, int chunk_num, size_t chunk_size){

bool file_exists[chunk_num + 1]; // +1 for the parity file
    int missing_file_count = 0;
    int missing_file_index = -1; // -1 means nothing missing, chunk_num means parity is missing

    // --- Step 1: Check which files exist ---
    for (int i = 0; i < chunk_num; i++) {
        char filename[BUFFER_SIZE];
        snprintf(filename, sizeof(filename), "%s/chunk_%d.bin", input_dir, i);
        FILE *f = fopen(filename, "rb");
        if (f) {
            file_exists[i] = true;
            fclose(f);
        } else {
            file_exists[i] = false;
            missing_file_count++;
            missing_file_index = i;
        }
    }

    char parity_filename[BUFFER_SIZE];
    snprintf(parity_filename, sizeof(parity_filename), "%s/parity.bin", input_dir);
    FILE *f = fopen(parity_filename, "rb");
    if (f) {
        file_exists[chunk_num] = true;
        fclose(f);
    } else {
        file_exists[chunk_num] = false;
        missing_file_count++;
        missing_file_index = chunk_num;
    }

    // check for missing files
    if (missing_file_count > 1) {
        syslog(LOG_ERR, "Unrecoverable error: %d files are missing. Cannot rebuild.", missing_file_count);
        return -1;
    }
    if (missing_file_count == 0) {
        syslog(LOG_INFO, "No files missing. Performing simple reassembly.");
    } else {
        syslog(LOG_INFO, "Detected missing file index %d. Attempting to rebuild.", missing_file_index);
    }

    FILE *fp_rebuilt = fopen(output_filename, "wb");
    if (!fp_rebuilt) {
        syslog(LOG_ERR, "Could not create final output file: %s", output_filename);
        return -1;
    }

    char *temp_buffer = malloc(chunk_size);
    char *rebuilt_buffer = calloc(chunk_size, 1);


    // rebuild from parity
    for (int i = 0; i < chunk_num; i++) {
        if (i == missing_file_index) {
            syslog(LOG_INFO, "Reconstructing missing chunk %d...", i);
            // XOR all *other* existing data chunks
            for (int j = 0; j < chunk_num; j++) {
                if (i == j) continue; // Skip the missing chunk itself
                if (!file_exists[j]) { syslog(LOG_ERR, "Logic error: trying to read non-existent chunk."); return -1; }
                
                char filename[BUFFER_SIZE];
                snprintf(filename, sizeof(filename), "%s/chunk_%d.bin", input_dir, j);
                FILE *fp_chunk = fopen(filename, "rb");
                size_t bytes_read = fread(temp_buffer, 1, chunk_size, fp_chunk);
                fclose(fp_chunk);
                for (size_t k = 0; k < bytes_read; k++) rebuilt_buffer[k] ^= temp_buffer[k];
            }
            // Finally, XOR the parity chunk
            if (!file_exists[chunk_num]) { syslog(LOG_ERR, "Cannot rebuild data chunk: parity file is also missing."); return -1; }
            FILE *fp_parity = fopen(parity_filename, "rb");
            size_t bytes_read = fread(temp_buffer, 1, chunk_size, fp_parity);
            fclose(fp_parity);
            for (size_t k = 0; k < bytes_read; k++) rebuilt_buffer[k] ^= temp_buffer[k];

            // Write the rebuilt chunk to the final file
            fwrite(rebuilt_buffer, 1, bytes_read, fp_rebuilt);

        } else {
            // --- Write existing data chunk ---
            char filename[BUFFER_SIZE];
            snprintf(filename, sizeof(filename), "%s/chunk_%d.bin", input_dir, i);
            FILE *fp_chunk = fopen(filename, "rb");
            size_t bytes_read = fread(temp_buffer, 1, chunk_size, fp_chunk);
            fclose(fp_chunk);
            fwrite(temp_buffer, 1, bytes_read, fp_rebuilt);
        }
    }

    fclose(fp_rebuilt);
    free(temp_buffer);
    free(rebuilt_buffer);
    return 0;
}

int store_image(char *input_file, size_t chunk_size, char *output_dir){
    int chunk_num = 0;

    // open source image
    FILE *fp_input = fopen(input_file, "rb");
    if (fp_input == NULL){
        syslog(LOG_ERR, "Failed to open input file");
	    //perror("fopen error"); 
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
	
	char output_filename[BUFFER_SIZE];
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


    char *parity_buffer = calloc(chunk_size, 1);
    char *chunk_buffer = malloc(chunk_size);
    if (!parity_buffer || !chunk_buffer) {
        syslog(LOG_ERR, "Failed to allocate memory for parity generation");
        return -1;
    }

    for (int i = 0; i < chunk_num; i++) {
        char in_filename[BUFFER_SIZE];
        snprintf(in_filename, sizeof(in_filename), "%s/chunk_%d.bin", output_dir, i);
        FILE *fp_chunk = fopen(in_filename, "rb");
        if (!fp_chunk) {
            syslog(LOG_ERR, "Failed to read chunk for parity calc: %s", in_filename);
            free(parity_buffer);
            free(chunk_buffer);
            return -1;
        }
        size_t bytes_read = fread(chunk_buffer, 1, chunk_size, fp_chunk);
        fclose(fp_chunk);

        for (size_t j = 0; j < bytes_read; j++) {
            parity_buffer[j] ^= chunk_buffer[j];
        }
    }
    free(chunk_buffer);

    char parity_filename[BUFFER_SIZE];
    snprintf(parity_filename, sizeof(parity_filename), "%s/parity.bin", output_dir);
    FILE *fp_parity = fopen(parity_filename, "wb");
    if (!fp_parity) {
        syslog(LOG_ERR, "Failed to create parity file.");
        free(parity_buffer);
        return -1;
    }
    fwrite(parity_buffer, 1, chunk_size, fp_parity);
    fclose(fp_parity);
    free(parity_buffer);
    syslog(LOG_INFO, "Parity chunk created successfully.");

    return chunk_num;

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

    syslog(LOG_INFO, "Pausing. Please rm one ramdisk then press enter");
    getchar();
    syslog(LOG_INFO, "Rebuilding...");
    rebuild_image("rebuilt_image.ppm", output_dir, split_file, chunk_size);
    syslog(LOG_INFO, "Rebuild complete");


}
