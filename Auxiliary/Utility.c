//
// Created by simone mazzi on 20/08/25.
//
#include "Utility.h"
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <stdint.h>
#include <math.h>
#include <pthread.h>
#include <inttypes.h>
#include <signal.h>

//conversione da uint64 a stringa
char* uint64_to_str(uint64_t num) {
    char* str = malloc(21);
    if (!str) return NULL;
    // Rappresentazione esadecimale a 16 caratteri
    snprintf(str, 21, "%" PRIx64, num);
    return str;
}

// void print_hex(const unsigned char *buf, size_t len) {
//     for (size_t i = 0; i < len; i++) {
//         printf("%02X ", buf[i]);  // stampa in maiuscolo, due cifre
//     }
//     printf("\n");
// }

void disableSig() {
    struct sigaction action;
    memset(&action, 0, sizeof(struct sigaction));
    action.sa_handler = SIG_IGN;
    sigaction(SIGINT, &action, NULL);
    sigaction(SIGALRM, &action, NULL);
    sigaction(SIGUSR1, &action, NULL);
    sigaction(SIGUSR2, &action, NULL);
    sigaction(SIGTERM, &action, NULL);
}

void activeSignals() {
    // Ripristina il comportamento di default per i segnali elencati
    signal(SIGINT, SIG_DFL);
    signal(SIGALRM, SIG_DFL);
    signal(SIGUSR1, SIG_DFL);
    signal(SIGUSR2, SIG_DFL);
    signal(SIGTERM, SIG_DFL);
}

/*
 funzione per fare lo xor bit a bit
*/
char*  xor64(const unsigned char *input, uint64_t k) {
    uint64_t block = 0;
    memcpy(&block, input, 8);
    block ^= k;
    char* out = malloc(8);
    if (!out) return NULL;
    memcpy(out, &block, 8);
    return out;
}

void* TranslateWithKey(void* arg) {
    const ThreadInfo* info = arg;

    for (int i = info->begin;i <info->end;i++) {
        char *cript = xor64(info->blockA[i], info->k);
        info->outputA[i] = (unsigned char*)cript;
    }
    pthread_exit(NULL);

}

/*
 *funzione per il multithreading sui blocchi
 */
void checkArr(int n_blocks, int p , unsigned char **data,unsigned char **output, uint64_t k ) {

    pthread_t threads[p];
    ThreadInfo threadInfo[p];

    int base = n_blocks/p;
    int extra = n_blocks% p;
    int offset_A = 0;

    for (int i=0;i < p; i++) {

        int components = base + (i< extra? 1: 0);

        threadInfo[i].outputA = output;
        threadInfo[i].blockA = data;
        threadInfo[i].begin = offset_A;
        threadInfo[i].end = offset_A + components;
        threadInfo[i].k = k;
        threadInfo[i].thread_N = i+1;
        pthread_create(&threads[i],NULL,TranslateWithKey,&threadInfo[i]);
        offset_A += components;
    }

    for (int i=0;i<p;i++) {
        pthread_join(threads[i],NULL);
    }

}

/*
 *funzione per dividere in blocchi
 */
unsigned char** split_Block(char* input, size_t textlen) {
    int num_B = (textlen + BLOCK_S - 1) / BLOCK_S;

    unsigned char **blocchi = malloc(num_B* sizeof(unsigned char*));

    if (!blocchi) {
        printf("Errore nell'allocare memoria per i blocchi\n");
        free(input);
        return NULL;
    }

    // Creazione e riempimento di ciascun blocco
    for (int i = 0; i < num_B; ++i) {
        blocchi[i] = malloc(BLOCK_S+1);
        blocchi[i][0] = '\0';
        if (!blocchi[i]) {
            printf("Errore nell'allocare memoria per il blocco %d\n", i);
            // cleanup parziale
            for (int j = 0; j < i; ++j) free(blocchi[j]);
            free(blocchi);
            free(input);
            return NULL;
        }

        int start = i * BLOCK_S;
        int bytes_to_copy = (start + BLOCK_S <= textlen) ? BLOCK_S : (textlen - start);
        memcpy(blocchi[i], input + start, bytes_to_copy);
        blocchi[i][bytes_to_copy] = '\0';
        // padding con zeri se ultimo blocco incompleto
        if (bytes_to_copy < BLOCK_S) {
            memset(blocchi[i] + bytes_to_copy, 0, BLOCK_S - bytes_to_copy);
        }
        uint64_t val;
        memcpy(&val, input+start, bytes_to_copy);


    }

    // Liberazione del buffer temporaneo
    free(input);

    return blocchi;
}

