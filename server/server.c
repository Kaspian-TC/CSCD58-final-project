#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <gmp.h>
#include <arpa/inet.h>

#include "server.h"
#define SERVER_PORT 5432
#define MAX_PENDING 5
#define MAX_LINE 256


#define BUFFER_SIZE 1048576 // 1 MB
#define DH_NUM_BITS 2048
#define DH_G 5
#define DH_KEY_SIZE 256
#define DH_NONCE_SIZE 16
#define AES_KEY_SIZE 32

const char* SERVER_IPS[] = {"server1", "server2", "server3"};
const int NUM_SERVERS = 3;

// Function to forward a single chunk of data to the appropriate storage server
void forward_to_server(const char* server_ip, const char* payload) {
    int sock = socket(AF_INET, SOCK_STREAM, 0);

    if (sock < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    struct sockaddr_in server;
    server.sin_family = AF_INET;
    server.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, server_ip, &server.sin_addr);

    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        perror("Connection to storage server failed");
        close(sock);
        exit(1);
    }

    send(sock, payload, strlen(payload), 0);
    printf("[ROUTER] Forwarded chunk '%s' to server %s\n", payload, server_ip);
    close(sock);
}

// Function to shard data and distribute chunks across servers
void shard_and_distribute(const char* data) {
    for (int i = 0; i < strlen(data); i++) {
        char chunk[2] = {data[i], '\0'}; // Create a single-character chunk
        const char* target_server = SERVER_IPS[i % NUM_SERVERS];
        forward_to_server(target_server, chunk);
    }
}

// Function to store data locally on the storage server
void store_data(const char* payload) {
    FILE* fp = fopen("data.txt", "a");
    if (fp == NULL) {
        perror("Failed to open storage file");
        exit(1);
    }

    fprintf(fp, "%s\n", payload);
    fclose(fp);
    printf("[STORAGE] Stored payload locally: %s\n", payload);
}

int main(int argc, char* argv[]) {
    struct sockaddr_in sin;
    char buf[MAX_LINE];
    int len, s, new_s;

    // Determine if the server acts as a router or storage server
    int is_router = (argc == 2 && strcmp(argv[1], "router") == 0);

    // Build address data structure
    bzero((char*)&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(SERVER_PORT);

    if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("Socket creation failed");
        exit(1);
    }

    if ((bind(s, (struct sockaddr*)&sin, sizeof(sin))) < 0) {
        perror("Binding failed");
        close(s);
        exit(1);
    }

    listen(s, MAX_PENDING);
    printf("[SERVER] %s started on port %d.\n", is_router ? "Router" : "Storage Server", SERVER_PORT);

    while (1) {
        if ((new_s = accept(s, (struct sockaddr*)&sin, &len)) < 0) {
            perror("Accept failed");
            close(s);
            exit(1);
        }

        len = recv(new_s, buf, sizeof(buf) - 1, 0);
        buf[len] = '\0';

        if (is_router) {
            printf("[ROUTER] Received data: %s\n", buf);
            shard_and_distribute(buf); // Split and distribute data to storage servers
        } else {
            printf("[STORAGE] Received data: %s\n", buf);
            store_data(buf); // Store the data locally
        }

        close(new_s);
    }

    close(s);
    return 0;
}
/*
int main()
{
    setbuf(stdout, NULL);
    struct sockaddr_in sin;
    char buf[MAX_LINE];
    unsigned int len;
    int s, new_s;
    // build address data structure
    bzero((char *)&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(SERVER_PORT);

    printf("[SERVER] Initializing server...\n");

    // setup passive open
    if ((s = socket(PF_INET, SOCK_STREAM,
                    0)) < 0)
    {
        perror("simplex-talk: socket");
        exit(1);
    }
    if ((bind(s, (struct sockaddr *)&sin, sizeof(sin))) < 0)
    {
        perror("simplex-talk: bind");
        exit(1);
    }
    listen(s, MAX_PENDING);

    printf("[SERVER] Server started successfully and is listening on port %d.\n", SERVER_PORT);

    gmp_randstate_t state; // make sure to call gmp_randclear(state); 
    // when done with state
    gmp_randinit_mt(state);
    gmp_randseed_ui(state, time(NULL));

    // wait for connection
    while (1) {
        printf("[SERVER] Waiting for a connection...\n");
        if ((new_s = accept(s, (struct sockaddr *)&sin, &len)) < 0)
        {
            perror("simplex-talk: accept failed");
            exit(1);
        }
        printf("[SERVER] Connection established.\n");

        // the client is sending the payload for Diffie-Hellman key exchange
        // for now, lets just recieve the payload and print it

        receive_client_hello(new_s, state);

        // close the connection
        close(new_s);

        printf("[SERVER] Connection closed.\n");
    }
    gmp_randclear(state);

    close(s); // This will never be reached but is good practice
    return 0;
}
*/
// TLS IMPLEMENTATION - Server side

