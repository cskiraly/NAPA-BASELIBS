/*
 * % vim: syntax=c ts=4 tw=76 cindent shiftwidth=4
 *
 * Simplistic "SOM" module for the Broadcaster demo application
 *
 */

#include "peer.h"

#include <ml.h>
#include <neighborlist.h>
#include <chunk.h>
#include <chunkbuffer.h>
#include <repoclient.h>

/** Configuration options for the NeighborList */
cfg_opt_t cfg_nlist[] = {
	CFG_INT("refreshPeriod", 30, CFGF_NONE),
	CFG_INT("size", 10, CFGF_NONE),
	CFG_INT("peerAge", 300, CFGF_NONE),
	CFG_END()
};

/** Configuration options for this "SOM" */
cfg_opt_t cfg_som[] = {
	CFG_STR("protocol", "passive", CFGF_NONE),
	CFG_STR("channel", NULL, CFGF_NONE),
	CFG_INT("publishPeerIDPeriod", 0, CFGF_NONE),
	CFG_SEC("neighborlist", cfg_nlist, CFGF_NONE),
	CFG_END()
};

HANDLE neighborlist = NULL;
char *channel = NULL;

NeighborListEntry *neighbors = NULL;
int neighbors_size = 0;
int num_neighbors = 0;

struct chunk_buffer *chunkbuffer = NULL;

void findServer(int fd, short event, void *arg);

/** Gets called by the ML on the establishment of a connection (regardless
 * of who initiated it 
 */
void connection_cb(int connectionID, void *arg) {
	debug("A connection to  %s has been established", arg);

	char remote_socket[SOCKETID_SIZE];
	socketID_handle remsocketID = (void *)remote_socket;
	mlStringToSocketID((char *)arg, remsocketID);

	activateMeasurements(remsocketID);
}

/** NeighborList callback: gets called when the neighborlist changes */
void neighborlist_cb(HANDLE nlist, void *arg) {
	num_neighbors = neighbors_size;
	int error = neighborlist_query(nlist, neighbors, &num_neighbors);
	if (error) {
		error("neighborlist_query returned error %d", error);
		return;
	}

	/* Open a connection to all the neighbors */
	int i;
	socketID_handle remsocketID = malloc(SOCKETID_SIZE);
	for (i = 0; i != num_neighbors; i++) {
		char *neighbor = strdup(neighbors[i].peer);

		mlStringToSocketID(neighbor, remsocketID);

		if (mlConnectionExist(remsocketID, false) < 0 && 
				strcmp(neighbors[i].peer, LocalPeerID) != 0) {
			debug("Opening connection to %s", neighbors[i].peer);
			mlOpenConnection(remsocketID, connection_cb, neighbors[i].peer,
					sendParams);
		}
		free(neighbor);
	}
	free(remsocketID);
}

/** This function gets called periodically and arranges publishing out own
 * PeerID to the Repository 
 */
void static publish_PeerID(HANDLE h, void *arg) {
		/* Publish our own PeerID */
		MeasurementRecord mr;
		memset(&mr, 0, sizeof(mr));
		mr.published_name = "PeerID";
		mr.channel = channel;
		mr.originator = mr.targetA = mr.targetB = LocalPeerID;
		publishMeasurementRecord(&mr, "Our own PeerID");
}

/** ChunkBuffer notifier: gets called whenever a new Chunk is inserted into
 * the buffer 
 */
void chunkbuffer_notifier(struct chunk_buffer *cb, void* cbarg, const struct
		chunk *c) {
	/* Send the good stuff to all the neighbors */
	int i;
	for (i = 0; i != num_neighbors; i++) {
		if (strcmp(neighbors[i].peer, LocalPeerID)) {
			debug("Sending chunk %u to %s",  chunkGetId(c), neighbors[i].peer);
			sendChunk(neighbors[i].peer, c);
		}
	}

	playout_chunk(c);
}

/** This function gets called when the "source locator" finishes.
  * Each "client" tries to find the PeerID of the source (a Peer with the
  * SrcLag measurement 0.0) and open a Connection towards it.
  * This function is needed only because a ML deficiency (sometimes a
  * Connection can be opened from A to B only after B has initiated to
  * connection to A).
  */
void findServer_cb(HANDLE rep, HANDLE id, void *cbarg, char **result, int
		nResults) {
	if (nResults != 1) {
		/* We couldn't find the server properly, try again in 30 secs. */
		struct timeval t = { 30, 0 };
		event_base_once(eventbase, -1, EV_TIMEOUT, &findServer, NULL, &t);
		return;
	}

	info("The server is at %s", result[0]);

	static char server[64];
    strcpy(server,result[0]);
	socketID_handle remsocketID = malloc(SOCKETID_SIZE);

	mlStringToSocketID(server, remsocketID);
	mlOpenConnection(remsocketID, connection_cb, server, sendParams);
	free(remsocketID);
	free(result[0]);
	free(result);
}

/** This function is the "source locator", that makes the repository query.
  * Each "client" tries to find the PeerID of the source (a Peer with the
  * SrcLag measurement 0.0) and open a Connection towards it in the
  * callback function.
  */
void findServer(int fd, short event, void *arg) {
	Constraint cons[1];

	cons[0].published_name = "SrcLag";
	cons[0].strValue = NULL;
	cons[0].minValue = cons[0].maxValue = 0.0;
	repGetPeers(repository, findServer_cb, NULL, 1, cons, 1, NULL, 0,
			channel);
}

/** Initializer for this "SOM" */
void som_init(cfg_t *som_config) {
	if (!som_config) return;

	const char *protocol = cfg_getstr(som_config, "protocol");
	channel = cfg_getstr(som_config, "channel");

	info("Scheduler is initializing protocol [ \"%s\" ] on channel %s",
			protocol,channel);

	chunkbuffer = chbInit("size=256,time=now");
	if (!chunkbuffer) 
		fatal("Error initialising the Chunk Buffer");
	chbRegisterNotifier(chunkbuffer, chunkbuffer_notifier, NULL);

	int publishPeerIDPeriod =  cfg_getint(som_config,
			"publishPeerIDPeriod");
	if (publishPeerIDPeriod) 
		napaSchedulePeriodic(NULL, 1.0/(float)publishPeerIDPeriod,
				publish_PeerID, NULL);

	/* If we are passive, we need to figure out who is the server, and send
	 * a message to it for ML to be able to work... Sigh... */
	if (!strcmp(protocol, "passive")) {
		struct timeval t = { 0, 0 };
		event_base_once(eventbase, -1, EV_TIMEOUT, &findServer, NULL, &t);
	}

	/* If the string "neighborlist" is present in the protocol name, we
	 * launch a neighborlist instance */
	if (strstr(protocol, "neighborlist")) {
		cfg_t *nlist_cfg = cfg_getsec(som_config, "neighborlist");
		neighbors_size = cfg_getint(nlist_cfg, "size");

		neighbors = calloc(sizeof(NeighborListEntry), neighbors_size);

		neighborlist = neighborlist_init(repository, 
			neighbors_size,
			cfg_getint(nlist_cfg, "refreshPeriod"),
			channel,
			neighborlist_cb, NULL);
	}
}


