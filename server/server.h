#ifndef SERVER_H
#define SERVER_H

#include <openssl/sha.h>  // Include OpenSSL's SHA-256 library

#define MAX_LINE 256
#define HASH_SIZE 65  // SHA-256 hash in hex + null terminator

typedef struct Block {
    char data[MAX_LINE];
    char hash[HASH_SIZE];
    char previous_hash[HASH_SIZE];
    struct Block* next;
} Block;

/* Global pointer to the head of the chain */
extern Block* blockchain_head;

/* Function Headers */
void store_data(const char* payload);
void retrieve_data(int client_sock);
void free_blockchain();
void compute_hash(Block* block, const char* previous_hash, char* output_hash);
int validate_blockchain();

#endif
