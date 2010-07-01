/**
 * @file chunk_external_interface.h
 * 
 * Interface file for API to push chunks to external video players or pull from video sources.
 *
 * This is the interface for external chunk generators (i.e. video sources + a chunker)
 * or chunk consumers (i.e. an unchunker + video player) to push chunks into
 * a peer's internal chunk buffer, or to retrieve chunks from a peer's internal chunk buffer,
 * respectively.
 * The interface objects use a simple http channel to move chunks out/in of the
 * main peer's application, and use a pointer to the chunkbuffer and the necessary
 * callbacks.
 * This interface is built upon a libevent-based simple http client and server model
 * with no threads, thus it is based on callbacks and timer events, as required
 * by the current implementation of the peer application.
 *
 * Napa-Wine project 2009-2010
 * @author: Giuseppe Tropea <giuseppe.tropea@lightcomm.it>
 */

#ifndef _CHUNK_EXTERNAL_INTERFACE_H
#define _CHUNK_EXTERNAL_INTERFACE_H

#include <stdio.h>
//this depends on libevent for many reasons
#include <event2/http.h>
#include <event2/http_struct.h>
//thus i need a global variable with the external heartbeat and timings
extern struct event_base *eventbase;

//#include "../include/chunk.h"
//#include "../include/trade_msg_la.h"
//#include "../include/grapes_log.h"
#include <chunk.h>
#include <trade_msg_la.h>
#include <grapes_log.h>

#include "ul_commons.h"

//moreover we are interfacing to the chunkbuffer
//we assume for now it is a global variable too
extern struct chunk_buffer *chunkbuffer;

/**
 * define a new data type for the pointer to a generic data_processor function
 */
typedef int (*data_processor_function)(const uint8_t *data, int data_len);


//DOCUMENTATION RELATIVE TO THE CHUNK_RECEIVER.C FILE
/**
 * Setup of a chunk receiver inside the peer.
 *
 * The chunk receiver receives chunks from an external application
 * via a regular http socket. The chunk receiver listens for chunks at
 * a specific address:port on the machine where the peer application
 * is running.
 * The chunk receiver uses for its internals a simple http server. When the
 * server receives a request, a callback is triggered to deal with
 * the incoming block of data (i.e. the incoming chunk).
 *
 * @param[in] address The IP address the receiver shall run at
 * @param[in] port The port the receiver shall listen at
 * @return 0 if OK, -1 if problems
 */
int ulChunkReceiverSetup(const char *address, unsigned short port);

/**
 * Callback for processing the received chunk.
 *
 * This function is called by the http server ProcessRequest function, which is
 * in turn triggered whenever a request with a data block is received.
 * This function receives from the internal http server the stripped block of bulk
 * data and interprets is as a chunk, pushing it into the peer's internal chunk buffer.
 *
 * @param[in] chunk The data to be processed (a chunk to be inserted into internal chunkbuffer)
 * @param[in] chunk_len The chunk length
 * @return 0 if OK, -1 if problems
 */
int ulPushChunkToChunkBuffer_cb(const uint8_t *encoded_chunk, int encoded_chunk_len);


//DOCUMENTATION RELATIVE TO THE EVENT_HTTP_SERVER.C FILE
/**
 * Sets up the internal http server.
 *
 * This function sets-up a libevent-based http server listening at a specific address:port.
 * When an http request is received by this server, the ProcessRequest callback is triggered,
 * which extracts the data embedded in the POST request, strips it and int turn gives it
 * to the specific data_processor function passed here in the setup operation.
 * Thus this http server is able to call a specific function to take care of
 * the received data, depending on who is setting it up.
 *
 * @param[in] address The IP address the server shall run at
 * @param[in] port The port the http server shall listen at
 * @param[in] data_processor The external function to call and pass the received data block to
 * @return -1 in case of error, when server does not get initialized, 0 if OK
 */
int ulEventHttpServerSetup(const char *address, unsigned short port, data_processor_function data_processor);

