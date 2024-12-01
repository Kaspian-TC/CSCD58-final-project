#include <gmp.h>
#include <stdint.h>	

#define BUFFER_SIZE 1048576 // 1 MB
#define DH_NUM_BITS 2048
#define DH_G 5
#define DH_KEY_SIZE 256
#define DH_NONCE_SIZE 16
#define AES_KEY_SIZE 32
#define AES_TAG_SIZE 16

void get_random_bytes(uint8_t *bytes, int length,gmp_randstate_t state);
void initialize_values(const mpz_t prime, mpz_t dhA_mpz, mpz_t a,
 gmp_randstate_t state);
void print_bytes(uint8_t *bytes, int length);
uint8_t *create_session_key(uint8_t *master_key, uint8_t *salt, uint8_t* session_key);
void create_salt(uint8_t *salt, uint8_t *n0, uint8_t *n1);

// aes functions
int aes_encrypt(uint8_t *plaintext, int plaintext_len,
 uint8_t *aad, int aad_len,
 uint8_t *key,
 uint8_t *iv, int iv_len,
 uint8_t *ciphertext,
 uint8_t *tag);

int aes_decrypt(uint8_t *ciphertext, int ciphertext_len,
    uint8_t *aad, int aad_len,
    uint8_t *tag,
    uint8_t *key,
    uint8_t *iv, int iv_len,
    uint8_t *plaintext);
int send_encypted_data(int socket, uint8_t *data, int data_len, uint8_t *session_key, gmp_randstate_t state);
uint8_t * receive_encypted_data(int socket, int * data_len, uint8_t *session_key);