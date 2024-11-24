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

#include "client.h"

void send_data(int socket, const char* data) {
    send(socket, data, strlen(data), 0);
    printf("[CLIENT] Sent: %s\n", data);
}

void retrieve_data(int socket) {
    char buf[MAX_LINE];
    int len;

    send_data(socket, "RETRIEVE");
    printf("[CLIENT] Sent retrieval request\n");

    // Receive data from the server
    printf("[CLIENT] Received data:\n");
    while ((len = recv(socket, buf, MAX_LINE - 1, 0)) > 0) {
        buf[len] = '\0';
        printf("%s", buf);
    }
    printf("\n[CLIENT] Data retrieval complete\n");
}

int main(int argc, char* argv[]) {
    struct hostent* hp;
    struct sockaddr_in sin;
    char* host;
    int s;

    if (argc != 3 || (strcmp(argv[1], "--store") != 0 && strcmp(argv[1], "--retrieve") != 0)) {
        fprintf(stderr, "Usage: %s --store|--retrieve <router_host>\n", argv[0]);
        exit(1);
    }

    char* mode = argv[1];
    host = argv[2];

    // Translate host name into peer's IP address
    hp = gethostbyname(host);
    if (!hp) {
        fprintf(stderr, "simplex-talk: unknown host: %s\n", host);
        exit(1);
    }

    // Build address data structure
    bzero((char*)&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    bcopy(hp->h_addr, (char*)&sin.sin_addr, hp->h_length);
    sin.sin_port = htons(SERVER_PORT);

    // Active open
    if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("simplex-talk: socket");
        exit(1);
    }
    if (connect(s, (struct sockaddr*)&sin, sizeof(sin)) < 0) {
        perror("simplex-talk: connect");
        close(s);
        exit(1);
    }

    printf("[CLIENT] Connected to server at %s\n", host);

    if (strcmp(mode, "--store") == 0) {
        // Generate and send a random string
        srand(time(NULL));
        char random_string[MAX_LINE];
        snprintf(random_string, sizeof(random_string), "RandomData_%d", rand());
        send_data(s, random_string);
    } else if (strcmp(mode, "--retrieve") == 0) {
        // Retrieve data from the router
        retrieve_data(s);
    }

    close(s);
    return 0;
}