#include <gmp.h>
void get_big_prime(mpz_t prime, gmp_randstate_t state);

// client tls functions
char * client_get_master_key(int socket, char * master_key, gmp_randstate_t state);

// server tls functions
void receive_client_hello(int socket, mpz_t prime, mpz_t dhA_mpz, gmp_randstate_t state);
char * send_server_hello(int socket,
 mpz_t prime, 
 mpz_t dhA_mpz, 
 gmp_randstate_t state, char * master_key_bytes);
char * server_get_master_key(int socket, char * master_key /* assumed 256 length */,
 gmp_randstate_t state);