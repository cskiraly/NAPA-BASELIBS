#ifndef _TRADING_H
#define _TRADING_H

/** @file trading.h
 *
 * Trading Logic API
 *
 * Trading Logic is responsible for conducting chunk and control information exchange with <b>local neighbors</b> 
 * (i.e Peers from the NeighborList (neighborlist.h), and maintain a well-supplied ChunkBuffer (chb.h).
 * Trading Logic functions may update entries of the NeighborList (neighborlist.h), for example adjust sending/receiving weights.
 *
 * The module is further subdivided into functions of:
 *	- <b>Data Exchange Protocol</b> handles timing and stateful behavior related to chunk exchange. It can be configured to adapt to the needs of different diffusion algorithms, e.g.
 *		- For pull based scheme, fixed chunk sending rate can be used.
		- For push based scheme, a fixed number of upload slots can be allocated.
		- It is possible to define further types of rate control like token bucket or limiting the number of unacknowledged chunks
		- When using pull or hybrid scheme requests and acknowledges are also tracked by the Data Exchange Protocol.
 *	- <b>Peer and Chunk Selection Policy</b> should be implemented in a callback-based way:
		- For <i>requesting</i> the chunk selection callback is invoked periodically and it returns 
	a list of peerID/chunkID pairs. 
		- For <i>sending</i> the chunk selection callback is invoked when the Data Exchange Protocol has a free slot.
 * 	- <b>Buffer Map Exchange</b> periodically traverses the active neighbor lists and sends an update message 
	to some of the active neighbors. Peer selection for buffer updates can be done similarly to 
	peer selection outlined above, based on e.g. measurement values or incentives
 * 
 * These sub-modules are implemented as callback-based <b>plugins</b> as described below.
 *
 * @section dep Data Exchange Protocol
 *
 * The Data Exchange Protocol typically registers callbacks on initialization. These callbacks are either
 * periodically timed (e.g. to control rate-constant sending) or invoked by the Messaging Layer on message arrivals.
 * 
 * @section pcsel Peer and Chunk Selection Policy
 * 
 * Peer and Chunk Selection is performed via these two primitives. 
 * These are called by the exchange protocol when an upload/download slot is available.
 *   - <i>selectPush</i> – returns a <peer_id, chunk_id> pair, specifying which chunk to send to which peer.
 *   - <i>selectPull</i> – gives a list of <peer_id, chunk_id> pairs, specifying which chunk to request from whom.
 * In both cases peers and chunks are selected by calling the evaluation functions:
 *   - int (*evaluatePeer)(peerId peer_id, int chunk_id)
 *   - int (*evaluateChunk)(peerId peer_id, int chunk_id)
 *
 * Some examples of evaluation functions include uniform probability random choice, most deprived peer, 
 * latest useful chunk etc.
 *
 * @section bme Buffer Map Exchange
 *
 * The Buffer Map Exchange protocol is implemented similarly to the Data Exchange Protocol. The implementation
 * registers periodic and control-message reception callbacks and manages signalling accordingly.
 * Possible further implementation strategies may include:
 * For determining destination peers for status updates, rate control based on control message slots and 
 * priorities or probabilities can be used:
 *   - Buffer map message rate can be specified as a fixed number of messages/second.
 *   - Active inbound peers are evaluated with a utility function.
 *   - In case of deterministic choice, top N peers are notified.
 *   - In case of probabilistic choice, peers are chosen with a probability given by the utility function.
 * For example peers can be selected in a uniform random way, so every second a fixed number of random neighbors 
 * are notified. Peers can also be weighted by their distance, or can be prioritized by history scores (TFT) or choking.
 *
 * @section suggestions Suggested simple scheduler algorithms for the pre-prototype
 * 
 * The initial version of the prototype might include an epidemic scheduler:
 *  - As a push-based protocol is used, the initial version won’t include any decision code for chunk requesting.
 *  - Buffer map updates are sent to all known peers. 
 *  - Peer and Chunk selection implemented in the first version could be mdp/ruc 
 *	but other similar algorithms can be implemented with a suitable utility function, 
 *	e.g. dp/rc, luc/up, luc/BA, ELp/DLc
 *  - Data Exchange Timing uses one upload slot. In the future this could be modified to a fixed number of upload slots. * 
 * The proposed scheduler is easily reconfigurable for future, more sophisticated experiments.
 * Implementing a pull-based approach similar to pulse is possible by defining suitable evaluation functions for 
 * rarest first requesting, least sent chunk sending and configuring the exchange protocol to send a fixed number 
 * of chunks per second.
 * 
 * Furthermore, a grant/request scheme can also be implemented by sending buffer map updates only to peers 
 * unchoking the local peer. Data exchange is done similarly to the epidemic diffusion, e.g. mdp/luc.
 *
 * @todo Define functions & details.
 * 
 */

