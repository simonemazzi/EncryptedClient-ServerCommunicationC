#ifndef CLIENT_H
#define CLIENT_H

#include <stdio.h>
#include <stdlib.h>
#include <getopt.h>
#include <string.h>
#include <stdint.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <signal.h>
#include "../Auxiliary/Utility.c"


#define BUFFER_SIZE 1024

typedef struct {
    FILE *file;
    uint64_t key;
    int parallelism;
    char *IP;
    int port;
} ClientSend;


void ClientSetup(ClientSend *cs, char **argv, int argc);
int Client(ClientSend *send_c);
char* serializeMessage(Message msg);
char* toHex(const unsigned char* data, size_t len);


#endif //CLIENT_H