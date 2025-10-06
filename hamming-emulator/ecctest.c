#include <stdio.h>
#include "ecclib.h"
#include <assert.h>

void flip_bit(ecc_t *ecc, unsigned int offset, int bit_pos){
    switch(bit_pos){
        // parity bits
        case 0: ecc->code_memory[offset] ^= PW_BIT; break;
        case 1: ecc->code_memory[offset] ^= P04_BIT; break;
        case 2: ecc->code_memory[offset] ^= P01_BIT; break;
        case 4: ecc->code_memory[offset] ^= P02_BIT; break;
        case 8: ecc->code_memory[offset] ^= P03_BIT; break;
        // data bits
        case 3: ecc->data_memory[offset] ^= DATA_BIT_1; break;
        case 5: ecc->data_memory[offset] ^= DATA_BIT_2; break;
        case 6: ecc->data_memory[offset] ^= DATA_BIT_3; break;
        case 7: ecc->data_memory[offset] ^= DATA_BIT_4; break;
        case 9: ecc->data_memory[offset] ^= DATA_BIT_5; break;
        case 10: ecc->data_memory[offset] ^= DATA_BIT_6; break;
        case 11: ecc->data_memory[offset] ^= DATA_BIT_7; break;
        case 12: ecc->data_memory[offset] ^= DATA_BIT_8; break;
        default:
            printf("Invalid bit position: %d\n", bit_pos);
            break;
    }
}  

int main(void){
    ecc_t ECC;
    unsigned char *base_addr=enable_ecc_memory(&ECC);
    unsigned char byteRead;
    int rc;

    printf("Starting test...\n");

    // Test for no errors
    //
    printf("--- Starting no error case ---\n");
    write_byte(&ECC, base_addr, 0xAB);
    rc = read_byte(&ECC, base_addr, &byteRead);
    assert(rc == NO_ERROR);
    assert(byteRead == (unsigned char)0xAB);
    printf("--- No error case passed ---\n\n");

    // Test for SBE 13 cases
    printf("--- Starting SBE test case ---\n");
    for (int i=0; i<=12; i++){
        // reset the memory
        write_byte(&ECC, base_addr, 0xAB);
        // flip the ith bit
        flip_bit(&ECC, 0, i);

        rc = read_byte(&ECC, base_addr, &byteRead);

        if (i == 0){
            assert(rc == PW_ERROR);
        } else {
           // assert(rc == i);
        }
        assert(byteRead == (unsigned char)0xAB);
        printf("--- SBE passed with %d \n---", i);
    }

    // Test for double bit error 13c2
    printf("--- Starting Double bit error case");
    int dbe_count = 0;
    for (int i=0; i<13; i++){
        for (int j=i+1; j<13; j++){
            printf("test case %d, %d\n", i, j);
            write_byte(&ECC, base_addr, 0xAB);
            // create DBE
            flip_bit(&ECC, 0, i);
            flip_bit(&ECC, 0, j);

            rc = read_byte(&ECC, base_addr, &byteRead);

            assert(rc == DOUBLE_BIT_ERROR);
            // track number of DBES
            dbe_count++;
            
        }
    }
    printf("--- Test passed with DBE count of %d\n", dbe_count);
    assert(dbe_count == 78);
    printf("Tests Passed");
    return 0;

}