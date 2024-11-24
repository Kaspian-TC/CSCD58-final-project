FROM gcc:latest

# Set the working directory
WORKDIR /app/client

# Copy the client folder to the container
COPY . /app/client/

# Ensure shared folder is available
VOLUME /app/shared

# Build the client executable during the image build process
RUN make

# Default command to start the client
CMD ["bash"]