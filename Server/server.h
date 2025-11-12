//
// Created by Narcis Lorenz Grecu on 19/08/25.
//
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <errno.h>
#include <pthread.h>
#include "../Auxiliary/Utility.c"
#include <getopt.h>


#ifndef SERVER_H
#define SERVER_H
#define  BUFFER_SIZE 1024

int n_clients=0;
int active_clients = 0;         // numero di client attivi
pthread_mutex_t lock;

pthread_mutex_t mutex = PTHREAD_MUTEX_INITIALIZER;  // per proteggere sezione critica
pthread_cond_t cond = PTHREAD_COND_INITIALIZER;    // per coordinare i turni
int turno = 0; // mutex per proteggere il contatore
pthread_mutex_t n_clients_mutex = PTHREAD_MUTEX_INITIALIZER;

typedef struct {
    int p; //parallelismo
    int l; // n_conn
    int porta;
    char *s; //prefisso
} SetupServer;

// Struttura per passare le informazioni a ciascun thread
typedef struct {
    const char *nome_file;  // file di destinazione
    const char *testo;      // stringa completa
    int start;              // indice di inizio del pezzo da scrivere
    int end;                // indice di fine
    int id;                 // ID thread
} ThreadArgs;


typedef struct {
    SetupServer *ss;
    int *client_socket;
}Management;

void *handle_client(void *arg);
int ServerSetup(SetupServer *ss,char **argv,int argc);
void *handle_server(SetupServer *ss);
void *FileWrite(void *arg);
void threadWrite(const char *nome_file, const char *testo, int p);
unsigned char* fromHex(const char* hexString);

#endif //SERVER_H

