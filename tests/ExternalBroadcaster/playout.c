/*
 * % vim: syntax=c ts=4 tw=76 cindent shiftwidth=4
 *
 * User layer services (playout) the Peer implementation for the RepoBroadcaster demo application
 *
 */

#include <netinet/in.h>
#include <sys/socket.h>

#include "peer.h"

#include <http_default_urls.h>
#include <chunk.h>
#include <mon.h>
#include <ul.h>

#define	VLC_PKTLEN	1316

cfg_opt_t cfg_playout[] = {
	CFG_STR("stream", NULL, CFGF_NONE),
	CFG_BOOL("const_bitrate", false, CFGF_NONE),
	CFG_END()
};


static char *stream = NULL;


void playout_chunk(const struct chunk *c) {
	if(!stream) //are we source?
		return;
	// as of now this just sends the chunk via HTTP to the external
	// chunker_player
	ulSendChunk(c);
	//debug("Just sent a chunk via HTTP");
}

void playout_init(cfg_t *playout_cfg) {
	char addr[128];
	int port,i;
	int position = 0;

	if (!playout_cfg) return;
	stream = cfg_getstr(playout_cfg, "stream");
	if (!stream) return;
//	playout.const_bitrate = cfg_getbool(playout_cfg, "const_bitrate");
//	if (playout.const_bitrate) info("Initializing PLAYOUT mode on stream %s with constant bitrate (based on chunk duration)", stream);
//	else info("Initializing PLAYOUT mode on stream %s with variable bitrate (based on per-packet information)", stream);

	if (sscanf(stream, "http://%127[^:]:%i", addr, &port) != 2) {
		fatal("Unable to parse source specification %s", stream);
	}

	int returnStatus = 0;

	port = UL_DEFAULT_EXTERNALPLAYER_PORT;
  //register the external player at the specified address,port
	info("Attempting to register external player at %s:%d%s postion %d", addr, port, UL_DEFAULT_EXTERNALPLAYER_PATH, position);
	ulRegisterApplication(addr, &port, UL_DEFAULT_EXTERNALPLAYER_PATH, &position);
	info("Registered external player at %s:%d%s postion %d", addr, port, UL_DEFAULT_EXTERNALPLAYER_PATH, position);
}



