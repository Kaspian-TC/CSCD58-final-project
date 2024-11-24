#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netdb.h>
#include <arpa/inet.h>

#define SERVER_PORT 5432
#define MAX_LINE 256

// Storage server hostnames
const char* SERVER_IPS[] = {"server1", "server2", "server3"};
const int NUM_SERVERS = 3;

// Function to retrieve all data from storage servers
void retrieve_from_servers(int client_sock) {
    char buf[MAX_LINE];
    char final_payload[MAX_LINE * NUM_SERVERS]; // To hold the concatenated data
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

            // Remove any `__END__` markers from the received data
            char* end_marker_pos = strstr(buf, "__END__");
            if (end_marker_pos != NULL) {
                *end_marker_pos = '\0'; // Truncate the string at `__END__`
            }

            // Append cleaned data to the final payload
            strncat(final_payload, buf, sizeof(final_payload) - strlen(final_payload) - 1);

            printf("[ROUTER] Received chunk from server %s: %s\n", SERVER_IPS[i], buf);
        }

        close(sock);
    }

    // Debug: Final payload before sending to client
    printf("[ROUTER] Final concatenated payload (before sending): %s\n", final_payload);

    // Send the final payload to the client
    send(client_sock, final_payload, strlen(final_payload), 0);

    printf("[ROUTER] Sent final payload to client.\n");
}

// Function to forward a chunk of data to a storage server
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

    // Send data chunk to server
    send(sock, payload, strlen(payload), 0);
    printf("[ROUTER] Forwarded chunk '%s' to server %s\n", payload, server_name);
    close(sock);
}

// Function to shard data into chunks and distribute across servers
void shard_and_distribute(const char* data) {
    int data_len = strlen(data);
    int chunk_size = (data_len + NUM_SERVERS - 1) / NUM_SERVERS; // Divide data into chunks

    for (int i = 0; i < NUM_SERVERS; i++) {
        int start = i * chunk_size;
        int end = (i + 1) * chunk_size;
        if (start >= data_len) break; // No more data to send
        if (end > data_len) end = data_len;

        char chunk[MAX_LINE];
        strncpy(chunk, &data[start], end - start);
        chunk[end - start] = '\0';

        forward_to_server(SERVER_IPS[i], chunk);
    }
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
            shard_and_distribute(buf); // Split and distribute data to storage servers
        }

        close(new_s);
    }

    close(s);
    return 0;
}