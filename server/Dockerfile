FROM gcc:latest

# Set the working directory
WORKDIR /shared/server/

# Copy the source code and Makefile into the container
COPY server.c .
COPY server.h .
COPY Makefile .

# Compile the source code into an executable
RUN make  # Build the `server` executable

# Expose the server port
EXPOSE 5432

# Set the default command to run the server
ENTRYPOINT ["./server"]
