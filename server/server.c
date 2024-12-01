#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include "server.h"

/* Global pointer to the head of the blockchain */
Block* blockchain_head = NULL;

/* Function to store data in a new block */
void store_data(const char* payload) {
    Block* new_block = (Block*)malloc(sizeof(Block));
    if (!new_block) {
        perror("[SERVER] Memory allocation failed");
        return;
    }

    strncpy(new_block->data, payload, MAX_LINE - 1);
    new_block->data[MAX_LINE - 1] = '\0';  // Ensure null-termination
    new_block->next = NULL;

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
}

/* Function to retrieve all data in the blockchain */
void retrieve_data(int client_sock) {
    char response[MAX_LINE * 10] = {0};  // Adjust size as needed for large data
    Block* current = blockchain_head;

    if (!current) {
        snprintf(response, sizeof(response), "NO_DATA");
    } else {
        while (current) {
            strncat(response, current->data, sizeof(response) - strlen(response) - 1);
            strncat(response, "\n", sizeof(response) - strlen(response) - 1);
            current = current->next;
        }
    }

    send(client_sock, response, strlen(response), 0);
    printf("[SERVER] Sent blockchain data:\n%s", response);
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
