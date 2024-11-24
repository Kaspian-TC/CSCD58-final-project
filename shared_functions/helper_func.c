#include <gmp.h>
void get_random_bytes(char *bytes, int length,gmp_randstate_t state)
{
    mpz_t random_number;
    mpz_init2(random_number, 8 * length);
    mpz_urandomb(random_number, state, 8 * length);
    mpz_export(bytes, NULL, 1, 1, 1, 0, random_number);
}