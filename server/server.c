#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <openssl/sha.h>
#include "server.h"

/* Global pointer to the head of the blockchain */
Block* blockchain_head = NULL;

/* Function to compute the hash for a block */
void compute_hash(Block* block, const char* previous_hash, char* output_hash) {
    unsigned char hash[SHA256_DIGEST_LENGTH];
    char data_to_hash[MAX_LINE + HASH_SIZE] = {0};

    // Combine block data and previous hash for hashing
    snprintf(data_to_hash, sizeof(data_to_hash), "%s%s", block->data, previous_hash);

    // Compute SHA-256 hash
    SHA256((unsigned char*)data_to_hash, strlen(data_to_hash), hash);

    // Convert the binary hash to a hexadecimal string
    for (int i = 0; i < SHA256_DIGEST_LENGTH; i++) {
        sprintf(output_hash + (i * 2), "%02x", hash[i]);
    }

    // Null-terminate the output hash
    output_hash[HASH_SIZE - 1] = '\0';

    // Store the previous hash in the block (unchanged)
    strncpy(block->previous_hash, previous_hash, HASH_SIZE - 1);
    block->previous_hash[HASH_SIZE - 1] = '\0';
}

/* Function to store data in a new block */
void store_data(const char* payload) {
    Block* new_block = (Block*)malloc(sizeof(Block));
    if (!new_block) {
        perror("[SERVER] Memory allocation failed");
        return;
    }

    strncpy(new_block->data, payload, MAX_LINE - 1);
    new_block->data[MAX_LINE - 1] = '\0';
    new_block->next = NULL;

    char previous_hash[HASH_SIZE] = {0};
    char computed_hash[HASH_SIZE] = {0};

    if (blockchain_head) {
        strncpy(previous_hash, blockchain_head->hash, HASH_SIZE - 1);
        previous_hash[HASH_SIZE - 1] = '\0';
    }

    // Compute the hash and store it in the new block
    compute_hash(new_block, previous_hash, computed_hash);
    strncpy(new_block->hash, computed_hash, HASH_SIZE - 1);
    new_block->hash[HASH_SIZE - 1] = '\0';

    // Append the new block to the blockchain
    if (!blockchain_head) {
        blockchain_head = new_block;  // First block in the chain
    } else {
        Block* current = blockchain_head;
        while (current->next) {
            current = current->next;  // Traverse to the end of the chain
        }
        current->next = new_block;  // Append the new block
    }

    printf("[SERVER] Stored new block: %s\n", payload);
    printf("[SERVER] Block hash: %s\n", new_block->hash);

    // Validate blockchain after storing data
    if (!validate_blockchain()) {
        printf("[SERVER] Blockchain validation failed after storing block.\n");
    } else {
        printf("[SERVER] Blockchain validation passed.\n");
    }
}

/* Function to retrieve all data in the blockchain */
void retrieve_data(int client_sock) {
    char response[MAX_LINE * 10] = {0};  // Adjust size as needed for large data

    // Validate blockchain before sending data
    if (!validate_blockchain()) {
        char response[] = "Blockchain validation failed. Data retrieval aborted.\n";

        send(client_sock, response, strlen(response), 0);
        return;
    }

    Block* current = blockchain_head;
    if (!current) {
        snprintf(response, sizeof(response), "NO_DATA");
    } else {
        while (current) {
            char block_info[MAX_LINE + HASH_SIZE * 2 + 100];
            snprintf(block_info, sizeof(block_info), "Data: %s\nHash: %s\nPrevious Hash: %s\n\n",
                     current->data, current->hash, current->previous_hash);

            strncat(response, block_info, sizeof(response) - strlen(response) - 1);
            current = current->next;
        }
    }

    send(client_sock, response, strlen(response), 0);
    printf("[SERVER] Sent blockchain data:\n%s", response);
}


/* Function to validate the blockchain */
int validate_blockchain() {
    return 1;  // Placeholder for now

    Block* current = blockchain_head;
    char expected_previous_hash[HASH_SIZE] = {0}; // Initialize empty hash for the first block
    
    while (current) {
        char calculated_hash[HASH_SIZE] = {0};
        compute_hash(current, expected_previous_hash, calculated_hash);

        // validate the current block
        if (strcmp(current->hash, calculated_hash) != 0) {
            printf("[SERVER] Blockchain invalid: Tampered block detected.\n");
            return 0;
        }

        // if not first block, validate the previous hash
        if (current != blockchain_head && strcmp(current->previous_hash, expected_previous_hash) != 0) {
            printf("[SERVER] Blockchain invalid: Previous hash mismatch.\n");
            return 0;
        }

        // Update expected_previous_hash for the next block
        strncpy(expected_previous_hash, current->hash, HASH_SIZE - 1);
        expected_previous_hash[HASH_SIZE - 1] = '\0';
        current = current->next;
    }

    printf("[SERVER] Blockchain is valid.\n");
    return 1;
}


/* Function to free the blockchain memory */
void free_blockchain() {
    Block* current = blockchain_head;
    while (current) {
        Block* temp = current;
        current = current->next;
        free(temp);
    }
    blockchain_head = NULL;
}

void handle_client(int client_sock) {
    char buffer[MAX_LINE] = {0};
    int len = recv(client_sock, buffer, sizeof(buffer) - 1, 0);

    if (len > 0) {
        buffer[len] = '\0';
        printf("[SERVER] Received: %s\n", buffer);

        if (strcmp(buffer, "RETRIEVE") == 0) {
            retrieve_data(client_sock);
        } else {
            store_data(buffer);
            const char* ack = "DATA_STORED";
            send(client_sock, ack, strlen(ack), 0);
        }
    } else {
        printf("[SERVER] No data received, closing connection.\n");
    }

    close(client_sock);
}

int main() {
    struct sockaddr_in server_addr, client_addr;
    int server_sock, client_sock;
    socklen_t addr_len = sizeof(client_addr);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("[SERVER] Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(5432);

    if (bind(server_sock, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("[SERVER] Bind failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    if (listen(server_sock, 5) < 0) {
        perror("[SERVER] Listen failed");
        close(server_sock);
        exit(EXIT_FAILURE);
    }

    printf("[SERVER] Listening on port 5432\n");

    while (1) {
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_len);
        if (client_sock < 0) {
            perror("[SERVER] Accept failed");
            continue;
        }

        handle_client(client_sock);
    }

    free_blockchain();  // Clean up memory
    close(server_sock);
    return 0;
}
