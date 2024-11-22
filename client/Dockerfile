FROM gcc:latest

# Set the working directory
WORKDIR /shared/client/

# Copy the source code into the container
COPY client.c /shared/client/

# Compile the source code into an executable
RUN gcc -o client client.c -lpthread -lm -lgmp

# Set the default command to run the client
CMD ["./client"]
