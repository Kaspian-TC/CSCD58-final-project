#include <gmp.h>
#include <stdio.h>	
#include <string.h>
#include <openssl/evp.h>
#include <openssl/kdf.h>
#include <openssl/params.h>
#include <openssl/err.h>
#include <sys/socket.h>
#include "helper_func.h"

void get_random_bytes(char *bytes, int length,gmp_randstate_t state)
{
    mpz_t random_number;
    mpz_init2(random_number, 8 * length);
    mpz_urandomb(random_number, state, 8 * length);
    mpz_export(bytes, NULL, 1, 1, 1, 0, random_number);
}

void initialize_values(const mpz_t prime, mpz_t dh, mpz_t secret,
 gmp_randstate_t state){
    mpz_t g;
    mpz_inits(secret,g,NULL);
    
    mpz_init2(dh,DH_NUM_BITS);
    
    gmp_printf("Prime number: %Zd\n", prime);

    mpz_urandomb(secret, state, DH_NUM_BITS);
    // compute dh = g^secret mod p
    // initialize g 
    mpz_set_ui(g,DH_G);

    mpz_powm(dh,g,secret,prime); // dhA = g^a mod p
    mpz_clear(g);
}

// create the session key using HKDF 
unsigned char *create_session_key(unsigned char *master_key, unsigned char *salt, unsigned char* session_key)
{
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
    *p++ = OSSL_PARAM_construct_octet_string("key", master_key, DH_KEY_SIZE);
    *p++ = OSSL_PARAM_construct_octet_string("salt", salt, DH_NONCE_SIZE * 2);
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
    /* printf("Session key: ");
    for (int i = 0; i < AES_KEY_SIZE; i++)
    {
        printf("%02x", session_key[i]);
    }
    printf("length of session key: %ld\n", sizeof(session_key)); */
    return session_key;
}

/* assumed salt is DH_NONCE_SIZE * 2; n0, n1 are DH_NONCE_SIZE */
void create_salt(char *salt, char *n0, char *n1){
    memcpy(salt, n0, DH_NONCE_SIZE);
    memcpy(salt + DH_NONCE_SIZE, n1, DH_NONCE_SIZE);
}

void print_bytes(char *bytes, int length)
{
	for (int i = 0; i < length; i++)
	{
		printf("%d ", bytes[i]);
	}
	printf("\n");
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
        perror("Decryption error: initialization");
    
    // Set IV length
    if(!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_IVLEN, iv_len, NULL))
        perror("Decryption error: setting IV length");

    // Initialize key and IV
    if(!EVP_DecryptInit_ex(ctx, NULL, NULL, key, iv))
        perror("Decryption error: Key and IV initialization");

    // Provide any AAD data
    if(!EVP_DecryptUpdate(ctx, NULL, &len, aad, aad_len))
        perror("Decryption error: AAD data");
    
    // Provide the message to be decrypted, and obtain the plaintext output
    if(!EVP_DecryptUpdate(ctx, plaintext, &len, ciphertext, ciphertext_len))
        perror("Decryption error");
    plaintext_len = len;

    // Set expected tag value
    if(!EVP_CIPHER_CTX_ctrl(ctx, EVP_CTRL_GCM_SET_TAG, 16, tag))
        perror("Decryption error: setting tag");

    // Finalize the decryption
    ret = EVP_DecryptFinal_ex(ctx, plaintext + len, &len);

    // Clean up
    EVP_CIPHER_CTX_free(ctx);

    if(ret > 0){
        plaintext_len += len;
        return plaintext_len;
    } else {
        return -1;
    }
}

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

int send_encypted_data(int socket, char *data, int data_len, char *session_key, gmp_randstate_t state){
    unsigned char tag[AES_TAG_SIZE];
    char nonce[DH_NONCE_SIZE];
    // generate a nonce
    get_random_bytes(nonce, DH_NONCE_SIZE, state);
    // Encrypt the data
    unsigned char *ciphertext = malloc(data_len);
    int ciphertext_len = aes_encrypt(
        (unsigned char *)data,
        data_len,
        NULL,
        0,
        (unsigned char *)session_key,
        (unsigned char *)nonce,
        DH_NONCE_SIZE,
        ciphertext,
        (unsigned char *)tag);
    // construct the payload (nonce + tag + ciphertext)
    int payload_len = DH_NONCE_SIZE + AES_TAG_SIZE + ciphertext_len;
    char payload[payload_len];
    memcpy(payload, nonce, DH_NONCE_SIZE);
    memcpy(payload + DH_NONCE_SIZE, tag, AES_TAG_SIZE);
    memcpy(payload + DH_NONCE_SIZE + AES_TAG_SIZE, ciphertext, ciphertext_len);

    free(ciphertext);
    // send the payload
    send(socket, payload, payload_len, 0);

    return payload_len;

}
char * receive_encypted_data(int socket, int * data_len, char *session_key){
    char payload[DH_NONCE_SIZE + AES_TAG_SIZE + 1024];
    int payload_len = recv(socket, payload, DH_NONCE_SIZE + AES_TAG_SIZE + 1024, 0);
    printf("Received payload of size %d\n", payload_len);
    char nonce[DH_NONCE_SIZE];
    char tag[AES_TAG_SIZE];
    long ciphertext_len = payload_len - DH_NONCE_SIZE - AES_TAG_SIZE;
    char ciphertext[ciphertext_len];

    // add the nonce, tag, and ciphertext to payload
    memcpy(nonce, payload, DH_NONCE_SIZE);
    memcpy(tag, payload + DH_NONCE_SIZE, AES_TAG_SIZE);
    memcpy(ciphertext, payload + DH_NONCE_SIZE + AES_TAG_SIZE, ciphertext_len);

    // decrypt the ciphertext
    unsigned char *plaintext = malloc(ciphertext_len);
    int plaintext_len = aes_decrypt(
        (unsigned char *)ciphertext,
        ciphertext_len,
        NULL, 0,
        (unsigned char *)tag,
        (unsigned char *)session_key,
        (unsigned char *)nonce,
        DH_NONCE_SIZE,
        plaintext);
    
    
    *data_len = plaintext_len;
    return (char *) plaintext;
}