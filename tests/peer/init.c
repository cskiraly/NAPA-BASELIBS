/*
 * Initialization and configuration processing for the peer
 */

#include	"peer.h"

char LocalPeerID[64] = "Uninitialized";

void init_messaging(cfg_t *ml_config);
void init_monitoring(cfg_t *mon_config);
void init_repoclient(cfg_t *repo_config);
void init_source(cfg_t *source_config);

void setup_http_injector(const char *url);
void setup_http_ejector(const char *url,int delay);

cfg_opt_t cfg_stun[] = {
	CFG_STR("server", "stun.ekiga.net", CFGF_NONE),
	CFG_INT("port", 3478, CFGF_NONE),
	CFG_END()
};

cfg_opt_t cfg_ml[] = {
	CFG_INT("port", 1234, CFGF_NONE),
	CFG_INT("timeout", 3, CFGF_NONE),
	CFG_STR("address", NULL, CFGF_NONE),
	CFG_SEC("stun", cfg_stun, CFGF_NONE),
	CFG_END()
};

cfg_opt_t cfg_rep[] = {
	CFG_STR("server", NULL, CFGF_NONE),
	CFG_END()
};

cfg_opt_t cfg_source[] = {
	CFG_STR("stream", NULL, CFGF_NONE),
	CFG_FLOAT("chunk_duration", 1.0, CFGF_NONE),
	CFG_END()
};

cfg_opt_t cfg_main[] = {
	CFG_SEC("network", cfg_ml, CFGF_NONE),
	CFG_SEC("repository", cfg_rep, CFGF_NONE),
	CFG_SEC("source", cfg_source, CFGF_NONE),
	CFG_END()
};

int messaging_ready = 0;

void new_chunk(struct chunk_buffer *cb, void *cbarg, const struct chunk *c) {
        info("New chunk in buffer");
}


Peer *peer_init(const char *configfile) {
	peer = calloc(sizeof(Peer),1);

	grapesInit(event_base_new());

	cfg_t *main_config = cfg_init(cfg_main, CFGF_NONE);
	if(cfg_parse(main_config, configfile) == CFG_PARSE_ERROR)
		fatal("Unable to parse config file %s", configfile);

	cfg_t *repoclient_cfg = cfg_getsec(main_config, "repository");
	if (repoclient_cfg) init_repoclient(repoclient_cfg);

	cfg_t *network_cfg = cfg_getsec(main_config, "network");
	init_messaging(network_cfg);

	init_monitoring(NULL);

	cfg_t *source_cfg = cfg_getsec(main_config, "source");
	if (source_cfg) init_source(source_cfg);

        /* cfg_free(main_config); */
	return peer;
}

void recv_localsocketID_cb(socketID_handle localID, int errorstatus) {
	if (errorstatus) fatal("Error %d on initializing Messaging Layer", errorstatus);
	mlSocketIDToString(localID, LocalPeerID, sizeof(LocalPeerID));
	if (errorstatus == 0) messaging_ready = 1;
}

void init_messaging(cfg_t *ml_config) {
	if (!ml_config) return;
	struct timeval timeout = { 0,0 };
	timeout.tv_sec = cfg_getint(ml_config, "timeout");

	cfg_t *stun = cfg_getsec(ml_config, "stun");
	char *stun_server = cfg_getstr(stun, "server");
	int stun_port = cfg_getint(stun, "port");

	char *address = cfg_getstr(ml_config, "address");
	if (!address) address = strdup(mlAutodetectIPAddress());
	int port = cfg_getint(ml_config, "port");

	debug("Calling init_messaging_layer with: %d, %u.%u, (%s, %d), stun(%s,%d)", 
		1, timeout.tv_sec, timeout.tv_usec, address, port, 
		stun_server, stun_port);
	mlInit(1, timeout, port, address, stun_port, stun_server, 
		recv_localsocketID_cb, eventbase);

	while (!messaging_ready) {
		grapesYield();
        }
	info("ML has been initialized, local addess is %s", LocalPeerID);
}

void init_monitoring(cfg_t *mon_config) { 
#if 0
	if (monInit()) fatal("Unable to init monitoring layer");
#endif
}

void init_repoclient(cfg_t *repo_config) { 
	repInit("");
	peer->repository = repOpen(cfg_getstr(repo_config, "server"),0);
        if (peer->repository == NULL) fatal("Unable to initialize repoclient");
}


void init_source(cfg_t *source_cfg) {

	char *stream = cfg_getstr(source_cfg, "stream");
	if (!stream) return;
	double chunk_duration = cfg_getfloat(source_cfg,"chunk_duration");

	info("Initializing SOURCE mode: tuning to stream %s, sampling rate is %lf Hz", 
		stream, (double)1.0/chunk_duration);
	 init_source_ul(stream, chunk_duration) ;

#if 0
	mlRegisterRecvConnectionCb(&receive_conn_cb);
	mlRegisterErrorConnectionCb(&conn_fail_cb);
#endif
}

void publish_callback(HANDLE client, HANDLE id, void *cbarg, int result) {
        info("PeerID publised");
	if (result) fatal("Unable to publish PeerID");
}

void client_init(cfg_t *client_config) {
#if 0
	info("Initializing CLIENT mode: publishing local PeerID");
	MeasurementRecord mr;
	memset(&mr, 0, sizeof(mr));
	mr.published_name = "PeerID";
	mr.originator = mr.targetA = mr.targetB = LocalPeerID;
	repPublish(peer->repository, publish_callback, NULL, &mr);

	info("Initializing CLIENT mode: starting http ejector at %s", peer->ejector_url);
	setup_http_ejector(peer->ejector_url,0);

	mlRegisterRecvDataCb(&recv_data_from_peer_cb,12);
	mlRegisterRecvDataCb(&recv_chunk_from_peer_cb,15);
#endif
}

