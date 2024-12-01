#include <openssl/pem.h>
#include <openssl/err.h>
#include <openssl/evp.h>
#include <openssl/x509.h>
#include <stdio.h>
#include <string.h>

void handle_openssl_error() {
    ERR_print_errors_fp(stderr);
    exit(EXIT_FAILURE);
}

void free_Mdctx_private(EVP_MD_CTX *mdctx, EVP_PKEY *private_key){
    EVP_MD_CTX_free(mdctx);
    EVP_PKEY_free(private_key);
}

int sign_data(const char *private_key_path /* string */,const  uint8_t *data, long data_len, uint8_t ** signature, size_t * signed_len, uint8_t * password) {

    // Load the private key FROM A FILE
    FILE *key_file = fopen(private_key_path, "r");
    EVP_PKEY *private_key = PEM_read_PrivateKey(key_file, NULL, NULL, password);
    fclose(key_file);
    if(!private_key){
        perror("Unable to read private key");
        handle_openssl_error();
    }

    // Create a context for signing
    EVP_MD_CTX *digest_context = EVP_MD_CTX_new();

    // Sets the algorithm for digest_context (with private key)
    EVP_DigestSignInit(digest_context, NULL, EVP_sha256(), NULL, private_key);

    // Add data to be signed
    EVP_DigestSignUpdate(digest_context, data, data_len);

    // Finalize the signature (start by getting length)
    EVP_DigestSignFinal(digest_context, NULL, signed_len);

    // Assigns value to signature
    *signature = malloc(*signed_len);
    EVP_DigestSignFinal(digest_context, (uint8_t*) *signature, signed_len);
    printf("Signature created successfully\n");
    
    // Clean up
    EVP_MD_CTX_free(digest_context);
    EVP_PKEY_free(private_key);

    /* printf("Signature: ");
    for (int i = 0; i < *signed_len; i++)
    {
        printf("%02x", (*signature)[i]);
    }
    printf("\n"); */
    return EXIT_SUCCESS;
}
int validate_signed_data(const char *public_key_file, const uint8_t *data, long data_len, uint8_t * signature, size_t sig_len){    
    int returnval;

    // Load public key
    FILE *key_file = fopen(public_key_file, "r");
    if (!key_file) {
        perror("Unable to open public key file");
        return 1;
    }
    EVP_PKEY *public_key = PEM_read_PUBKEY(key_file, NULL, NULL, NULL);
    fclose(key_file);
    if (!public_key) {
        handle_openssl_error();
    }

    // Create a verification context
    EVP_MD_CTX *digest_context = EVP_MD_CTX_new();
    
    // Initialize the verification context with the public key
    EVP_DigestVerifyInit(digest_context, NULL, EVP_sha256(), NULL, public_key);
    // Provide the data to verify
    EVP_DigestVerifyUpdate(digest_context, data, data_len);
    
    // Verify the signature
    returnval = EVP_DigestVerifyFinal(digest_context, (uint8_t *)signature, sig_len);
    
    // Cleanup
    EVP_MD_CTX_free(digest_context);
    EVP_PKEY_free(public_key);
    return returnval;
}

void get_public_key(const char *private_key_path, uint8_t **public_key, size_t *public_key_len,uint8_t * password){
    // Load the private key FROM A FILE
    FILE *key_file = fopen(private_key_path, "r");
    // load private key 
    EVP_PKEY *private_key = PEM_read_PrivateKey(key_file, NULL, NULL, password);
    fclose(key_file);
    if(!private_key){
        perror("Unable to read private key");
        handle_openssl_error();
    }
    // basic input output type for open ssl, set to save to memory
    BIO *bio = BIO_new(BIO_s_mem());
    PEM_write_bio_PUBKEY(bio, private_key);

    // get A POINTER TO the public key in bio (does not allocate)
    uint8_t *pem_data = NULL;
    long pem_len = BIO_get_mem_data(bio, &pem_data);
    *public_key = (uint8_t *) malloc(pem_len);
    *public_key_len = pem_len;
    memcpy(*public_key, pem_data, pem_len);

    BIO_free(bio);
    EVP_PKEY_free(private_key);

    return;
}
