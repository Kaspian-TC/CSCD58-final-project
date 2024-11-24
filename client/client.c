#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <gmp.h>
#include <time.h>

#include "client.h"

#define SERVER_PORT 5432
#define MAX_LINE 256
// test comment
// another test commment

void send_data(int socket, const char* data) {
    send(socket, data, strlen(data), 0);
    printf("[CLIENT] Sent: %s\n", data);
}

void retrieve_data(int socket) {
    char buf[MAX_LINE];
    int len;

    send_data(socket, "RETRIEVE");
    printf("[CLIENT] Sent retrieval request\n");

    // Receive data from the server
    printf("[CLIENT] Received data:\n");
    while ((len = recv(socket, buf, MAX_LINE - 1, 0)) > 0) {
        buf[len] = '\0';
        printf("%s", buf);
    }
    printf("\n[CLIENT] Data retrieval complete\n");
}

int main(int argc, char* argv[]) {
    struct hostent* hp;
    struct sockaddr_in sin;
    char* host;
    int s;

    if (argc != 3 || (strcmp(argv[1], "--store") != 0 && strcmp(argv[1], "--retrieve") != 0)) {
        fprintf(stderr, "Usage: %s --store|--retrieve <router_host>\n", argv[0]);
        exit(1);
    }

    char* mode = argv[1];
    host = argv[2];

    // Translate host name into peer's IP address
    hp = gethostbyname(host);
    if (!hp) {
        fprintf(stderr, "simplex-talk: unknown host: %s\n", host);
        exit(1);
    }

    // Build address data structure
    bzero((char*)&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    bcopy(hp->h_addr, (char*)&sin.sin_addr, hp->h_length);
    sin.sin_port = htons(SERVER_PORT);

    // Active open
    if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("simplex-talk: socket");
        exit(1);
    }
    if (connect(s, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
        perror("simplex-talk: connect");
        close(s);
        exit(1);
    }

    printf("[CLIENT] Connected to server at %s\n", host);

    if (strcmp(mode, "--store") == 0) {
        // Generate and send a random string
        srand(time(NULL));
        char random_string[MAX_LINE];
        snprintf(random_string, sizeof(random_string), "RandomData_%d", rand());
        send_data(s, random_string);
    } else if (strcmp(mode, "--retrieve") == 0) {
        // Retrieve data from the router
        retrieve_data(s);
    }

    close(s);
    return 0;
}

/* PREVIOUS IMPLEMENTATION */
void get_big_prime(mpz_t prime, gmp_randstate_t state)
{
    // mpz_t z;
    // gmp_printf ("%s is an mpz %Zd\n", "here", z);
    mpz_t random_number;
    mpz_init(random_number);
    mpz_urandomb(random_number, state, 2048);

    mpz_nextprime(prime, random_number);
    
    mpz_clear(random_number);
    // mpz_nextprime(prime, 2);
    
}


/*
int main(int argc, char *argv[])
{
    
    FILE *fp;
    struct hostent *hp;
    struct sockaddr_in sin;
    char *host;
    char buf[MAX_LINE];
    int s;
    int len;
    if (argc == 2)
    {
        host = argv[1];
    }
    else
    {
        fprintf(stderr, "usage: simplex-talk host\n");
        exit(1);
    }
    // translate host name into peer’s IP address
    hp = gethostbyname(host);
    if (!hp)
    {
        fprintf(stderr, "simplex-talk: unknown host: %s\n",
                host);
        exit(1);
    }
    // build address data structure
    bzero((char *)&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    bcopy(hp->h_addr, (char *)&sin.sin_addr, hp->h_length);
    sin.sin_port = htons(SERVER_PORT);
    
    if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("simplex-talk: socket");
        exit(1);
    }
    if (connect(s, (struct sockaddr *)&sin,
                sizeof(sin)) < 0)
    {
        perror("simplex-talk: connect");
        close(s);
        exit(1);
    }
    // main loop: get and send lines of text
    printf("has connected to server at %s\n", host);

    // set state for random number generation (mercenene twister)
    gmp_randstate_t state; // make sure to call gmp_randclear(state); 
    // when done with state
    gmp_randinit_mt(state);
    gmp_randseed_ui(state, time(NULL));

    
    mpz_t prime, dhA_mpz, a;
    initialize_values(prime, dhA_mpz, a, state);
    send_client_hello(s,prime, dhA_mpz, a);
    // close the connection
    close(s);
    return 0;


    // while (fgets(buf, sizeof(buf), stdin))
    // {
    //     len = strlen(buf) + 1;
    //     send(s, buf, len, 0);

    //     char buf_2[MAX_LINE+60];
    //     buf_2[MAX_LINE + 60 - 1] = '\0';
    //     len = recv(s, buf_2, sizeof(buf_2), 0);
    //     printf("%s",buf_2);
	// 	if (strcmp(buf, ">>> Ciao-Ciao\n") == 0 || strcmp(buf, "Ciao-Ciao\n") == 0)
	// 	{
	// 		close(s);
	// 		return 0;
	// 	}
	// } 
}
*/

// TLS IMPLEMENTATION - Client side
// Diffie-Hellman key exchange
// Client creates p using fixed g such that g is coprime to p-1
// client creates a secret number a and computes dhA = g^a mod p
// client creates a nonce and sends p, dhA, nonce to server

#define BUFFER_SIZE 1048576 // 1 MB
#define DH_NUM_BITS 2048
#define DH_G 5
#define DH_KEY_SIZE 256
#define DH_NONCE_SIZE 16
#define AES_KEY_SIZE 32

void initialize_values(mpz_t prime, mpz_t dhA_mpz, mpz_t a,
 gmp_randstate_t state){
    mpz_t g;
    mpz_inits(a,g,NULL);
    
    mpz_init2(dhA_mpz,DH_NUM_BITS);
    mpz_init2(prime,DH_NUM_BITS);//make sure to call mpz_clear(); after using
    
    get_big_prime(prime,state);

    gmp_printf("Prime number: %Zd\n", prime);

    mpz_urandomb(a, state, DH_NUM_BITS);
    // compute dhA = g^a mod p
    // initialize g 
    mpz_set_ui(g,DH_G);

    mpz_powm(dhA_mpz,g,a,prime); // dhA = g^a mod p
    mpz_clear(g);
}

// send_client_hello function contains all information passed to command line
// we want to generate p, a, dhA, nonce and send it to server as payload
void send_client_hello(int socket,
 mpz_t prime, 
 mpz_t dhA_mpz, 
 mpz_t a){

    // convert dhA to string of bytes
    char dhA_bytes[DH_KEY_SIZE];
    /* array, count (# of words produced), size(bytes per), order(set 1), endian 
    (1 for big, -1 for little, 0 for native cpu), nails (The number of most 
     significant bits to skip), op (The integer to convert)
    */
    mpz_export(dhA_bytes, NULL, 1, 1, 1, 0, dhA_mpz);
    char prime_bytes[DH_KEY_SIZE];
    mpz_export(prime_bytes, NULL, 1, 1, 1, 0, prime);
    
    // nonce is a random byte string of length DH_NONCE_SIZE
    char nonce[DH_NONCE_SIZE];
    for (int i = 0; i < DH_NONCE_SIZE; i++)
    {
        nonce[i] = rand() % 256;
    }

    // send p, dhA, nonce to server
    // payload = p (bytes) + dhA (bytes) + nonce (bytes)
    char payload[DH_KEY_SIZE + DH_KEY_SIZE + DH_NONCE_SIZE];
    memcpy(payload, prime_bytes, sizeof(prime_bytes));
    memcpy(payload + DH_KEY_SIZE, dhA_bytes, DH_KEY_SIZE);
    memcpy(payload + DH_KEY_SIZE + DH_KEY_SIZE, nonce,
     DH_NONCE_SIZE);

    printf("[CLIENT] Sending payload to server of size %ld\n",
     sizeof(payload));
    gmp_printf("[CLIENT] p = %Zd, dhA = %Zd, nonce = %d\n", prime, dhA_mpz,
     nonce[0]);

    // print nonce
    for (int i = 0; i < DH_NONCE_SIZE; i++)
    {
        printf("%d ", nonce[i]);
    }

    send(socket, payload, sizeof(payload), 0);
}


