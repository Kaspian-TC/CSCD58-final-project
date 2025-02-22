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

void get_big_prime(mpz_t prime, gmp_randstate_t state)
{
    mpz_t random_number;
    mpz_init(random_number);
    mpz_urandomb(random_number, state, DH_NUM_BITS);

    mpz_nextprime(prime, random_number);
    
    mpz_clear(random_number);

    // determine if 
    
}

// send_client_hello function contains all information passed to command line
// we want to generate p, a, dhA, nonce and send it to server as payload
void send_client_hello(int socket,
 mpz_t prime, 
 mpz_t dhA_mpz, 
 mpz_t a,
 gmp_randstate_t state, uint8_t * n0){

    // convert dhA to string of bytes
    uint8_t dhA_bytes[DH_KEY_SIZE];
    /* array, count (# of words produced), size(bytes per), order(set 1), endian 
    (1 for big, -1 for little, 0 for native cpu), nails (The number of most 
     significant bits to skip), op (The integer to convert)
    */
    mpz_export(dhA_bytes, NULL, 1, 1, 1, 0, dhA_mpz);
    uint8_t prime_bytes[DH_KEY_SIZE];
    mpz_export(prime_bytes, NULL, 1, 1, 1, 0, prime);
    
    // nonce is a random byte string of length DH_NONCE_SIZE
    get_random_bytes(n0, DH_NONCE_SIZE, state);

    // send p, dhA, nonce to server
    // payload = p (bytes) + dhA (bytes) + nonce (bytes)
    int payload_size = DH_KEY_SIZE + DH_KEY_SIZE + DH_NONCE_SIZE;
    uint8_t payload[DH_KEY_SIZE + DH_KEY_SIZE + DH_NONCE_SIZE];
    memcpy(payload, prime_bytes, DH_KEY_SIZE);
    memcpy(payload + DH_KEY_SIZE, dhA_bytes, DH_KEY_SIZE);
    memcpy(payload + DH_KEY_SIZE + DH_KEY_SIZE, n0,
     DH_NONCE_SIZE);

    send(socket, payload, payload_size, 0);
}

static void get_public_key_file(
    const char *server_public_key_file, 
    uint8_t **server_public_key, 
    size_t *server_public_key_len)
    {

    FILE *server_key_file = fopen(server_public_key_file, "r");
    if (!server_key_file) {
        perror("Unable to open public key file");
    }
    // find file size
    fseek(server_key_file, 0, SEEK_END);
    long file_size = ftell(server_key_file);

    rewind(server_key_file);
    
    // read into buffer
    *server_public_key = (uint8_t *) malloc(file_size);
    *server_public_key_len = fread(*server_public_key, 1, file_size, server_key_file);


    fclose(server_key_file);
}


/* Helper function for receive_server_hello */
static int is_server_response_sign_valid(
    uint8_t* n0,uint8_t* n1,uint8_t* dhA_bytes,uint8_t* dhB_bytes,
    uint8_t* signature, long signature_len, uint8_t * public_key_file,
    size_t public_key_len){

    // get the server's public key
    const char *server_public_key_file = "./keys/server.pem"; // here it should call the "CA"

    // load in the server public key from file
    uint8_t *server_public_key;
    size_t server_public_key_len;
    /* server_public_key is malloced */
    get_public_key_file(
        server_public_key_file, &server_public_key,
         &server_public_key_len);
    // compare read in public key with the public key from the server
    if (memcmp((char *)server_public_key, (char *)public_key_file, public_key_len) != 0){
        perror("[CLIENT] Server public key does not match\n");
        free(server_public_key);
        return 0;
    }
    // make data n0 + n1 + dhA + dhB + certificate
    long data_len = DH_NONCE_SIZE*2 + DH_KEY_SIZE*2 + server_public_key_len;
    uint8_t data[data_len];
    memcpy(data, n0, DH_NONCE_SIZE);
    memcpy(data + DH_NONCE_SIZE, n1, DH_NONCE_SIZE);
    memcpy(data + DH_NONCE_SIZE*2, dhA_bytes, DH_KEY_SIZE);
    memcpy(data + DH_NONCE_SIZE*2 + DH_KEY_SIZE, dhB_bytes, DH_KEY_SIZE);
    memcpy(data + DH_NONCE_SIZE*2 + DH_KEY_SIZE*2, server_public_key, server_public_key_len);

    // veryify the signature
    int valid = validate_signed_data(server_public_key_file, data, data_len, signature, signature_len);
    free(server_public_key);
    return valid;
}

