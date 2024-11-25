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
#include <openssl/evp.h>
#include <openssl/kdf.h>
#include <openssl/params.h>
#include <openssl/err.h>

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



int main()
{
    setbuf(stdout, NULL);
    struct sockaddr_in sin;
    char buf[MAX_LINE];
    unsigned int len;
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

    gmp_randstate_t state; // make sure to call gmp_randclear(state); 
    // when done with state
    gmp_randinit_mt(state);
    gmp_randseed_ui(state, time(NULL));

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

        receive_client_hello(new_s, state);

        // close the connection
        close(new_s);

        printf("[SERVER] Connection closed.\n");
    }
    gmp_randclear(state);

    close(s); // This will never be reached but is good practice
    return 0;
}

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
    unsigned char salt[DH_NONCE_SIZE*2];
    memcpy(salt, n0, DH_NONCE_SIZE);
    memcpy(salt + DH_NONCE_SIZE, n1, DH_NONCE_SIZE);
    printf("[SERVER] Salt: ");
    for (int i = 0; i < strlen(salt); i++)
    {
        printf("%d ", salt[i]);
    }

    unsigned char master_key_bytes[DH_KEY_SIZE];
    mpz_export(master_key_bytes, NULL, 1, 1, 1, 0, master_key);
    unsigned char *session_key = create_session_key(master_key_bytes, salt);
    printf("[SERVER] Session key: ");
    for (int i = 0; i < AES_KEY_SIZE; i++)
    {
        printf("%02x", session_key[i]);
    }
    printf("size of session key: %ld\n", sizeof(session_key));


    // send dhB, nonce to client
    // payload = dhB (bytes) + nonce (bytes)
    char server_payload[DH_KEY_SIZE + DH_NONCE_SIZE];
    memcpy(server_payload, dhB_bytes, DH_KEY_SIZE);
    memcpy(server_payload + DH_KEY_SIZE, n1, DH_NONCE_SIZE);

    printf("[SERVER] Sending payload to client of size %ld\n", sizeof(server_payload));
    send(socket, server_payload, sizeof(server_payload), 0);
}

// create the session key using HKDF 
unsigned char *create_session_key(unsigned char *master_key, unsigned char *salt)
{
    static unsigned char session_key[AES_KEY_SIZE];
    EVP_KDF *kdf;
    EVP_KDF_CTX *kctx;
    OSSL_PARAM params[5], *p = params;

    // Find and allocate the HKDF algorithm
    if ((kdf = EVP_KDF_fetch(NULL, "HKDF", NULL)) == NULL)
    {
        perror("Error fetching HKDF algorithm\n");
    }
    kctx = EVP_KDF_CTX_new(kdf);
    EVP_KDF_free(kdf);
    if (kctx == NULL)
    {
        perror("Error creating KDF context\n");
    }

    // Set the parameters for the HKDF algorithm
    *p++ = OSSL_PARAM_construct_utf8_string("digest", "SHA256", strlen("SHA256"));
    *p++ = OSSL_PARAM_construct_octet_string("key", master_key, strlen(master_key));
    *p++ = OSSL_PARAM_construct_octet_string("salt", salt, strlen(salt));
    *p++ = OSSL_PARAM_construct_octet_string("info", "Session key", strlen("Session key"));
    *p = OSSL_PARAM_construct_end();

    if (EVP_KDF_CTX_set_params(kctx, params) <= 0)
    {
        perror("Error setting KDF parameters\n");
        ERR_print_errors_fp(stderr);
    }

    // Derive the session key
    if (EVP_KDF_derive(kctx, session_key, AES_KEY_SIZE, params) <= 0)
    {
        perror("Error deriving session key\n");
        ERR_print_errors_fp(stderr);
    }
    EVP_KDF_CTX_free(kctx);
    printf("Session key: ");
    for (int i = 0; i < AES_KEY_SIZE; i++)
    {
        printf("%02x", session_key[i]);
    }
    printf("length of session key: %ld\n", sizeof(session_key));
    return session_key;

}
