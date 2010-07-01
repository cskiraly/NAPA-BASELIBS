/*
 * Minimalistic UserLayer for a source node
 * Will read the stream from an UDP socket and appends them to the ChunkBuffer
 */

#include	"peer.h"

#include <netinet/in.h>
#include <sys/socket.h>

#define READ_SIZE	65536

struct source_stream {
	struct bufferevent *bev;
	double chunk_duration;
	void *read_buffer;
	int read_size;
	int chunkid;
};

void read_stream(HANDLE h, void *arg) {
	struct source_stream *src = (struct source_stream *)arg;
	int ptr = 0;

	size_t rd = READ_SIZE;
	while (rd == READ_SIZE) {
		rd = bufferevent_read(src->bev, src->read_buffer + ptr, src->read_size - ptr);
		if (rd == (src->read_size - ptr)) { /* Need to grow buffer */
			src->read_size += READ_SIZE;
			src->read_buffer = realloc(src->read_buffer, src->read_size);
			ptr += rd;
		}
	}
	info("New chunk(%d): %d bytes read", src->chunkid++, ptr+rd);
}

int init_source_ul(const char *stream, double chunk_duration) {
	char addr[128];
	int port;

	if (sscanf(stream, "udp://%127[^:]:%i", addr, &port) != 2) {
		fatal("Unable to parse source specification %s", stream);
	}

	struct sockaddr_in udpsrc;
	int returnStatus = 0;
	evutil_socket_t udpSocket;

	udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	evutil_make_socket_nonblocking(udpSocket);

	udpsrc.sin_family = AF_INET;
	udpsrc.sin_addr.s_addr = inet_addr(addr);
	udpsrc.sin_port = htons(port);

	int status = bind(udpSocket,(struct sockaddr *)&udpsrc,sizeof(udpsrc));
	if (status) {
		fatal("Unable to bind socket to udp://%s:%d", addr, port);
	}

	struct source_stream *src = malloc(sizeof(struct source_stream));
	src->read_size = READ_SIZE;
	src->read_buffer = malloc(src->read_size);

	src->bev = (struct bufferevent *)bufferevent_socket_new((struct event_base *)eventbase, udpSocket, 0);
	bufferevent_enable(src->bev, EV_READ);
	src->chunk_duration = chunk_duration;
	grapesSchedulePeriodic(NULL, 1.0/chunk_duration, read_stream, src);
}


