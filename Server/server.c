//
// Created by Narcis Lorenz Grecu on 19/08/25.
//
#include "server.h"

#include <stdlib.h>
#include <ctype.h>
#include <stdio.h>

/*
 funzione per la conversione
 */
unsigned char* fromHex(const char* hexString) {
    if (!hexString) return NULL;

    size_t len = strlen(hexString);// lunghezza della stringa

    if (len % 2 != 0) return NULL; // la stringa deve avere lunghezza pari

    size_t byteLen = len / 2;
    unsigned char* out = malloc(byteLen);
    if (!out) return NULL;

    for (size_t i = 0; i < byteLen; i++) {
        char high = hexString[2*i];
        char low  = hexString[2*i + 1];


        if (!((high >= '0' && high <= '9') || (high >= 'A' && high <= 'F'))) { free(out); return NULL; }
        if (!((low  >= '0' && low  <= '9') || (low  >= 'A' && low  <= 'F'))) { free(out); return NULL; }

        out[i] = ((high >= 'A' ? high - 'A' + 10 : high - '0') << 4) |
                 (low  >= 'A' ? low  - 'A' + 10 : low  - '0');
    }

    return out;
}

/*
 *funzione per analizzare gli input da linea di comando
 */
int ServerSetup(SetupServer *ss,char **argv,int argc) {
    static struct option long_options[] = {
        {"users", required_argument, 0, 'l'},
        {"header", required_argument, 0, 's'},
        {"parallelismo", required_argument, 0, 'p'},
        {"porta", required_argument, 0, 't'},
        {0, 0, 0, 0}
    };

    int opt;
    while ((opt = getopt_long(argc, argv, "l:s:p:t:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'p': {
                // grado di parallelismo
                ss->p = atoi(optarg);
                break;
            }
            case 't': {
                // porta del server
                ss->porta = atoi(optarg);
                break;
            }
            case 'l': {
                //num client
                ss->l = atoi(optarg);
                break;
            }
            case 's': {
                //header
                ss->s = optarg;
                break;
            }
            default: {
                printf("Uso: client -s <header> -l <user> -p <parallelismo> -t <porta> \n");
            }
        }
    }
    if (ss->s == NULL || ss->l == 0 || ss->porta == 0 || ss->p <= 0) {
        printf("Errore: parametri obbligatori mancanti\n");
        printf("Uso: client -s <header> -l <user> -p <parallelismo> -t <porta> \n");
        exit(EXIT_FAILURE);
    }
    return 0;
}

// Funzione che gestisce un client
void *handle_client(void *arg) { // invece di portarmi solo la socket mi porto tutto, cosi ho anche il parallelismo
    Management *m = arg;
    int client_socket=*m->client_socket;

    const char *reply = "ACK";

    char buffer[BUFFER_SIZE];

    Message msg;
    char* mess=NULL;


    size_t totale=0,n;
    while ((n=recv(client_socket,buffer,BUFFER_SIZE,0)) > 0) {
        char *temp=realloc(mess,totale+n+1);
        if(temp==NULL) {
            printf("Errore allocazione dal client\n");
            free(mess);
            free(temp);
            return NULL;
        }
        mess=temp;
        memcpy(mess+totale,buffer,n);
        totale+=n;
    }

    if (mess != NULL) mess[totale] = '\0'; // assicura terminatore
    // Parsing del messaggio serializzato: "Key|Lenght|Testo"
    char *key_str = strtok(mess, "|");
    char *lenght_str = strtok(NULL, "|");
    char *text2 = strtok(NULL, "");
    unsigned char *text = fromHex(text2);

    if (!key_str || !lenght_str || !text) {
        fprintf(stderr, "Errore nel parsing del messaggio\n");
        close(client_socket);
        pthread_exit(NULL);
    }
    uint64_t key=strtoull(key_str,NULL,10); // DA SISTEMARE

    int lenght = atoi(lenght_str);



    char* res = decryptMsg(text,key,lenght,m->ss->p);
    printf("Invio ACK...");
    send(client_socket, reply, strlen(reply), 0);


    char nome_file[255];
    snprintf(nome_file, sizeof(nome_file), "../Server/Write/%s_%d.txt", m->ss->s,n_clients);
    pthread_mutex_lock(&n_clients_mutex);
    n_clients++;
    pthread_mutex_unlock(&n_clients_mutex);

    threadWrite(nome_file,res,m->ss->p);

    printf("Connessione con un client chiusa.\n");
    close(client_socket);

    // Decrementa il contatore in modo sicuro
    pthread_mutex_lock(&lock);
    active_clients--;
    pthread_mutex_unlock(&lock);
    free(m);
    // fclose(file);

    pthread_exit(NULL);
}

/*
 funzione per gestire socket lato server
 */
