#include <stdio.h>
#include <string.h>
#include <stdlib.h>

void store_data(const char* filename, const char* key, const char* value) {
    FILE* file = fopen(filename, "a+");
    if (file == NULL) {
        perror("Error opening file");
        return;
    }
    fprintf(file, "%s=%s\n", key, value);
    fclose(file);
}

const char* retrieve_data(const char* filename, const char* key) {
    static char value[256];
    char line[512];
    FILE* file = fopen(filename, "r");
    if (file == NULL) {
        perror("Error opening file");
        return "NOT_FOUND";
    }
    while (fgets(line, sizeof(line), file)) {
        char file_key[256], file_value[256];
        if (sscanf(line, "%255[^=]=%255s", file_key, file_value) == 2) {
            if (strcmp(file_key, key) == 0) {
                strcpy(value, file_value);
                fclose(file);
                return value;
            }
        }
    }
    fclose(file);
    return "NOT_FOUND";
}

int main(int argc, char* argv[]) {
    const char* filename = "server_data.txt";  // Change this based on the server (e.g., server1.txt)
    
    if (argc < 2) {
        fprintf(stderr, "Usage: %s --store key=value | --retrieve key\n", argv[0]);
        return 1;
    }

    if (strcmp(argv[1], "--store") == 0 && argc == 3) {
        char key[256], value[256];
        if (sscanf(argv[2], "%255[^=]=%255s", key, value) == 2) {
            store_data(filename, key, value);
            printf("Data stored: %s=%s\n", key, value);
        } else {
            fprintf(stderr, "Invalid format for --store. Use key=value.\n");
        }
    } else if (strcmp(argv[1], "--retrieve") == 0 && argc == 3) {
        const char* result = retrieve_data(filename, argv[2]);
        printf("Retrieved: %s\n", result);
    } else {
        fprintf(stderr, "Invalid arguments.\n");
    }

    return 0;
}
