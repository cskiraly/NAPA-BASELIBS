#ifndef _PEER_H
#define _PEER_H

/** @file peer.h
 *
 * Local header file defining nuts 'n bolts for the peer implementation.
 *
 */

#define _GNU_SOURCE
#define ATTRIBUTES_HEADER_SIZE	16

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<math.h>

#include	<event2/event.h>
#include	<event2/buffer.h>
#include	<event2/bufferevent.h>
#include	<event2/http.h>
#include	<event2/http_struct.h>
#include	<confuse.h>
 
#include	<grapes.h>
#include	<grapes_log.h>
#include	<ml.h>
#include	<mon.h>
#include	<chunk.h>
#include	<msg_types.h>

#include	"crc.h"

/** Declarations for per-section config definitions */
extern cfg_opt_t cfg_ml[];
extern cfg_opt_t cfg_mon[];
extern cfg_opt_t cfg_rep[];
extern cfg_opt_t cfg_som[];
extern cfg_opt_t cfg_source[];
extern cfg_opt_t cfg_playout[];

/** We store the local peer ID here */
extern char LocalPeerID[];

/** Repository client handle */
extern HANDLE repository;

/** Channel we broadcast/receive */
extern char *channel;

/** The ChunkBuffer we share */
extern struct chunk_buffer *chunkbuffer;

/** Version string */
extern char version[];

/** Default send parameters for ML */
extern send_params sendParams;

/** Initialize the ML related parts of the Peer 

  @param[in] libconfuse-parsed "network" section of the config
  @param[in] port value, if not 0, it overrides the config file
*/ 
void ml_init(cfg_t *ml_config, int port);

/** Initialize the MONL related parts of the Peer 

  @param[in] libconfuse-parsed "mon" section of the config
  @param[in] channel name
*/ 
void mon_init(cfg_t *mon_config, char *channel);

/** Let the monitoring code set its configuration validators 

  @param[in] config un-parsed configuration.
*/ 
void mon_validate_cfg(cfg_t *config);

/** Activate the MONL measurements for a new connection *

  @param[in] remsocketiID  the socket ID for the new connection
*/
void activateMeasurements(socketID_handle remsocketID);

/** Initialize the repoclient related parts of the Peer 

  @param[in] libconfuse-parsed "repository" section of the config
*/ 
void rep_init(cfg_t *rep_config);

/** Initialize the "source" functionality (i.e. generating Chunks) of the Peer 

  @param[in] libconfuse-parsed "source" section of the config
*/ 
void src_init(cfg_t *source_cfg);

/** Initialize the "playout" functionality (i.e. streaming out) of the Peer 

  @param[in] libconfuse-parsed "source" section of the config
*/ 
void playout_init(cfg_t *playout_cfg);

/** Play out the chunk received 

  @param[in] c the chunk
*/
void playout_chunk(const struct chunk *c);

/** Convenience function to publish a measurement record

  @param[in] mr the MeasurementRecord
  @param[in] msg message to display if an error occurs
*/

void publishMeasurementRecord(MeasurementRecord *mr, char *msg);

/** Convenience function to print a string array 

  @param[in] list the string array to print
  @param[in] n numer of items in the array
  @param[in] should_free if true, the list (and its elements) are freed after printing
  @return the array as a string in a static buffer
*/
const char *print_list(char **list, int n, bool should_free);

#endif
