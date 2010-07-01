/*
 * % vim: syntax=c ts=4 tw=76 cindent shiftwidth=4
 *
 * User layer services (playout) the Peer implementation for the RepoBroadcaster demo application
 *
 */

#include <netinet/in.h>
#include <sys/socket.h>

#include "peer.h"

#include <chunk.h>
#include <mon.h>
#include <ul.h>

#define	VLC_PKTLEN	1316

cfg_opt_t cfg_playout[] = {
	CFG_STR("stream", NULL, CFGF_NONE),
	CFG_BOOL("const_bitrate", false, CFGF_NONE),
	CFG_END()
};

struct playout_stream {
	int udpSocket;
	struct sockaddr_in dst;
	bool const_bitrate;
	int last_chunk;
	int curr_chunk;
	int last_chunk_id;
	bool skip_chunk;
} playout;

struct playout_chunk {
	int id;
	int seqnum;
	char *data;
	int packets;
	int currentpacket;
	void *attributes;
	int alreadyplayed;
	int size;
	double duration;
};

static char *stream = NULL;

void playout_packet(int fd, short event, void *arg) {
	struct playout_chunk *po_c = arg;

	if (po_c->seqnum > playout.curr_chunk) {//waiting for our turn
		int queuelen = po_c->seqnum - playout.curr_chunk;
		struct timeval t = { 0, 0 };
		if (queuelen > 1) {
			info("Chunk(%d) waiting in queue for turn",po_c->id);
			double waitchunks = (queuelen - 1) * po_c->duration;
			t.tv_usec = (int)(waitchunks*1000000) + 500;
		}
		else {
			t.tv_usec = 500;
		}
		event_base_once(eventbase, -1, EV_TIMEOUT, &playout_packet, po_c, &t);
	}
	else {//actually playout the packets of this chunk
		if (po_c->currentpacket != po_c->packets) {//still some packets to be played
			if (po_c->currentpacket == 0)
				info("Playing out chunk(%d): %u B, %d packets", po_c->id, po_c->size, po_c->packets);
			char *c_psize_i = malloc(7 * sizeof(char));
			memcpy(c_psize_i ,po_c->attributes + (po_c->currentpacket * sizeof(char) * ATTRIBUTES_HEADER_SIZE), 6 * sizeof(char));
			memcpy(c_psize_i + 6, "\0", sizeof(char));
			int i_p_size_i = atoi(c_psize_i);

			double d_ptime_i;
			if (!playout.const_bitrate) {
				char *c_ptime_i = malloc(9 * sizeof(char));
				if (po_c->currentpacket != (po_c->packets - 1)) {
					memcpy(c_ptime_i, po_c->attributes + ((po_c->currentpacket+1) * sizeof(char) * ATTRIBUTES_HEADER_SIZE) + 7 * sizeof(char), 8 * sizeof(char));
					memcpy(c_ptime_i + 8, "\0", sizeof(char));
					d_ptime_i = atof(c_ptime_i);
				}
				else {//delay after the last packet played is assumed from the last delay measured
					memcpy(c_ptime_i, po_c->attributes + ((po_c->currentpacket) * sizeof(char) * ATTRIBUTES_HEADER_SIZE) + 7 * sizeof(char), 8 * sizeof(char));
					memcpy(c_ptime_i + 8, "\0", sizeof(char));
					d_ptime_i = atof(c_ptime_i);
				}
			}
			else {
				if (po_c->currentpacket != (po_c->packets - 1)) d_ptime_i = (0.8 * po_c->duration) / ((double)po_c->packets);
				else d_ptime_i = 0.0;
			}

			debug("Playing out packet(%d): %d B after %8.6f secs", po_c->currentpacket + 1, i_p_size_i, d_ptime_i);
			sendto(playout.udpSocket, po_c->data + po_c->alreadyplayed, i_p_size_i, 0,
					(struct sockaddr *)&(playout.dst), sizeof(struct sockaddr_in));
			po_c->alreadyplayed += i_p_size_i;
			po_c->currentpacket++;

			struct timeval t = { 0, 0 };
			t.tv_sec = (int)d_ptime_i;
			t.tv_usec = (int)(d_ptime_i*1000000);
			event_base_once(eventbase, -1, EV_TIMEOUT, &playout_packet, po_c, &t);
		}
		else {//finished with the playout of the current chunk
			playout.curr_chunk++;
			debug("Sequence number of the last chunk played: %d", playout.curr_chunk);
			if (po_c->alreadyplayed != po_c->size)
				warn("Chunk ERROR - Chunk(%d) playout incomplete: size: %u bytes, played: %d bytes",
						po_c->id, po_c->size, po_c->alreadyplayed);
		}
	}
}

