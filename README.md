# Group Members

- **Kaspian Thoft-Christensen**
  - Username: chris473
  - Student ID: 1007066336
- **Jerry Han**
  - Username: hanmiao1
  - Student ID: 1007423566
- **Ankit Shrivastava**
  - Username: shriva53
  - Student ID: 1006884409

# Project Description

This project is a custom network security protocol designed to secure data transmission between a client and server, leveraging key features of TLS. It also implements a distributed server system where data is split among multiple servers, all while using the security protocol for communication.

### Primary Features

- **Secure Key Exchange:** Implements Diffie-Hellman key exchange algorithm.
- **Data Encryption:** Uses AES for encryption of transmitted data.
- **Certificate Authority:** Includes a basic certificate authority for signing and verifying certificates.
- **Distributed Database:** Implements a sharded database with three servers, where data is split and stored securely.

# Distributed Blockchain Database

This project mimics a blockchain-style distributed database system.

### Blockchain Overview

- **Blockchain Structure:** A blockchain is a linked list of blocks containing:
  - Data.
  - The current hash of the block.
  - The hash of the previous block.
- **Tamper Detection:** If a block is tampered with, its hash will no longer match the `previous_hash` field of the next block, ensuring integrity.

### Our Setup

- **Sharded Database:** The project uses sharding with one router coordinating three shards/servers. Each shard has its own blockchain.
- **Blockchain Validation:** Each server maintains its own chain and validates it to detect tampering.
- **TLS Encryption:** All communication between the client, router, and servers is encrypted using TLS.

### Workflow

1. **Storing Data:**
   - The client sends a store request with a payload to the router.
   - The router forwards the request to one of the shards/servers in a round-robin fashion.
   - The shard stores the data in a linked list of blocks (its blockchain).

2. **Retrieving Data:**
   - The client sends a retrieve request to the router.
   - The router queries all shards/servers and retrieves their blockchain data.
   - The router concatenates the data and returns it to the client.

### Block Structure

Each block in the blockchain contains:
```c
typedef struct Block {
    char data[MAX_LINE];
    char hash[HASH_SIZE];
    char previous_hash[HASH_SIZE];
    struct Block* next;
} Block;
```

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

## Analysis and Discussion


This project demonstrates a secure and efficient method of transmitting data using TLS-like encryption and a distributed blockchain-based database. Here are key insights and potential extensions for the system:

### Extending the Network

- **Multiple Nodes Per Shard:** 
  - Currently, each shard contains only one server (node). The network can be extended to include multiple nodes per shard.
  - Each node within a shard would maintain a copy of the shard's blockchain, ensuring redundancy and fault tolerance.
  - Nodes within a shard can synchronize their blockchains periodically to ensure data consistency.

- **Basic Proof of Work:**
  - Each shard could implement a lightweight version of blockchain proof-of-work (PoW).
  - Before storing a block, nodes would solve a simple computational puzzle to validate the block. This would prevent malicious or invalid data from being added to the blockchain.

- **Inter-Node Communication:**
  - Nodes within a shard can communicate to reach a consensus before adding a new block or responding to a retrieve request.
  - Consensus protocols, such as a simplified version of Raft or Paxos, can be implemented to enhance reliability.


## Concluding remarks