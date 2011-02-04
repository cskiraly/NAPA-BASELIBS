#include <event2/event.h>
#include <unistd.h>
#include <string.h>
#include <math.h>
#include <netinet/in.h>
#include <sys/socket.h>

#include "nvtest2.h"

// Defines
#define SOCKETID_PUBLISH_NAME "SocketId"
#define SOCKETID_PUBLISH_VALUE_MIN 0
#define SOCKETID_PUBLISH_VALUE_MAX 1000

// Global variables containing peer information
Peer *peer = NULL;
int messaging_ready = 0;
int active = 0; // initially a peer is passive (before the first remote peer is chosen)
int currentConnectionID = -1;
bool dummymode = 0; // if dummymode is on, no real traffic is sent
int chunkcounter = 0;
int chunktoken = 1;

// Global variables containing the remote peer information (to whom requests are sent)
socketID_handle remsocketID = NULL; // the remote SocketId
char remsocketID_string[SOCKETID_STRING_SIZE] = "Uninitialized"; // the remote SocketId string conterpart

// Messages - Note first characters identifies Request form Replies and must be maintaned - DummyMode ONLY
char request[] = "APing";
char reply[] = "BPong";

// Structure containing the source stream information
struct source_stream {
	struct bufferevent *bev;
	double chunk_duration;
	void *read_buffer;
	int read_size;
	int chunkid;
};

// default send_params
send_params sParams;

// Structure for a received chunk
struct recievedchunk {
	int currentpkt;
	int pkts;
	char *buffer;
	int chunkid;
};

// Structure (and instance) containing the playout stream information
struct playout_stream {
	int udpSocket;
	struct sockaddr_in dst;
	int read_packet_size;
	int palyout_packet_size;
	int playout_frequency;
	int cycle_buffer;
} playout;

// Types for the config file parser
cfg_opt_t cfg_stun[] = {
	CFG_STR("server", "stun.ekiga.net", CFGF_NONE),
	CFG_INT("port", 3478, CFGF_NONE),
	CFG_END()
};

cfg_opt_t cfg_peer[] = {
	CFG_INT("value", 0, CFGF_NONE),
	CFG_END()
};

cfg_opt_t cfg_ml[] = {
	CFG_INT("port", 9000, CFGF_NONE),
	CFG_INT("timeout", 3, CFGF_NONE),
	CFG_STR("address", NULL, CFGF_NONE),
	CFG_SEC("stun", cfg_stun, CFGF_NONE),
	CFG_SEC("peerid", cfg_peer, CFGF_NONE),
	CFG_END()
};

cfg_opt_t cfg_rep[] = {
	CFG_STR("server", NULL, CFGF_NONE),
	CFG_END()
};

cfg_opt_t cfg_nblist[] = {
	CFG_INT("desired_size", 10, CFGF_NONE),
	CFG_INT("update_period", 15, CFGF_NONE),
	CFG_STR("channel", "Nvtest2", CFGF_NONE),
	CFG_END()
};

cfg_opt_t cfg_log[] = {
	CFG_INT("level", LOG_DEBUG, CFGF_NONE),
	CFG_END()
};

cfg_opt_t cfg_stream[] = {
	CFG_STR("source", NULL, CFGF_NONE),
	CFG_STR("destination", NULL, CFGF_NONE),
	CFG_FLOAT("chunk_duration", 1.0, CFGF_NONE),
	CFG_INT("read_packet_size", 1316, CFGF_NONE),
	CFG_INT("playout_packet_size", 1316, CFGF_NONE),
	CFG_INT("playout_frequency", 200, CFGF_NONE),
	CFG_INT("cycle_buffer", 100, CFGF_NONE),
	CFG_END()
};

cfg_opt_t cfg_main[] = {
	CFG_SEC("network", cfg_ml, CFGF_NONE),
	CFG_SEC("repository", cfg_rep, CFGF_NONE),
	CFG_SEC("neighborlist", cfg_nblist, CFGF_NONE),
	CFG_SEC("logging", cfg_log, CFGF_NONE),
	CFG_SEC("stream", cfg_stream, CFGF_NONE),
	CFG_END()
};

