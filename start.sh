#!/bin/bash

# Exit immediately if a command exits with a non-zero status
set -e

echo "Building Docker images..."
docker-compose build --no-cache

echo "Starting Docker containers..."
docker-compose up -d  # Run containers in detached mode

echo "All containers are up and running."
echo "To access the client, use: docker exec -it client bash"
echo "To stop the containers, use: make stop"
