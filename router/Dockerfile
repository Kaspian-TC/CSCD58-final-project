FROM gcc:latest
WORKDIR /app
COPY router.c .
COPY router.h .
RUN gcc -o router router.c
CMD ["./router"]
