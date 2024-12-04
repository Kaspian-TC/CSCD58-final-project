#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <pthread.h>
#include "router.h"
#include "../shared_functions/helper_func.h"
#include "../shared_functions/key_exchange.h"

#define NUM_SERVERS 3
#define MAX_LINE 256

const char* server_ips[NUM_SERVERS] = {"10.0.1.1", "10.0.2.1", "10.0.3.1"};
int current_server = 0;

void forward_to_server(const char* server_ip, const char* message, char* response,gmp_randstate_t state) {
    int sockfd;
    struct sockaddr_in server_addr;
    char buffer[MAX_LINE] = {0};

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("[ROUTER] Socket creation failed");
        strcpy(response, "ERROR");
        return;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    inet_pton(AF_INET, server_ip, &server_addr.sin_addr);

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("[ROUTER] Connection to server failed");
        close(sockfd);
        strcpy(response, "ERROR");
        return;
    }
    // get the session key, but this time from the client side
    uint8_t router_server_session_key[AES_KEY_SIZE];
    client_get_session_key(sockfd, router_server_session_key, state);
    // send the message to the server
    send_encypted_data(sockfd, (uint8_t *)message, strlen(message), router_server_session_key, state);
    // send(sockfd, message, strlen(message), 0);
    // receive the response from the server
    int len;
    uint8_t * ubuffer = receive_encypted_data(sockfd, &len, router_server_session_key);
    memcpy(buffer, ubuffer, len);
    free(ubuffer);
    // int len = recv(sockfd, buffer, sizeof(buffer) - 1, 0);
    if (len > 0) {
        buffer[len] = '\0';
        strncpy(response, buffer, MAX_LINE - 1);
    } else {
        strcpy(response, "NO_RESPONSE");
    }

    close(sockfd);
}

void handle_client(int client_sock,gmp_randstate_t state) {
    
    uint8_t session_key[AES_KEY_SIZE]; 
        
    server_get_session_key(client_sock, session_key, state);
    
    /* printf("[ROUTER] Session key: ");   
    print_bytes(session_key, AES_KEY_SIZE); */
    // char buffer[MAX_LINE] = {0};

    int len;
    uint8_t* ubuffer = receive_encypted_data(client_sock, &len,
            session_key);
    char buffer[len+1];
    memcpy(buffer, ubuffer, len);
    free(ubuffer);
    // len = recv(client_sock, buffer, sizeof(buffer) - 1, 0);

    if (len > 0) {
        buffer[len] = '\0';
        printf("[ROUTER] Received: %s\n", buffer);

        char formatted_response[MAX_LINE * NUM_SERVERS] = {0};
        int formatted_response_len = 0;
        if (strcmp(buffer, "RETRIEVE") == 0) {
            for (int i = 0; i < NUM_SERVERS; i++) {
                char server_response[MAX_LINE] = {0};
                forward_to_server(server_ips[i], buffer, server_response,state);

                // Format the response to include server information
                char formatted_server_response[MAX_LINE * 2];
                snprintf(formatted_server_response, sizeof(formatted_server_response),
                         "Server %d:\n%s\n", i + 1, server_response);
                formatted_response_len = strlen(formatted_response);
                strncat(formatted_response, formatted_server_response,
                        sizeof(formatted_response) - formatted_response_len - 1);
            }
            // send(client_sock, formatted_response, formatted_response_len, 0);
        } else {
            char * server_response = formatted_response;
            const char* target_server_ip = server_ips[current_server];
            current_server = (current_server + 1) % NUM_SERVERS;
            forward_to_server(target_server_ip, buffer, server_response,state);
            formatted_response_len = strlen(server_response);
            // send(client_sock, server_response, strlen(server_response), 0);
        }
        send_encypted_data(client_sock, (uint8_t *)formatted_response, formatted_response_len, session_key,state);
        // send(client_sock, formatted_response, formatted_response_len, 0);
    }

    close(client_sock);
}

int main() {
    int router_sock, client_sock;
    struct sockaddr_in router_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);

    router_sock = socket(AF_INET, SOCK_STREAM, 0);
    if (router_sock < 0) {
        perror("[ROUTER] Socket creation failed");
        exit(EXIT_FAILURE);
    }
    // Allow the socket to be reused immediately after the server terminates
    int option = 1;
    setsockopt(router_sock, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

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

    gmp_randstate_t state; // make sure to call gmp_randclear(state); 
    // when done with state
    gmp_randinit_mt(state);
    gmp_randseed_ui(state, time(NULL));
    while (1) {
        client_sock = accept(router_sock, (struct sockaddr*)&client_addr, &addr_len);
        if (client_sock < 0) {
            perror("[ROUTER] Accept failed");
            continue;
        }

        handle_client(client_sock,state);
    }

    close(router_sock);
    return 0;
}
