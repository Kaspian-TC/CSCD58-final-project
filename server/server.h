#ifndef SERVER_H
#define SERVER_H

#define MAX_LINE 256

typedef struct Block {
    char data[MAX_LINE];
    struct Block* next;
} Block;

/* Global pointer to the head of the chain */
extern Block* blockchain_head;

/* Function Headers */
void store_data(const char* payload);
void retrieve_data(int client_sock);
void free_blockchain();

#endif
