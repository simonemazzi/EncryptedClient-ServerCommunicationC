
#include "client.h"

// converte dati binari in stringa esadecimale
char* toHex(const unsigned char* data, size_t len) {
    static const char hex[] = "0123456789ABCDEF";
    char *out = malloc(len*2 + 1);
    if (!out) return NULL;
    for (size_t i = 0; i < len; i++) {
        out[2*i]     = hex[data[i] >> 4];
        out[2*i + 1] = hex[data[i] & 0x0F];
    }
    out[len*2] = '\0';
    return out;
}

/*
 *funzione che analizza i parametri presi da linea di comando
 */

void ClientSetup(ClientSend *cs,char **argv,int argc) {
    static struct option long_options[] = {
        {"file", required_argument, 0, 'f'},
        {"key", required_argument, 0, 'k'},
        {"parallelism", required_argument, 0, 'p'},
        {"ip", required_argument, 0, 'i'},
        {"port", required_argument, 0, 't'},
        {0, 0, 0, 0}
    };
    int opt;
    while ((opt = getopt_long(argc, argv, "f:k:p:i:t:", long_options, NULL)) != -1) {
        switch (opt) {
            case 'f': {
                // nome del file
                FILE *input= fopen(optarg,"r");
                if (input==NULL) {
                    perror("Impossibile aprire il file.");
                    exit(EXIT_FAILURE);
                }
                cs->file = input;
                break;
            }
            case 'k': {
                if (strlen(optarg) != 8) {
                    fprintf(stderr, "Key must be of 8 char!\n");
                    exit(1);
                }
                memcpy(&cs->key, optarg, 8);  // Copa 8 bytes in  uint64_t
                break;
            }
            case 'p': {
                // grado di parallelismo
                cs->parallelism = atoi(optarg);
                break;
            }
            case 'i': {
                // indirizzo IP del server
                cs->IP = optarg;
                break;
            }
            case 't': {
                // porta del server
                cs->port = atoi(optarg);
                break;
            }
            default: {
                printf("Uso: client -f <file> -k <key> -p <parallelismo> -i <ip> -t <porta> \n");
            }
        }
    }
    if (cs->file == NULL || cs->IP == NULL || cs->port == 0 || cs->parallelism <= 0) {
        printf("Errore: parametri obbligatori mancanti\n");
        printf("Uso: client -f <file> -k <key> -p <parallelismo> -i <ip> -t <porta> \n");
        exit(EXIT_FAILURE);
    }
}

/*
funzione per la conversione
 */
char* serializeMessage(Message msg) {
    // Calcolo della lunghezza totale
    int key_len = 21;  // max cifre uint64_t + terminatore
    int len_len = 12;  // max cifre int + terminatore
    int total_len = key_len + len_len + (msg.Lenght)*BLOCK_S + 3; // +2 per separatori +1 terminatore
    char *buffer2 = malloc(total_len);
    if (!buffer2) return NULL;

    int written = snprintf(buffer2, total_len, "%" PRIu64 "|%d|", msg.Key, msg.Lenght);
    if (written < 0 || written >= total_len) {
        free(buffer2);
        return NULL;
    }
    memcpy(buffer2+written, toHex(msg.text_to_send,msg.Lenght*BLOCK_S*2),(msg.Lenght)*BLOCK_S*2);
    buffer2[written + msg.Lenght*BLOCK_S*2] = '\0';
    return buffer2;
}

/*
 *creazione socket lato client
 */
int Client(ClientSend *send_c) {
    int sock = 0;
    struct sockaddr_in serv_addr;
    char buffer[BUFFER_SIZE] = {0};
    int size=0;
    Message msg;
    unsigned char *text_to_send= encryptMsg(send_c->file,send_c->key,send_c->parallelism,&size);
     if (text_to_send == NULL) {
         fprintf(stderr, "encryptMsg ha restituito NULL\n");
         return EXIT_FAILURE;

     }

    // Creazione del socket
    if ((sock = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Errore nella creazione del socket");
        return EXIT_FAILURE;

        }

    serv_addr.sin_family = AF_INET;
    serv_addr.sin_port = htons(send_c->port);

    // Converti l'indirizzo IP da testo a binario
    if (inet_pton(AF_INET, send_c->IP, &serv_addr.sin_addr) <= 0) {
        perror("Indirizzo non valido o non supportato");
        close(sock);
        return EXIT_FAILURE;
    }

    // Connessione al server
    if (connect(sock, (struct sockaddr *)&serv_addr, sizeof(serv_addr)) < 0) {
        perror("Errore nella connessione");
        close(sock);
        return EXIT_FAILURE;
    }


    // Invio messaggio al server
    msg.text_to_send=text_to_send;
    msg.Key=send_c->key;
    msg.Lenght=size;
    char *message= serializeMessage(msg);
    printf("Messaggio cifrato : %s \n",message );
    send(sock, message, strlen(message), 0);
    shutdown(sock, SHUT_WR); // chiudo la scrittura
    free(message);

    printf("Messaggio inviato al server.\n");

    printf("attendo ACK...\n");
    // Ricezione risposta dal server
    while (strcmp(buffer,"ACK") != 0) {
        read(sock, buffer, BUFFER_SIZE);
    }
    printf("ACK ricevuto.\n");


    // Chiudi la connessione
    close(sock);
    free(text_to_send);
    return 0;
}

int main(int argc, char *argv[]) {
    ClientSend *cs = malloc(sizeof(ClientSend));
    if (cs == NULL) {
        perror("Errore nella creazione del server");
        exit(EXIT_FAILURE);
    }
    disableSig();
    ClientSetup(cs,argv,argc);
    if (Client(cs) == EXIT_FAILURE) {
        free(cs);
        exit(EXIT_FAILURE);
    }
    activeSignals();
    free(cs);
    return 0;
}