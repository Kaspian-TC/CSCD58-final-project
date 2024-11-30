#include <gmp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include "helper_func.h"
#include "certificates.h"
#include <openssl/evp.h>


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
#define AES_IV_SIZE 12
#define AES_TAG_SIZE 16

void get_big_prime(mpz_t prime, gmp_randstate_t state)
{
    mpz_t random_number;
    mpz_init(random_number);
    mpz_urandomb(random_number, state, 2048);

    mpz_nextprime(prime, random_number);
    
    mpz_clear(random_number);
    
}

// AES Encryption
int aes_encrypt(unsigned char *plaintext, int plaintext_len,
 unsigned char *aad, int aad_len,
 unsigned char *key,
 unsigned char *iv, int iv_len,
 unsigned char *ciphertext,
 unsigned char *tag){
    EVP_CIPHER_CTX *ctx;
    int len;
    int ciphertext_len;

    // Create and initialize the context
    if(!(ctx = EVP_CIPHER_CTX_new())) 
        perror("Encryption error");
    
    // Initialize the encryption operation
    if(1 != EVP_EncryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL))
        perror("Encryption error");
    
    // Set IV length
    if(1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv_len, NULL))
        perror("Encryption error");

    // Initialize key and IV
    if(1 != EVP_EncryptInit_ex(ctx, NULL, NULL, key, iv))
        perror("Encryption error");

    // Provide any AAD data
    if(1 != EVP_EncryptUpdate(ctx, NULL, &len, aad, aad_len))
        perror("Encryption error");
    
    // Provide the message to be encrypted, and obtain the encrypted output
    if(1 != EVP_EncryptUpdate(ctx, ciphertext, &len, plaintext, plaintext_len))
        perror("Encryption error");
    ciphertext_len = len;

    // Finalize the encryption
    if(1 != EVP_EncryptFinal_ex(ctx, ciphertext + len, &len))
        perror("Encryption error");
    ciphertext_len += len;

    // Get the tag
    if(1 != EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_GET_TAG, 16, tag))
        perror("Encryption error");

    // Clean up
    EVP_CIPHER_CTX_free(ctx);

    return ciphertext_len;
}

// AES Decryption
int aes_decrypt(unsigned char *ciphertext, int ciphertext_len,
 unsigned char *aad, int aad_len,
 unsigned char *tag,
 unsigned char *key,
 unsigned char *iv, int iv_len,
 unsigned char *plaintext){
    EVP_CIPHER_CTX *ctx;
    int len;
    int plaintext_len;
    int ret;

    // Create and initialize the context
    if(!(ctx = EVP_CIPHER_CTX_new())) 
        perror("Decryption error");
    
    // Initialize the decryption operation
    if(!EVP_DecryptInit_ex(ctx, EVP_aes_256_gcm(), NULL, NULL, NULL))
        perror("Decryption error");
    
    // Set IV length
    if(!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv_len, NULL))
        perror("Decryption error");

    // Initialize key and IV
    if(!EVP_DecryptInit_ex(ctx, NULL, NULL, key, iv))
        perror("Decryption error");

    // Provide any AAD data
    if(!EVP_DecryptUpdate(ctx, NULL, &len, aad, aad_len))
        perror("Decryption error");
    
    // Provide the message to be decrypted, and obtain the plaintext output
    if(!EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
        perror("Decryption error");
    plaintext_len = len;

    // Set expected tag value
    if(!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, tag))
        perror("Decryption error");

    // Finalize the decryption
    ret = EVP_DecryptFinal_ex(ctx, plaintext + len, &len);

    // Clean up
    EVP_CIPHER_CTX_free(ctx);

    if(ret > 0){
        /* Success */
        plaintext_len += len;
        return plaintext_len;
    } else {
        /* Verify failed */
        return -1;
    }
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

