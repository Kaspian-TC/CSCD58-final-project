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

## Analysis and discussion

## Concluding remarks


