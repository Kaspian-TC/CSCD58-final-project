#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include "server.h"

char global_data[MAX_LINE] = "";

void store_data(const char* payload) {
    strncpy(global_data, payload, MAX_LINE - 1);
    global_data[MAX_LINE - 1] = '\0';
    printf("[SERVER] Stored: %s\n", global_data);
}

void retrieve_data(int client_sock) {
    if (strlen(global_data) > 0) {
        send(client_sock, global_data, strlen(global_data), 0);
        printf("[SERVER] Sent: %s\n", global_data);
    } else {
        const char* no_data = "NO_DATA";
        send(client_sock, no_data, strlen(no_data), 0);
        printf("[SERVER] No data to send.\n");
    }
}

void handle_client(int client_sock) {
    char buffer[MAX_LINE] = {0};
    int len = recv(client_sock, buffer, sizeof(buffer) - 1, 0);

    if (len > 0) {
        buffer[len] = '\0';
        if (strcmp(buffer, "RETRIEVE") == 0) {
            retrieve_data(client_sock);
        } else {
            store_data(buffer);
            const char* ack = "DATA_STORED";
            send(client_sock, ack, strlen(ack), 0);
        }
    }

    close(client_sock);
}

int main() {
    printf("[SERVER] Starting server process...\n");
    // flush stdout to make sure the message is displayed
    fflush(stdout);

    int server_sock, client_sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    server_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (server_sock < 0) {
        perror("[SERVER] Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_addr.s_addr = INADDR_ANY;
    server_addr.sin_port = htons(SERVER_PORT);

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

    printf("[SERVER] Listening on port %d\n", SERVER_PORT);

    while (1) {
        client_sock = accept(server_sock, (struct sockaddr*)&client_addr, &addr_len);
        if (client_sock < 0) {
            perror("[SERVER] Accept failed");
            continue;
        }

        handle_client(client_sock);
    }

    close(server_sock);
    return 0;
}
