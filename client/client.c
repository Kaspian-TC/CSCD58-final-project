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

void store_data(int sockfd, uint8_t* session_key, gmp_randstate_t state) {
    char buffer[MAX_LINE] = {0};
    generate_random_data(buffer, sizeof(buffer));

    printf("[CLIENT] Sending encrypted data: %s\n", buffer);
    send_encypted_data(sockfd, (uint8_t*)buffer, strlen(buffer), session_key, state);
    //send(sockfd, buffer, strlen(buffer), 0);
    printf("[CLIENT] Sent: %s\n", buffer);

    // char response[MAX_LINE] = {0};
    // recv(sockfd, response, sizeof(response), 0);
    // printf("[CLIENT] Received: %s\n", response);

    int data_len;
    uint8_t* decrypted_response = receive_encypted_data(sockfd, &data_len, session_key);
    
    if (data_len == -1) {
        fprintf(stderr, "[CLIENT] store_data(): Error receiving encrypted data, closing connection.\n");
        if (decrypted_response) {
            free(decrypted_response);
        }
        close(sockfd);
        return;
    }

    printf("[CLIENT] Received: %.*s\n", data_len, decrypted_response);
    free(decrypted_response);
}

void retrieve_data(int sockfd, uint8_t *session_key, gmp_randstate_t state) {
    char buffer[MAX_LINE] = "RETRIEVE";

    // Encrypt the request before sending
    send_encypted_data(sockfd, (uint8_t*)buffer, strlen(buffer), session_key, state);
    printf("[CLIENT] Sent encrypted request.\n");

    // Receive encrypted response and decrypt
    int data_len;
    uint8_t* decrypted_response = receive_encypted_data(sockfd, &data_len, session_key);

    if (data_len == -1) {
        fprintf(stderr, "[CLIENT] retrieve_data: Error receiving encrypted data, closing connection.\n");
        if (decrypted_response) {
            free(decrypted_response);
        }
        close(sockfd);
        return;
    }

    printf("[CLIENT] Received:\n%.*s\n", data_len, decrypted_response);
    free(decrypted_response);
}

int main(int argc, char** argv) {
    // the usage shuold be ./client --session <router_host>
    if (argc != 3 || strcmp(argv[1], "--session") != 0) {
        fprintf(stderr, "Usage: %s --session <router_host>\n", argv[0]);
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

    while (1) {
        printf("[CLIENT] Enter command [store|retrieve|exit]: ");
        fflush(stdout);

        char command[MAX_LINE];
        if (!fgets(command, sizeof(command), stdin)) {
            break; // user ended input
        }
        command[strcspn(command, "\n")] = 0; // remove newline

        if (strcmp(command, "store") == 0) {
            store_data(sockfd, session_key, state);
        } else if (strcmp(command, "retrieve") == 0) {
            retrieve_data(sockfd, session_key, state);
        } else if (strcmp(command, "exit") == 0) {
            // Send "EXIT" to router to indicate end of session
            send_encypted_data(sockfd, (uint8_t*)"EXIT", 4, session_key, state);
            printf("[CLIENT] Exiting session.\n");
            break;
        } else {
            printf("[CLIENT] Unknown command.\n");
        }
    }

    // Cleanup and close
    close(sockfd);
    gmp_randclear(state);
    return 0;

}
