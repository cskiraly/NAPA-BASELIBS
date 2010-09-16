#ifndef _REPOCLIENT_IMPL_H
#define _REPOCLIENT_IMPL_H

/** @file repoclient_impl.h
 *
 * Local header file defining nuts 'n bolts for the repoclient implementation.
 *
 */

#define _GNU_SOURCE

#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>
#include	<math.h>

#include	<event2/event.h>
#include	<event2/buffer.h>
#include        <event2/http.h>
#include        <event2/http_struct.h>
 
#include	<grapes.h>
#include	<grapes_log.h>
#include	<mon.h>		// for MeasurementRecord

#include	<repoclient.h>

#define REPOSERVER_MAGIC        0xAABB
#define PUBLISH_BUFFER_SIZE	1024

/** Struct maintaining streambuffer data. Used internally */
struct streambuffer {
#ifndef WINNT
	FILE *stream;
#endif
	char *buffer;
	size_t len;
};

extern struct streambuffer publish_streambuffer;

struct deferred_publish;

/** Internal structure to store a reposerver's connection data */
struct reposerver {
	/** IP Address part of the server URI */
	char *address;
	/** Port part of the server URI */
	unsigned short port;
	/** http connection from libevent */
	struct evhttp_connection *evhttp_conn;
	/** publish_delay */
	int publish_delay;
	/** publish buffer */
	struct deferred_publish *publish_buffer;
	/** publish buffer counter */
	int publish_buffer_entries;
	/* deferred publishs in transit */
	struct deferred_publish *in_transit;
	/** in-transit buffer counter */
	int in_transit_entries;
	/** magic value for paranoid people */
	int magic;
};

/** Holds callback address and id for HTTP response callbacks, as well as a data field */
typedef struct {
	/** callback address */
        void *cb;
	/** callback user argument */
	void *cbarg;
	/** id of the request */
        HANDLE id;
	/** the server this request belongs to */
	struct reposerver *server;
	/** arbitrary data field (e.g. used for maxresults) */
        int data;
} request_data;

/** Internal structure for holding batch publish records */
struct deferred_publish {
	MeasurementRecord r;
	request_data *requestdata;
};

/** Helper callback for operations returning a string-list (i.e. char **) to be called by libevent

  @param req the http request struct
  @param arg we store the pointer to the corresponding request_data
*/
void _stringlist_callback(struct evhttp_request *req,void *arg);


/** Check the validity (i.e. it is open) of a repoclient handle 
 
  @param h the handle
  @param fn name of the function check_handle is called from (to aid error reporting)
  @return 1 if the handle is valid, 0 if not
*/
int check_handle(HANDLE h, const char *fn);

/** Helper for HTTP GET queries 

  @param uri request string
  @param callback callback function
  @param cb_arg callback arg
*/
void make_request(const char *uri, void (*callback)(struct evhttp_request *, void *), void *cb_arg);

/** Helper for HTTP POST queries 

  @param uri request string
  @param data POST DATA (as 0-terminated string)
  @param callback callback function
  @param cb_arg callback arg
  @retun 0 on success, <0 on error
*/
int make_post_request(const char *uri, const char *data, void (*callback)(struct evhttp_request *, void *), void *cb_arg);

/** Parse a measurement record from the HTTP encoding 

  @param line the line as sent by the repo server
  @return the record parsed or NULL if invalid. The record is dynamically allocated and needs to be freed by the caller.
*/
MeasurementRecord parse_measurementrecord(const char *line);

/** print a Constraint array according to the HTTP encoding used in the reposerver communication 

  @param ranks array of Constraint s
  @param len length of the constraints array
  @return textual representation of the constraint struct in a static buffer
*/
const char *constraints2str(Constraint *constraints, int len);

/** print a Ranking array according to the HTTP encoding used in the reposerver communication 

  @param ranks array of Ranking s
  @param len length of the ranks array
  @return textual representation of the rankings struct in a static buffer
*/
const char *rankings2str(Ranking *ranks, int len);

/** print a MeasurementRecord 

  @param r the record
  @return pointer to an internal buffer containing string representation
*/
const char *encode_measurementrecord(const MeasurementRecord *r);

/** free the contents of a MeasurementRecord 

  @param r pointer to the record
*/
void free_measurementrecord(MeasurementRecord *r);

/** copy a measurementrecord for deferred publishing (by strdup-ing string elements)

  @param dst destination MR address
  @param src source MR
*/
void copy_measurementrecord(MeasurementRecord *dst, const MeasurementRecord *src);

#endif
