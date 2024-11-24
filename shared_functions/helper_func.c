#include <gmp.h>

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

void initialize_values(mpz_t prime, mpz_t dh, mpz_t secret,
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