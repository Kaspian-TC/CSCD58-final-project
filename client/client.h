#ifndef CLIENT_H
#define CLIENT_H

#define SERVER_PORT 5432
#define MAX_LINE 256

/* Function Headers */
void store_data(int sockfd);
void retrieve_data(int sockfd);

#endif
