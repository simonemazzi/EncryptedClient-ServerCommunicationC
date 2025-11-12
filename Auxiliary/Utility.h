//
// Created by simone mazzi on 20/08/25.
//

#ifndef UNTITLED4_UTILITY_H
#define UNTITLED4_UTILITY_H

#define BLOCK_S 8

typedef struct  {
    unsigned char **outputA;
    unsigned char **blockA;
    int begin;
    int end;
    int thread_N;
    uint64_t k;
}ThreadInfo;

typedef struct {
    unsigned char *text_to_send;
    uint64_t Key;
    int Lenght;
} Message;

char* uint64_to_str(uint64_t num);
void disableSig();
void activeSignals();
char*  xor64(const unsigned char *input, uint64_t k);
void* TranslateWithKey(void* arg);
void checkArr(int n_blocks, int p , unsigned char **data,unsigned char **output, uint64_t k );
unsigned char** split_Block(char* input,size_t textlen);
unsigned char* encryptMsg(FILE *file , uint64_t k ,int p,int *num_B);
char* decryptMsg(unsigned char *input, uint64_t k,int num_B,int p) ;
unsigned char* createRes(int num_B,unsigned char** output);
void print_hex(const unsigned char *buf, size_t len);
#endif //UNTITLED4_UTILITY_H