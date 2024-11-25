#include <gmp.h>
/* FUNCTION DECLARATIONS */
void receive_client_hello(int socket, gmp_randstate_t state);

unsigned char *create_session_key(unsigned char *master_key, unsigned char *salt);