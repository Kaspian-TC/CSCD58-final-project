FROM gcc:latest

# Set the working directory
WORKDIR /app

# Copy the source code into the container
COPY server.c /app/

# Compile the source code into an executable
RUN gcc -o server server.c -lpthread -lm

# Expose the server port
EXPOSE 5432

# Set the default command to run the server
CMD ["./server"]
