#include <gmp.h>

void get_random_bytes(char *bytes, int length,gmp_randstate_t state);
void initialize_values(mpz_t prime, mpz_t dhA_mpz, mpz_t a,
 gmp_randstate_t state);
void print_bytes(char *bytes, int length);
char *create_session_key(char *master_key, char *salt, char* session_key);
void create_salt(char *salt, char *n0, char *n1);