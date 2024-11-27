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

int sign_test_data(const char *private_key_file /* string */,const  char *data, long data_len, char ** signature, size_t * signed_len) {

    // Load the private key FROM A FILE
    FILE *key_file = fopen(private_key_file, "r");
    EVP_PKEY *private_key = PEM_read_PrivateKey(key_file, NULL, NULL, NULL);
    fclose(key_file);

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
    EVP_DigestSignFinal(digest_context, (unsigned char*) *signature, signed_len);
    printf("Signature: ");
    for (int i = 0; i < *signed_len; i++)
    {
        printf("%02x", (*signature)[i]);
    }
    printf("\n");
    printf("Signature created successfully\n");
    // write signature to file called signature
    FILE *signature_file = fopen("signature", "w");
    fwrite(*signature, 1, *signed_len, signature_file);
    
    // Clean up
    EVP_MD_CTX_free(digest_context);
    EVP_PKEY_free(private_key);

    printf("Signature: ");
    for (int i = 0; i < *signed_len; i++)
    {
        printf("%02x", (*signature)[i]);
    }
    printf("\n");
    return EXIT_SUCCESS;
}
int check_signed_data(const char *public_key_file, const char *data, long data_len, char * signature, size_t sig_len){    
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
    if (EVP_DigestVerifyFinal(digest_context, (unsigned char *)signature, sig_len) == 1) {
        returnval = 1;
        printf("Signature is valid.\n");
    } else {
        returnval = 0;
        printf("Signature is invalid.\n");
    }

    // Cleanup
    EVP_MD_CTX_free(digest_context);
    EVP_PKEY_free(public_key);
    return returnval;
}
