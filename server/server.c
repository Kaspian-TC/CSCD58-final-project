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
#define SERVER_PORT 5432
#define MAX_PENDING 5
#define MAX_LINE 256


#define BUFFER_SIZE 1048576 // 1 MB
#define DH_NUM_BITS 2048
#define DH_G 5
#define DH_KEY_SIZE 256
#define DH_NONCE_SIZE 16
#define AES_KEY_SIZE 32

/* FUNCTION DECLARATIONS */
void receive_client_hello(int socket);

int main()
{
    setbuf(stdout, NULL);
    struct sockaddr_in sin;
    char buf[MAX_LINE];
    int len;
    int s, new_s;
    /* build address data structure */
    bzero((char *)&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(SERVER_PORT);

    printf("[SERVER] Initializing server...\n");

    /* setup passive open */
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

    /* Wait for connection */
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

        receive_client_hello(new_s);

        // close the connection
        close(new_s);

        printf("[SERVER] Connection closed.\n");
    }

    close(s); // This will never be reached but is good practice
    return 0;
}

// TLS IMPLEMENTATION - Server side

// Diffie-Hellman key exchange
void receive_client_hello(int socket)
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

    memcpy(&p, client_payload, sizeof(p));
    memcpy(&dhA, client_payload + sizeof(p), sizeof(dhA));
    memcpy(n0, client_payload + sizeof(p) + sizeof(dhA), sizeof(n0));

    printf("[SERVER] Received p = %d, dhA = %d, nonce = %d\n", p, dhA, n0[0]);

    // print n0
    printf("[SERVER] Received nonce: ");
    for (int i = 0; i < DH_NONCE_SIZE; i++)
    {
        printf("%d ", n0[i]);
    }
    printf("\n");

    int b = 4; // placeholder for secret number
    int dhB = (int)pow(DH_G, b) % p;

    // master key m = dha^b mod p
    int m = (int)pow(dhA, b) % p;

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
    memcpy(server_payload, &dhB, sizeof(dhB));
    memcpy(server_payload + sizeof(dhB), n1, sizeof(n1));

    printf("[SERVER] Sending payload to client of size %d\n", sizeof(server_payload));
    send(socket, server_payload, sizeof(server_payload), 0);
}
