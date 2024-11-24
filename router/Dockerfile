FROM gcc:latest
WORKDIR /app
COPY router.c .
RUN gcc -o router router.c
CMD ["./router"]
