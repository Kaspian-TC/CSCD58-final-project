FROM gcc:latest

# Set the working directory for the server
WORKDIR /app/server

# Copy the server folder to the container
COPY . /app/server/

# Ensure shared folder is available
VOLUME /app/shared

# Build the server executable during the image build process
RUN make

# Default command to start the server
CMD ["./server"]
