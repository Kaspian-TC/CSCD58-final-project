#include <gmp.h>
void get_random_bytes(char *bytes, int length,gmp_randstate_t state);
void initialize_values(mpz_t prime, mpz_t dhA_mpz, mpz_t a,
 gmp_randstate_t state);