# Group members

- Kaspian Thoft-Christensen
	- chris473
    - 1007066336
- Jerry Han
    - hanmiao1
    - 1007423566
- Ankit Shrivastava
    - shriva53
    - 1006884409

# Report

## Project description

This project is a custom network security protocol that secures data transmission between a client and a server, using the key features in TLS. It also implements a distributed server, such that data is split among multiple servers, using the previously mentional security protocol to communicate.

The primary features of the security protocol include:

- Securly exchanging keys using the Diffie-Hellman key exchange algorithm 
- Encrypting the data using AES
- Signing data, and checking certificates
    - Having a basic certificate authority to get certificates from
- A distributed server system that stores splits data between three servers

## Team member contribution

- Kaspian:
    - Prime number generation
        - Using using prime numbers with HKDF
    - Certificate signing and checking 
- Jerry:
    - HKDF key generation
    - AES encryption
- Ankit:
    - Distributed server setup

## How to run tests

## Implementation details and documentation

### key_exchange.c

`void get_big_prime(mpz_t prime, gmp_randstate_t state)`

- This function takes in a state and produces a prime of size 2048 bits.

`void send_client_hello(int socket,
 mpz_t prime, 
 mpz_t dhA_mpz, 
 mpz_t a,
 gmp_randstate_t state, uint8_t * n0)`

- In the key exchange between a client and a server, prepares the values for the client to send to send to the server. 
- This is initiating the key exchange from the client side.
- It sends a random large prime number, the dhA (g^a mod p), and a nonce (n0)

`static void get_public_key_file(
    const char *server_public_key_file, 
    uint8_t **server_public_key, 
    size_t *server_public_key_len)`

- Takes in a public key and assigns the server_public_key variable to the contents of that public key.

`static int is_server_response_sign_valid(
    uint8_t* n0,uint8_t* n1,uint8_t* dhA_bytes,uint8_t* dhB_bytes,uint8_t* signature, long signature_len, uint8_t * public_key_file, size_t public_key_len)`

- This is a helper function for receive_server_hello
- The client takes in the a bit of data signed by the server, and validates if that was actually signed by the server
- The data is of the form, cleint nonce + server nonce + dhA + dhB + server public key

`uint8_t * receive_server_hello(int socket, 
mpz_t prime, 
mpz_t dhA_mpz, 
mpz_t a,
gmp_randstate_t state, 
uint8_t * master_key_bytes,
uint8_t * n0, 
uint8_t * n1,
uint8_t * session_key /* Assume 32 bytes */)`

- This function handles the receiving of data from the server for the key exchange.
- This function receives, dhB (g^b mod p), the server nonce (n1), a tag for verifying encrypted data, and a ciphertext to decrypt
    - the ciphertext contains a signature, and the server certificate
- This function produces the session key on the client side, and uses it to decrypt the ciphertext, and check the signature within that ciphertext
- It returns the session key, which lets the client communicate with the server

`uint8_t * client_get_session_key(int socket, uint8_t * session_key /* assumed 256 length */,
 gmp_randstate_t state)`

- This function manages the entire diffie-hellman key exchange for the client, and returns the session key

`void receive_client_hello(int socket, mpz_t prime, mpz_t dhA_mpz,
gmp_randstate_t state,uint8_t* n0,uint8_t* n1)`

- Receives the initial key exchange values from the client
- It receives a random large prime number, the dhA (g^a mod p), and a nonce (n0)

`static void sign_data_to_client(
    uint8_t* n0,
    uint8_t* n1,
    uint8_t* dhA_bytes,
    uint8_t* dhB_bytes,
    uint8_t** signature, 
    size_t * signature_len,
    uint8_t *public_key, 
    size_t public_key_len,
    const char *private_key_file)`

- Helper function for send_server_hello, that signs the data in the form: client nonce + server nonce + dhA + dhB + server certificate

`uint8_t * send_server_hello(int socket,
 mpz_t prime, 
 mpz_t dhA_mpz, 
 gmp_randstate_t state, uint8_t * master_key_bytes /* assume 256 bytes */,
 uint8_t * n0, uint8_t * n1, uint8_t * session_key /* assume 32 */)`

- Generates the session key, encrypting 
- Encrypts a ciphertext that includes a signature and the certificate of the server
- This function sends, dhB (g^b mod p), the server nonce (n1), a tag for verifying encrypted data, and a ciphertext to decrypt to the client

`uint8_t * server_get_session_key(int socket,  uint8_t * session_key  /* assumed 32 length */,
 gmp_randstate_t state)`

- This function manages the entire diffie-hellman key exchange for the server, and returns the session key

### certificates.c

`void handle_openssl_error()`

- Handles errors for openssl signing and exits

`int sign_data(const char *private_key_path /* string */,const  uint8_t *data, long data_len, uint8_t ** signature, size_t * signed_len, char * password)`

- Takes in a private key, and data, to then sign the data.

`int validate_signed_data(const char *public_key_file, const uint8_t *data, long data_len, uint8_t * signature, size_t sig_len)`

- Takes in a public key, signed data, and the expect unsigned data, and confirms if the signed data was signed with the public key

`void get_public_key(const char *private_key_path, uint8_t **public_key, size_t *public_key_len,char * password)`

- Produces a public key from a private key, without having to create a file for it. 
- This is used when the server needs to send the certificate (their public key) to the client

`void get_random_bytes(uint8_t *bytes, int length,gmp_randstate_t state)`

- Generates an array of random bytes given a length, and state

`void initialize_values(const mpz_t prime, mpz_t dh, mpz_t secret,
 gmp_randstate_t state)`

- Computes the dh (g^secret mod p) for a given a large prime
- Also computes the secret number

`uint8_t *create_session_key(uint8_t *master_key, uint8_t *salt, uint8_t* session_key)`

- Given the master key computed using the diffie-hellman exchange, and a salt(client nonce + server nonce), create an HKDF key for encrypting using AES-256-GCM 

`int aes_decrypt(uint8_t *ciphertext, int ciphertext_len,
 uint8_t *aad, int aad_len,
 uint8_t *tag,
 uint8_t *key,
 uint8_t *iv, int iv_len,
 uint8_t *plaintext)`

- Decrypt a given ciphertext with a key, and output it into a plaintext
- Uses a nonce to decrypt it (from the sender of the data)
- Check to make sure that the tag verifies the encryption signature

`int aes_encrypt(uint8_t *plaintext, int plaintext_len,
 uint8_t *aad, int aad_len,
 uint8_t *key,
 uint8_t *iv, int iv_len,
 uint8_t *ciphertext,
 uint8_t *tag)`

- Encrypt a given plaintext with a key, and output it into a ciphertext
- Uses a nonce to encrypt
- Also outputs a tag

`int send_encypted_data(int socket, uint8_t *data, int data_len, uint8_t *session_key, gmp_randstate_t state)`

- Given a socket id, data to be sent, and a hkdf session key, send encrypted data to the socket id
- This function makes sure to use a randomly generated nonce to encrypt the data

`uint8_t * receive_encypted_data(int socket, int * data_len, uint8_t *session_key)`

- Processes receiving data, and unencrypt it using the session key provided

## Analysis and discussion

## Concluding remarks


