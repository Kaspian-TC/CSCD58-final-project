FROM gcc:latest

# Set working directory
WORKDIR /app

# Copy source code into the container
COPY server.c /app/

# Expose the port for the server
EXPOSE 8080

# Compile and run the server at container startup
CMD gcc -o server server.c -lpthread && ./server
