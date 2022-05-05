/* DreamShell ##version##

   module.c - bitcoin module
   Copyright (C)2022 SWAT 
*/

#include "ds.h"
#include <secp256k1.h>

DEFAULT_MODULE_EXPORTS_CMD(bitcoin, "Bitcoin Core");

void secp256k1_default_illegal_callback_fn(const char *message, void *data) {
   (void)data;
   ds_printf("DS_ERROR: %s\n", message);
}

void secp256k1_default_error_callback_fn(const char *message, void *data) {
   (void)data;
   ds_printf("DS_ERROR: %s\n", message);
}

static void print_hex(char *str, uint8 *buffer, size_t size) {
   char tmp[4];
   memset(tmp, 0, sizeof(tmp));
   for (size_t i = 0; i < size; ++i) {
      snprintf(tmp, 4, "%02x", buffer[i]);
      strncpy(str, tmp, 2);
      str += 2;
   }
}

// FIXME: More random
size_t fill_random(uint8 *buffer, size_t size) {
   size_t i;
   for (i = 0; i < size; ++i) {
      buffer[i] = (rand() % 254) + 1;
   }
   return i;
}

int bitcoin_generate_keys(const char *output_filename) {

   /* Instead of signing the message directly, we must sign a 32-byte hash.
    * Here the message is "Hello, world!" and the hash function was SHA-256.
    * An actual implementation should just call SHA-256, but this case
    * hardcodes the output to avoid depending on an additional library. */
   uint8 msg_hash[32] = {
      0x31, 0x5F, 0x5B, 0xDB, 0x76, 0xD0, 0x78, 0xC4,
      0x3B, 0x8A, 0xC0, 0x06, 0x4E, 0x4A, 0x01, 0x64,
      0x61, 0x2B, 0x1F, 0xCE, 0x77, 0xC8, 0x69, 0x34,
      0x5B, 0xFC, 0x94, 0xC7, 0x58, 0x94, 0xED, 0xD3
   };

   uint8 seckey[32];
   uint8 randomize[32];
   uint8 compressed_pubkey[33];
   uint8 serialized_signature[64];
   size_t len;
   int return_val;
   secp256k1_pubkey pubkey;
   secp256k1_ecdsa_signature sig;
   secp256k1_context *ctx = secp256k1_context_create(SECP256K1_CONTEXT_SIGN | SECP256K1_CONTEXT_VERIFY);

   ds_printf("DS_PROCESS: Generating Bitcoin keys...\n");

   if (!ctx) {
      ds_printf("Failed to create secp256k1 context\n");
      goto out;
   }

   if (!fill_random(randomize, sizeof(randomize))) {
      ds_printf("Failed to generate randomness\n");
      goto out;
   }

   /* Randomizing the context is recommended to protect against side-channel
    * leakage See `secp256k1_context_randomize` in secp256k1.h for more
    * information about it. This should never fail. */
   return_val = secp256k1_context_randomize(ctx, randomize);

   if (!return_val) {
      ds_printf("Failed to randomize context\n");
      goto out;
   }

   /*** Key Generation ***/

   /* If the secret key is zero or out of range (bigger than secp256k1's
    * order), we try to sample a new key. Note that the probability of this
    * happening is negligible. */
   while (1) {
      return_val = fill_random(seckey, sizeof(seckey));
      if (!return_val) {
         ds_printf("Failed to generate randomness\n");
         goto out;
      }
      if (secp256k1_ec_seckey_verify(ctx, seckey)) {
         break;
      }
   }

   /* Public key creation using a valid context with a verified secret key should never fail */
   return_val = secp256k1_ec_pubkey_create(ctx, &pubkey, seckey);

   if (!return_val) {
      ds_printf("Failed create public key\n");
      goto out;
   }

   /* Serialize the pubkey in a compressed form(33 bytes). Should always return 1. */
   len = sizeof(compressed_pubkey);
   return_val = secp256k1_ec_pubkey_serialize(ctx, compressed_pubkey, &len, &pubkey, SECP256K1_EC_COMPRESSED);

   if (!return_val) {
      ds_printf("Failed serialize the signature\n");
      goto out;
   }

   /* Should be the same size as the size of the output, because we passed a 33 byte array. */
   if (len != sizeof(compressed_pubkey)) {
      goto out;
   }

   /*** Signing ***/

   /* Generate an ECDSA signature `noncefp` and `ndata` allows you to pass a
    * custom nonce function, passing `NULL` will use the RFC-6979 safe default.
    * Signing with a valid context, verified secret key
    * and the default nonce function should never fail. */
   return_val = secp256k1_ecdsa_sign(ctx, &sig, msg_hash, seckey, NULL, NULL);

   if (!return_val) {
      ds_printf("Failed sign\n");
      goto out;
   }

   /* Serialize the signature in a compact form. Should always return 1
    * according to the documentation in secp256k1.h. */
   return_val = secp256k1_ecdsa_signature_serialize_compact(ctx, serialized_signature, &sig);

   if (!return_val) {
      ds_printf("Failed serialize the signature\n");
      goto out;
   }

   /*** Verification ***/

   /* Deserialize the signature. This will return 0 if the signature can't be parsed correctly. */
   return_val = secp256k1_ecdsa_signature_parse_compact(ctx, &sig, serialized_signature);

   if (!return_val) {
      ds_printf("Failed parsing the signature\n");
      goto out;
   }

   /* Deserialize the public key. This will return 0 if the public key can't be parsed correctly. */
   return_val = secp256k1_ec_pubkey_parse(ctx, &pubkey, compressed_pubkey, sizeof(compressed_pubkey));

   if (!return_val) {
      ds_printf("Failed parsing the public key\n");
      goto out;
   }

   /* Verify a signature. This will return 1 if it's valid and 0 if it's not. */
   return_val = secp256k1_ecdsa_verify(ctx, &sig, msg_hash, &pubkey);

   if (!return_val) {
      ds_printf("DS_ERROR: Signature is invalid, please try again.\n");
      goto out;
   }

   char _result_str[512];
   char *result_str = _result_str;
   char hex_str[256];
   memset(result_str, 0, sizeof(_result_str));
   memset(hex_str, 0, sizeof(hex_str));

   result_str = strcat(result_str, "Secret Key: 0x");
   print_hex(hex_str, seckey, sizeof(seckey));
   result_str = strncat(result_str, hex_str, sizeof(hex_str));
   result_str = strcat(result_str, "\nPublic Key: 0x");
   print_hex(hex_str, compressed_pubkey, sizeof(compressed_pubkey));
   result_str = strncat(result_str, hex_str, sizeof(hex_str));
   result_str = strcat(result_str, "\nSignature: 0x");
   print_hex(hex_str, serialized_signature, sizeof(serialized_signature));
   result_str = strncat(result_str, hex_str, sizeof(hex_str));
   result_str = strcat(result_str, "\n");

   if (output_filename) {

      file_t fd = fs_open(output_filename, O_WRONLY);

      if (fd < 0) {
         ds_printf("Can't open file: %s\n", output_filename);
      } else {
         fs_write(fd, result_str, strlen(result_str));
         fs_close(fd);
      }
   } else {
      ds_printf(result_str);
   }

   ds_printf("DS_OK: Generated successfully.\n");

out:
   if (ctx) {
      secp256k1_context_destroy(ctx);
   }
   memset(seckey, 0, sizeof(seckey));
   return (return_val ? CMD_OK : CMD_ERROR);
}


int builtin_bitcoin_cmd(int argc, char *argv[]) {

   if(argc < 2) {
      ds_printf("Usage: %s options args\n"
                "Options: \n"
                " -g, --gen    -Generate keys\n\n");
      ds_printf("Arguments: \n"
                " -f, --file   -Save keys to file instead of print screen\n\n"
                "Example: %s -g -f /ram/btc.txt\n\n", argv[0]);
      return CMD_NO_ARG;
   }

   int gen = 0;
   char *file = NULL;

   struct cfg_option options[] = {
      {"gen",  'g', NULL, CFG_BOOL, (void *) &gen, 0},
      {"file", 'f', NULL, CFG_STR, (void *) &file, 0},
      CFG_END_OF_LIST
   };

  	CMD_DEFAULT_ARGS_PARSER(options);

   if (gen) {
      return bitcoin_generate_keys(file);
   }

   return CMD_NO_ARG;
}
