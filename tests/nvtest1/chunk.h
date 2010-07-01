#include <stdlib.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>

#include <event2/event.h>
#include <event2/buffer.h>
#include <event2/bufferevent.h>
#include <event2/http.h>

//#include <event2/event_struct.h>
//#include <event2/bufferevent_struct.h>
#include <event2/http_struct.h>

#include <event2/buffer.h>
#include <sys/time.h>
#include <time.h>


extern struct event_base *evb;

struct chunk {
    struct timeval rectime;
    unsigned int chunk_num;
    int len;
    unsigned char buf[0];
};

/**
 *	Get chunk_num of the the latest chunk
*/

unsigned int chbuf_get_current(void);


/**
 *	enqueue chunk. Input evbuffer ownership remains with the caller.
 *  @param eb   :   event buffer (data)
 *  @param chunk_num   :   chunk number
 *  @param timestamp   :   pointer to data timestamp (or NULL)
*/
void chbuf_enqueue_from_evbuf(struct evbuffer *eb, int chunk_num, struct timeval* timestamp);

/**
 *	enqueue chunk. Input chunk ownership remains with the caller.
 *  @param ch   :   input chunk
*/
void chbuf_enqueue_chunk(struct chunk *);

/* get a chunk identified by
 * returns chunk data length, if found,
 * otherwise returns  CB_NOT_YET_HERE if chunk has not been received yet, or CB_OVERWRITTEN, if a newer chunk has replaced it.
 * chunk_data, and chunk_time are normally filled up with data, but both may be NULL
*/
#define CB_NOT_YET_HERE -1
#define CB_OVERWRITTEN -2
int chbuf_get(int chunk_num, unsigned char **chunk_data, struct timeval *chunk_time);

void *chbuf_add_listener(int delay_usec, void (*callback)(struct chunk *, void *arg), void *arg);

void chbuf_remove_listener(void *listener_handle);

/* *********** WAITTIME UTILITY **************** */
/* get the remaining time (in timeval format) for a timestamp delayed by delay;
       return 0;0 if this is delay has already elapsed. */

void calc_waittime_for(struct timeval *base, int delay_usec, struct timeval *to_set);