void playout_chunk(const struct chunk *c) {
	if (!stream) return;
	if (playout.last_chunk_id == -1) {//first chunk we've received in this session
		info("First chunk from the new source received!");
		playout.curr_chunk = 0;
		playout.last_chunk_id = c->id;
	}
	else if (playout.last_chunk_id != (c->id)-1) {
		warn("Chunk ERROR - sequence mismatch [last received: %d, just received: %d, expected: %d] - chunk(%d) skipped",
				playout.last_chunk_id, c->id, (playout.last_chunk_id)+1, c->id);
		//... retransmission logic may be inserted here later
		//now we just temporarily skip the missing chunks
		playout.last_chunk_id = c->id;
		playout.skip_chunk = 1;
	}
	else {
		playout.last_chunk_id++;
		playout.last_chunk++;
		playout.skip_chunk = 0;
	}

	if (!playout.skip_chunk) {
		if (c->attributes_size > 0) {//chunk is encoded
			struct playout_chunk *po_c = malloc(sizeof(struct playout_chunk));
			po_c->id = c->id;
			po_c->seqnum = playout.last_chunk + 1;
			po_c->alreadyplayed = 0;
			po_c->attributes = c->attributes;
			po_c->currentpacket = 0;
			po_c->data = c->data;
			po_c->packets = ((c->attributes_size - 12) / (ATTRIBUTES_HEADER_SIZE * sizeof(char)));
			po_c->size = c->size;
			//with constant bitrate the packets are played out in the 80% of the chunk duration
			po_c->duration = ((double)c->timestamp)/1000000;

			//...
			//char *strs = c->attributes;
			//info("c->attributes:%s",strs);

			info("Buffering chunk(%d): %u B, %d packets", po_c->id, po_c->size, po_c->packets);
			debug("Sequence number of the last chunk received: %d", playout.last_chunk);

			//struct timeval t = { 0, 0 };
			struct timeval t = { 3, 0 };
			event_base_once(eventbase, -1, EV_TIMEOUT, &playout_packet, po_c, &t);
		}
		else {//only the data was received
			int pkts = c->size / VLC_PKTLEN;
			if (pkts * VLC_PKTLEN != c->size) {
				warn("Chunk ERROR - Damaged chunk received: %d, size: %u bytes", c->id, c->size);
				return;
			}
			int i;
			char *data = c->data;
			info("Playing out chunk(%d) of length %u", c->id, c->size);
			for (i = 0; i != pkts; i++) {
				sendto(playout.udpSocket, data + i * VLC_PKTLEN, VLC_PKTLEN, 0,
						(struct sockaddr *)&(playout.dst), sizeof(struct sockaddr_in));
			}
		}
	}
}

void playout_init(cfg_t *playout_cfg) {
	char addr[128];
	int port,i;

	if (!playout_cfg) return;
	stream = cfg_getstr(playout_cfg, "stream");
	if (!stream) return;

	if (!strncmp("udp://", stream, 6)) {
		/* VLC-style player */
		playout.const_bitrate = cfg_getbool(playout_cfg, "const_bitrate");
		if (playout.const_bitrate) info("Initializing PLAYOUT mode on stream %s with constant bitrate (based on chunk duration)", stream);
		else info("Initializing PLAYOUT mode on stream %s with variable bitrate (based on per-packet information)", stream);

		if (sscanf(stream, "udp://%127[^:]:%i", addr, &port) != 2) {
			fatal("Unable to parse source specification %s", stream);
		}

		int returnStatus = 0;

		playout.udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
		playout.dst.sin_family = AF_INET;
		playout.dst.sin_addr.s_addr = inet_addr(addr);
		playout.dst.sin_port = htons(port);
		playout.last_chunk = -1;
		playout.curr_chunk = -1;
		playout.last_chunk_id = -1;
		playout.skip_chunk = 0;
	}
	else if (!strncmp("http://", stream, 7)) {
		int position = 0;
		char path[128];
		if (sscanf(stream, "http://%127[^:]:%i%s", addr, &port, path) != 3) {
			fatal("Unable to parse source specification %s", stream);
		}

		int returnStatus = 0;

		//register the external player at the specified
		//address,port
		info("Attempting to register external player at %s:%d%s postion %d", 
				addr, port, path, position);
		ulRegisterApplication(addr, &port, path, &position);
		info("Registered external player at %s:%d%s postion %d", 
			addr, port, path, position);
	}
	else fatal("Unable to parse source specification %s", stream);
}