// Diffie-Hellman key exchange
void receive_client_hello(int socket, gmp_randstate_t state)
{
    // receive p, dhA, nonce from client
    char client_payload[DH_NUM_BITS + DH_KEY_SIZE + DH_NONCE_SIZE];
    int payload_len = recv(socket, client_payload, sizeof(client_payload), 0);
    printf("[SERVER] Received payload of size %d\n", payload_len);

    // payload is in the format p (bytes) + dhA (bytes) + nonce (bytes)
    // extract p, dhA, nonce from payload
    int p;
    int dhA;
    char n0[DH_NONCE_SIZE];
    // use mpz_import to convert back to mpz_t
    
    char dhA_bytes[DH_KEY_SIZE];
    char prime_bytes[DH_KEY_SIZE];

    memcpy(prime_bytes, client_payload, DH_KEY_SIZE);
    memcpy(dhA_bytes, client_payload + DH_KEY_SIZE, DH_KEY_SIZE);
    memcpy(n0, client_payload + DH_KEY_SIZE + DH_KEY_SIZE, DH_NONCE_SIZE);

    mpz_t prime;
    mpz_init2(prime,DH_NUM_BITS);
    mpz_import(prime, DH_KEY_SIZE, 1, 1, 1, 0, prime_bytes);
    
    mpz_t dhA_mpz;
    mpz_init2(dhA_mpz,DH_NUM_BITS);
    mpz_import(dhA_mpz, DH_KEY_SIZE, 1, 1, 1, 0, dhA_bytes);

    gmp_printf("dhB_mpz: %Zd\n", dhA_mpz); // this is a test
    gmp_printf("[SERVER] Received p = %Zd, dhA = %Zd, nonce = %d\n", prime, dhA_mpz, n0[0]);

    // print n0
    printf("[SERVER] Received nonce: ");
    for (int i = 0; i < DH_NONCE_SIZE; i++)
    {
        printf("%d ", n0[i]);
    }
    printf("\n");
    
    
    
    mpz_t b;
    mpz_t g;
    mpz_inits(b,g,NULL);
    
    mpz_t dhB_mpz;
    mpz_init2(dhB_mpz,DH_NUM_BITS);
    mpz_urandomb(b, state, DH_NUM_BITS);

    mpz_set_ui(g,DH_G);

    mpz_powm(dhB_mpz,g,b,prime); // dhB = g^b mod p
    // int dhB = (int)pow(DH_G, b) % p;

    mpz_t master_key;
    mpz_init(master_key);
    mpz_powm(master_key,dhB_mpz,b,prime); // m = dhA^b mod p
    // int m = (int)pow(dhA, b) % p;

    gmp_printf("[SERVER] Calculated dhB = %Zd, master key = %Zd\n", dhB_mpz, master_key);
    // convert dhB to string of bytes
    char dhB_bytes[DH_KEY_SIZE];
    mpz_export(dhB_bytes, NULL, 1, 1, 1, 0, dhB_mpz);

    // nonce is a random byte string of length DH_NONCE_SIZE
    int n1[DH_NONCE_SIZE];
    for (int i = 0; i < DH_NONCE_SIZE; i++)
    {
        n1[i] = rand() % 256;
    }

    // create session key HDKF
    int session_key = 1234; // placeholder for session key

    // send dhB, nonce to client
    // payload = dhB (bytes) + nonce (bytes)
    char server_payload[DH_KEY_SIZE + DH_NONCE_SIZE];
    memcpy(server_payload, dhB_bytes, DH_KEY_SIZE);
    memcpy(server_payload + DH_KEY_SIZE, n1, DH_NONCE_SIZE);

    printf("[SERVER] Sending payload to client of size %ld\n", sizeof(server_payload));
    send(socket, server_payload, sizeof(server_payload), 0);
}
