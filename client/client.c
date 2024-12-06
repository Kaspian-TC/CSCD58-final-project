#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <time.h>
#include "../shared_functions/helper_func.h"
#include "../shared_functions/key_exchange.h"

#define MAX_LINE 256
#define SERVER_PORT 5432

void generate_random_data(char* buffer, size_t length) {
    srand(time(NULL));
    snprintf(buffer, length, "RandomData_%d", rand());
}

void store_data(int sockfd) {
    char buffer[MAX_LINE] = {0};
    generate_random_data(buffer, sizeof(buffer));
    send(sockfd, buffer, strlen(buffer), 0);
    printf("[CLIENT] Sent: %s\n", buffer);

    char response[MAX_LINE] = {0};
    recv(sockfd, response, sizeof(response), 0);
    printf("[CLIENT] Received: %s\n", response);
}

void retrieve_data(int sockfd) {
    char buffer[MAX_LINE] = "RETRIEVE";
    send(sockfd, buffer, strlen(buffer), 0);
    printf("[CLIENT] Sent: %s\n", buffer);

    char response[MAX_LINE * 10] = {0};  // Adjust size as needed
    int len = recv(sockfd, response, sizeof(response) - 1, 0);

    if (len > 0) {
        response[len] = '\0';
        printf("[CLIENT] Received:\n%s\n", response);
    } else {
        printf("[CLIENT] No data received.\n");
    }
}

int main(int argc, char** argv) {
    if (argc != 3) {
        fprintf(stderr, "Usage: %s --store|--retrieve <router_host>\n", argv[0]);
        return 1;
    }

    int sockfd;
    struct sockaddr_in server_addr;

    if ((sockfd = socket(AF_INET, SOCK_STREAM, 0)) < 0) {
        perror("[CLIENT] Socket creation failed");
        return 1;
    }

    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(SERVER_PORT);
    if (inet_pton(AF_INET, argv[2], &server_addr.sin_addr) <= 0) {
        perror("[CLIENT] Invalid router IP address");
        close(sockfd);
        return 1;
    }

    if (connect(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr)) < 0) {
        perror("[CLIENT] Connection failed");
        close(sockfd);
        return 1;
    }

    gmp_randstate_t state;
    gmp_randinit_mt(state);
    gmp_randseed_ui(state, time(NULL));

    uint8_t session_key[AES_KEY_SIZE] = {0};

    printf("[CLIENT] Performing key exchange with server...\n");
    client_get_session_key(sockfd, session_key, state);

    printf("[CLIENT] Key exchange completed: ");
    print_bytes(session_key, AES_KEY_SIZE);

    if (strcmp(argv[1], "--store") == 0) {
        store_data(sockfd);
    } else if (strcmp(argv[1], "--retrieve") == 0) {
        retrieve_data(sockfd);
    } else {
        fprintf(stderr, "Invalid operation: %s\n", argv[1]);
    }

    // gmp_randclear(state);
    close(sockfd);
    return 0;
}