uint8_t * receive_server_hello(int socket, 
mpz_t prime, 
mpz_t dhA_mpz, 
mpz_t a,
gmp_randstate_t state, 
uint8_t * master_key_bytes,
uint8_t * n0, 
uint8_t * n1,
uint8_t * session_key /* Assume 32 bytes */){
    // wait to receive a response from the server
    int client_payload_len = DH_KEY_SIZE + DH_NONCE_SIZE + AES_TAG_SIZE + 1024;
    uint8_t client_payload[client_payload_len]; // 1024 is for padding
    int payload_len = recv(socket, client_payload, 
     client_payload_len, 0);
    
    if (payload_len < DH_KEY_SIZE + DH_NONCE_SIZE + AES_TAG_SIZE) {
        perror("Invalid payload size: receive server hello");
        return 0;
    }

    uint8_t dhB_bytes[DH_KEY_SIZE];
    uint8_t tag[AES_TAG_SIZE];
    long ciphertext_len = payload_len - DH_KEY_SIZE - DH_NONCE_SIZE - AES_TAG_SIZE;
    uint8_t ciphertext[ciphertext_len];

    memcpy(dhB_bytes, client_payload, DH_KEY_SIZE);
    memcpy(n1, client_payload + DH_KEY_SIZE, DH_NONCE_SIZE);
    memcpy(tag, client_payload + DH_KEY_SIZE + DH_NONCE_SIZE, AES_TAG_SIZE);
    memcpy(ciphertext, client_payload + DH_KEY_SIZE + DH_NONCE_SIZE + AES_TAG_SIZE, ciphertext_len);

    //compute session key
    mpz_t dhB_mpz;
    mpz_init2(dhB_mpz,DH_NUM_BITS);
    // mpz_init(dhB_mpz); // regular init makes sure that empty bytes aren't used
    mpz_import(dhB_mpz, DH_KEY_SIZE, 1, 1, 1, 0, dhB_bytes);    

    generate_session_key(session_key, dhB_mpz, a, prime, state, master_key_bytes, n0, n1);
    mpz_clear(dhB_mpz);

    // decrypt the ciphertext and extract the signature, public key
    uint8_t *plaintext = malloc(ciphertext_len);
    int plaintext_len = aes_decrypt(
        (uint8_t *) ciphertext,
        ciphertext_len,
        NULL, 0,
        (uint8_t *) tag,
        (uint8_t *) session_key,
        (uint8_t *) n1,
        DH_NONCE_SIZE,
        plaintext);

    long signature_len = 256; // signature length is always 256
    uint8_t signature[signature_len];
    long server_public_key_len = plaintext_len - signature_len;
    uint8_t server_public_key[server_public_key_len];   
    memcpy(signature, plaintext, signature_len);
    memcpy(server_public_key, plaintext + signature_len, server_public_key_len);

    // print out server_public_key

    uint8_t dhA_bytes[DH_KEY_SIZE];
    mpz_export(dhA_bytes, NULL, 1, 1, 1, 0, dhA_mpz);
    if (!is_server_response_sign_valid(n0,n1,dhA_bytes,dhB_bytes,signature, signature_len, server_public_key, server_public_key_len)){
        perror("[CLIENT] Server response is invalid\n");   
    }
    // Clean up
    free(plaintext);

	return session_key;
}

