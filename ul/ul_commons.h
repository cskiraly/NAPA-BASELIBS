#ifndef _UL_COMMONS_H
#define _UL_COMMONS_H


#define UL_RETURN_OK 0
#define UL_RETURN_FAIL -1
#define UL_IP_ADDRESS_SIZE 20
#define UL_PATH_SIZE 128
#define UL_URL_SIZE 256
#define UL_MAX_EXTERNAL_APPLICATIONS 5

/**
 * define a new data type for the aggregated info about an application registering itself as chunk receiver
 */
typedef struct {
  char address[UL_IP_ADDRESS_SIZE];
  int port;
  char path[UL_PATH_SIZE];
  int status;
} ApplicationInfo;

/**
 * commodity function to print a block of bytes
 */
void print_block(const uint8_t *b, int size);


#endif
