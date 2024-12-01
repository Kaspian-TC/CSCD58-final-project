#include <gmp.h>

#define BUFFER_SIZE 1048576 // 1 MB
#define DH_NUM_BITS 2048
#define DH_G 5
#define DH_KEY_SIZE 256
#define DH_NONCE_SIZE 16
#define AES_KEY_SIZE 32
#define AES_TAG_SIZE 16

void get_random_bytes(char *bytes, int length,gmp_randstate_t state);
void initialize_values(const mpz_t prime, mpz_t dhA_mpz, mpz_t a,
 gmp_randstate_t state);
void print_bytes(char *bytes, int length);
unsigned char *create_session_key(unsigned char *master_key, unsigned char *salt, unsigned char* session_key);
void create_salt(char *salt, char *n0, char *n1);

// aes functions
int aes_encrypt(unsigned char *plaintext, int plaintext_len,
 unsigned char *aad, int aad_len,
 unsigned char *key,
 unsigned char *iv, int iv_len,
 unsigned char *ciphertext,
 unsigned char *tag);

int aes_decrypt(unsigned char *ciphertext, int ciphertext_len,
    unsigned char *aad, int aad_len,
    unsigned char *tag,
    unsigned char *key,
    unsigned char *iv, int iv_len,
    unsigned char *plaintext);
int send_encypted_data(int socket, char *data, int data_len, char *session_key, gmp_randstate_t state);
char * receive_encypted_data(int socket, int * data_len, char *session_key);