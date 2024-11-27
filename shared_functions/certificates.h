#include <stdio.h>
int sign_test_data(const char *private_key_file,const char *data, long data_len, char ** signature,size_t * signed_len);
int check_signed_data(const char *public_key_file, const char *data, long data_len, char * signature, size_t sig_len);