uint8_t * client_get_session_key(int socket, uint8_t * session_key /* assumed 256 length */,
 gmp_randstate_t state){
	mpz_t prime, dhA_mpz, a;
    mpz_init2(prime,DH_NUM_BITS);//make sure to call mpz_clear(); after using
    uint8_t master_key[DH_KEY_SIZE];
    // memset(master_key, 0, DH_KEY_SIZE);
    uint8_t n0[DH_NONCE_SIZE];
    uint8_t n1[DH_NONCE_SIZE];
    get_big_prime(prime,state);

    initialize_values(prime, dhA_mpz, a, state);
    send_client_hello(socket,prime, dhA_mpz, a,state,n0);
    receive_server_hello(socket,
     prime, dhA_mpz, a,state, master_key,n0,n1,session_key);
	mpz_clears(prime,dhA_mpz,a,NULL);

	return session_key;
}

// TLS IMPLEMENTATION - Server side

// Diffie-Hellman key exchange
void receive_client_hello(int socket, mpz_t prime, mpz_t dhA_mpz,
gmp_randstate_t state,uint8_t* n0,uint8_t* n1)
{
    // receive p, dhA, nonce from client
    int client_payload_len = DH_KEY_SIZE + DH_KEY_SIZE + DH_NONCE_SIZE;
    uint8_t client_payload[client_payload_len];
    int payload_len = recv(socket, client_payload, 
    client_payload_len, 0);

    if (payload_len < DH_KEY_SIZE + DH_KEY_SIZE + DH_NONCE_SIZE) {
        perror("Invalid payload size: receive client hello");
        return;
    }

    // payload is in the format p (bytes) + dhA (bytes) + nonce (bytes)
    // extract p, dhA, nonce from payload
    int p;
    int dhA;
    
    // use mpz_import to convert back to mpz_t
    
    uint8_t dhA_bytes[DH_KEY_SIZE];
    uint8_t prime_bytes[DH_KEY_SIZE];

    memcpy(prime_bytes, client_payload, DH_KEY_SIZE);
    memcpy(dhA_bytes, client_payload + DH_KEY_SIZE, DH_KEY_SIZE);
    memcpy(n0, client_payload + DH_KEY_SIZE + DH_KEY_SIZE, 
    DH_NONCE_SIZE);
    
    mpz_init2(prime,DH_NUM_BITS);
    mpz_import(prime, DH_KEY_SIZE, 1, 1, 1, 0, prime_bytes);
    
    mpz_init2(dhA_mpz,DH_NUM_BITS);
    mpz_import(dhA_mpz, DH_KEY_SIZE, 1, 1, 1, 0, dhA_bytes);
}

static void sign_data_to_client(
    uint8_t* n0,
    uint8_t* n1,
    uint8_t* dhA_bytes,
    uint8_t* dhB_bytes,
    uint8_t** signature, 
    size_t * signature_len,
    uint8_t *public_key, 
    size_t public_key_len,
    const char *private_key_file){

    // make data n0 + n1 + dhA + dhB + certificate
    uint8_t data[DH_NONCE_SIZE*2 + DH_KEY_SIZE*2 + public_key_len];
    memcpy(data, n0, DH_NONCE_SIZE);
    memcpy(data + DH_NONCE_SIZE, n1, DH_NONCE_SIZE);
    memcpy(data + DH_NONCE_SIZE*2, dhA_bytes, DH_KEY_SIZE);
    memcpy(data + DH_NONCE_SIZE*2 + DH_KEY_SIZE, dhB_bytes, DH_KEY_SIZE);
    memcpy(data + DH_NONCE_SIZE*2 + DH_KEY_SIZE*2, public_key, public_key_len);
    
    long data_len = DH_NONCE_SIZE*2 + DH_KEY_SIZE*2 + public_key_len;

    sign_data(private_key_file, data, data_len,signature,signature_len,"server");
}

