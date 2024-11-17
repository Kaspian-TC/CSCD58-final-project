#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define SERVER_PORT 5432
#define MAX_LINE 256

int main(int argc, char *argv[]) {
    struct hostent *hp;
    struct sockaddr_in sin;
    char *host;
    char buf[MAX_LINE];
    int s;
    int len;

    if (argc == 2) {
        host = argv[1];
    } else {
        fprintf(stderr, "usage: simplex-talk host\n");
        exit(1);
    }

    /* Translate host name into peer's IP address */
    printf("Resolving hostname: %s\n", host);
    hp = gethostbyname(host);
    if (!hp) {
        fprintf(stderr, "simplex-talk: unknown host: %s\n", host);
        exit(1);
    }

    /* Build address data structure */
    bzero((char *)&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    bcopy(hp->h_addr, (char *)&sin.sin_addr, hp->h_length);
    sin.sin_port = htons(SERVER_PORT);

    /* Active open */
    printf("Creating socket...\n");
    if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0) {
        perror("simplex-talk: socket");
        exit(1);
    }

    printf("Connecting to server at %s:%d...\n", host, SERVER_PORT);
    if (connect(s, (struct sockaddr *)&sin, sizeof(sin)) < 0) {
        perror("simplex-talk: connect");
        close(s);
        exit(1);
    }
    printf("Connected to server successfully!\n");

    /* Send predefined messages */
    const char *messages[] = {
        "Hello from the client!",
        "How are you, server?",
        ">>> Ciao-Ciao" // Terminates the server connection
    };
    int num_messages = sizeof(messages) / sizeof(messages[0]);

    for (int i = 0; i < num_messages; i++) {
        strncpy(buf, messages[i], MAX_LINE);
        len = strlen(buf) + 1;

        printf("Sending message: %s\n", buf);
        send(s, buf, len, 0);

        char response[MAX_LINE + 60];
        bzero(response, sizeof(response));
        recv(s, response, sizeof(response), 0);
        printf("Received from server: %s\n", response);
    }

    /* Close the connection */
    printf("Closing connection to server.\n");
    close(s);
    return 0;
}
