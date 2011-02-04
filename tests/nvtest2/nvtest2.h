//#ifndef _PEER_H
//#define _PEER_H

/** @file peer.h
 *
 * Local header file defining nuts 'n bolts for the peer implementation.
 *
 */

//#define _GNU_SOURCE

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/http.h>
#include <event2/http_struct.h>
#include <confuse.h>
 
#include <napa.h>
#include <napa_log.h>
#include <ml.h>
#include <mon.h>		
#include <repoclient.h>
#include <neighborlist.h>
#include <chunkbuffer.h>
#include <chunk.h>

/** Struct maintainging data for this Peer instance */
typedef struct _peer {
	/** Local PeerID (string), obtained from ML (address) */
	char LocalID[64];
	/** Local PeerID (socketID), obtained from ML (address) */
	socketID_handle LocalSocketID;
	/** Local PeerID integer value, obtained from config file */
	int PeerIDvalue;
	/** Repository handle */
	HANDLE repository;	
	/** Neighborlist handle */
	HANDLE neighborlist;
	/** Neighborhood size */
	int neighborlist_size;
	/** Neighborlist array */
	NeighborListEntry *neighbors;
	/** No. of neighbors in the list */
	int num_neighbors;
	/** Desired number of neighbors in the list */
	int nblist_desired_size;
	/** Update frequency of neighbor list */
	int nblist_update_period;
	/** ChannelID (string) to separate ourselves from others in the Repository */
	char *channel;
	/* ChunkBuffer */
	struct chunk_buffer *cb;
} Peer;

/** Global peer structure */
extern Peer *peer;

/** Read config file and initialize the local peer instance accordingly 

  @param[in] configfile		name of configuration file
  @return			pointer to an initialized Peer structure
**/
Peer *peer_init(const char *configfile);

/** Tell whether client mode is active 
 *
 * @return 	boolean value indicating client mode
 */
//bool client_mode();

/** Tell whether source mode is active 
 *
 * @return 	boolean value indicating source mode
 */
//bool source_mode();

/** Notifier callback for new Peers in the NeighborList

  @param h Handle to the NeighborList instance generating the notification.
  @param cbarg arbitrary user-provided parameter for the callback
*/
//void newPeers_cb(HANDLE rep, void *cbarg);

/**
  * The peer receives data per callback from the messaging layer.
  * @param *buffer A pointer to the buffer
  * @param buflen The length of the buffer
  * @param msgtype The message type
  * @param *arg An argument that receives metadata about the received data
  */
//void recv_data_from_peer_cb(char *buffer,int buflen,unsigned char msgtype,void *arg);

/**
  * The peer receives a chunk per callback from the messaging layer.
  * @param *buffer A pointer to the buffer
  * @param buflen The length of the buffer
  * @param msgtype The message type
  * @param *arg An argument that receives metadata about the received data
  */
//void recv_chunk_from_peer_cb(char *buffer,int buflen,unsigned char msgtype,void *arg);

/**
  * A callback function that tells a connection has been established.
  * @param connectionID The connection ID
  * @param *arg An argument for data about the connection
  */
//void receive_conn_cb (int connectionID, void *arg);

/**
  * A funtion that prints a connection establishment has failed
  * @param connectionID The connection ID
  * @param *arg An argument for data about the connection
  */
//void conn_fail_cb(int connectionID,void *arg);

//struct chunk;
//void chunk_transmit_callback(struct chunk *chunk, void *arg);

/**
  * A callback function that tells a connection has been established.
  * @param connectionID The connection ID
  * @param *arg An argument for data about the connection
  */
//void receive_outconn_cb (int connectionID, void *arg);

/**
  * Initialize the mini-UserLayer
  * @param source name of the source stream (currently: udp://ipaddr:port is supported)
  * @param chunk_duration length of a chunk in secords
  * @return 0 on success
*/
//int init_source_ul(const char *source, double chunk_duration);

//void new_chunk(struct chunk_buffer *cb, void *cbarg, const struct chunk *c) ;


//#endif
