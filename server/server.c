#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
// adding libraries necessary to compile
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#define SERVER_PORT 5432
#define MAX_PENDING 5
#define MAX_LINE 256
int main()
{
    struct sockaddr_in sin;
    char buf[MAX_LINE];
    int len;
    int s, new_s;
    /* build address data structure */
    bzero((char *)&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;
    sin.sin_port = htons(SERVER_PORT);

    printf("[SERVER] Initializing server...\n");

    /* setup passive open */
    if ((s = socket(PF_INET, SOCK_STREAM,
                    0)) < 0)
    {
        perror("simplex-talk: socket");
        exit(1);
    }
    if ((bind(s, (struct sockaddr *)&sin, sizeof(sin))) < 0)
    {
        perror("simplex-talk: bind");
        exit(1);
    }
    listen(s, MAX_PENDING);

    printf("[SERVER] Server started successfully and is listening on port %d.\n", SERVER_PORT);

    /* Wait for connection, then receive and echo messages */
    while (1) {
        printf("[SERVER] Waiting for a connection...\n");
        if ((new_s = accept(s, (struct sockaddr *)&sin, &len)) < 0)
        {
            perror("simplex-talk: accept failed");
            exit(1);
        }
        printf("[SERVER] Connection established.\n");

        while ((len = recv(new_s, buf, sizeof(buf) - 1, 0)) > 0) {
            buf[len] = '\0'; // Null-terminate received data
            printf("[SERVER] Message received: '%s'\n", buf);

            if (strcmp(buf, "Ciao-Ciao") == 0) {
                printf("[SERVER] Termination message received. Closing client connection.\n");
                break; // Exit the inner loop but keep the server running
            }

            /* Echo the message back to the client */
            if (send(new_s, buf, len, 0) == -1) {
                perror("[SERVER] Error sending message");
                break;
            }
        }

        if (len == 0) {
            printf("[SERVER] Connection closed by client.\n");
        } else if (len < 0) {
            perror("[SERVER] Error receiving data");
        }

        close(new_s); // Close the client connection
    }

    close(s); // This will never be reached but is good practice
    return 0;
}