// Function prototypes
const char *print_list_nblist(NeighborListEntry *list, int n);
const char *print_list(char **list, int n, bool should_free);
Peer *peer_init(const char *configfile);
void init_monitoring(cfg_t *mon_config);
void init_repoclient(cfg_t *repo_config);
void init_neighborreqs(cfg_t *nb_config);
void init_messaging(cfg_t *ml_config);
void init_source(cfg_t *source_config);
void periodic_nblst_query(HANDLE h, void *arg);
void publish_peerid();
void publish_connection_req();
void choose_remote_peer();
void open_conn_cb_active(int connectionID, void *arg);
void send_data_cb(int fd, short event, void *arg);
void rx_data_stream_cb(char *buffer, int buflen, MsgType mt, recv_params *rparam);
void periodic_buffer_playout(int fd, short event, void *arg);
void rx_data_cb(char *buffer, int buflen, MsgType mt, recv_params *rparam);
void open_con_cb_passive(int c_id, void *arg);
void send_reply(int c_id, socketID_handle sock);
void conn_fail_cb(int connectionID, void *arg);
void receive_local_socketID_cb(socketID_handle local_socketID, int status);
void get_peers_cb(HANDLE client, HANDLE id, void *cbarg, char **result, int n);
int init_dest_ul(const char *stream, int packet_size, int frequency, int read_size, int cycle_buffer);
int init_source_ul(const char *stream, double chunk_duration);
void check_conn_reqs();
void check_conn_reqs_cb(HANDLE client, HANDLE id, void *cbarg, MeasurementRecord *result, int nResults);
void open_conn_cb_reply(int connectionID, void *arg);
void read_stream(HANDLE h, void *arg);

// Helper to print a string list of Peers for the periodic update of the Peers list
const char *print_list_nblist(NeighborListEntry *list, int n) {
	static char buffer[4096];
	struct timeval now;
	gettimeofday(&now, NULL);
	strcpy(buffer, "[ ");
	int i;
	for (i = 0; i != n; i++) {
		if (i) strcat(buffer, ", ");
		strcat(buffer, list[i].peer);
		char buf[32];
		sprintf(buf, "(%d)", (int)(now.tv_sec - list[i].update_timestamp.tv_sec));
		strcat(buffer, buf);
	}
	strcat(buffer, " ]");
	return buffer;
}

// Helper to print a string list and to store it in the Peer structure
const char *print_list(char **list, int n, bool should_free) {
	static char buffer[4096];
	int i;
	for (i = 0; i < n; i++) {
		if (i) strcat(buffer, "\n");
		strcat(buffer, list[i]);
		if (should_free) free(list[i]);
	}
	if (should_free) free(list);
	return buffer;
}

// Initialization of a peer
Peer *peer_init(const char *configfile) {
	peer = calloc(sizeof(Peer), 1);

	// Init napa: log facility and libevent
	napaInit(event_base_new());
	info("NAPA initialized.");

	// Parse config file */
	cfg_t *main_config = cfg_init(cfg_main, CFGF_NONE);
	if(cfg_parse(main_config, configfile) == CFG_PARSE_ERROR)
		fatal("Unable to parse config file %s!", configfile);

	// Initialize logging/monitoring
	cfg_t *logging_cfg = cfg_getsec(main_config, "logging");
	if (logging_cfg) init_monitoring(logging_cfg);

	// Initialize repository connection
	cfg_t *repoclient_cfg = cfg_getsec(main_config, "repository");
	if (repoclient_cfg) init_repoclient(repoclient_cfg);

	// Initialize neighborlist
	cfg_t *nblist_cfg = cfg_getsec(main_config, "neighborlist");
	if (nblist_cfg) init_neighborreqs(nblist_cfg);

	// Initialize messaging layer
	cfg_t *network_cfg = cfg_getsec(main_config, "network");
	init_messaging(network_cfg);

	// Initialize connection opener loop for requested connections
	struct timeval begin = { 10, 0 };
	napaSchedulePeriodic(&begin, 1.0/(double)peer->nblist_update_period, check_conn_reqs, NULL);

	// Initialize stream source and destination
	if (!dummymode) {
		cfg_t *stream_cfg = cfg_getsec(main_config, "stream");
		if (stream_cfg) init_source(stream_cfg);
	}
	
	/* cfg_free(main_config); */

	return peer;
}

