/*
 * % vim: syntax=c ts=4 tw=76 cindent shiftwidth=4
 *
 * Messaging services fror the Peer implementation for the Broadcaster demo application
 *
 */

#include "peer.h"

#include <ml.h>
#include <msg_types.h>
#include <chunk.h>

cfg_opt_t cfg_stun[] = {
	CFG_STR("server", NULL, CFGF_NONE),
	CFG_INT("port", 3478, CFGF_NONE),
	CFG_END()
};

cfg_opt_t cfg_ml[] = {
	CFG_INT("port", 1234, CFGF_NONE),
	CFG_INT("timeout", 3, CFGF_NONE),
	CFG_STR("address", NULL, CFGF_NONE),
	CFG_SEC("stun", cfg_stun, CFGF_NONE),
	CFG_INT("keepalive", 0, CFGF_NONE),
	CFG_END()
};

bool messaging_ready = false;
char LocalPeerID[64] = "Uninitialized";
send_params sendParams;

void recv_localsocketID_cb(socketID_handle localID, int errorstatus) {
	if (errorstatus) fatal("Error %d on initializing Messaging Layer", errorstatus);
	mlSocketIDToString(localID, LocalPeerID, sizeof(LocalPeerID));
	if (errorstatus == 0) messaging_ready = true;
}

/**
  Utility function to send a Chunk to a Peer

  @param[in] peedID send to this peer
  @param[in] c the chunk
  */
void sendChunk(const char *peerID, const struct chunk *c) {

	if (strcmp(peerID, LocalPeerID) == 0) {
		warn("Refusing to send a chunk to myself...");
		return;
	}

	socketID_handle remsocketID = NULL; // the remote

	remsocketID = malloc(SOCKETID_SIZE);
	mlStringToSocketID(peerID, remsocketID);

	int connid = mlConnectionExist(remsocketID, true);
	free(remsocketID);

	if (connid < 0) return;

	if (c->attributes_size != 0) {//chunk is encoded
		debug("Sending chunk %u to %s with attributes",  chunkGetId(c), peerID);
	    /* code snipplet from the "real ml.c" */
		int buff_len;
	    uint8_t *buff;
	    buff_len = 20 + c->size + c->attributes_size;
	    buff = malloc(buff_len + 1);
	    if (!buff) {
	        warn("Error: unable to allocate memory %d bytes for sending chunk %d.", (buff_len+1) , c->id);
	        return;
	    }
	    int res = encodeChunk(c, buff + 1, buff_len);
	    buff[0] = MSG_TYPE_CHUNK;
		mlSendData(connid, buff, buff_len + 1,
				MSG_TYPE_CHUNK, NULL);
	}
	else {//chunk is raw data only
		debug("Sending chunk %u to %s",  chunkGetId(c), peerID);
		mlSendData(connid, chunkGetData(c, false), chunkGetSize(c),
				MSG_TYPE_CHUNK, NULL);
	}
}

/**
 * Receiver ML_KEEPALIVE callback. 
 * These are just ignored now.
 * @param *buffer A pointer to the buffer
 * @param buflen The length of the buffer
 * @param msgtype The message type
 * @param *arg An argument that receives metadata about the
 * received data
 */
void recv_keepalive_from_peer_cb(char *buffer,int buflen,unsigned char
		msgtype,recv_params *params) {

	char peer[64] = "";
	if (mlSocketIDToString(params->remote_socketID, peer, sizeof(peer))) {
		fatal("Internal error: Unable to decode sender in "
				"recv_keepalive_from_peer_cb");
	}

	if (buflen < 32 && buffer[buflen] == 0) {
		debug("Keepalive message received: %s from %s", buffer, peer);
	}
	else {
		warn("Bogus keepalive message received from %s", peer);
	}
}

