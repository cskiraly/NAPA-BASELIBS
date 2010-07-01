#ifndef _UL_H
#define _UL_H

/** @file ul.h
 * 
 * @brief GRAPES User Layer Interface 
 * 
 * The User Layer API describes how a GRAPES peer interacts with the user.
 * There are two major operation modes:
 *	- player: displays the received video stream, and provides basic 
 *		controls (e.g. start/stop, channel switch etc.)
 *	- source: each video stream shared has a single source, where it
 *		enters the network (<i>ingested</i> by the P2P network).
 *
 * The main task for the source node is to tranlsate the video streams into
 * <i>chunks</i>, ready to be transmitted over to other peers.
 * Conversely, `player' nodes need to reconstruct the stream from the chunks
 * received.
 * These operations are referred to as <i>chunkization</i>/<i>de-chunkization</i>.
 * 
 * In order to provide maximum flexibility, these processes are loosely coupled 
 * to the core GRAPES via network pipes. For simplicity, in the reference 
 * implementation we propose the HTTP protocol for (de)chunkizer-GRAPES core
 * interaction.
 *
 * This interface is conceptually clean, easy to integrate with other programs, 
 * and straightforward to implement.
 *
 * As a princile to start with, the GRAPES application handles chunks on its external interfaces. 
 * At the ingestion side, it receives chunks sent by a <i>sequencer</i> in approximately correct timing. 
 * At the receiver/player nodes, GRAPES+ should issue chunks in correct sequence, 
 * and (if possible) again with a roughly correct timing. 
 * As a simple approach, each receiver/player client may request a delay 
 * when it connects to GRAPES (e.g. 5 secs), and it receives the replay of 
 * ingestion events offset by this delay.
 * 
 * This approach also decouples the `network' and `multimedia' aspects of the software: 
 * the player and ingestor are not linked with the GRAPES application and has maximum flexibility 
 * (e.g. with codec legal and technology aspects).

 * `Roughly correct timing'  means that the sending time for chunks are synchronized 
 * (to the media time they contain) with an accuracy of about or less than one chunk period. 
 * This way, within GRAPES, new chunks may directly enter the ChunkBuffer (replacing the oldest one), 
 * and the dechunkizers will also have to buffer only a small number of (1-3) chunks. 
 * 
 * For low-level interface, we propose using HTTP POST operations: HTTP is a well-known protocol 
 * supported by many  languages and platforms. Also, due to the transactional nature of HTTP, 
 * the boundaries between chunks are clear (as opposed to pipes or TCP sockets). 
 * HTTP interfaces are also easy to test.
 * 
 * The application may support  multiple simultaneous clients (players with different delays, analyzers, etc.) 
 * on the receiver side. The service offered for this registration by GRAPES is the same HTTP 
 * service used by the chunkizer to send new chunks.
 *
 * <b> Source (ingestor) Protocol Description</b>
 *
 * The source module implements a HTTP listenerwhere it receives regular HTTP POST requests
 * from the ingestor. The format is a follows:<br>
 * <tt>http://<addr>:<port>/<path-id>/<chunk-number>?timestamp=<date></tt><br>
 * accompanied by the binary chunk in the POST message
 * The chunk data is opaque for GRAPES and is delivered unchanged.
 *
 * <b> Player (client) Protocol Description: Control Interface</b>
 * 
 * The GRAPES core module implements a HTTP listener to receive registrations
 * from clients (e.g. Display/GUI or analyzer modules). 
 * Registrations are encoded as HTTP GET requests as follows:
 * <tt>http://<hostname>:<port>/register-receptor?url=<url>&channel=<channel>&delay=<delay>&validity=<validity></tt><br>
 * Parameters are:
 *	- <hostname> and <port> are startup parameters for GRAPES
 *	- <url> the URL where posts should be sent (URL encoded). MANDATORY.
 *	- <channel> channel identifier
 * 	- <delay> the delay, expressed in seconds, fractional values are supported. Delay defaults to 5 seconds.
 * 	- <validity> TImeout for this registration: if there is no additional registration 
 *	within <validity> seconds, GRAPES will stop posting chunks.  Default is 300 seconds.
 * 
 * Then, each registered client module is supposed to implement a HTTP listener on the specified URL  
 * where GRAPES will send chunks to. Sending is implemented as a HTTP POST using the source (ingestor) protocol
 * above.
 * The client-registration HTTP service above can be easily extended with optional commands implementing GUI
 * controls.
 *
 * @author Arpad Bakay			<arpad.bakay@netvisor.hu>
 * @author Tivadar Szemethy		<tivadar.szemethy@netvisor.hu>
 * @author Giuseppe Tropea		<giuseppe.tropea@lightcomm.it>
 *
 * @date 01/13/2010
 * 
 */

#include	"grapes.h"

/**
  Initialize source functionality within a peer's User Layer.

  @param[in] ingest_url URL where the node listens for the chunks from the ingestor
  @return a handle for the initized source UL or NULL on error
*/
HANDLE initULSource(const char *ingestor_url);

/**
  Shut down a source peer's User Layer.

  @param ul_source source instance to shut down
  @see initULSource
*/
void shutdownULSource(HANDLE ul_source);


/**
  Initialize a `player' (dechunkizer) functionality  within a peer's User Layer..

  @param[in] reg_url register-receptor URL where GRAPES listens
  @return a handle for the initized peer UL or NULL on error
*/
HANDLE initULPeer(const char *reg_url);

/**
  Shut down a regular peer's User Layer.

  @param ul_peer peer instance to shut down
  @see initULPeer
*/
void shutdownULPeer(HANDLE ul_peer);

#endif