/*
 *funzione per allocare lo spazio per l'output della cifratura
 */
unsigned char *createRes(int num_B, unsigned char **out_ptrs) {
    size_t block_size = 8;
    size_t total_size = num_B * block_size;

    unsigned char *result = malloc(total_size);
    if (!result) return NULL;

    for (int i = 0; i < num_B; i++) {
        if (!out_ptrs[i]) {
            free(result);
            return NULL;
        }
        memcpy(result + i * block_size, out_ptrs[i], block_size);
    }

    return result; // contiene tutti i byte, anche \0
}


/*
 *funzione di cifratura
 */
unsigned char* encryptMsg(FILE *file , uint64_t k ,int p,int *num_B) {
    // vai alla fine del file per sapere la lunghezza
    fseek(file, 0, SEEK_END);
    long len = ftell(file);
    rewind(file);

    // alloca buffer
    char *testo = malloc(len);
    if (!testo) return NULL;

    // leggi contenuto
    size_t read = fread(testo, 1, len, file);
    long bit_read = read * 8;
    testo[read] = '\0'; // terminatore di stringa

    *num_B = (read + BLOCK_S - 1) / BLOCK_S;
    // Allocazione array di puntatori per output per blocco
    unsigned char **out_ptrs = malloc(*num_B* sizeof(unsigned char*));
    for (int i = 0; i < *num_B; i++) {
        out_ptrs[i]=malloc(BLOCK_S);
        if (!out_ptrs[i]) {
            for (int j = 0; j < i; ++j) free(out_ptrs[j]);
            return NULL;
        }
    }
    if (!out_ptrs) {
        perror("errore allocazione");
        free(testo);
        return NULL;
    }

    unsigned char **encBLock = split_Block(testo, len);
    checkArr(*num_B, p, encBLock, out_ptrs, k);
    for (int i=0; i < *num_B; i++) {
        uint64_t val;
        memcpy(&val, out_ptrs[i], sizeof(uint64_t));

    }

    return createRes(*num_B,out_ptrs);

}

/*
 *funzione per decriptare
 */
char* decryptMsg(unsigned char *input, uint64_t k,int num_B,int p) {
    unsigned char **out_ptrs = malloc(num_B * sizeof(unsigned char*));
    if (!out_ptrs) {
        perror("errore allocazione");
        return NULL;
    }
    unsigned char **encBlocks= split_Block(input, num_B*BLOCK_S);
    if (!encBlocks) return NULL;
    char **decBlocks = malloc(num_B * sizeof(char*));
    if (!decBlocks) {
        return 0;
    }

    for (int i = 0; i < num_B; i++) {
        // Per ogni blocco decifrato alloca la memoria per farlo lungo abbastanza
        decBlocks[i] = malloc((BLOCK_S + 1) * sizeof(char));
        if (!decBlocks[i]) {
            return NULL;
        }

        memcpy(decBlocks[i], encBlocks[i], BLOCK_S);
        decBlocks[i][BLOCK_S] = '\0';
        uint64_t val;
        memcpy(&val, encBlocks[i], BLOCK_S);

    }
    checkArr(num_B, p, encBlocks, out_ptrs, k);

    return (char*) createRes(num_B,out_ptrs);

}

