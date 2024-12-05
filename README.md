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

### Setup/Config

Ideally, the .ova file will find its way, and this setup will not be required, but in the case that it does not, here is a guide on how to set up a mininet environment for running the tests:

- Make sure to use a mininet image, so that we can run mininet.
- Create a folder in the home directory of the vm called `mininet_project`. This will be where all files will be sent to
- From the given repository, take the client, router, and server folder, and put them under `mininet_project`. Then take the folder just called `mininet` a move its contents into the root of `mininet_project`. 
- The layout of the folder should now look like this:
```
.
|-- client/
|-- compile_all.sh
|-- deploy_and_run.py
|-- deploy_topology.py
|-- get_openssl.sh
|-- router/
|-- server/
|-- shared_functions/
`-- topology.py

```

- This project uses two libraries, Openssl and LibGMP. Libgmp should already be included, but if not, you can run `sudo apt install libgmp-dev` to install it.
- Openssl is a bit trickier. Due to some older versions of ubuntu LTS rolling back the latest supported version of Openssl, we need to get a newer version of Openssl. In the case where you have openssl 3.x.x already installed, you do not need to continue here
    - In the root of `mininet_project`, run `./get_openssl.sh` with sudo permissions. This will remove the existing libssl-dev library, download a newer version (September 2024), build it, install it, then link the libraries.
    - THIS WILL TAKE A LONG TIME. Please expect at least 10 minutes for it to finish building and installing.
    - Check that you are on the correct version of Openssl by running `openssl version`. It should be 3.0.15

### Running the tests


- There is a simple test which just shows the client side. Run `sudo python3 deploy_and_run.py`, to start the topology and start the router and servers.
- The client will automatically send data to the distributed servers, and then will ask to retrieve that data from the servers.
- In the mininet terminal, you can run h1 `./client --store 10.0.0.2` or `./client --retrieve 10.0.0.2`

- A more detailed test is described below
- First start by setting the X11 magic cookie as the same for the root user (make sure you connect over ssh)
- Navigate to `mininet_project` and compile and the necessariy code by running `./compile_all.sh`. Then to start the topology, run `sudo python3 deploy_topology.py`.
    - For some unknown reason, this sometimes does not work. Some fixes have been
        - Using python (no 3)
        - Not running the command in ssh, instead running it in the (not recommended for next step)
- In the mininet terminal, open xterm for h1,h2,h3,h4, and h5. h1 is meant for simulating the client, h2 is meant for simulating the "router" (a server which interacts directly with the client) and h3-h5 are distributed servers which provide data to the router.
- In h2, run `cd router` then `make run` to start the router
- In h3-h5, run `cd server` then `make run` to start the distributed servers.
- In h1, run `cd router` then `make retrieve` to simulate retrieving the data from server.
    - This should result in the server sending back a response showing that no data is being stored.
- Run `make store` to store some data in the server (this will be randomly generated data)
- Running `make retrieve` again will show that there is now data in the first server (h3)
- Running `make store` subsequent times will store data in the h4 and h5 servers.

### Alternative execution

- There is an alternative test mode, that was used for just talking between a client and server, but uses docker.
- Switch to the aes branch in the repository (which should be bundled)
- If you have docker and tmux installed, run `./launch_cli_ser.sh`
    - This should make a split tmux window with the server running in the right pane and the client window in the left
- Start the client by running `make run` in the left pane pane
- Now you can communicate with the server. Everything sent to the server gets sent back with a timestamp, just like in A1.

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

### scripts
### helper_func.c

`void get_random_bytes(uint8_t *bytes, int length,gmp_randstate_t state)`

- Generates an array of random bytes of the specified length using GMP's random state.

`void initialize_values(const mpz_t prime, mpz_t dh, mpz_t secret, gmp_randstate_t state)`

- Initializes values for a Diffie-Hellman key exchange:
    - Generates a random secret value within the specified bit size (DH_NUM_BITS).
    - Computes the Diffie-Hellman value (dh = g^secret mod prime), where g is a predefined generator (DH_G) and prime is the shared prime.

`uint8_t *create_session_key(uint8_t *master_key, uint8_t *salt, uint8_t* session_key)`
 
- Uses the HKDF (HMAC-based Key Derivation Function) algorithm to derive a session key:
    - Configures HKDF with SHA256 as the hash function and a context-specific info string ("Session key").
    - Generates the derived session key and stores it in session_key.

`void create_salt(uint8_t *salt, uint8_t *n0, uint8_t *n1)`
 
- Creates a salt for the HKDF key derivation by concatenating two nonces (n0 and n1) into the salt buffer.

`void print_bytes(uint8_t *bytes, int length)`
 
- Prints the contents of a byte array in decimal format for debugging or analysis purposes.

`int aes_decrypt(uint8_t *ciphertext, int ciphertext_len,
 uint8_t *aad, int aad_len,
 uint8_t *tag,
 uint8_t *key,
 uint8_t *iv, int iv_len,
 uint8_t *plaintext)`
 
- Performs AES-GCM decryption:
    - Decrypts ciphertext using the provided key and initialization vector (iv).
    - Uses optional Additional Authenticated Data (aad) to enhance security.
    - Validates the integrity of the ciphertext using the provided authentication tag (tag).

`int aes_encrypt(uint8_t *plaintext, int plaintext_len,
 uint8_t *aad, int aad_len,
 uint8_t *key,
 uint8_t *iv, int iv_len,
 uint8_t *ciphertext,
 uint8_t *tag)`
 
- Performs AES-GCM encryption:
    - Encrypts plaintext using the provided key and initialization vector (iv).
    - Generates an authentication tag (tag) for integrity verification.
    - Supports optional Additional Authenticated Data (aad).

`int send_encypted_data(int socket, uint8_t *data, int data_len, uint8_t *session_key, gmp_randstate_t state)`
 
- Encrypts the data using AES-GCM with the provided session_key and nonce, constructs a payload containing the nonce, authentication tag, and ciphertext and sends the payload over the provided socket.

`uint8_t * receive_encypted_data(int socket, int * data_len, uint8_t *session_key)`
 
- Receives and extracts the components from the payload. Decrypts data using AES-GCM with the provided session_key and return it.

### server.c

`void compute_hash(Block* block, const char* previous_hash, char* output_hash)`

- Calculates a SHA-256 hash for a block by combining its data with the hash of the previous block. Produces the hash as a hexadecimal string for integrity verification.

`void store_data(const char* payload)`

- Creates a new block with the provided payload, computes its hash, links it to the blockchain, and validates the blockchain to ensure its integrity.

`void retrieve_data(int client_sock, char* response, int* response_length)`
 
- Prepares and sends a response containing all blocks in the blockchain, including their data, hashes, and previous hashes. Ensures the blockchain is validated before data retrieval and handles cases where no data exists.

`int validate_blockchain()`
 
- Checks the integrity of the entire blockchain by verifying that each block’s hash is accurate and matches its previous_hash field. Detects and reports tampering if any discrepancies are found.

`void free_blockchain()`
 
- Releases all memory allocated for blocks in the blockchain and resets the blockchain to an empty state to prevent memory leaks.

`void handle_client(int client_sock,gmp_randstate_t state)`
 
- Manages client communication, including secure session key negotiation, encrypted data reception, and request processing. Supports data storage in the blockchain and blockchain retrieval. Ensures all responses are encrypted for secure transmission.

`int main()`
 
- Initializes and runs the blockchain server. 
- Configures the network socket, handles incoming client connections, and delegates request handling to handle_client. 
- Ensures blockchain integrity is maintained throughout server operation. Cleans up resources on termination.

### client.c

`void generate_random_data(char* buffer, size_t length)`

- Generates a random string with a prefix (RandomData_) and stores it in the provided buffer. The randomness is seeded using the current time

`void store_data(int sockfd,gmp_randstate_t state, uint8_t * session_key /* Assumed AES_BYTES long */)`

- Sends randomly generated data securely(encrypted) to the server, then wait for receiving and decrypts the server's response.

`void retrieve_data(int sockfd,gmp_randstate_t state, uint8_t * session_key /* Assumed AES_BYTES long */)`
 
- Requests all data stored on the server's blockchain and decrypts + prints the received data.

`int main(int argc, char** argv)`
 
- Acts as the entry point for the client application. 
- Establishes a TCP connection with the server and negotiates a secure session key.
- Manages client-server communication for storing or retrieving blockchain data.

### router.c

`void forward_to_server(const char* server_ip, const char* message, char* response,gmp_randstate_t state)`

- Facilitates communication between the router and a specific server. 
- Establishes a connection to the specified server, performs a secure session key exchange, forwards the client's encrypted message to the server, and retrieves the server's encrypted response. 
- The response is decrypted and stored for further processing.

`void handle_client(int client_sock,gmp_randstate_t state)`

- Manages communication with a connected client and forwards their requests to the appropriate servers. Establishes a secure session with the client and processes their requests:
- For RETRIEVE, queries all servers, aggregates their responses, and returns the consolidated result to the client.
- For other messages, routes the request to one server based on a round-robin strategy and relays the server’s response back to the client. All communication is encrypted for security.

`int main()`
 
- Initializes and operates the router application as a TCP server that listens for incoming client connections, forwards their requests to servers, and handles secure communication. 
Configures socket settings, uses a secure random state for key exchange, and delegates each client connection to handle_client for processing. 


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

This assignment really made us realize how difficult it is to parse data between a client and server, especially using sockets in C. Using mininet for the distributed systems was not the easiest either, and led to many issues. In the future, it might be easier to use another form of simulating a network.