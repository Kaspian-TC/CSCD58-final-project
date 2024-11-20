FROM gcc:latest

# Set the working directory
WORKDIR /shared

# Copy the source code into the container
COPY client.c /shared/

# Compile the source code into an executable
RUN gcc -o client client.c -lpthread

# Set the default command to run the client
CMD ["./client"]
