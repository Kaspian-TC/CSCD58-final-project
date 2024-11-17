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
    /* wait for connection, then receive and print text */
    while (1)
    {
        if ((new_s = accept(s, (struct sockaddr *)&sin, &len)) < 0)
        {
            perror("simplex-talk: accept");
            exit(1);
        }
        while (len = recv(new_s, buf, sizeof(buf), 0))
        {
            if (strcmp(buf, ">>> Ciao-Ciao\n") == 0 || strcmp(buf, "Ciao-Ciao\n") == 0)
            {
                close(new_s);
                return 0;
            }
            fputs(buf, stdout);
            time_t t = time(NULL);
            struct tm tm = *localtime(&t);
            char send_str[MAX_LINE + 60]; //+60 for the time
            strcpy(send_str, buf);
            sprintf(send_str + len-1,
                    "%d-%02d-%02d %02d:%02d:%02d\n",
                    tm.tm_year + 1900,
                    tm.tm_mon + 1,
                    tm.tm_mday,
                    tm.tm_hour,
                    tm.tm_min,
                    tm.tm_sec);
            send(new_s, send_str, strlen(send_str)+1, 0);
        }
        close(new_s);
    }
}