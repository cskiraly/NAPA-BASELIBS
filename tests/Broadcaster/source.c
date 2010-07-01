/*
 * % vim: syntax=c ts=4 tw=76 cindent shiftwidth=4
 *
 * User layer services the Peer implementation for the Broadcaster demo application
 *
 */

#include <netinet/in.h>
#include <sys/socket.h>
#include <time.h>

#include "peer.h"

#include <chunk.h>
#include <mon.h>
#include <ul.h>

#define MAX_PACKET_SIZE			2048
#define INITIAL_PKTBUFFER_NUM	10

/** Configuration options for the Source */
cfg_opt_t cfg_source[] = {
	CFG_STR("stream", NULL, CFGF_NONE),
	CFG_FLOAT("chunk_duration", 1.0, CFGF_NONE),
	CFG_END()
};

/** This function gets called periodically and arranges the publishing of a
 * MeasurementRecord with SrcLag = 0.0 
 */
void periodicPublishSrcLag(HANDLE h, void *arg) {
	MeasurementRecord mr;
	memset(&mr, 0, sizeof(mr));
	mr.published_name = "SrcLag";
	mr.value = 0.0;
	mr.originator = mr.targetA = mr.targetB = LocalPeerID;
	mr.channel = channel;
	publishMeasurementRecord(&mr, "SrcLag");
}

void src_init(cfg_t *source_cfg) {
	char *stream = cfg_getstr(source_cfg, "stream");
	if (!stream) return;
	crcInit();

	if (!strncmp("udp://", stream, 6)) {
		double chunk_duration = cfg_getfloat(source_cfg,"chunk_duration");
		info("Initializing SOURCE mode: tuning to stream %s, sampling rate is "
			"%.2lf Hz", stream, (double)1.0/chunk_duration);
		init_source_ul(stream, chunk_duration);
	} else if (!strncmp("http://", stream, 7)) {
		char addr[128];
		int port,i;

		if (sscanf(stream, "http://%127[^:]:%i", addr, &port) != 2) 
			fatal("Unable to parse source specification %s", stream);

		if (ulChunkReceiverSetup(addr, port)) 
			fatal("Unable to start the HTTP receiver to http://%s:%d", 
					addr, port);
	} else fatal("Unable to parse stream specification %s", stream);
	grapesSchedulePeriodic(NULL, 1/60.0, periodicPublishSrcLag, NULL);
}

struct source_stream {
	int udpSocket;
	double chunk_duration;
	int chunkid;
	void **buffers;
	int *bufferlengths;
	int num_buffers;
	int atts_size;
	char *atts;
};

