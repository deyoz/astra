#ifndef __CRYPTING_H_
#define __CRYPTING_H_

#ifdef HAVE_SSL

// All these functions return 0 on success or error code on failure
int get_sym_key(const char *head, int hlen, unsigned char *key_str, size_t key_len, int *sym_key_id);
int get_our_private_key(char *privkey_str, int *privkey_len );
int get_public_key(const char *head, int hlen, char *pubkey_str, int *pubkey_len);

int RSA_decrypt(const unsigned char *src_str, size_t src_len,
                unsigned char *dest_str, size_t* dest_len,
                RSA *rsa_our, RSA *rsa_remote);

int RSA_encrypt(const unsigned char *src_str, size_t src_len,
                unsigned char *dest_str, size_t *dest_len,
                RSA *rsa_our, RSA *rsa_remote);

#endif // HAVE_SSL

#endif // __CRYPTING_H_