#include	"pulse.h"
#include	"som.h"

/** This protocol plugin handles timing and stateful information for data exchange. 

  The below <i>_strategy</i> primitives provide simple ways to implement basic schedulers (e.g. rate-constant), 
  but protocol implementations may also work on their own by subscribing to callbacks from the Messaging Layer or 
  NeighborList (neighborlist.h). In this case interaction with the Message Layer (sending and receiving) 
  must be done internally from the protocol implementation after consulting PeerChunkSelectionPlugin. */
typedef struct {
	/** Name of the plugin - for diagnostic purposes */
	const char *name;
	/** Initialization function */
	int (*startup)(HANDLE som);
	/** Shutdown function */
	int (*shutdown)();
	/** Send Strategy Implementation. 

	Called once after startup and re-called if reschedule is set. 
	After calling, the returned number of <peer,chunk> pairs is obtained via PeerChunkSelection and sent. */
	int (*send_strategy)(HANDLE som, TimeStamp *reschedule);
	/** Request Strategy Implementation. 

	Called once after startup and re-called if reschedule is set. 
	After calling, the returned number of <peer,chunk> pairs is obtained via PeerChunkSelection and request(s) 
	are sent for them.  */
	int (*req_strategy)(HANDLE som, TimeStamp *reschedule);
	/** Called on an explicit chunk request. If returns true, the chunk is placed to the sending queue. */
	bool (*req_received)(HANDLE som, PeerID peer, int chunk_id);
	/** Peer has ACKed the chunk. Call send_strategy immediately if returns true. */
	bool (*ack_received)(HANDLE som, PeerID peer, int chunk_id);
	/** Get textual status description for the plugin - for diagnostic purposes. */
        const char *    (*getStatusDescr)();
} DataExchangeProtocolPlugin;

/** Plugin governing Peer and Chunk selection for the Data Exchange Protocol. 

  Two Selection Plugins are registered (one for Sending, one for Requesting). 
  The <i>evaluate</i> functions return a weight/probability, and based on this, the
  Trading module composes the send/request <PeerID,ChunkID> lists using the composeList function */
typedef struct {
	/** Name of the plugin - for diagnostic purposes */
	const char *name;
	/** Initialization function */
	int (*startup)(HANDLE som);
	/** Shutdown function */
	int (*shutdown)();
	/** return a weight for sending/requesting for peer */
	double (*evaluatePeer)(PeerID peer);
	/** return a weight for sending/requesting this chunk */
	double (*evaluateChunk)(ChunkID chunk);
	/** List composition function to assemble a <PeerID, ChunkID> list based on the evaluation results */
	void (*composeList)(int *result_len, PeerID *result_peer, ChunkID *result_chunk, int n_peers, PeerID *peers,double *peer_weights, int n_chunks, ChunkID *chunks, double *chunk_weights);
} PeerChunkSelectionPlugin;

/** Implementation details for the buffer map exchange protocol. 

  Apart from startup and shutdown, the plugin manages everything else (i.e. message sending a receiving) internally */
typedef struct {
	/** Name of the plugin - for diagnostic purposes */
	const char *name;
	/** Initialization function */
	int (*startup)(HANDLE som);
	/** Shutdown function */
	int (*shutdown)();
	/** Get textual status description for the plugin - for diagnostic purposes. */
        const char *    (*getStatusDescr)();
} BufferMapExchangePlugin;

/**
  Select a list of <PeerID, ChunkID> pairs for PULL or PUSHing.

  Called internally by the Data Exchange Protocol.
  Uses the evaluate functions of the PeerChunkSelectionPlugin and the NeighborList.

  @param[in] som Handle to the enclosing SOM instance.
  @param[in] dir selects PUSH or PULL operation (SEND/RECEIVE)
  @param[out] peers Caller-provided array of PeerIDs pointers. The caller is responsible for freeing the PeerIDs placed into the array (and the array itself).
  @param[out] chunk_ids Caller-provided array of ChunkIDs. 
  @param[in] n maximum number of pairs to return.
  @return number of pairs returned. 0 if no selection can be made.
*/
int trading_select(HANDLE som, Direction dir, PeerID **peers, int *chunk_ids, int n);

#endif