/**
 * The peer receives data per callback from the messaging layer.
 * @param *buffer A pointer to the buffer
 * @param buflen The length of the buffer
 * @param msgtype The message type
 * @param *arg An argument that receives metadata about the
 * received data
 */
void recv_data_from_peer_cb(char *buffer,int buflen,unsigned char
		msgtype,recv_params *params) {

	char peer[64] = "";
	if (mlSocketIDToString(params->remote_socketID, peer, sizeof(peer))) {
		fatal("Internal error: Unable to decode sender in "
				"recv_data_from_peer_cb");
	}

	if (strcmp(peer, LocalPeerID) == 0) {
		warn("Received a chunk from myself - this should not have happened");
		return;
	}
	static int chunkid = 0;
	debug("CLIENT_MODE: Received data from %s: msgtype %d buflen %d (fragments: %d)", peer, msgtype, buflen, params->recvFragments);

	if (params->nrMissingBytes == 0) {
		if (buffer[0] == MSG_TYPE_CHUNK) {//chunk is encoded
			struct chunk c;
			int res = decodeChunk(&c, buffer + 1, buflen);
			if (res > 0) {
/* disable this because the attributes are already used by ExternalChunker
				//CRC checking...
				int packets = (c.attributes_size - 12) / ATTRIBUTES_HEADER_SIZE;

				char *rcvd_crc = malloc(11);
				memcpy(rcvd_crc, c.attributes + (packets * ATTRIBUTES_HEADER_SIZE), 10 );
				rcvd_crc[10] = 0;

				crc datacrc;
				datacrc = crcFast(c.data, c.size);
				char gen_crc[11];
				sprintf(gen_crc, "%10X", (unsigned int)datacrc);

				if (!strcmp(rcvd_crc,gen_crc)) {//CRC check passed
					int res;
*/
					res = chbAddChunk(chunkbuffer, &c);
					if (res == -2) warn("Chunk ERROR - Received chunk(%d) duplicate", c.id);
/*
				}
				else {//CRC check failed
					//... retransmission logic may be inserted here later
					warn("Chunk ERROR - CRC check failed for chunk(%d)", c.id);
				}
*/
			}
		}
		else {//only the data was received
			struct chunk *c;
			c = chbCreateChunk(true, buffer, buflen, chunkid,
					40*chunkid);
			if (c) chbAddChunk(chunkbuffer, c);
		}
		chunkid++;
	}
	else {
		warn("Chunk ERROR - Received message is missing %d bytes, chunk dropped (chunk duration is too high on source side)",
				params->nrMissingBytes);
	}
}

void ml_init(cfg_t *ml_config, int override_port) {
	if (!ml_config) return;
	struct timeval timeout = { 0,0 };
	timeout.tv_sec = cfg_getint(ml_config, "timeout");

	cfg_t *stun = cfg_getsec(ml_config, "stun");
	char *stun_server = cfg_getstr(stun, "server");
	int stun_port = cfg_getint(stun, "port");

	char *address = cfg_getstr(ml_config, "address");
	if (!address) address = strdup(mlAutodetectIPAddress());
	int port = cfg_getint(ml_config, "port");
	if (override_port) port = override_port;

	sendParams.keepalive = cfg_getint(ml_config, "keepalive");

	crcInit();
	debug("Calling init_messaging_layer with: %d, %u.%u, (%s, %d), stun(%s,%d)", 
		1, timeout.tv_sec, timeout.tv_usec, address, port, 
		stun_server, stun_port);
	mlInit(1, timeout, port, address, stun_port, stun_server, 
		recv_localsocketID_cb, eventbase);

	while (!messaging_ready) {
		grapesYield();
	}
	mlRegisterRecvDataCb(recv_data_from_peer_cb, MSG_TYPE_CHUNK);
	mlRegisterRecvDataCb(recv_keepalive_from_peer_cb, MSG_TYPE_ML_KEEPALIVE);

	info("ML has been initialized, local addess is %s", LocalPeerID);
}


