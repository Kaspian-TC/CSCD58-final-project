#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <math.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <gmp.h>
#include <arpa/inet.h>

#include "server.h"

// Function to store data locally
void store_data(const char* payload) {
    FILE* fp = fopen("data.txt", "a"); // Open file in append mode
    if (fp == NULL) {
        perror("[STORAGE] Failed to open or create data file");
        return;
    }

    fwrite(payload, sizeof(char), strlen(payload), fp); // Write raw data
    fclose(fp);

    printf("[STORAGE] Stored payload locally: %s\n", payload);
}

// Function to retrieve stored data
void retrieve_data(int socket) {
    FILE* fp = fopen("data.txt", "r");
    char buf[MAX_LINE];

    if (fp == NULL) {
        // Handle the missing file case
        perror("[STORAGE] Data file does not exist, sending empty response.");
        send(socket, "__END__", 7, 0); // Send end marker to indicate no data
        return;
    }

    // Read and send file contents
    while (fgets(buf, sizeof(buf), fp)) {
        send(socket, buf, strlen(buf), 0);
    }
    fclose(fp);

    // Send end marker to indicate completion
    send(socket, "__END__", 7, 0);
    printf("[STORAGE] Sent all data to router.\n");
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