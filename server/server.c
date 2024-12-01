#include <stdint.h>
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include <gmp.h>
#include "../shared_functions/helper_func.h"
#include "../shared_functions/key_exchange.h"

#include "server.h"
#define SERVER_PORT 5432
#define MAX_PENDING 5
#define MAX_LINE 256





int main()
{
    setbuf(stdout, NULL);
    struct sockaddr_in sin;
    char buf[MAX_LINE];
    unsigned int len;
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

    // Allow the socket to be reused immediately after the server terminates
    int option = 1;
    setsockopt(s, SOL_SOCKET, SO_REUSEADDR, &option, sizeof(option));

    if ((bind(s, (struct sockaddr *)&sin, sizeof(sin))) < 0)
    {
        perror("simplex-talk: bind");
        exit(1);
    }
    listen(s, MAX_PENDING);

    printf("[SERVER] Server started successfully and is listening on port %d.\n", SERVER_PORT);

    gmp_randstate_t state; // make sure to call gmp_randclear(state); 
    // when done with state
    gmp_randinit_mt(state);
    gmp_randseed_ui(state, time(NULL));

    /* Wait for connection */
    while (1) {
        printf("[SERVER] Waiting for a connection...\n");
        if ((new_s = accept(s, (struct sockaddr *)&sin, &len)) < 0)
        {
            perror("simplex-talk: accept failed");
            exit(1);
        }
        printf("[SERVER] Connection established.\n");

        // the client is sending the payload for Diffie-Hellman key exchange
        uint8_t session_key[AES_KEY_SIZE]; 
        
        server_get_session_key(new_s, session_key, state);
        
        printf("[SERVER] Session key: ");   
        print_bytes(session_key, AES_KEY_SIZE);

        int continue_connection = 1;
        while(continue_connection){
            int data_len;
            // receive encrypted data for the client
            uint8_t * data = receive_encypted_data(new_s, &data_len,
            session_key);

            // print out the plaintext
            printf("Plaintext: ");
            for(int i = 0; i < data_len; i++){
                printf("%c", data[i]);
            }
            printf("\n");

            // send back plaintext + current time to client
            const char * format = "Current time: %d-%d-%d %d:%d:%d\n";
            uint8_t * response = malloc(data_len + strlen(format));
            memcpy(response, data, data_len);
            time_t t = time(NULL);
            struct tm tm = *localtime(&t);
            char current_time[100];
            sprintf(current_time, format , tm.tm_year + 1900, tm.tm_mon + 1, tm.tm_mday, tm.tm_hour, tm.tm_min, tm.tm_sec);

            memcpy(response + data_len, current_time, strlen(current_time));
            send_encypted_data(new_s, response, data_len + strlen(current_time), session_key, state);
            

            // close connection if client sends ">>> Ciao-Ciao"
            if (strncmp((char *) data, ">>> Ciao-Ciao\n",data_len) == 0 || strncmp((char *)data, "Ciao-Ciao\n",data_len) == 0)
            {
                continue_connection = 0;
            }

            free(data);
            free(response);

        }
        // close the connection
        close(new_s);
        printf("[SERVER] Connection closed.\n");
    }
    gmp_randclear(state);

    close(s); // This will never be reached but is good practice
    return 0;
}