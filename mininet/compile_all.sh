#!/bin/bash

# Ensure all scripts in the project directory are executable
echo "Making all scripts executable..."
find . -name "*.sh" -exec chmod +x {} \;

echo "Compiling Client..."
cd client
if [ -f Makefile ]; then
    make clean
    make
else
    rm -f client
    gcc client.c -o client
fi
cd ..

echo "Compiling Router..."
cd router
if [ -f Makefile ]; then
    make clean
    make
else
    rm -f router
    gcc router.c -o router
fi
cd ..

echo "Compiling Server..."
cd server
if [ -f Makefile ]; then
    make clean
    make
else
    rm -f server
    gcc server.c -o server
fi
cd ..

echo "Compilation Complete!"

