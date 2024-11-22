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
#define SERVER_PORT 5432
#define MAX_LINE 256

void getBigPrime(mpz_t prime)
{
    // mpz_t z;
    // gmp_printf ("%s is an mpz %Zd\n", "here", z);
    mpz_t random_number;
    mpz_init(random_number);
    gmp_randstate_t state;
    gmp_randinit_mt(state);
    gmp_randseed_ui(state, time(NULL));
    mpz_urandomb(random_number, state, 2048);
    mpz_nextprime(prime, random_number);
    gmp_randclear(state);
    mpz_clear(random_number);
    // mpz_nextprime(prime, 2);
    
}
/* FUNCTION DECLARATIONS */
void send_client_hello(int socket);

int main(int argc, char *argv[])
{
    mpz_t prime;
    mpz_init2(prime,2048);
    getBigPrime(prime);

    gmp_printf("Prime number: %Zd\n", prime);
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
    /* translate host name into peer’s IP address */
    hp = gethostbyname(host);
    if (!hp)
    {
        fprintf(stderr, "simplex-talk: unknown host: %s\n",
                host);
        exit(1);
    }
    /* build address data structure */
    bzero((char *)&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    bcopy(hp->h_addr, (char *)&sin.sin_addr, hp->h_length);
    sin.sin_port = htons(SERVER_PORT);
    /* active open */
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
    /* main loop: get and send lines of text */
    printf("has connected to server at %s\n", host);

    // just test sending the payload for Diffie-Hellman key exchange
    send_client_hello(s);
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

// send_client_hello function contains all information passed to command line
// we want to generate p, a, dhA, nonce and send it to server as payload
void send_client_hello(int socket)
{
    int p = 7; // placeholder for 2048 bit prime number
    int a = 3; // placeholder for secret number

    // compute dhA = g^a mod p
    int dhA = (int)pow(DH_G, a) % p;
    
    // nonce is a random byte string of length DH_NONCE_SIZE
    char nonce[DH_NONCE_SIZE];
    for (int i = 0; i < DH_NONCE_SIZE; i++)
    {
        nonce[i] = rand() % 256;
    }

    // send p, dhA, nonce to server
    // payload = p (bytes) + dhA (bytes) + nonce (bytes)
    char payload[DH_NUM_BITS + DH_KEY_SIZE + DH_NONCE_SIZE];
    memcpy(payload, &p, sizeof(p));
    memcpy(payload + sizeof(p), &dhA, sizeof(dhA));
    memcpy(payload + sizeof(p) + sizeof(dhA), nonce, sizeof(nonce));

    printf("[CLIENT] Sending payload to server of size %d\n", sizeof(payload));
    printf("[CLIENT] p = %d, dhA = %d, nonce = %d\n", p, dhA, nonce[0]);

    // print nonce
    for (int i = 0; i < DH_NONCE_SIZE; i++)
    {
        printf("%d ", nonce[i]);
    }

    send(socket, payload, sizeof(payload), 0);
}

