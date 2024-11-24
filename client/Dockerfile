FROM gcc:latest

# Set the working directory
WORKDIR /shared/client/

# Copy the source code and Makefile into the container
COPY client.c .
COPY client.h .
COPY Makefile .

# Compile the source code into an executable
RUN make  # Use Makefile to build the client executable

# Set the default command to run 