static void get_public_key_file(
    const char *server_public_key_file, 
    char **server_public_key, 
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
    *server_public_key = (char *) malloc(file_size);
    *server_public_key_len = fread(*server_public_key, 1, file_size, server_key_file);


    fclose(server_key_file);
}


/* Helper function for receive_server_hello */
static int is_server_response_sign_valid(
    char* n0,char* n1,char* dhA_bytes,char* dhB_bytes,char* signature, long signature_len, char * public_key_file, size_t public_key_len){

    // get the server's public key
    const char *server_public_key_file = "./keys/private.pem"; // here it should call the "CA"

    // load in the server public key from file
    char *server_public_key;
    size_t server_public_key_len;
    /* server_public_key is malloced */
    get_public_key_file(
        server_public_key_file, &server_public_key,
         &server_public_key_len);
    // compare read in public key with the public key from the server
    if (strncmp(server_public_key, public_key_file, public_key_len) != 0){
        free(server_public_key);
        return 0;
    }
    // make data n0 + n1 + dhA + dhB + certificate
    long data_len = DH_NONCE_SIZE*2 + DH_KEY_SIZE*2 + server_public_key_len;
    char data[data_len];
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

char * receive_server_hello(int socket, 
mpz_t prime, 
mpz_t dhA_mpz, 
mpz_t a,
gmp_randstate_t state, 
char * master_key_bytes,
char * n0, 
char * n1){
    // wait to receive a response from the server
    char client_payload[DH_KEY_SIZE + DH_NONCE_SIZE + AES_IV_SIZE + AES_TAG_SIZE + 1024]; // 1024 is for padding
    int payload_len = recv(socket, client_payload, 
     sizeof(client_payload), 0);
    printf("[CLIENT] Received payload from server of size %d\n", payload_len);
    char dhB_bytes[DH_KEY_SIZE];
    char iv[AES_IV_SIZE];
    char tag[AES_TAG_SIZE];
    long ciphertext_len = payload_len - DH_KEY_SIZE - DH_NONCE_SIZE - AES_IV_SIZE - AES_TAG_SIZE;
    char ciphertext[ciphertext_len];

    memcpy(dhB_bytes, client_payload, DH_KEY_SIZE);
    memcpy(n1, client_payload + DH_KEY_SIZE, DH_NONCE_SIZE);
    memcpy(iv, client_payload + DH_KEY_SIZE + DH_NONCE_SIZE, AES_IV_SIZE);
    memcpy(tag, client_payload + DH_KEY_SIZE + DH_NONCE_SIZE + AES_IV_SIZE, AES_TAG_SIZE);
    memcpy(ciphertext, client_payload + DH_KEY_SIZE + DH_NONCE_SIZE + AES_IV_SIZE + AES_TAG_SIZE, ciphertext_len);

    //compute session key
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

    // decrypt the ciphertext and extract the signature, public key
    unsigned char *plaintext = malloc(ciphertext_len);
    int plaintext_len = aes_decrypt(
        (unsigned char *) ciphertext,
        ciphertext_len,
        NULL, 0,
        (unsigned char *) tag,
        (unsigned char *) session_key,
        (unsigned char *) iv,
        AES_IV_SIZE,
        plaintext);

    long signature_len = 256;
    char signature[signature_len];
    long server_public_key_len = plaintext_len - signature_len;
    char server_public_key[server_public_key_len];   
    memcpy(signature, plaintext, signature_len);
    memcpy(server_public_key, plaintext + signature_len, server_public_key_len);

    printf("[CLIENT] Signature: ");
    print_bytes(signature, signature_len);
    printf("[CLIENT] Server public key: ");
    print_bytes(server_public_key, server_public_key_len);
    // print out server_public_key

    char dhA_bytes[DH_KEY_SIZE];
    mpz_export(dhA_bytes, NULL, 1, 1, 1, 0, dhA_mpz);
    if (!is_server_response_sign_valid(n0,n1,dhA_bytes,dhB_bytes,signature, signature_len, server_public_key, server_public_key_len)){
        perror("[CLIENT] Server response is invalid\n");   
    }

    // Clean up
    free(plaintext);

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

static void sign_data_to_client(
    char* n0,
    char* n1,
    char* dhA_bytes,
    char* dhB_bytes,
    char** signature, 
    size_t * signature_len,
    char *public_key, 
    size_t public_key_len,
    const char *private_key_file){

    // make data n0 + n1 + dhA + dhB + certificate
    char data[DH_NONCE_SIZE*2 + DH_KEY_SIZE*2 + public_key_len];
    memcpy(data, n0, DH_NONCE_SIZE);
    memcpy(data + DH_NONCE_SIZE, n1, DH_NONCE_SIZE);
    memcpy(data + DH_NONCE_SIZE*2, dhA_bytes, DH_KEY_SIZE);
    memcpy(data + DH_NONCE_SIZE*2 + DH_KEY_SIZE, dhB_bytes, DH_KEY_SIZE);
    memcpy(data + DH_NONCE_SIZE*2 + DH_KEY_SIZE*2, public_key, public_key_len);
    
    long data_len = DH_NONCE_SIZE*2 + DH_KEY_SIZE*2 + public_key_len;

    sign_data(private_key_file, data, data_len,signature,signature_len,"server");
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

    // get n1 nonce
    get_random_bytes(n1, DH_NONCE_SIZE, state);

    // get the session key for aes
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
    
    const char *private_key_file = "./keys/private.pem"; // should be the same for both client and server    
    // load public key
    char *public_key;
    size_t public_key_len;
    /* public key is malloced */
    get_public_key(
        private_key_file, 
        &public_key, 
        &public_key_len,
        "server"); // change password later
    
    // create the signed data
    char * signature;
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
    char * plaintext = malloc(plaintext_len);
    memcpy(plaintext, signature, signed_len);
    memcpy(plaintext + signed_len, public_key, public_key_len);

    printf("[SERVER] signature: ");
    print_bytes(signature, signed_len);
    printf("[SERVER] public key: ");
    print_bytes(public_key, public_key_len);

    unsigned char iv[AES_IV_SIZE];
    unsigned char tag[AES_TAG_SIZE];
    unsigned char *ciphertext = malloc(plaintext_len);

    get_random_bytes(iv, AES_IV_SIZE, state);

    int ciphertext_len = aes_encrypt(
        (unsigned char *)plaintext,
        plaintext_len,
        NULL,
        0,
        (unsigned char *)session_key,
        iv,
        AES_IV_SIZE,
        ciphertext,
        tag);

    printf("[SERVER] Ciphertext");
    print_bytes(ciphertext, ciphertext_len);
    printf("size of iv: %ld\n", sizeof(iv));
    printf("size of tag: %ld\n", sizeof(tag));

    size_t payload_len = DH_KEY_SIZE + DH_NONCE_SIZE + AES_IV_SIZE + AES_TAG_SIZE + ciphertext_len;
    char * server_payload = malloc(payload_len);

    memcpy(server_payload, dhB_bytes, DH_KEY_SIZE);
    memcpy(server_payload + DH_KEY_SIZE, n1, DH_NONCE_SIZE);
    memcpy(server_payload + DH_KEY_SIZE + DH_NONCE_SIZE, iv, AES_IV_SIZE);
    memcpy(server_payload + DH_KEY_SIZE + DH_NONCE_SIZE + AES_IV_SIZE, tag, AES_TAG_SIZE);
    memcpy(server_payload + DH_KEY_SIZE + DH_NONCE_SIZE + AES_IV_SIZE + AES_TAG_SIZE, ciphertext, ciphertext_len);

    printf("[SERVER] Sending payload to client of size %ld\n", payload_len);
    send(socket, server_payload, payload_len , 0);
    
    mpz_clears(dhB_mpz,b,master_key,NULL);
    free(server_payload);
    free(public_key);
    free(signature);
    free(plaintext);
    free(ciphertext);
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

