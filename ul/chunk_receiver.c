/**
 * @file chunk_receiver.c
 *
 * Chunk Receiver "object".
 *
 * No threads, thus it is based on callbacks and timer events.
 *
 * Napa-Wine project 2009-2010
 * @author: Giuseppe Tropea <giuseppe.tropea@lightcomm.it>
 */

#include "chunk_external_interface.h"

int ulChunkReceiverSetup(const char *address, unsigned short port) {
  if(ulEventHttpServerSetup(address, port, &ulPushChunkToChunkBuffer_cb)) {
    return UL_RETURN_FAIL;
  }
  else {
    debug("Chunk receiver setup OK");
    return UL_RETURN_OK;
  }
}

int ulPushChunkToChunkBuffer_cb(const uint8_t *encoded_chunk, int encoded_chunk_len) {
  Chunk decoded_chunk;
  debug("Processing incoming chunk of %d encoded bytes", encoded_chunk_len);
//print_block(encoded_chunk, encoded_chunk_len);
  int size = decodeChunk(&decoded_chunk, encoded_chunk, encoded_chunk_len);
  debug("Just decoded chunk %d", decoded_chunk.id);
  //push new chunk into chunk buffer
  chbAddChunk(chunkbuffer, &decoded_chunk);
  debug("Just pushed chunk %d of %d bytes into chunkbuf", decoded_chunk.id, size);
  return UL_RETURN_OK;
}

void print_block(const uint8_t *b, int size) {
int i=0;
printf("BEGIN OF %d BYTES---\n", size);
for(i=0; i<size; i++) {
printf("%d ", *(b+i));
}
printf("END OF %d BYTES---\n", size);
}
