#include <gmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include "helper_func.h"
#include "certificates.h"



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

void get_big_prime(mpz_t prime, gmp_randstate_t state)
{
    mpz_t random_number;
    mpz_init(random_number);
    mpz_urandomb(random_number, state, 2048);

    mpz_nextprime(prime, random_number);
    
    mpz_clear(random_number);
    
}
// send_client_hello function contains all information passed to command line
// we want to generate p, a, dhA, nonce and send it to server as payload
void send_client_hello(int socket,
 mpz_t prime, 
 mpz_t dhA_mpz, 
 mpz_t a,
 gmp_randstate_t state, char * n0){

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
    get_random_bytes(n0, DH_NONCE_SIZE, state);

    // send p, dhA, nonce to server
    // payload = p (bytes) + dhA (bytes) + nonce (bytes)
    char payload[DH_KEY_SIZE + DH_KEY_SIZE + DH_NONCE_SIZE];
    memcpy(payload, prime_bytes, sizeof(prime_bytes));
    memcpy(payload + DH_KEY_SIZE, dhA_bytes, DH_KEY_SIZE);
    memcpy(payload + DH_KEY_SIZE + DH_KEY_SIZE, n0,
     DH_NONCE_SIZE);

    printf("[CLIENT] Sending payload to server of size %ld\n",
     sizeof(payload));
    gmp_printf("[CLIENT] p = %Zd, dhA = %Zd, nonce = \n", prime, dhA_mpz);

    // print nonce
    for (int i = 0; i < DH_NONCE_SIZE; i++)
    {
        printf("%d ", n0[i]);
    }
    printf("\n");

    send(socket, payload, sizeof(payload), 0);
}

/* Helper function for receive_server_hello */
static int is_server_response_certificate_valid(
    char* n0,char* n1,char* dhA_bytes,char* dhB_bytes,char* signature){

    // make data n0 + n1 + dhA + dhB + certificate (not there yet)
    char data[DH_NONCE_SIZE*2 + DH_KEY_SIZE*2];
    long data_len = DH_NONCE_SIZE*2 + DH_KEY_SIZE*2;
    memcpy(data, n0, DH_NONCE_SIZE);
    memcpy(data + DH_NONCE_SIZE, n1, DH_NONCE_SIZE);
    memcpy(data + DH_NONCE_SIZE*2, dhA_bytes, DH_KEY_SIZE);
    memcpy(data + DH_NONCE_SIZE*2 + DH_KEY_SIZE, dhB_bytes, DH_KEY_SIZE);

    print_bytes(data, data_len);
    // veryify the signature
    const char *public_key_file = "./keys/server.pem"; // should be the same for both client and server
    int valid = validate_signed_data(public_key_file, data, data_len, signature, 256);
    if (valid == 1)
    {
        printf("[CLIENT] Signature is valid\n");
    }
    else
    {
        printf("[CLIENT] Signature is invalid\n");
    }
    return valid;
}

