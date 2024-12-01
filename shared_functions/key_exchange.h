#include <gmp.h>
#include <stdint.h>
void get_big_prime(mpz_t prime, gmp_randstate_t state);

// client tls functions
uint8_t * client_get_session_key(int socket, uint8_t * session_key, gmp_randstate_t state);

// server tls functions
void receive_client_hello(int socket, mpz_t prime, mpz_t dhA_mpz, gmp_randstate_t state,uint8_t* n0,uint8_t* n1);
uint8_t * send_server_hello(int socket,
 mpz_t prime, 
 mpz_t dhA_mpz, 
 gmp_randstate_t state, uint8_t * master_key_bytes,
 uint8_t * n0, uint8_t * n1);
uint8_t * server_get_session_key(int socket, uint8_t * session_key /* assumed 256 length */,
 gmp_randstate_t state);
