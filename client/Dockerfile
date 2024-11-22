FROM gcc:latest

# Set the working directory
WORKDIR /app/client/

# Copy the source code into the container
COPY ./ /app/client/

# Compile the source code into an executable
RUN make

# Set the default command to run the client
CMD ["./client"]