// Initialize logging/monitoring
void init_monitoring(cfg_t *mon_config) {
	int verbosity = LOG_DEBUG;
	verbosity = cfg_getint(mon_config, "level");

	// Initialize logging
	napaInitLog(verbosity, NULL, NULL);
	info("Logging initialized with verbosity: %d", verbosity);

	// Init monitoring layer
	if (monInit(eventbase, NULL)) fatal("Unable to init monitoring layer!");
}

// Initialize repository connection
void init_repoclient(cfg_t *repo_config) {
	// Init repository client
	repInit("");
	peer->repository = repOpen(cfg_getstr(repo_config, "server"),0);
        if (peer->repository == NULL) fatal("Unable to initialize repository client!");
    info("Repository connection initialized to: %s", cfg_getstr(repo_config, "server"));
}

// Initialize neighborlist and start periodic update
void init_neighborreqs(cfg_t *nb_config) {
	peer->nblist_desired_size = cfg_getint(nb_config, "desired_size");
	peer->nblist_update_period = cfg_getint(nb_config, "update_period");	
	peer->channel = cfg_getstr(nb_config, "channel");
	peer->neighborlist = neighborlist_init(peer->repository, peer->nblist_desired_size, peer->nblist_update_period, peer->channel, NULL, NULL);

	struct timeval begin = { 5, 0 };
	napaSchedulePeriodic(&begin, 1.0/(double)peer->nblist_update_period, periodic_nblst_query, peer->neighborlist);
}

// Initialize messaging layer
void init_messaging(cfg_t *ml_config) {
	if (!ml_config) return;
	struct timeval timeout = { 0, 0 };
	timeout.tv_sec = cfg_getint(ml_config, "timeout");

	cfg_t *stun = cfg_getsec(ml_config, "stun");
	char *stun_server = cfg_getstr(stun, "server");
	int stun_port = cfg_getint(stun, "port");

	cfg_t *peerid = cfg_getsec(ml_config, "peerid");
	peer->PeerIDvalue = cfg_getint(peerid, "value");

	char *address = cfg_getstr(ml_config, "address");
	if (!address) address = strdup(mlAutodetectIPAddress());
	int port = cfg_getint(ml_config, "port");

	if (dummymode) mlRegisterRecvDataCb(&rx_data_cb, 3);
	else mlRegisterRecvDataCb(&rx_data_stream_cb, 3);
	mlRegisterErrorConnectionCb(&conn_fail_cb);

	info("Calling init_messaging_layer with: %d, %u.%u, (%s, %d), stun(%s, %d)",
		1, timeout.tv_sec, timeout.tv_usec, address, port,
		stun_server, stun_port);
	mlInit(1, timeout, port, address, stun_port, stun_server,
		receive_local_socketID_cb, (void*)eventbase);

	while (!messaging_ready) {
		napaYield();
	}
	info("ML has been initialized, with local addess: %s", peer->LocalID);
}

// Initialize video source
void init_source(cfg_t *str_config) {
	char *source = cfg_getstr(str_config, "source");
	if (!source) return;
	char *destination = cfg_getstr(str_config, "destination");
	if (!destination) return;
	double chunk_duration = cfg_getfloat(str_config, "chunk_duration");
	int read_size = cfg_getint(str_config, "read_packet_size");
	int packet_size = cfg_getint(str_config, "playout_packet_size");
	int frequency = cfg_getint(str_config, "playout_frequency");
	int cycle_buffer = cfg_getint(str_config, "cycle_buffer");

	info("Initializing Destination: tuning to output stream %s [packet size: %d kbytes, frequency: %d]",
			destination, packet_size, frequency);
	init_dest_ul(destination, packet_size, frequency, read_size, cycle_buffer);

	info("Initializing Source: tuning to input stream %s [sampling rate: %lf Hz]",
			source, (double)1.0/chunk_duration);
	init_source_ul(source, chunk_duration);
}

