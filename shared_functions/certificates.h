#include <stdio.h>
#include <stdint.h>
int sign_data(const char *private_key_file,const uint8_t *data, long data_len, uint8_t ** signature,size_t * signed_len, char * password);
int validate_signed_data(const char *public_key_file, const uint8_t *data, long data_len, uint8_t * signature, size_t sig_len);
void get_public_key(const char *private_key_path, uint8_t **public_key, size_t *public_key_len,char * password);