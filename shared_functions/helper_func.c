#include <gmp.h>
#include <stdio.h>	
#include <string.h>
#include <openssl/evp.h>
#include <openssl/kdf.h>
#include <openssl/params.h>
#include <openssl/err.h>


#define BUFFER_SIZE 1048576 // 1 MB
#define DH_NUM_BITS 2048
#define DH_G 5
#define DH_KEY_SIZE 256
#define DH_NONCE_SIZE 16
#define AES_KEY_SIZE 32

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
char *create_session_key(unsigned char *master_key, unsigned char *salt, unsigned char* session_key)
{
    // static unsigned char session_key[AES_KEY_SIZE];
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