// Getting peer list (storing and printing) - choose the remote peer - publish again the localID
void periodic_nblst_query(HANDLE h, void *arg) {
	peer->neighborlist = (HANDLE)arg;
	peer->neighborlist_size = peer->nblist_desired_size;
	peer->neighbors = NULL;
	peer->neighbors = calloc(sizeof(NeighborListEntry), peer->neighborlist_size);

	//Periodically publish our ID in the repository
	publish_peerid();

	int success = neighborlist_query(peer->neighborlist, peer->neighbors, &peer->neighborlist_size);
	if (!success && peer->neighborlist_size > 0) {
		// Print the neighbor list if it's not empty
		info("GetPeers done: %d Peers currently in the repository. List:\n%s", peer->neighborlist_size, print_list_nblist(peer->neighbors, peer->neighborlist_size));
		// Choose a random remote peer from the neighbors list to whom requests will be sent to (if messaging layer is ready)
		if (messaging_ready) choose_remote_peer();
	}
	else
		info("GetPeers done: No results for the periodic query!");
}

// Publish our Id in the repository
void publish_peerid() {
	MeasurementRecord mr;
	memset(&mr, 0, sizeof(mr));
	mr.originator = mr.targetA = mr.targetB = peer->LocalID;
	mr.string_value = NULL;
	mr.published_name = SOCKETID_PUBLISH_NAME;
	mr.value = peer->PeerIDvalue;
	mr.channel = peer->channel;
	gettimeofday(&(mr.timestamp), NULL);

	repPublish(peer->repository, NULL, NULL, &mr);
}

// Publish our connection request in the repository (for the remote peer to see)
void publish_connection_req() {
	MeasurementRecord mr;
	memset(&mr, 0, sizeof(mr));
	mr.originator = mr.targetA = peer->LocalID;
	mr.targetB = remsocketID_string;
	mr.string_value = NULL;
	mr.published_name = "ConnReq";
	mr.value = 0.0;
	mr.channel = peer->channel;

	repPublish(peer->repository, NULL, NULL, &mr);
}

// Choose a random remote peer from the neighbors list to whom requests will be sent to
void choose_remote_peer() {
	// At least two peers need to be present in the repository to be able to make a connection
	if (peer->neighborlist_size > 1) {
		int random_integer = 0;
		srand((unsigned)time(0));
		random_integer = rand() % peer->neighborlist_size;

		if (strcmp(remsocketID_string, peer->neighbors[random_integer].peer) != 0) {

			// Check whether or not self is selected
			if (strcmp(peer->neighbors[random_integer].peer, peer->LocalID) != 0) {
				info("SocketID[%d] is chosen as the remote peer: %s", random_integer, peer->neighbors[random_integer].peer);
				if (active == 0) {
					// Initialize the active connection to the chosen remote peer
					active = 1;
					remsocketID = malloc(SOCKETID_SIZE);
					mlStringToSocketID(peer->neighbors[random_integer].peer, remsocketID);
					mlSocketIDToString(remsocketID, remsocketID_string, sizeof(remsocketID_string));
					info("Opening initial connection to %s ...", remsocketID_string);
					publish_connection_req();
					mlOpenConnection(remsocketID, &open_conn_cb_active, NULL, sParams);
				}
				else {
					// Modify the active connection to the newly chosen remote peer
					info("Holding current connection to %s [ConnectionID: %d]...", remsocketID_string,currentConnectionID);
					remsocketID = NULL;
					remsocketID = malloc(SOCKETID_SIZE);
					mlStringToSocketID(peer->neighbors[random_integer].peer, remsocketID);
					mlSocketIDToString(remsocketID, remsocketID_string, sizeof(remsocketID_string));
					publish_connection_req();
					mlOpenConnection(remsocketID, &open_conn_cb_active, NULL, sParams);
				}
			}
			else info("Skip modification of remote peer for now. (Self invalid)");
		}
		else info("Skip modification of remote peer for now. (Same selected)");
	}
}