/** This is our very primitive "Chunkizer" */
void read_stream(HANDLE h, void *arg) {
	struct source_stream *src = (struct source_stream *)arg;

	int num = 1;
	struct timeval tv_start, tv_end;
	double start, end, diff;

	int rd = recv(src->udpSocket, src->buffers[0], MAX_PACKET_SIZE, MSG_DONTWAIT);
	gettimeofday(&tv_start, NULL);
	start = tv_start.tv_sec + ((float)tv_start.tv_usec / 1000000);

	src->bufferlengths[0] = rd;
	size_t total = (rd <= 0) ? 0 : rd;
	while (rd > 0)  {

		//fill packet size info for attributes header
		if (num == 1) sprintf(src->atts, "%6d#0.000000$", rd);
		else {
			char buff1[8];
			sprintf(buff1, "%6d#",rd);
			strcat(src->atts, buff1);
			//fill packet time length info for attributes header
			char buff2[10];
			sprintf(buff2, "%8.6f$", diff);
			strcat(src->atts, buff2);
		}

		rd = recv(src->udpSocket, src->buffers[num], MAX_PACKET_SIZE, MSG_DONTWAIT);
		gettimeofday(&tv_end, NULL);
		end = tv_end.tv_sec + ((float)tv_end.tv_usec / 1000000);
		diff = end - start;
		gettimeofday(&tv_start, NULL);
		start = tv_start.tv_sec + ((float)tv_start.tv_usec / 1000000);

		src->bufferlengths[num] = rd;
		if (rd <= 0) break;
		total += rd;
		if (++num == src->num_buffers) {
			src->buffers = realloc(src->buffers, sizeof(void *) *
					(1 + src->num_buffers));
			src->atts = realloc(src->atts, sizeof(char) * ((ATTRIBUTES_HEADER_SIZE *
					(1 + src->num_buffers)) + 1));
			src->bufferlengths = realloc(src->bufferlengths, sizeof(int) *
					(1 + src->num_buffers));
			src->bufferlengths[src->num_buffers] = 0;
			src->buffers[src->num_buffers] = malloc(MAX_PACKET_SIZE);
			src->num_buffers++;
		}
	} 

	if (total <= 0) return;

	/* and now we create a new chunk */
	void *data = malloc(total);
	int ptr = 0;
	int i;
	for (i = 0; i != num; i++) {
		memcpy(data + ptr, src->buffers[i], src->bufferlengths[i]);
		ptr += src->bufferlengths[i];
	}

	//fill packet checksum info for attributes header
	crc datacrc;
	datacrc = crcFast(data, total);
	char buff3[12];
	sprintf(buff3, "%10X$", (unsigned int)datacrc);
	src->atts = realloc(src->atts, (ATTRIBUTES_HEADER_SIZE * src->num_buffers) + 12);
	strcat(src->atts,buff3);

	src->atts_size = (num * sizeof(char) * ATTRIBUTES_HEADER_SIZE) + 12;
	struct chunk *c;
	if (src->atts_size > 0) {//chunk is encoded (timestamp is the chunk duration in milliseconds)
		c = chbCreateChunkWithAtts(false, data, total, src->chunkid,
				(int)(1000000*src->chunk_duration), src->atts, src->atts_size);
	}
	else {//raw data chunk (if packets were received this never happens)
		c = chbCreateChunk(false, data, total, src->chunkid,
				40*src->chunkid);
	}
	info("Adding chunk(%d): %d bytes, %d packets", src->chunkid++, total,
			num);

	if (c) chbAddChunk(chunkbuffer, c);

	//...
	//char *strs = c->attributes;
	//info("c->attributes:%s",strs);
}

int init_source_ul(const char *stream, double chunk_duration) {
	char addr[128];
	int port,i;

	if (sscanf(stream, "udp://%127[^:]:%i", addr, &port) != 2) {
		fatal("Unable to parse source specification %s", stream);
	}

	struct source_stream *src = malloc(sizeof(struct source_stream));
	struct sockaddr_in udpsrc;
	int returnStatus = 0;

	src->udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	udpsrc.sin_family = AF_INET;
	udpsrc.sin_addr.s_addr = inet_addr(addr);
	udpsrc.sin_port = htons(port);

	int status = bind(src->udpSocket,(struct sockaddr *)&udpsrc,sizeof(udpsrc));
	if (status) {
		fatal("Unable to bind socket to udp://%s:%d", addr, port);
	}

	src->chunk_duration = chunk_duration;
	src->chunkid = 0;
	src->buffers = calloc(sizeof(void *), INITIAL_PKTBUFFER_NUM);
	src->atts = calloc(sizeof(char), ATTRIBUTES_HEADER_SIZE * INITIAL_PKTBUFFER_NUM);
	src->atts = realloc(src->atts, sizeof(char) * ((ATTRIBUTES_HEADER_SIZE *
			INITIAL_PKTBUFFER_NUM) + 1));
	src->bufferlengths = calloc(sizeof(int), INITIAL_PKTBUFFER_NUM);
	for (src->num_buffers = 0; src->num_buffers != INITIAL_PKTBUFFER_NUM; src->num_buffers++)
		src->buffers[src->num_buffers] = malloc(MAX_PACKET_SIZE);
	src->atts_size = 0;

	grapesSchedulePeriodic(NULL, 1.0/chunk_duration, read_stream, src);
}



