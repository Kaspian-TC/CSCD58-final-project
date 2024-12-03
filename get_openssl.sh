#!/bin/bash
#wnload the latest OpenSSL source
sudo apt remove libssl-dev

wget https://www.openssl.org/source/openssl-3.0.15.tar.gz
tar -xvzf openssl-3.0.15.tar.gz
cd openssl-3.0.15

# Configure, build, and install
./Configure
make
sudo make install

# make sure to link the libraries
sudo ldconfig /usr/local/lib64/