// Called once the connection has been established
void open_conn_cb_active(int connectionID, void *arg) {
	struct timeval t = { 0, 0 };
	char measurename[4096];
	currentConnectionID = connectionID;

	int *con_id = malloc(sizeof(int));
	*con_id = connectionID;

	info("Opened connection to %s with ID %d.", remsocketID_string, connectionID);
	info("Start sending requests...");

	int ret;
	MonHandler mh;
	enum stat_types st[] = { AVG };
	/*
	mh = monCreateMeasure(HOPCOUNT, TXRX | PACKET | IN_BAND);
	sprintf(measurename,"HopCount_peerId-%d_connId-%d", peer->PeerIDvalue, connectionID);
	monPublishStatisticalType(mh, measurename, st, sizeof(st)/sizeof(enum stat_types), peer->repository);
	ret = monActivateMeasure(mh, remsocketID, 3);

	mh = monCreateMeasure(CLOCKDRIFT, TXRX | DATA | IN_BAND);
	sprintf(measurename,"ClockDrift_peerId-%d_connId-%d", peer->PeerIDvalue, connectionID);
	monSetParameter (mh, P_CLOCKDRIFT_ALGORITHM, 1);
	monPublishStatisticalType(mh, measurename, st, sizeof(st)/sizeof(enum stat_types), peer->repository);
	ret = monActivateMeasure(mh, remsocketID, 12);

	mh = monCreateMeasure(CORRECTED_DELAY, TXRX | DATA | IN_BAND);
	sprintf(measurename,"CorrectedDelay_peerId-%d_connId-%d", peer->PeerIDvalue, connectionID);
	monPublishStatisticalType(mh, measurename, st, sizeof(st)/sizeof(enum stat_types), peer->repository);
	ret = monActivateMeasure(mh, remsocketID, 12);

	mh = monCreateMeasure(CAPACITY_CAPPROBE, TXRX | DATA | OUT_OF_BAND);
	sprintf(measurename,"Capacity_peerId-%d_connId-%d", peer->PeerIDvalue, connectionID);
	monSetParameter (mh, P_CAPPROBE_DELAY_TH, 100);
	monSetParameter (mh, P_CAPPROBE_PKT_TH, 100);
	monSetParameter (mh, P_CAPPROBE_IPD_TH, 60);
	monPublishStatisticalType(mh, measurename, st, sizeof(st)/sizeof(enum stat_types), peer->repository);
	ret = monActivateMeasure(mh, remsocketID, 12);

	mh = monCreateMeasure(AVAILABLE_BW_FORECASTER, TXRX | DATA | OUT_OF_BAND);
	sprintf(measurename,"AvailBwForecast_peerId-%d_connId-%d", peer->PeerIDvalue, connectionID);
	monSetParameter (mh, P_FORCASTER_PKT_TH, 500);
	monSetParameter (mh, P_FORCASTER_DELAY_TH, 100);
	monPublishStatisticalType(mh, measurename, st, sizeof(st)/sizeof(enum stat_types), peer->repository);
	ret = monActivateMeasure(mh, remsocketID, 12);

	mh = monCreateMeasure(LOSS_BURST, TXRX | DATA | IN_BAND);
	sprintf(measurename,"LossBurst_peerId-%d_connId-%d", peer->PeerIDvalue, connectionID);
	monPublishStatisticalType(mh, measurename, st, sizeof(st)/sizeof(enum stat_types), peer->repository);
	ret = monActivateMeasure(mh, remsocketID, 12);

	mh = monCreateMeasure(RTT, TXRX | DATA | IN_BAND);
	sprintf(measurename,"RoundTripTime_peerId-%d_connId-%d", peer->PeerIDvalue, connectionID);
	monPublishStatisticalType(mh, measurename, st, sizeof(st)/sizeof(enum stat_types), peer->repository);
	ret = monActivateMeasure(mh, remsocketID, 12);
	*/

	if (dummymode) event_base_once(eventbase, -1, EV_TIMEOUT, &send_data_cb, con_id , &t);
}