uint8_t * send_server_hello(int socket,
 mpz_t prime, 
 mpz_t dhA_mpz, 
 gmp_randstate_t state, uint8_t * master_key_bytes /* assume 256 bytes */,
 uint8_t * n0, uint8_t * n1, uint8_t * session_key /* assume 32 */){
    mpz_t b, g, dhB_mpz;
    initialize_values(prime,dhB_mpz,b,state);
    
    // convert dhB to string of bytes
    uint8_t dhB_bytes[DH_KEY_SIZE];
    mpz_export(dhB_bytes, NULL, 1, 1, 1, 0, dhB_mpz);

    // get n1 nonce
    get_random_bytes(n1, DH_NONCE_SIZE, state);

    generate_session_key(session_key, dhA_mpz, b, prime, state, master_key_bytes, n0, n1);


    // convert dhA to string of bytes
    uint8_t dhA_bytes[DH_KEY_SIZE];
    mpz_export(dhA_bytes, NULL, 1, 1, 1, 0, dhA_mpz);
    
    const char *private_key_file = "./keys/private.pem"; // should be the same for both client and server    
    // load public key
    uint8_t *public_key;
    size_t public_key_len;
    /* public key is malloced */
    get_public_key(
        private_key_file, 
        &public_key, 
        &public_key_len,
        "server"); // change password later
    
    // create the signed data
    uint8_t * signature;
    size_t signed_len;
    /* signature is malloced */
    sign_data_to_client(
        n0,
        n1,
        dhA_bytes,
        dhB_bytes,
        &signature,
        &signed_len,
        public_key,
        public_key_len, 
        private_key_file);

    // Concatenate signature and certificate
    size_t plaintext_len = signed_len + public_key_len;
    uint8_t * plaintext = malloc(plaintext_len+10);
    memcpy(plaintext, signature, signed_len);
    memcpy(plaintext + signed_len, public_key, public_key_len);

    uint8_t tag[AES_TAG_SIZE];
    uint8_t *ciphertext = malloc(plaintext_len+10);

    int ciphertext_len = aes_encrypt(
        (uint8_t *)plaintext,
        plaintext_len,
        NULL,
        0,
        (uint8_t *)session_key,
        (uint8_t *)n1,
        DH_NONCE_SIZE,
        ciphertext,
        tag);
    if(ciphertext_len != plaintext_len){
        printf("plaintext is %d while ciphertext is %d\n",(int)plaintext_len,ciphertext_len);
    }

    size_t payload_len = DH_KEY_SIZE + DH_NONCE_SIZE + AES_TAG_SIZE + ciphertext_len;
    uint8_t * server_payload = malloc(payload_len + 100); // 100 for padding

    memcpy(server_payload, dhB_bytes, DH_KEY_SIZE);
    memcpy(server_payload + DH_KEY_SIZE, n1, DH_NONCE_SIZE);
    memcpy(server_payload + DH_KEY_SIZE + DH_NONCE_SIZE , tag, AES_TAG_SIZE);
    memcpy(server_payload + DH_KEY_SIZE + DH_NONCE_SIZE + AES_TAG_SIZE, ciphertext, ciphertext_len);

    send(socket, server_payload, payload_len , 0);

    mpz_clears(dhB_mpz,b,NULL);
    free(server_payload);
    free(public_key);
    free(signature);
    free(plaintext);
    free(ciphertext);
    return master_key_bytes;
}

uint8_t * server_get_session_key(int socket,  uint8_t * session_key  /* assumed 32 length */,
 gmp_randstate_t state){
    uint8_t master_key[DH_KEY_SIZE];
    uint8_t n0[DH_NONCE_SIZE];
    uint8_t n1[DH_NONCE_SIZE];
    mpz_t prime;
    mpz_t dhA_mpz;
    receive_client_hello(socket, prime, dhA_mpz, state, n0,n1);
    send_server_hello(socket,prime,dhA_mpz,state,master_key,n0,n1,session_key);
    mpz_clears(prime,dhA_mpz,NULL);

    return session_key;
}

