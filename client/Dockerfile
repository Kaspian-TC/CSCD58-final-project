FROM gcc:latest

# Set working directory
WORKDIR /app

# Copy source code into the container
COPY client.c /app/

# Compile and run the client at container startup
CMD gcc -o client client.c -lpthread && ./client