// Event to send periodically dummy-traffic after the connection has been established  - DummyMode ONLY
void send_data_cb(int fd, short event, void *arg) {
	int *con_id = arg;
	struct timeval t = { 1, 0 };

	if (*con_id == currentConnectionID) {
		debug("Sending request on ConnectionID: %d", *con_id);
		mlSendData(*con_id, request, strlen(request) + 1, 3, NULL);
		// Reschedule
		event_base_once(eventbase, -1, EV_TIMEOUT, &send_data_cb, arg, &t);
	}
	else {
		info("End sending requests to the previous remote peer on ConnectionID: %d.", *con_id);
	}
}

// Transfer received messages to the given output UDP stream
void rx_data_stream_cb(char *buffer, int buflen, MsgType mt, recv_params *rparam) {
	chunkcounter++;
	char str[SOCKETID_STRING_SIZE];

	mlSocketIDToString(rparam->remote_socketID, str, sizeof(str));
	//debug("Received message [%d] from %s on MsgType %d.", chunkcounter, str ,mt);
	info("Received message [%d] from %s on MsgType %d.", chunkcounter, str ,mt);

	int pkts = buflen / playout.palyout_packet_size;
	if (pkts * playout.palyout_packet_size != buflen) {
		warn("Damaged chunk! (%d Packets, %d Buffersize)", pkts, buflen);
		//return;
	}

	struct recievedchunk *rc = malloc(sizeof(struct recievedchunk));
	rc->buffer = buffer;
	rc->currentpkt = 0;
	rc->pkts = pkts;
	rc->chunkid = chunkcounter;

	int diff = chunkcounter-chunktoken;
	if (diff <= playout.cycle_buffer) { // still space available for the next chunk
		struct timeval t = { 0, 0 };
		event_base_once(eventbase, -1, EV_TIMEOUT, &periodic_buffer_playout, rc, &t);
	}
	else{ // we drop the chunks (buffer overflow)
		warn("Buffer overflow (chunk %d dropped)!", chunkcounter);
		chunkcounter--;
	}
}

// Transfer received packets to the given output UDP stream with periodic callbacks
void periodic_buffer_playout(int fd, short event, void *arg) {
	struct recievedchunk *rc = arg;
	if (rc->chunkid > chunktoken) { // waiting for token
		struct timeval t = { 0, 0 };
		double period = (1.0/(double)playout.playout_frequency)*1000000;// period time in microseconds
		t.tv_usec = (int)period;
		event_base_once(eventbase, -1, EV_TIMEOUT, &periodic_buffer_playout, rc, &t);
	}
	else if (rc->chunkid == chunktoken) { // playout the buffer
		if (rc->currentpkt != rc->pkts) {
			struct timeval t = { 0, 0 };
			double period = (1.0/(double)playout.playout_frequency)*1000000;// period time in microseconds
			t.tv_usec = (int)period;
			sendto(playout.udpSocket, rc->buffer + rc->currentpkt * playout.palyout_packet_size, playout.palyout_packet_size, 0,
					(struct sockaddr *)&(playout.dst), sizeof(struct sockaddr_in));
			rc->currentpkt++;
			event_base_once(eventbase, -1, EV_TIMEOUT, &periodic_buffer_playout, rc, &t);
		}
		else {
			chunktoken++;
			debug("Next chunkID in the buffer: %d.", chunktoken);
		}
	}
}

