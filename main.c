#include <inttypes.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#define TEST 1

int main(int argc, char** argv)
{
    uint8_t SUBKEY[192];

    // Key whitening
    uint64_t whiten(uint64_t text, uint64_t key){
        uint64_t whitened = 0;
        for(int i = 3; i >= 0; i--){
            whitened |= (text >> 16 * i & 0xFFFF) ^ (key >> 16 * i & 0xFFFF);
            if(i != 0){
                whitened <<= 16;
            }
        }
        return whitened;
    }
     
    // Function to generate all keys up front
    int keygen(int64_t key){
        for(int i = 0; i < 16; i++){
            for(int j = 0; j < 12; j++){
                key = (key < 0) ? (key << 1) + 1 : key << 1;
                if(i % 2 == 0){
                    SUBKEY[12 * i + j] = key >> 8 * (j % 4) & 0xFF;
                }
                else{
                    SUBKEY[12 * i + j] = key >> 8 * (j % 4 + 4) & 0xFF;
                }
            }
        }
        return 0;
    }

    // The F-table
    uint8_t ftable(uint8_t byte){
        uint8_t table[256] = {
        0xa3,0xd7,0x09,0x83,0xf8,0x48,0xf6,0xf4,0xb3,0x21,0x15,0x78,0x99,0xb1,0xaf,0xf9,
        0xe7,0x2d,0x4d,0x8a,0xce,0x4c,0xca,0x2e,0x52,0x95,0xd9,0x1e,0x4e,0x38,0x44,0x28,
        0x0a,0xdf,0x02,0xa0,0x17,0xf1,0x60,0x68,0x12,0xb7,0x7a,0xc3,0xe9,0xfa,0x3d,0x53,
        0x96,0x84,0x6b,0xba,0xf2,0x63,0x9a,0x19,0x7c,0xae,0xe5,0xf5,0xf7,0x16,0x6a,0xa2,
        0x39,0xb6,0x7b,0x0f,0xc1,0x93,0x81,0x1b,0xee,0xb4,0x1a,0xea,0xd0,0x91,0x2f,0xb8,
        0x55,0xb9,0xda,0x85,0x3f,0x41,0xbf,0xe0,0x5a,0x58,0x80,0x5f,0x66,0x0b,0xd8,0x90,
        0x35,0xd5,0xc0,0xa7,0x33,0x06,0x65,0x69,0x45,0x00,0x94,0x56,0x6d,0x98,0x9b,0x76,
        0x97,0xfc,0xb2,0xc2,0xb0,0xfe,0xdb,0x20,0xe1,0xeb,0xd6,0xe4,0xdd,0x47,0x4a,0x1d,
        0x42,0xed,0x9e,0x6e,0x49,0x3c,0xcd,0x43,0x27,0xd2,0x07,0xd4,0xde,0xc7,0x67,0x18,
        0x89,0xcb,0x30,0x1f,0x8d,0xc6,0x8f,0xaa,0xc8,0x74,0xdc,0xc9,0x5d,0x5c,0x31,0xa4,
        0x70,0x88,0x61,0x2c,0x9f,0x0d,0x2b,0x87,0x50,0x82,0x54,0x64,0x26,0x7d,0x03,0x40,
        0x34,0x4b,0x1c,0x73,0xd1,0xc4,0xfd,0x3b,0xcc,0xfb,0x7f,0xab,0xe6,0x3e,0x5b,0xa5,
        0xad,0x04,0x23,0x9c,0x14,0x51,0x22,0xf0,0x29,0x79,0x71,0x7e,0xff,0x8c,0x0e,0xe2,
        0x0c,0xef,0xbc,0x72,0x75,0x6f,0x37,0xa1,0xec,0xd3,0x8e,0x62,0x8b,0x86,0x10,0xe8,
        0x08,0x77,0x11,0xbe,0x92,0x4f,0x24,0xc5,0x32,0x36,0x9d,0xcf,0xf3,0xa6,0xbb,0xac,
        0x5e,0x6c,0xa9,0x13,0x57,0x25,0xb5,0xe3,0xbd,0xa8,0x3a,0x01,0x05,0x59,0x2a,0x46
        };

        unsigned int high = byte >> 4 & 0xF;
        unsigned int low = byte & 0xF;
        return table[16 * high + low];
    }

    // G-permutation
    uint16_t g(uint16_t w, uint8_t k0, uint8_t k1, uint8_t k2, uint8_t k3){
        uint8_t g1 = w >> 8 & 0xFF;
        uint8_t g2 = w & 0xFF;
        uint8_t g3 = ftable(g2 ^ k0) ^ g1;
        uint8_t g4 = ftable(g3 ^ k1) ^ g2;
        uint8_t g5 = ftable(g4 ^ k2) ^ g3;
        uint8_t g6 = ftable(g5 ^ k3) ^ g4;
#if TEST == 1
        printf("g1: 0x%x g2: 0x%x g3: 0x%x g4: 0x%x g5: 0x%x g6: 0x%x\n", g1, g2, g3, g4, g5, g6);
#endif
        return g5 << 8 | g6;
    }

    // Main F function
    unsigned int f(uint16_t r0, uint16_t r1, int round){
        uint8_t key[12];
        for(int i = 0; i < 12; i++){
            key[i] = SUBKEY[12 * round + i];
        }

#if TEST == 1
        printf("Keys: ");
        for(int i = 0; i < 12; i++){
            printf("0x%x ", key[i]);
        }
        printf("\n");
#endif
        uint16_t t0 = g(r0, key[0], key[1], key[2], key[3]);
        uint16_t t1 = g(r1, key[4], key[5], key[6], key[7]);
        uint16_t f0 = (t0 + 2 * t1 + (key[8] << 8 | key[9])) % (1 << 16);
        uint16_t f1 = (2 * t0 + t1 + (key[10] << 8 | key[11])) % (1 << 16);
#if TEST == 1
        printf("t0: 0x%x t1: 0x%x\n", t0, t1);
        printf("f0: 0x%x f1: 0x%x\n", f0, f1);
#endif
        return f0 << 16 | f1;
    }

    // One round of encryption round -1 means only swap
    uint64_t crypt_step(uint64_t text, int round, int decrypt){
        uint64_t r[4] = {text >> 48 & 0xFFFF, text >> 32 & 0xFFFF, 
                         text >> 16 & 0xFFFF, text & 0xFFFF};

        if(decrypt){ // run rounds in reverse to decrypt
            round = 15 - round;
        }

        if(round >= 0){ // skip if final round
            uint32_t f01 = f(r[0], r[1], round);
            r[2] = (f01 >> 16 & 0xFFFF) ^ r[2];
            r[3] = (f01 & 0xFFFF) ^ r[3];
        }

        return r[2] << 48 | (r[3] << 32 | (r[0] << 16 | (r[1])));
    }
    
    // Will encrypt/decrypt a 64bit block of data given a 64bit key
    uint64_t xcrypt(uint64_t plaintext, uint64_t key, int decrypt){
        keygen(key);
        uint64_t current_text = whiten(plaintext, key); // whiten input
        for(int r = 0; r < 16; r++){
            current_text = crypt_step(current_text, r, decrypt); // main en/de-cryption
#if TEST == 1
            printf("Block: 0x%lx\n", current_text);
            printf("End of Round: %d\n\n", r);
#endif
        }
        current_text = crypt_step(current_text, -1, 0); // final swap
        return whiten(current_text, key); // return whitened output
    }

    char * stringify(uint64_t text, char * buffer){
        for(int i = 0; i < 8; i++){
            buffer[i] = text >> (56 - i*8) & 0xFF;
        }
        buffer[8] = '\0';
        return buffer;
    }

    void encrypt(){
        FILE * ptfile = fopen("plaintext.txt", "r");
        FILE * keyfile = fopen("key.txt", "r");
        FILE * cipherfile = fopen("ciphertext.txt", "w");
        int64_t key = 0;
        int fileout = 0;
        int block_count = 0;

        if (ptfile==NULL) perror ("Error opening plaintext.txt");
        if (keyfile==NULL) perror ("Error opening key.txt");
        if (cipherfile==NULL) perror ("Error creating file ciphertext.txt");

        fscanf(keyfile, "%lx", &key);
        fclose(keyfile);

        while(fileout >= 0){
            uint64_t text_block = 0;

            for(int i = 0; i < 8; i++){
                if(fileout >= 0) fileout = fgetc(ptfile);
                if(fileout >= 0) text_block |= fileout;
                if(i < 7) text_block <<= 8;
            }

            printf("Text Block %d: 0x%lx\n\n", block_count, text_block);
            uint64_t cipher_block = xcrypt(text_block, key, 0);
            printf("Cipher Block %d: 0x%016lx\n", block_count, cipher_block);
            printf("Writing to ciphertext.txt\n\n\n");
            fprintf(cipherfile, "%016lx", cipher_block);

            block_count++;
        }
        fclose(ptfile);
        fclose(cipherfile);
    }

    void decrypt(){
        FILE * ptfile = fopen("plaintext.txt", "w");
        FILE * keyfile = fopen("key.txt", "r");
        FILE * cipherfile = fopen("ciphertext.txt", "r");
        int64_t key = 0;
        uint64_t fileout = 0;
        int block_count = 0;
        char buffer[17];

        if (ptfile==NULL) perror ("Error opening plaintext.txt");
        if (keyfile==NULL) perror ("Error opening key.txt");
        if (cipherfile==NULL) perror ("Error creating file ciphertext.txt");

        fscanf(keyfile, "%lx", &key);
        fclose(keyfile);

        while(feof(cipherfile) == 0){
            uint64_t text_block = 0;

            for(int i = 0; i < 16; i++){
                buffer[i] = fgetc(cipherfile);
            }
            buffer[16] = '\0';

            if(feof(cipherfile) == 0){
                uint64_t cipher_block = strtoul(buffer, NULL, 16);
                printf("Cipher Block %d: 0x%lx\n\n", block_count, cipher_block);
                uint64_t plain_block = xcrypt(cipher_block, key, 1);
                char string_block[9];
                stringify(plain_block, string_block);
                printf("Plaintext Block %d: %s\n", block_count, string_block);
                printf("Writing to plaintext.txt\n\n\n");
                fprintf(ptfile, "%s", string_block);

                block_count++;
            }
        }
        fclose(ptfile);
        fclose(cipherfile);
    }

    if(argc == 1) // encrypt by default with no arguments
        encrypt();
    else if(strcmp(argv[1], "decrypt") == 0 || strcmp(argv[1], "d") == 0)
        decrypt();
    else if(strcmp(argv[1], "encrypt") == 0 || strcmp(argv[1], "e") == 0)
        encrypt(); 
    else
        printf("Invalid argument(s)\n");
   

    return 0;
}
