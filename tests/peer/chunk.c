#include "chunk.h"

/* *********** CHUNKBUFFER **************** */

#define CB_LEN 100

static struct listener {
    int delay;
    void (*callback)(struct chunk *, void *);
    struct listener *next;
	void *arg;
} *chunkbuffer_listeners = NULL;

struct chunk_env {
    struct event *ev;
    struct listener *next_listener;
    struct chunk chunk;
} *chunkbuffer[CB_LEN];

static unsigned int chunkbuffer_head_num = 0;

unsigned int chbuf_get_current(void) {
  return chunkbuffer_head_num;
}


/* *********** WAITTIME UTILITY **************** */
void calc_waittime_for(struct timeval *base, int delay_usec, struct timeval *to_set) {
     if(to_set == NULL) return;
     *to_set = *base;
     struct timeval now;

     gettimeofday(&now, NULL);

     to_set->tv_usec += delay_usec;
     to_set->tv_sec  += to_set->tv_usec / 1000000;
     to_set->tv_usec %= 1000000;

     to_set->tv_usec += 1000000 - now.tv_usec;
     to_set->tv_sec += 50000 - now.tv_sec;
     if(to_set->tv_usec < 1000000) to_set->tv_sec--;
     else to_set->tv_usec -= 1000000;
     if(to_set->tv_sec < 50000) to_set->tv_sec = to_set->tv_usec = 0;  // schedule for immediate execution
     else to_set->tv_sec -= 50000;
}


static void chbuf_notif(evutil_socket_t type, short evmode, void *arg) {
    int chunk_num = (int)arg;
    struct chunk_env *currchunk = chunkbuffer[chunk_num%CB_LEN];
    if(currchunk == NULL || currchunk->chunk.chunk_num != chunk_num) return;

    currchunk->next_listener->callback(&currchunk->chunk, currchunk->next_listener->arg);
    if((currchunk->next_listener = currchunk->next_listener->next) != NULL) {
          struct timeval diff;
          calc_waittime_for(&currchunk->chunk.rectime, currchunk->next_listener->delay, &diff);
          evtimer_add(currchunk->ev, &diff);
     }

}

static void chbuf_enqueue(struct chunk_env *newchunk) {
	 int chunk_num = newchunk->chunk.chunk_num;
     {
       int pos = chunk_num % CB_LEN;
       if(chunkbuffer[pos] != NULL) {
              free(chunkbuffer[pos]);
       }
       chunkbuffer[pos] = newchunk;
     }
	 chunkbuffer_head_num = chunk_num;

     if((newchunk->next_listener = chunkbuffer_listeners) != NULL) {
          struct timeval diff;
          calc_waittime_for(&newchunk->chunk.rectime, newchunk->next_listener->delay, &diff);
          newchunk->ev = event_new(eventbase, -1, 0, chbuf_notif, (void *)chunk_num);
          evtimer_add(newchunk->ev, &diff);
     }
}

void chbuf_enqueue_from_evbuf(struct evbuffer *eb, int chunk_num, struct timeval* timestamp) {
	 if(chbuf_get(chunk_num, NULL, NULL) == CB_OLDER) {
		 struct chunk_env *newchunk = (struct chunk_env *)malloc(sizeof(struct chunk_env) + evbuffer_get_length(eb));
		 newchunk->ev = NULL;
		 if(timestamp != NULL && timestamp->tv_sec != 0 && timestamp->tv_sec != 0) {
			newchunk->chunk.rectime = *timestamp;
		 }
		 else gettimeofday(&newchunk->chunk.rectime, NULL);
		 newchunk->chunk.chunk_num = chunk_num;
		 newchunk->chunk.len = evbuffer_get_length(eb);
		 evbuffer_remove(eb, newchunk->chunk.buf, newchunk->chunk.len);
		 chbuf_enqueue(newchunk);
	 }
}


void chbuf_enqueue_chunk(int chunk_num, int length, struct timeval *timestamp, unsigned char *buffer) {
	if(chbuf_get(chunk_num, NULL, NULL) == CB_OLDER) {
		struct chunk_env *newchunk = (struct chunk_env *)malloc(sizeof(struct chunk_env) + length);
		newchunk->chunk.chunk_num = chunk_num;
		newchunk->chunk.len = length;
		newchunk->chunk.rectime = *timestamp;
	    memcpy(newchunk->chunk.buf,buffer,length);
        chbuf_enqueue(newchunk);
	}
}


int chbuf_get(int chunk_num, unsigned char **chunk_data, struct timeval *chunk_time) {
    struct chunk_env *currchunk = chunkbuffer[chunk_num%CB_LEN];
    if(currchunk == NULL) return -1;
	else if(currchunk->chunk.chunk_num != chunk_num) return currchunk->chunk.chunk_num > chunk_num ? CB_NEWER : CB_OLDER;
    if(chunk_data != NULL) *chunk_data = currchunk->chunk.buf;
    if(chunk_time != NULL) *chunk_time = currchunk->chunk.rectime;
    return currchunk->chunk.len;
}

/* ********************** LISTENER ************************ */


void *chbuf_add_listener(int delay_usec, void (*callback)(struct chunk *, void *arg), void *arg) {
   struct listener **l = &chunkbuffer_listeners;

   struct listener *nl = (struct listener*)malloc(sizeof(struct listener));
   if(nl == NULL) return NULL;
   nl->delay = delay_usec;
   nl->callback = callback;
   nl->arg = arg;
   while(*l != NULL) {
      if((*l)->delay > nl->delay) break;
      l = &((*l)->next);
   }
   nl->next = *l;
   *l = nl;
   return nl;
}

void chbuf_remove_listener(void *listener_handle) {
     struct listener *cl = (struct listener *)listener_handle;
     struct listener **l = &chunkbuffer_listeners;
     while(*l != NULL) {
       if(*l == cl) {
             *l = cl->next;
             free(cl);
       }
       l = &((*l)->next);
     }
}