// Open connection and send replies to active peer on dummy-requests - DummyMode ONLY
void rx_data_cb(char *buffer, int buflen, MsgType mt, recv_params *rparam) {
	char str[SOCKETID_STRING_SIZE];

	mlSocketIDToString(rparam->remote_socketID, str, sizeof(str));
	//debug("Received message from %s with MsgType %d: %s", str, mt, buffer+1);
	info("Received message from %s with MsgType %d: %s", str, mt, buffer+1);

	if(buffer[0] == 'A') { //First character defines Request
		int c_id = mlConnectionExist(rparam->remote_socketID, false);
		if(c_id >= 0) {
			if(mlGetConnectionStatus(c_id))
				send_reply(c_id, rparam->remote_socketID);
		}
		else {
			int passive_conn_id = -1;
			socketID_handle sock = malloc(SOCKETID_SIZE);
			memcpy(sock, rparam->remote_socketID, SOCKETID_SIZE);
			publish_connection_req();
			passive_conn_id = mlOpenConnection(rparam->remote_socketID, &open_con_cb_passive, sock, sParams);
			debug("Passive connection initialized with connection ID: %d", passive_conn_id);
		}
	}
}

// Open connection for a reply (if connection not open yet) - DummyMode ONLY
void open_con_cb_passive(int c_id, void *arg) {
	char str[SOCKETID_STRING_SIZE];

	mlSocketIDToString(arg, str, sizeof(str));
	debug("Opened passive connection with %s", str);
	send_reply(c_id, arg);
}

// Reply sending - DummyMode ONLY
void send_reply(int c_id, socketID_handle sock) {
	char str[SOCKETID_STRING_SIZE];
	MsgType mt = 3;

	mlSocketIDToString(sock, str, sizeof(str));
	debug("Sending reply to %s msg_type: %d", str, mt);
	mlSendData(c_id, reply, strlen(reply) + 1, 3, NULL);
}

// Called if the connection opening fails
void conn_fail_cb(int connectionID, void *arg) {
	error("Connection could not be established!\n");
}

// Called after the ML finished initializing. Used to open the initial connection
void receive_local_socketID_cb(socketID_handle local_socketID, int status) {
	if(status ) {
		info("Still trying to do NAT traversal...");
		return;
	}

	info("NAT traversal completed");

	// Store the local SocketID and it's string counterpart
	peer->LocalSocketID = local_socketID;
	mlSocketIDToString(local_socketID, peer->LocalID, sizeof(peer->LocalID));

	if (status == 0) messaging_ready = 1;

	info("The local SocketId is: %s", peer->LocalID);

	// Publish our Id in the repository from data in the config file
	publish_peerid();

	// Get the initial peer list
	Constraint cons;
	cons.published_name = SOCKETID_PUBLISH_NAME;
	cons.strValue = NULL;
	cons.minValue = SOCKETID_PUBLISH_VALUE_MIN;
	cons.maxValue = SOCKETID_PUBLISH_VALUE_MAX;

	repGetPeers(peer->repository, get_peers_cb, NULL, peer->nblist_desired_size, &cons, 1, NULL, 0, peer->channel);

	info("Waiting for incoming requests...");
}

// Print the list of active peers in the repository
void get_peers_cb(HANDLE client, HANDLE id, void *cbarg, char **result, int n) {
	info("GetPeers done: %d Peers initially in the repository. List:\n%s", n, print_list(result, n, 1));
}

// Setup the localhost destination where chunks are transfered to
int init_dest_ul(const char *stream, int packet_size, int frequency, int read_size, int cycle_buffer) {
	char addr[128];
	int port;

	if (sscanf(stream, "udp://%127[^:]:%i", addr, &port) != 2) {
		fatal("Unable to parse destination specification %s, set to localhost.", stream);
	}

	playout.udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
	playout.dst.sin_family = AF_INET;
	playout.dst.sin_addr.s_addr = inet_addr(addr);
	playout.dst.sin_port = htons(port);
	playout.palyout_packet_size = packet_size;
	playout.playout_frequency = frequency;
	playout.read_packet_size = read_size;
	playout.cycle_buffer = cycle_buffer;
}