char * receive_server_hello(int socket, 
mpz_t prime, 
mpz_t dhA_mpz, 
mpz_t a,
gmp_randstate_t state, 
char * master_key_bytes,
char * n0, 
char * n1){
    // wait to receive a response from the server
    char client_payload[DH_KEY_SIZE + DH_NONCE_SIZE + 256];
    int payload_len = recv(socket, client_payload, 
     sizeof(client_payload), 0);
    printf("[CLIENT] Received payload from server of size %d\n", payload_len);
    char dhB_bytes[DH_KEY_SIZE];
    char signature[256]; //TODO: change this to a constant
    memcpy(dhB_bytes, client_payload, DH_KEY_SIZE);
    memcpy(n1, client_payload + DH_KEY_SIZE, DH_NONCE_SIZE);
    memcpy(signature, client_payload + DH_KEY_SIZE + DH_NONCE_SIZE, 256);
    
    mpz_t dhB_mpz;
    mpz_init2(dhB_mpz,DH_NUM_BITS);
    mpz_import(dhB_mpz, DH_KEY_SIZE, 1, 1, 1, 0, dhB_bytes);

    gmp_printf("[CLIENT] Received dhB = %Zd\n", dhB_mpz);

    mpz_t master_key;
    mpz_init2(master_key, DH_NUM_BITS);
    mpz_powm(master_key,dhB_mpz,a,prime); // m = dhB^a mod p
    gmp_printf("[CLIENT] master = %Zd\n", master_key);
	// convert master key to bytes
	mpz_export(master_key_bytes, NULL, 1, 1, 1, 0, master_key);
    mpz_clears(dhB_mpz,master_key,NULL);


    char salt[DH_NONCE_SIZE*2];
    create_salt(salt,n0,n1);
    char session_key[AES_KEY_SIZE]; 
    create_session_key(master_key_bytes, salt,session_key);
    printf("[CLIENT] Session key: ");
    
    print_bytes(session_key, AES_KEY_SIZE);
    printf("size of session key: %ld\n", sizeof(session_key));


    char dhA_bytes[DH_KEY_SIZE];
    mpz_export(dhA_bytes, NULL, 1, 1, 1, 0, dhA_mpz);
    is_server_response_certificate_valid(n0,n1,dhA_bytes,dhB_bytes,signature);
	return master_key_bytes;
}

char * client_get_master_key(int socket, char * master_key /* assumed 256 length */,
 gmp_randstate_t state){
	mpz_t prime, dhA_mpz, a;
    mpz_init2(prime,DH_NUM_BITS);//make sure to call mpz_clear(); after using
    
    char n0[DH_NONCE_SIZE];
    char n1[DH_NONCE_SIZE];
    get_big_prime(prime,state);

    initialize_values(prime, dhA_mpz, a, state);
    send_client_hello(socket,prime, dhA_mpz, a,state,n0);
    receive_server_hello(socket, prime, dhA_mpz, a,state, master_key,n0,n1);
	mpz_clears(prime,dhA_mpz,a,NULL);

	return master_key;
}

// TLS IMPLEMENTATION - Server side

// Diffie-Hellman key exchange
void receive_client_hello(int socket, mpz_t prime, mpz_t dhA_mpz, gmp_randstate_t state,char* n0,char* n1)
{
    // receive p, dhA, nonce from client
    char client_payload[DH_NUM_BITS + DH_KEY_SIZE + DH_NONCE_SIZE];
    int payload_len = recv(socket, client_payload, 
    sizeof(client_payload), 0);
    printf("[SERVER] Received payload of size %d\n", payload_len);

    // payload is in the format p (bytes) + dhA (bytes) + nonce (bytes)
    // extract p, dhA, nonce from payload
    int p;
    int dhA;
    
    // use mpz_import to convert back to mpz_t
    
    char dhA_bytes[DH_KEY_SIZE];
    char prime_bytes[DH_KEY_SIZE];

    memcpy(prime_bytes, client_payload, DH_KEY_SIZE);
    memcpy(dhA_bytes, client_payload + DH_KEY_SIZE, DH_KEY_SIZE);
    memcpy(n0, client_payload + DH_KEY_SIZE + DH_KEY_SIZE, 
    DH_NONCE_SIZE);
    
    mpz_init2(prime,DH_NUM_BITS);
    mpz_import(prime, DH_KEY_SIZE, 1, 1, 1, 0, prime_bytes);
    
    mpz_init2(dhA_mpz,DH_NUM_BITS);
    mpz_import(dhA_mpz, DH_KEY_SIZE, 1, 1, 1, 0, dhA_bytes);

    gmp_printf("[SERVER] Received p = %Zd, dhA = %Zd, nonce = %d\n", prime, dhA_mpz, n0[0]);

    // print n0
    printf("[SERVER] Received nonce: ");
    for (int i = 0; i < DH_NONCE_SIZE; i++)
    {
        printf("%d ", n0[i]);
    }
    printf("\n");
}

