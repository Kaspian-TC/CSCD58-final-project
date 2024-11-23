#include <gmp.h>
/* FUNCTION DECLARATIONS */
void send_client_hello(int socket, 
mpz_t prime, 
mpz_t dhA_mpz, 
mpz_t a);

void initialize_values(mpz_t prime, mpz_t dhA_mpz, mpz_t a,
 gmp_randstate_t state);

