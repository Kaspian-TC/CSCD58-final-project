#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
//adding libraries necessary to compile
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <gmp.h>
#include <time.h>
#define SERVER_PORT 5432
#define MAX_LINE 256

void getBigPrime(mpz_t prime)
{
    // mpz_t z;
    // gmp_printf ("%s is an mpz %Zd\n", "here", z);
    mpz_t random_number;
    mpz_init(random_number);
    gmp_randstate_t state;
    gmp_randinit_mt(state);
    gmp_randseed_ui(state, time(NULL));
    mpz_urandomb(random_number, state, 2048);
    mpz_nextprime(prime, random_number);
    gmp_randclear(state);
    mpz_clear(random_number);
    // mpz_nextprime(prime, 2);
    
}
int main(int argc, char *argv[])
{
    mpz_t prime;
    mpz_init2(prime,2048);
    getBigPrime(prime);

    gmp_printf("Prime number: %Zd\n", prime);
    FILE *fp;
    struct hostent *hp;
    struct sockaddr_in sin;
    char *host;
    char buf[MAX_LINE];
    int s;
    int len;
    if (argc == 2)
    {
        host = argv[1];
    }
    else
    {
        fprintf(stderr, "usage: simplex-talk host\n");
        exit(1);
    }
    /* translate host name into peer’s IP address */
    hp = gethostbyname(host);
    if (!hp)
    {
        fprintf(stderr, "simplex-talk: unknown host: %s\n",
                host);
        exit(1);
    }
    /* build address data structure */
    bzero((char *)&sin, sizeof(sin));
    sin.sin_family = AF_INET;
    bcopy(hp->h_addr, (char *)&sin.sin_addr, hp->h_length);
    sin.sin_port = htons(SERVER_PORT);
    /* active open */
    if ((s = socket(PF_INET, SOCK_STREAM, 0)) < 0)
    {
        perror("simplex-talk: socket");
        exit(1);
    }
    if (connect(s, (struct sockaddr *)&sin,
                sizeof(sin)) < 0)
    {
        perror("simplex-talk: connect");
        close(s);
        exit(1);
    }
    /* main loop: get and send lines of text */
    printf("has connected to server at %s\n", host);
    while (fgets(buf, sizeof(buf), stdin))
    {
        len = strlen(buf) + 1;
        send(s, buf, len, 0);

        char buf_2[MAX_LINE+60];
        buf_2[MAX_LINE + 60 - 1] = '\0';
        len = recv(s, buf_2, sizeof(buf_2), 0);
        printf("%s",buf_2);
		if (strcmp(buf, ">>> Ciao-Ciao\n") == 0 || strcmp(buf, "Ciao-Ciao\n") == 0)
		{
			close(s);
			return 0;
		}
	}
}
