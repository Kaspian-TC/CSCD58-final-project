#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include "router.h"
#include "../shared_functions/helper_func.h"
#include "../shared_functions/key_exchange.h"

#define NUM_SERVERS 3
#define MAX_LINE 256

const char* server_ips[NUM_SERVERS] = {"10.0.1.1", "10.0.2.1", "10.0.3.1"};
int current_server = 0;

// Arrays to store server sockets and session keys after a single key exchange
int server_sockets[NUM_SERVERS];
uint8_t server_session_keys[NUM_SERVERS][AES_KEY_SIZE];

void connect_and_key_exchange_with_servers(gmp_randstate_t state) {
    for (int i = 0; i < NUM_SERVERS; i++) {
        int sockfd;
        struct sockaddr_in server_addr;
        if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
            perror("[ROUTER] Socket creation failed for server");
            exit(EXIT_FAILURE);
        }

        memset(&server_addr, 0, sizeof(server_addr));
        server_addr.sin_family = AF_INET;
        server_addr.sin_port = htons(SERVER_PORT);
        if (inet_pton(AF_INET, server_ips[i], &server_addr.sin_addr) <= 0) {
            perror("[ROUTER] Invalid server IP address");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
            perror("[ROUTER] Connection to server failed");
            close(sockfd);
            exit(EXIT_FAILURE);
        }

        printf("[ROUTER] Performing key exchange with server %d at %s...\n", i+1, server_ips[i]);
        client_get_session_key(sockfd, server_session_keys[i], state);
        printf("[ROUTER] Key exchange with server %d completed. Session key: ", i+1);
        print_bytes(server_session_keys[i], AES_KEY_SIZE);

        server_sockets[i] = sockfd;
    }
}

void send_command_to_server(int server_index, const char* message, char* response, gmp_randstate_t state) {
    int sockfd = server_sockets[server_index];
    uint8_t* session_key = server_session_keys[server_index];

    // Send encrypted data
    send_encypted_data(sockfd, (uint8_t*)message, strlen(message), session_key, state);

    // Receive encrypted response
    int len;
    uint8_t* ubuffer = receive_encypted_data(sockfd, &len, session_key);

    if (len > 0) {
        memcpy(response, ubuffer, len);
        response[len] = '\0';
    } else {
        printf("[ROUTER] No response from server %d\n", server_index + 1);
        strcpy(response, "Reached size limit for server response");
    }

    free(ubuffer);
}

void handle_client(int client_sock, gmp_randstate_t state) {
    uint8_t session_key[AES_KEY_SIZE] = {0};

    // Perform the Diffie-Hellman key exchange once with the client
    server_get_session_key(client_sock, session_key, state);
    printf("[ROUTER] Key exchange completed with CLIENT: ");
    print_bytes(session_key, AES_KEY_SIZE);

    while (1) {
        int data_len;
        uint8_t* decrypted_data = receive_encypted_data(client_sock, &data_len, session_key);

        if (data_len <= 0) {
            printf("[ROUTER] No data or connection closed, ending session.\n");
            free(decrypted_data);
            break; // connection closed or error
        }

        decrypted_data[data_len] = '\0';

        if (strcmp((char*)decrypted_data, "EXIT") == 0) {
            free(decrypted_data);
            printf("[ROUTER] Received EXIT from client. Keeping connection open.\n");
            break;
        } else if (strcmp((char*)decrypted_data, "RETRIEVE") == 0) {
            char formatted_response[MAX_LINE * NUM_SERVERS * 100] = {0};

            for (int i = 0; i < NUM_SERVERS; i++) {
                char server_response[MAX_LINE * 100] = {0};
                send_command_to_server(i, (char*)decrypted_data, server_response, state);

                char formatted_server_response[MAX_LINE * 2];
                snprintf(formatted_server_response, sizeof(formatted_server_response),
                         "Server %d:\n%s\n", i + 1, server_response);
                strncat(formatted_response, formatted_server_response,
                        sizeof(formatted_response) - strlen(formatted_response) - 1);
            }

            send_encypted_data(client_sock, (uint8_t*)formatted_response, strlen(formatted_response), session_key, state);
        } else {
            char server_response[MAX_LINE * 100] = {0};
            const char* target_server_ip = server_ips[current_server];
            int server_index = current_server;
            current_server = (current_server + 1) % NUM_SERVERS;

            send_command_to_server(server_index, (char*)decrypted_data, server_response, state);
            send_encypted_data(client_sock, (uint8_t*)server_response, strlen(server_response), session_key, state);
        }

        free(decrypted_data);
    }

    close(client_sock);
}

int main() {
    printf("[ROUTER] Starting router...\n");
    fflush(stdout);

    int router_sock, client_sock;
    struct sockaddr_in router_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    router_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (router_sock < 0) {
        perror("[ROUTER] Socket creation failed");
        exit(EXIT_FAILURE);
    }

    memset(&router_addr, 0, sizeof(router_addr));
    router_addr.sin_family = AF_INET;
    router_addr.sin_addr.s_addr = INADDR_ANY;
    router_addr.sin_port = htons(SERVER_PORT);

    if (bind(router_sock, (struct sockaddr*)&router_addr, sizeof(router_addr)) < 0) {
        perror("[ROUTER] Bind failed");
        close(router_sock);
        exit(EXIT_FAILURE);
    }

    if (listen(router_sock, 5) < 0) {
        perror("[ROUTER] Listen failed");
        close(router_sock);
        exit(EXIT_FAILURE);
    }

    printf("[ROUTER] Listening on port %d\n", SERVER_PORT);
    fflush(stdout);

    gmp_randstate_t state;
    gmp_randinit_mt(state);
    gmp_randseed_ui(state, time(NULL));

    // Connect to all servers and perform key exchange once
    connect_and_key_exchange_with_servers(state);

    while (1) {
        printf("[ROUTER] Waiting for client connections...\n");
        client_sock = accept(router_sock, (struct sockaddr*)&client_addr, &addr_len);
        if (client_sock < 0) {
            perror("[ROUTER] Accept failed");
            continue; // Keep listening for new connections
        }

        printf("[ROUTER] Accepted connection from client\n");
        fflush(stdout);

        // Handle the client in a separate function
        handle_client(client_sock, state);
    }

    gmp_randclear(state);
    close(router_sock);
    return 0;
}