char * send_server_hello(int socket,
 mpz_t prime, 
 mpz_t dhA_mpz, 
 gmp_randstate_t state, char * master_key_bytes /* assume 256 bytes */,
 char * n0, char * n1){
    mpz_t b, g, dhB_mpz;
    initialize_values(prime,dhB_mpz,b,state);
    
    mpz_t master_key;
    mpz_init(master_key);
    mpz_powm(master_key,dhA_mpz,b,prime); // m = dhA^b mod p
    // convert master key to bytes
    mpz_export(master_key_bytes, NULL, 1, 1, 1, 0, master_key);


    gmp_printf("[SERVER] Calculated dhB = %Zd, master key = %Zd\n", dhB_mpz, master_key);
    // convert dhB to string of bytes
    char dhB_bytes[DH_KEY_SIZE];
    mpz_export(dhB_bytes, NULL, 1, 1, 1, 0, dhB_mpz);

    get_random_bytes(n1, DH_NONCE_SIZE, state);
    printf("[SERVER] sending nonce: ");
    for (int i = 0; i < DH_NONCE_SIZE; i++)
    {
        printf("%d ", n1[i]);
    }
    printf("\n");

    char salt[DH_NONCE_SIZE*2];
    create_salt(salt,n0,n1);
    char session_key[AES_KEY_SIZE]; 
    create_session_key(master_key_bytes, salt,session_key);
    printf("[SERVER] Session key: ");
    print_bytes(session_key, AES_KEY_SIZE);
    printf("size of session key: %ld\n", sizeof(session_key));


    // convert dhA to string of bytes
    char dhA_bytes[DH_KEY_SIZE];
    mpz_export(dhA_bytes, NULL, 1, 1, 1, 0, dhA_mpz);
    // sign the data
    const char *private_key_file = "./keys/private.pem"; // should be the same for both client and server
    
    // make data n0 + n1 + dhA + dhB
    char data[DH_NONCE_SIZE*2 + DH_KEY_SIZE*2];
    memcpy(data, n0, DH_NONCE_SIZE);
    memcpy(data + DH_NONCE_SIZE, n1, DH_NONCE_SIZE);
    memcpy(data + DH_NONCE_SIZE*2, dhA_bytes, DH_KEY_SIZE);
    memcpy(data + DH_NONCE_SIZE*2 + DH_KEY_SIZE, dhB_bytes, DH_KEY_SIZE);
    
    long data_len = DH_NONCE_SIZE*2 + DH_KEY_SIZE*2;

    print_bytes(data, data_len);

    char * signature;
    size_t signed_len;
    sign_data(private_key_file, data, data_len,&signature,&signed_len,"server");

    // send dhB, nonce to client
    // payload = dhB (bytes) + nonce (bytes) + signature (bytes)
    // char server_payload[DH_KEY_SIZE + DH_NONCE_SIZE];
    long payload_len = DH_KEY_SIZE + DH_NONCE_SIZE + signed_len;
    char * server_payload = malloc(payload_len);
    
    memcpy(server_payload, dhB_bytes, DH_KEY_SIZE);
    memcpy(server_payload + DH_KEY_SIZE, n1, DH_NONCE_SIZE);
    memcpy(server_payload + DH_KEY_SIZE + DH_NONCE_SIZE, signature, signed_len);

    printf("[SERVER] Sending payload to client of size %ld\n", payload_len);
    send(socket, server_payload, DH_KEY_SIZE + DH_NONCE_SIZE + signed_len , 0);
    
    mpz_clears(dhB_mpz,b,master_key,NULL);
    free(server_payload);
    return master_key_bytes;
}

char * server_get_master_key(int socket, char * master_key /* assumed 256 length */,
 gmp_randstate_t state){
    char n0[DH_NONCE_SIZE];
    char n1[DH_NONCE_SIZE];
    mpz_t prime;
    mpz_t dhA_mpz;
    receive_client_hello(socket, prime, dhA_mpz, state, n0,n1);
    send_server_hello(socket,prime,dhA_mpz,state,master_key,n0,n1);
    mpz_clears(prime,dhA_mpz,NULL);
    return master_key;
}

