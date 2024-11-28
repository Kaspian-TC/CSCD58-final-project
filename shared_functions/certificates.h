#include <stdio.h>
int sign_data(const char *private_key_file,const char *data, long data_len, char ** signature,size_t * signed_len, char * password);
int validate_signed_data(const char *public_key_file, const char *data, long data_len, char * signature, size_t sig_len);
void get_public_key(const char *private_key_path, char **public_key, size_t *public_key_len,char * password);