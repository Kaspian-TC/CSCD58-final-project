#!/bin/bash

# start docker
docker compose up -d 

# check if work session already exists, kill it
tmux has-session -t work && tmux kill-session -t work

# start tmux session
tmux new -s work -d

# open into the client
tmux send-keys -t work 'docker exec -it client bash' C-m
# run make in client
tmux send-keys -t work 'make' C-m
# split window
tmux split-window -h -t work
tmux send-keys -t work 'docker exec -it server bash' C-m
tmux send-keys -t work 'make run' C-m

tmux attach -t work
