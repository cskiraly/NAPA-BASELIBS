/**
 * @file ul_init.c
 *
 * Initialization call for the UserLayer module.
 *
 * Napa-Wine project 2009-2010
 * @author: Giuseppe Tropea <giuseppe.tropea@lightcomm.it>
 */

#include "chunk_external_interface.h"

//register the player in the list of registered chunk receivers
//register the ulNewChunkArrived into the chunkbuffer's RegisterNotifier callback

//fill the global pointer to the chunk_buffer
cb = NULL;