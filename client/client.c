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

void store_data(int sockfd,gmp_randstate_t state, uint8_t * session_key /* Assumed AES_BYTES long */) {
    

    char buffer[MAX_LINE] = {0};
    int buffer_length = sizeof(buffer);
    generate_random_data(buffer, buffer_length);

    send_encypted_data(sockfd, (uint8_t *)buffer, buffer_length, session_key, state);
    printf("[CLIENT] Sent: %d\n", buffer_length);

    /* send(sockfd, buffer, strlen(buffer), 0);
    printf("[CLIENT] Sent: %s\n", buffer); */

    uint8_t * response;
    int response_length;

    response = receive_encypted_data(sockfd, &response_length, session_key);
    printf("[CLIENT] Received: %s\n", response);

    /* recv(sockfd, response, sizeof(response), 0);
    printf("[CLIENT] Received: %s\n", response); */
    free(response);
}

void retrieve_data(int sockfd,gmp_randstate_t state, uint8_t * session_key /* Assumed AES_BYTES long */) {
    char buffer[MAX_LINE] = "RETRIEVE";
    int buffer_length = sizeof(buffer);
    // send(sockfd, buffer, strlen(buffer), 0);
    send_encypted_data(sockfd, (uint8_t *)buffer, buffer_length, session_key, state);
    printf("[CLIENT] Sent: %s\n", buffer);

    uint8_t * response;
    int len;

    response = receive_encypted_data(sockfd, &len, session_key);

    if (len > 0) {
        response[len] = '\0';
        printf("[CLIENT] Received:\n%s\n", response);
    } else {
        printf("[CLIENT] No data received.\n");
    }
    free(response);
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

    gmp_randstate_t state; // make sure to call gmp_randclear(state); 
    // when done with state
    gmp_randinit_mt(state);
    gmp_randseed_ui(state, time(NULL));
    uint8_t session_key[AES_KEY_SIZE];
    client_get_session_key(sockfd, session_key, state);
    /* printf("[CLIENT] Session key: ");   
    print_bytes(session_key, AES_KEY_SIZE); */


    if (strcmp(argv[1], "--store") == 0) {
        store_data(sockfd, state,session_key);
    } else if (strcmp(argv[1], "--retrieve") == 0) {
        retrieve_data(sockfd,state,session_key);
    } else {
        fprintf(stderr, "Invalid operation: %s\n", argv[1]);
    }

    close(sockfd);
    gmp_randclear(state);
    return 0;
}
