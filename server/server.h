#ifndef SERVER_H
#define SERVER_H

#define SERVER_PORT 5432
#define MAX_LINE 256

/* Global storage */
extern char global_data[MAX_LINE];

/* Function Prototypes */
void store_data(const char* payload);
void retrieve_data(int client_sock);
void handle_client(int client_sock);

#endif
