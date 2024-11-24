FROM gcc:latest

# Set the working directory for the router
WORKDIR /app/router

# Copy the router folder to the container
COPY . /app/router/

# Ensure shared folder is available
VOLUME /app/shared

# Build the router executable during the image build process
RUN make

# Default command to start the router
CMD ["./router"]
