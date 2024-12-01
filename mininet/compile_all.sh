#!/bin/bash

echo "Compiling Client..."
cd client
if [ -f Makefile ]; then
    make clean
    make
else
    gcc client.c -o client
fi
cd ..

echo "Compiling Router..."
cd router
if [ -f Makefile ]; then
    make clean
    make
else
    gcc router.c -o router
fi
cd ..

echo "Compiling Server..."
cd server
if [ -f Makefile ]; then
    make clean
    make
else
    gcc server.c -o server
fi
cd ..

echo "Compilation Complete!"