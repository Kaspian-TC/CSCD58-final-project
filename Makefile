.PHONY: run clean stop

# Target to start the containers
run:
	@echo "Running start.sh script..."
	@bash start.sh

# Target to stop and clean up containers
stop:
	@echo "Stopping and removing Docker containers..."
	@docker-compose down

# Target to clean up everything
clean:
	@echo "Removing all containers, networks, and images..."
	@docker-compose down --rmi all --volumes --remove-orphans
