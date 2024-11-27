/* Defines */
#define SERVER_PORT 5432
#define MAX_LINE 256
#define NUM_SERVERS 3

/* Storage server hostnames */
const char* SERVER_IPS[] = {"server1", "server2", "server3"};

/* Function Headers */
void retrieve_from_servers(int);
void forward_to_server(const char*, const char*);
void distribute_to_server(const char*);
int main();