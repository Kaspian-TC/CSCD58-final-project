#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#include "router.h"

static int current_server_index = 0; // To keep track of the next server to store data

// Function to retrieve all data from storage servers
void retrieve_from_servers(int client_sock) {
    char buf[MAX_LINE];
    char final_payload[MAX_LINE * NUM_SERVERS]; // To hold concatenated data
    int len;

    // Initialize the final payload
    bzero(final_payload, sizeof(final_payload));

    for (int i = 0; i < NUM_SERVERS; i++) {
        struct hostent* hp = gethostbyname(SERVER_IPS[i]);
        if (!hp) {
            fprintf(stderr, "[ROUTER] Unknown host: %s\n", SERVER_IPS[i]);
            continue;
        }

        struct sockaddr_in server;
        int sock = socket(AF_INET, SOCK_STREAM, 0);
        if (sock < 0) {
            perror("[ROUTER] Socket creation failed");
            continue;
        }

        bzero((char*)&server, sizeof(server));
        server.sin_family = AF_INET;
        bcopy(hp->h_addr, (char*)&server.sin_addr, hp->h_length);
        server.sin_port = htons(SERVER_PORT);

        if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
            perror("[ROUTER] Connection to storage server failed");
            close(sock);
            continue;
        }

        // Request stored data
        send(sock, "GET_DATA", 8, 0);
        printf("[ROUTER] Sent retrieval request to server %s\n", SERVER_IPS[i]);

        // Receive data from the server
        while ((len = recv(sock, buf, MAX_LINE - 1, 0)) > 0) {
            buf[len] = '\0';

            // Append received data to the final payload
            strncat(final_payload, buf, sizeof(final_payload) - strlen(final_payload) - 1);
        }

        close(sock);
    }

    // Send the final payload to the client
    send(client_sock, final_payload, strlen(final_payload), 0);
    printf("[ROUTER] Sent concatenated data to client.\n");
}

// Function to forward a block of data to a storage server
void forward_to_server(const char* server_name, const char* payload) {
    struct hostent* hp;
    struct sockaddr_in server;
    int sock;

    // Resolve server hostname
    hp = gethostbyname(server_name);
    if (!hp) {
        fprintf(stderr, "[ROUTER] Unknown host: %s\n", server_name);
        return;
    }

    // Create socket
    sock = socket(AF_INET, SOCK_STREAM, 0);
    if (sock < 0) {
        perror("[ROUTER] Socket creation failed");
        return;
    }

    // Build server address structure
    bzero((char*)&server, sizeof(server));
    server.sin_family = AF_INET;
    bcopy(hp->h_addr, (char*)&server.sin_addr, hp->h_length);
    server.sin_port = htons(SERVER_PORT);

    // Connect to storage server
    if (connect(sock, (struct sockaddr*)&server, sizeof(server)) < 0) {
        perror("[ROUTER] Connection to storage server failed");
        close(sock);
        return;
    }

    // Send data block to the server
    send(sock, payload, strlen(payload), 0);
    printf("[ROUTER] Sent block '%s' to server %s\n", payload, server_name);
    close(sock);
}

// Function to distribute data across servers in a round-robin manner
void distribute_to_server(const char* data) {
    const char* target_server = SERVER_IPS[current_server_index];

    // Send the full block to the chosen server
    forward_to_server(target_server, data);

    // Move to the next server in a round-robin fashion
    current_server_index = (current_server_index + 1) % NUM_SERVERS;
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
        perror("[ROUTER] Socket creation failed");
        exit(1);
    }

    if ((bind(s, (struct sockaddr*)&sin, sizeof(sin))) < 0) {
        perror("[ROUTER] Binding failed");
        close(s);
        exit(1);
    }

    listen(s, 5);
    printf("[ROUTER] Listening on port %d...\n", SERVER_PORT);

    while (1) {
        if ((new_s = accept(s, (struct sockaddr*)&sin, &len)) < 0) {
            perror("[ROUTER] Accept failed");
            close(s);
            exit(1);
        }

        len = recv(new_s, buf, sizeof(buf) - 1, 0);
        buf[len] = '\0';

        if (strcmp(buf, "RETRIEVE") == 0) {
            printf("[ROUTER] Received retrieval request\n");
            retrieve_from_servers(new_s);
        } else {
            printf("[ROUTER] Received data: %s\n", buf);
            distribute_to_server(buf); // Send data to a single server
        }

        close(new_s);
    }

    close(s);
    return 0;
}