// Setup the localhost source where data is received from
int init_source_ul(const char *stream, double chunk_duration) {
	char addr[128];
	int port;

	if (sscanf(stream, "udp://%127[^:]:%i", addr, &port) != 2) {
		fatal("Unable to parse source specification %s, set to localhost.", stream);
	}

	struct sockaddr_in udpsrc;
	int returnStatus = 0;
	evutil_socket_t udpSocket;

	udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

	evutil_make_socket_nonblocking(udpSocket);

	udpsrc.sin_family = AF_INET;
	udpsrc.sin_addr.s_addr = inet_addr(addr);
	udpsrc.sin_port = htons(port);

	int status = bind(udpSocket,(struct sockaddr *)&udpsrc, sizeof(udpsrc));
	if (status) {
		fatal("Unable to bind socket to udp://%s:%d", addr, port);
	}

	struct source_stream *src = malloc(sizeof(struct source_stream));
	src->read_size = playout.read_packet_size;
	src->read_buffer = malloc(src->read_size);

	src->bev = (struct bufferevent *)bufferevent_socket_new((struct event_base *)eventbase, udpSocket, 0);
	bufferevent_enable(src->bev, EV_READ);
	src->chunk_duration = chunk_duration;
	src->chunkid = 0;

	napaSchedulePeriodic(NULL, 1.0/chunk_duration, read_stream, src);
}

// Get the list of requested connections' list from the repository
void check_conn_reqs() {
	info("Checking connection requests by others...");
	repGetMeasurements(peer->repository, check_conn_reqs_cb, NULL, peer->nblist_desired_size, NULL, NULL, peer->LocalID, "ConnReq", peer->channel);
}

// Open connection if someone is trying to make a connection to us
void check_conn_reqs_cb(HANDLE client, HANDLE id, void *cbarg, MeasurementRecord *result, int nResults) {
	info("%d connections waiting for reply...", nResults);
	int i;
	for (i = 0; i != nResults; i++) {
		static char requestee[SOCKETID_STRING_SIZE];
		strcpy(requestee,result[i].originator);
		socketID_handle requesteeID = malloc(SOCKETID_SIZE);

		mlStringToSocketID(requestee, requesteeID);
		mlOpenConnection(requesteeID, &open_conn_cb_reply, NULL, sParams);
	}
	free(result);
}

// Called once the connection has been established for a request
void open_conn_cb_reply(int connectionID, void *arg) {
	info("Opened connection (reply) for a request with ID %d.", connectionID);
}

// Read out the buffer from the stream source and send chunks to the selected remote peer
void read_stream(HANDLE h, void *arg) {
	struct source_stream *src = (struct source_stream *)arg;
	int ptr = 0;
	src->read_size = playout.read_packet_size;
	src->read_buffer = malloc(src->read_size);
	int numofpackets = 0;

	if (currentConnectionID > -1) {
		size_t rd = playout.read_packet_size;
		while (rd == playout.read_packet_size) {
			numofpackets++;
			rd = bufferevent_read(src->bev, src->read_buffer + ptr, src->read_size - ptr);
			if (rd == (src->read_size - ptr)) { /* Need to grow buffer */
				src->read_size += playout.read_packet_size;
				src->read_buffer = realloc(src->read_buffer, src->read_size);
				ptr += rd;
			}
		}
		if (ptr+rd != 0) {
			//debug("Socket info: New chunk(%d): %d bytes read (%d packets)", src->chunkid++, ptr+rd, numofpackets);
			info("Socket info: New chunk(%d): %d bytes read (%d packets)", src->chunkid++, ptr+rd, numofpackets);
			mlSendData(currentConnectionID, src->read_buffer, src->read_size, 3, NULL);
		}
		else debug("Socket info: No new chunk read from input stream.");
	}
	else debug("Socket info: No active connection yet, reading skipped.");
}

// main function
int main(int argc, char *argv[]) {
	if ((argc < 2) || (argc > 3)) {
		fprintf(stderr, "ERROR - Correct Usage: ./nvtest2 peer.cfg [-dummy]\n");
		exit(-1);
	}

	if (argc > 2) {
		fprintf(stdout, "INFO - DummyMode active - no real traffic will be generated.\n");
		dummymode = 1;
	}

	// Initialize the NAPA infrastructure
	peer = peer_init(argv[1]);

	// Start everything
	event_base_dispatch(eventbase);
	free(remsocketID);
}