void *handle_server(SetupServer *ss) {

    int server_fd, *new_socket;
    struct sockaddr_in address;
    int addrlen = sizeof(address);

    pthread_mutex_init(&lock, NULL);

    // Creazione socket
    if ((server_fd = socket(AF_INET, SOCK_STREAM, 0)) == -1) {
        perror("Errore nella creazione del socket");
        exit(EXIT_FAILURE);
    }

    address.sin_family = AF_INET;
    address.sin_addr.s_addr = INADDR_ANY;
    address.sin_port = htons(ss->porta);

    // Bind
    if (bind(server_fd, (struct sockaddr *)&address, sizeof(address)) < 0) {
        perror("Errore nel bind");
        close(server_fd);
        exit(EXIT_FAILURE);
    }


    if (listen(server_fd, ss->l) < 0) {
        perror("Errore nella listen");
        close(server_fd);
        exit(EXIT_FAILURE);
    }

    printf("Server in ascolto sulla porta %d...\n", ss->porta);

    while (1) {
        new_socket = malloc(sizeof(int));
        if ((*new_socket = accept(server_fd, (struct sockaddr *)&address,
                                  (socklen_t *)&addrlen)) < 0) {
            perror("Errore nell'accept");
            free(new_socket);
            break;
        }

        pthread_mutex_lock(&lock);
        if (active_clients >= ss->l) {
            pthread_mutex_unlock(&lock);

            // Troppi client → rifiuto connessione
            const char *busy = "Server occupato, riprova più tardi.\n";
            send(*new_socket, busy, strlen(busy), 0);
            close(*new_socket);
            free(new_socket);
            printf("Connessione rifiutata: troppi client attivi.\n");
            break;
        }

        active_clients++;
        pthread_mutex_unlock(&lock);

        printf("Nuovo client connesso! Client attivi: %d\n", active_clients);

        pthread_t thread_id;
        Management *mg=malloc(sizeof(Management));
        mg->ss = ss;
        mg->client_socket = new_socket;
        if (pthread_create(&thread_id, NULL, handle_client, mg) != 0) {
            perror("Errore nella creazione del thread");
            close(*new_socket);
            free(new_socket);

            pthread_mutex_lock(&lock);
            active_clients--;
            pthread_mutex_unlock(&lock);
            free(mg);
        } else {
            pthread_detach(thread_id);
        }
    }

    close(server_fd);

    pthread_mutex_destroy(&lock);
    return 0;
}

/*
 *scrivere su file
 */
void *FileWrite(void *arg) {
    ThreadArgs *args = arg;

    pthread_mutex_lock(&mutex);

    // Ogni thread aspetta finché non è il suo turno
    while (args->id != turno) {
        pthread_cond_wait(&cond, &mutex);
    }

    // Apre il file in modalità append
    FILE *fp = fopen(args->nome_file, "a");
    if (fp == NULL) {
        perror("Errore apertura file");
        pthread_mutex_unlock(&mutex);
        pthread_exit(NULL);
    }

    // Scrive la parte di stringa assegnata
    for (int i = args->start; i < args->end; i++) {
        fputc(args->testo[i], fp);
    }
    fclose(fp);

    // Passa il turno al thread successivo
    turno++;
    pthread_cond_broadcast(&cond); // sveglia gli altri thread in attesa

    pthread_mutex_unlock(&mutex);

    pthread_exit(NULL);
}

/*
 *gestione dei thread per scrivere su file
 */
void threadWrite(const char *nome_file, const char *testo, int p) {
    disableSig();
    int len = (int)strlen(testo);
    int chunk = len / p;      // parte "base" per ogni thread
    int resto = len % p;      // caratteri in più da distribuire ai primi thread

    pthread_t *threads = malloc(sizeof(pthread_t) * p);
    ThreadArgs *args = malloc(sizeof(ThreadArgs) * p);

    int start = 0;
    turno = 0;
    for (int i = 0; i < p; i++) {
        int extra = (i < resto) ? 1 : 0; // distribuisce uniformemente i resti
        int end = start + chunk + extra;

        args[i].nome_file = nome_file;
        args[i].testo = testo;
        args[i].start = start;
        args[i].end = end;
        args[i].id = i;

        // Crea il thread
        if (pthread_create(&threads[i], NULL, FileWrite, &args[i]) != 0) {
            perror("Errore creazione thread");
        }

        start = end; // aggiorna per la prossima fetta
    }

    // Aspetta che tutti i thread abbiano finito
    for (int i = 0; i < p; i++) {
        pthread_join(threads[i], NULL);
    }

    free(threads);
    free(args);
    activeSignals();
}


int main(int argc, char *argv[]) {
    disableSig();
    SetupServer *ss = malloc(sizeof(SetupServer));
    if (ss==NULL) {
        perror("Errore nella creazione del server");
        exit(EXIT_FAILURE);
    }
    ServerSetup(ss,argv,argc);
    handle_server(ss);
    activeSignals();
    free(ss);
    return 0;

}
