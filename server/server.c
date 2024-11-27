#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include "server.h"

// Function to store data locally in separate files for each block
void store_data(const char* payload) {
    static int block_id = 1; // Incremental block IDs
    char filename[64];
    snprintf(filename, sizeof(filename), "block%d.txt", block_id);

    FILE* fp = fopen(filename, "w");
    if (fp == NULL) {
        perror("[STORAGE] Failed to create block file");
        return;
    }

    fprintf(fp, "%s\n", payload);
    fclose(fp);

    printf("[STORAGE] Stored block in %s\n", filename);
    block_id++;
}

// Function to retrieve all stored blocks
void retrieve_data(int socket) {
    char buf[MAX_LINE];
    char final_payload[MAX_LINE * 100] = {0};

    for (int i = 1; i <= 100; i++) { // Assume a maximum of 100 blocks
        char filename[64];
        snprintf(filename, sizeof(filename), "block%d.txt", i);

        FILE* fp = fopen(filename, "r");
        if (fp == NULL) break; // Stop if no more blocks

        while (fgets(buf, sizeof(buf), fp)) {
            strcat(final_payload, buf);
        }
        fclose(fp);
    }

    send(socket, final_payload, strlen(final_payload), 0);
    printf("[STORAGE] Sent all stored blocks.\n");
}

int main() {
    struct sockaddr_in sin;
    char buf[MAX_LINE];
    int len, s, new_s;

    // Build address data structure
    bzero((char*)&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(SERVER_PORT);

    if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("[STORAGE] Socket creation failed");
        exit(1);
    }

    if ((bind(s, (struct sockaddr*)&sin, sizeof(sin))) < 0) {
        perror("[STORAGE] Binding failed");
        close(s);
        exit(1);
    }

    listen(s, 5);
    printf("[STORAGE] Listening on port %d...\n", SERVER_PORT);

    while (1) {
        if ((new_s = accept(s, (struct sockaddr*)&sin, &len)) < 0) {
            perror("[STORAGE] Accept failed");
            close(s);
            exit(1);
        }

        len = recv(new_s, buf, sizeof(buf) - 1, 0);
        buf[len] = '\0';

        if (strcmp(buf, "GET_DATA") == 0) {
            printf("[STORAGE] Received data retrieval request\n");
            retrieve_data(new_s);
        } else {
            printf("[STORAGE] Received data: %s\n", buf);
            store_data(buf);
        }

        close(new_s);
    }

    close(s);
    return 0;
}