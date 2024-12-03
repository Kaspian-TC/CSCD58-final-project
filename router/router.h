#include <gmp.h>
/* Defines */
#define SERVER_PORT 5432
#define MAX_LINE 256
#define NUM_SERVERS 3
#define BUFFER_SIZE 1024


/* Function Headers */
void retrieve_from_servers(int);
void forward_to_server(const char*, const char*, char*,gmp_randstate_t);
void distribute_to_server(const char*);
int main();