/**
 * Processes the received http request.
 *
 * Extract the bulk data buffer from the http POST request
 * and pass it to an external function for processing it specifically, by giving
 * a pointer to the data block and its length
 *
 * @param[in] req Contains info about the request, from libevent
 * @param[in] context A context for this request (an external data_processor function in this case)
 * @return -1 in case no data was extracted from the request (bad path, GET insted of POST, etc...), 0 if OK
 */
int ulEventHttpServerProcessRequest(struct evhttp_request *req, void *context);


//DOCUMENTATION RELATIVE TO THE CHUNK_SENDER.C FILE
/**
 * Send a chunk to all registered receivers.
 *
 * The chunk send is called whenever a fresh chunk is inserted into the
 * internal chunk buffer of the peer from the P2P network, in order to send
 * a copy of the fresh chunk out to external applications, as soon as
 * it gets into the peer.
 * Generally the fresh chunk is pushed to an external unchunking and video player
 * application, or a statistical or analysis application.
 * The chunk sender uses a simple regular http client writing to a socket
 * for its internal working.
 * This chunk sender is triggered via a callback activated by the internal chunk buffer
 * receiving a fresh chunk.
 * This chunk sender holds a static array of active connections to remote
 * applications, and initializes the connections at first time.
 *
 * @param[in] remote_address The IP address of the external application
 * @param[in] remote_port The port of the external application
 * @param[out] htc The pointer where the caller wants the connection to be stored
 * @return 0 if OK, -1 if problems
 */
int ulSendChunk(Chunk *c);

/**
 * Send out the chunk to an external application.
 *
 * This function is passed a chunk and sends it out to an external application via
 * the libevent's evhttp_connection object htc.
 * This function as of now is just a readability wrapper for the HttpClientPostData
 * function of the simple http client code.
 *
 * @param[in] c The chunk
 * @param[in] c_len The chunk length
 * @param[in] htc The pointer to the established connection with the external application
 * @param path The path within the remote application to push data to
 * @return 0 if OK, -1 if problems
 */
int ulPushChunkToRemoteApplication(Chunk *c, struct evhttp_connection *htc, const char *addr, const int port, const char *path);


//DOCUMENTATION RELATIVE TO THE EVENT_HTTP_CLIENT.C FILE
/**
 * Sets up a libevent-based http connection to a remote address:port
 *
 * This is the setup function to create an http socket from this peer to an external
 * application waiting for chunks on a specified address:port.
 * Pass a pointer to libevent's evhttp_connection object to this function
 * in order to get back an handler to the socket connection.
 *
 * @param[in] address The IP address of the remote http server to connect to
 * @param[in] port The port the remote http server is listening at
 * @param[out] htc The pointer to where the caller wants the connection pointer to be stored
 * @return -1 in case of error, when pointer to connection does not get initialized, 0 if OK
 */
int ulEventHttpClientSetup(const char *address, unsigned short port, struct evhttp_connection **htc);

/**
 * Post a block of data via http to a remote application.
 *
 * Performs a POST http request operation towards a remote http server in order to transfer a bulk
 * block of bytes to the server entity through the already establised connection.
 *
 * @param data A pointer to the block of bytes to be transferred
 * @param data_len The length of the data block
 * @param htc A pointer to the already established libevent-based http connection to the server
 * @param path The path within the remote application to push data to
 * @return -1 in case of error, or when pointer to connection does not get initialized, 0 if OK
 */
int ulEventHttpClientPostData(uint8_t *data, unsigned int data_len, struct evhttp_connection *htc, const char *addr, const int port, const char *path);


//DOCUMENTATION RELATIVE TO THE RECEIVERS_REGISTRY.C FILE
/**
 * Register a new receiver application.
 *
 * The remote application registers itself by declaring something
 * like "http://address:port/path/to/receiver/"
 * The function can be used to retrieve information about an already registered
 * application at a specific position, by giving NULL inputs except
 * for the pos parameter.
 *
 * @param[in,out] address The application IP address
 * @param[in,out] port The application port
 * @param[in,out] path The path where to send chunks
 * @param[in,out] pos The position in the registration array
 * @return 0 if OK, -1 if problems
 */
int ulRegisterApplication(char *address, int *port, char* path, int *pos);


#endif	/* CHUNK_EXTERNAL_INTERFACE_H */
