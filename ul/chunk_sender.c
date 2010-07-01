/**
 * @file chunk_sender.c
 *
 * Chunk Sender "object".
 *
 * No threads, thus it is based on callbacks and timer events.
 *
 * Napa-Wine project 2009-2010
 * @author: Giuseppe Tropea <giuseppe.tropea@lightcomm.it>
 */

#include <string.h>

#include "chunk_external_interface.h"
#include "ul_commons.h"

//this definition should seat in a central place...
#define UL_ENCODED_CHUNK_HEADER_SIZE 20

int ulSendChunk(Chunk *c) {
  //they will be NULL just the first time
  static struct evhttp_connection *htc[UL_MAX_EXTERNAL_APPLICATIONS]; //initialized to NULL by compiler
  char addr[UL_IP_ADDRESS_SIZE] = "";
  int port = 0;
  char path[UL_PATH_SIZE] = "";
  int i = 0;
  //the code in this function cycles through all registered applications,
  //so the return code should be an "OR" of the singular return values...
  int ret = 0;

  for(i=0; i<UL_MAX_EXTERNAL_APPLICATIONS; i++) {
    addr[0] = '\0';
    port = 0;
    path[0] = '\0';

    //check whether we have a registered application at position i
    if(ulRegisterApplication(addr, &port, path, &i) == UL_RETURN_FAIL) {
      //some problem occurred
      error("ulSendChunk unable to check connection at position %d", i);
      ret = UL_RETURN_FAIL;
    }
    else {
      //lets check if the RegisterApplication has given back an address for the empty slot
      if(strlen(addr)) {
/*
        if(htc[i] == NULL) { //we have an address but not a connection yet
          //setup a new connection with the newly registered application and put it in position i
          if(ulEventHttpClientSetup(addr, port, &htc[i]) == UL_RETURN_FAIL) {
            error("ulSendChunk unable to setup a connection to %s:%d at position %d", addr, port, i);
            ret = UL_RETURN_FAIL;
          }
        }
*/
        //push the chunk
        if(ulPushChunkToRemoteApplication(c, htc[i], addr, port, path)) {
          error("ulSendChunk unable to push chunk to application %s:%d%s through connection at position %d", addr, port, path, i);
          ret = UL_RETURN_FAIL;
        }
        else {
          //debug("ulSendChunk pushed chunk to application %s:%d%s through connection at position %d", addr, port, path, i);
          ret = UL_RETURN_OK;
        }
      }
      else {
        debug("ulSendChunk no application seems to be registered at position %d", i);
      }
    } //application registry check ok
  } //cycle all positions
  return ret;
}

int ulPushChunkToRemoteApplication(Chunk *c, struct evhttp_connection *htc, const char *addr, const int port, const char *path) {
  int encoded_chunk_size = 0;
  uint8_t *buff = NULL;
  int size = 0;
  int ret = 0;

  //encode the chunk into a bitstream
  encoded_chunk_size = UL_ENCODED_CHUNK_HEADER_SIZE + c->size + c->attributes_size;
  if( (buff=(uint8_t *)malloc(encoded_chunk_size)) == NULL ) {
    error("memory allocation failed in encoding chunk to push via http");
    return UL_RETURN_FAIL;
  }
  size = encodeChunk(c, buff, encoded_chunk_size);

  if(size > 0) {
    ret = ulEventHttpClientPostData(buff, size, htc, addr, port, path);
    debug("Just HTTP pushed chunk %d of %d bytes encoded into %d bytes. Ret value of %d", c->id, c->size, size, ret);
    free(buff);
    return UL_RETURN_OK;
  }
  else {
    warn("size zero in a encode chunk!!!!");
    free(buff);
    return UL_RETURN_FAIL;
  }
}
