#include <event2/event.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#include "ml.h"
#include "mon.h"
#include "grapes_log.h"
#include "grapes.h"
#include "repoclient.h"

#define SOCKETID_PUBLISH_NAME "SocketId"
#define SOCKETID_PUBLISH_VALUE 0

#define MSG_TYPE 3

char request[] = "APing";
char reply[] = "BPong";
//Note first characters identifies Request form Replies and must be maintaned (it is not printed out)

struct peer_info {
	MonHandler m_list[10];
	int m_list_len;
	int con_id;
	socketID_handle rem_peer;
	struct peer_info *next;
	struct peer_info *previous;
};

struct peer_info *peers_head = NULL;

char local_id[SOCKETID_STRING_SIZE];

HANDLE repoclient;

//check period in [s]
int cycle = 500;
char *channel = "MonTestDist";

double avg_pause = 10000000; // [us]

void gen_pause(struct timeval *t) {
	/*generate random ipg*/
	double p = (double) random() / (double) INT_MAX;
	p = -1 * avg_pause * log(1-p);
	t->tv_sec = (unsigned long)p / 1000000;
	t->tv_usec = (unsigned long)fmod(p, 100000.0);
}


/* Event to send periodically traffic after the connection has been established */
void send_data_cb(int fd, short event,void *arg){
	struct peer_info *peer = arg;
	struct timeval t;

	if(peer->con_id < 0) {
		if(peer->next != NULL)
			peer->next->previous = peer->previous;
		if(peer->previous != NULL)
			peer->previous->next = peer->next;
		free(peer);
		return;
	}

	mlSendData(peer->con_id, request, strlen(request) + 1, MSG_TYPE, NULL);

	gen_pause(&t);
	//reschedule
	event_base_once(eventbase, -1, EV_TIMEOUT, &send_data_cb, arg, &t);
}

/* Called once the connection has been established  */
void open_conn_cb_active (int connectionID, void *arg){
	char str[SOCKETID_STRING_SIZE];
	struct timeval t = {0,0};
	struct peer_info *peer = (struct peer_info *) arg;
	enum stat_types st[] = {AVG, MIN, MAX};
	int i;

	peer->con_id = connectionID;

	mlSocketIDToString(peer->rem_peer,str, sizeof(str));
	info("Opened connection to %s. Starting measures ...",str);


	/* (Static) List of measures to perform */
	/* Notice: Capprobe generates 100 Kbit/s of traffic and Forecaster 1 Mbit/s */
	/*         In band measures are taken on the above generated traffic */

	/* HopCount */
	i=0;
 	peer->m_list[i] = monCreateMeasure(HOPCOUNT, TXRXUNI | PACKET | IN_BAND);
 	monPublishStatisticalType(peer->m_list[i], NULL, channel, st , sizeof(st)/sizeof(enum stat_types), repoclient);
 	monSetParameter (peer->m_list[i], P_PUBLISHING_RATE, 60);
 	monActivateMeasure(peer->m_list[i], peer->rem_peer, MSG_TYPE);
	i++;

// 	/* Clickdrift (not published) */
// 	peer->m_list[i] = monCreateMeasure(CLOCKDRIFT, TXRX | DATA | IN_BAND);
// 	monSetParameter (peer->m_list[i], P_CLOCKDRIFT_ALGORITHM, 1);
// 	monSetParameter (peer->m_list[i], P_PUBLISHING_RATE, 100);
// 	monActivateMeasure(peer->m_list[i], peer->rem_peer,12);
// 	i++;
//
// 	/* Capacity */
// 	peer->m_list[i] = monCreateMeasure(CAPACITY_CAPPROBE, TXRX | DATA | OUT_OF_BAND);
// 	monSetParameter (peer->m_list[i], P_CAPPROBE_DELAY_TH, 100);
// 	monSetParameter (peer->m_list[i], P_CAPPROBE_PKT_TH, 100);
// 	monSetParameter (peer->m_list[i], P_CAPPROBE_IPD_TH, 60);
// 	monPublishStatisticalType(peer->m_list[i], NULL, st , sizeof(st)/sizeof(enum stat_types), repoclient);
// 	monActivateMeasure(peer->m_list[i], peer->rem_peer, 12);
// 	i++;
// 
// 	/* Loss */
// 	peer->m_list[i] = monCreateMeasure(LOSS, TXRX | DATA | IN_BAND);
// 	monSetParameter (peer->m_list[i], P_WINDOW_SIZE, 100);
// 	monSetParameter (peer->m_list[i], P_PUBLISHING_RATE, 100);
// 	monPublishStatisticalType(peer->m_list[i], NULL, st , sizeof(st)/sizeof(enum stat_types), repoclient);
// 	monActivateMeasure(peer->m_list[i], peer->rem_peer, 12);
// 	i++;
// 
// 	/* Loss Burst */
// //	peer->m_list[i] = monCreateMeasure(LOSS_BURST, TXRX | DATA | IN_BAND);
// //	monSetParameter (peer->m_list[i], P_WINDOW_SIZE, 100);
// //	monSetParameter (peer->m_list[i], P_PUBLISHING_RATE, 100);
// //	monPublishStatisticalType(peer->m_list[i], NULL, st , sizeof(st)/sizeof(enum stat_types), repoclient);
// //	monActivateMeasure(peer->m_list[i], peer->rem_peer, 12);
// //	i++;
// 
// 	/* Round trip time */
// 	peer->m_list[i] = monCreateMeasure(RTT, TXRX | DATA | IN_BAND);
// 	monSetParameter (peer->m_list[i], P_PUBLISHING_RATE, 100);
// 	monPublishStatisticalType(peer->m_list[i], NULL, st , sizeof(st)/sizeof(enum stat_types), repoclient);
// 	monActivateMeasure(peer->m_list[i], peer->rem_peer, 12);
// 	i++;
// 
	/* Available Bandwith */
// 	peer->m_list[i] = monCreateMeasure(AVAILABLE_BW_FORECASTER, TXRX | DATA | OUT_OF_BAND);
// 	monSetParameter (peer->m_list[i], P_FORCASTER_PKT_TH, 500);
// 	monSetParameter (peer->m_list[i], P_FORCASTER_DELAY_TH, 100);
// 	monPublishStatisticalType(peer->m_list[i], NULL, st , sizeof(st)/sizeof(enum stat_types), repoclient);
// 	monActivateMeasure(peer->m_list[i], peer->rem_peer, 12);
// 	i++;

	peer->m_list_len = i;

	info("Started %d measures", peer->m_list_len);

	gen_pause(&t);

	/* Start sending traffic */
	event_base_once(eventbase, -1, EV_TIMEOUT, &send_data_cb, peer, &t);
}

/** Helper to print a string list */
const char *print_list(char **list, int n, bool should_free) {
	static char buffer[4096];
	int i;
	buffer[0] = '\0';
	for (i = 0; i < n; i++) {
		if (i) strcat(buffer, "\n");
		strcat(buffer, list[i]);
		if (should_free) free(list[i]);
	}
	if (should_free) free(list);
	return buffer;
}

void start_measures(char **peers ,int p_len, int should_free) {
	char str[SOCKETID_STRING_SIZE];
	int i;
	send_params sParams;
	struct peer_info *t;

	for(i=0; i < p_len; i++) {
		if(strcmp(local_id, peers[i]) == 0)
			continue;

		t = (struct peer_info *) malloc(sizeof(struct peer_info));
		memset(t,0,sizeof(struct peer_info));

		t->rem_peer = (socketID_handle) malloc(SOCKETID_SIZE);
		mlStringToSocketID(peers[i], t->rem_peer);

		if (should_free)
			free(peers[i]);

		mlSocketIDToString(t->rem_peer,str, sizeof(str));
		info("Opening connection to %s ...",str);
		
		mlOpenConnection(t->rem_peer, &open_conn_cb_active, t, sParams);

		t->previous = NULL;
		t->next = peers_head;
		if(peers_head != NULL)
			peers_head->previous = t;
		peers_head = t;
	}

	if (should_free)
		free(peers);
};

void stop_measures(void) {
	int i;
	struct peer_info *t;

	/* Stop any running measurments and free up memory */
	while(peers_head != NULL) {
		for(i=0; i < peers_head->m_list_len; i++) {
			monDestroyMeasure(peers_head->m_list[i]);
		}
		info("Stopped %d measures", peers_head->m_list_len);

		free(peers_head->rem_peer);

		mlCloseConnection(peers_head->con_id);
		peers_head->con_id = -1;

		peers_head = peers_head->next;
	}
};

/* Called upon receiving the answer from the repository */
void get_peers_cb(HANDLE client, HANDLE id, void *cbarg, char **result, int n) {
	//print out list
	info("GetPeers done: %d Peers. List:\n%s", n, print_list(result, n, 0));
	//start measures
	start_measures(result, n, 1);
}

/* Event to periodically check for available peers, stop old measurements and start new measurements*/
void check_for_new_peers(int fd, short event,void *arg){
	// Stop any running measurements
	stop_measures();

	// Get peer list
	Constraint cons;
	cons.published_name = SOCKETID_PUBLISH_NAME;
	cons.strValue = NULL;
	cons.minValue = SOCKETID_PUBLISH_VALUE;
	cons.maxValue = SOCKETID_PUBLISH_VALUE;

	repGetPeers(repoclient, get_peers_cb, NULL, 10, &cons, 1, NULL, 0, NULL);

	// reschedule next measurement session
	struct timeval tv = {cycle, 0};
	event_base_once(eventbase, -1, EV_TIMEOUT, &check_for_new_peers, NULL , &tv);
}

void send_reply(int c_id, socketID_handle sock) {
	char str[SOCKETID_STRING_SIZE];

	mlSocketIDToString(sock,str, sizeof(str));
	info("Sending reply to %s msg_type: %d", str, MSG_TYPE);

	mlSendData(c_id, reply, strlen(reply) + 1, MSG_TYPE, NULL);
}

void open_con_cb_passive(int c_id, void *arg) {
	char str[SOCKETID_STRING_SIZE];

	mlSocketIDToString(arg, str, sizeof(str));
	info("Opened connection with %s", str);

	send_reply(c_id, arg);

	free(arg);
}

/* pasive replies to active peer */
void rx_data_cb(char *buffer,int buflen, MsgType mt, recv_params *rparam){
	char str[SOCKETID_STRING_SIZE];

	mlSocketIDToString(rparam->remote_socketID,str, sizeof(str));
	info("Received message from %s on MsgType %d: %s",str ,mt, buffer+1);

	if(buffer[0] == 'A') { //Request?
		int c_id = mlConnectionExist(rparam->remote_socketID, false);
		if(c_id >= 0) {
			if(mlGetConnectionStatus(c_id))
				send_reply(c_id, rparam->remote_socketID);
		} else {
			socketID_handle sock = malloc(SOCKETID_SIZE);
			memcpy(sock, rparam->remote_socketID, SOCKETID_SIZE);
			send_params sParams;
			mlOpenConnection(rparam->remote_socketID, &open_con_cb_passive, sock, sParams);
			info("Waiting to open connection");
		}
	}
}


/* Called after the ML finished initialising */
void receive_local_socketID_cb(socketID_handle local_socketID, int status){
	int res, conID;
	char str[SOCKETID_STRING_SIZE];
	
	if(status) {
		info("Still trying to do NAT traversal");
		return;
	}

	info("Nat traversal completed");

	mlSocketIDToString(local_socketID, local_id, sizeof(str));
	warn("My local SocketId is: %s", local_id);

	//publish our Id in the repository
	MeasurementRecord mr;
	mr.originator = mr.targetA = mr.targetB = local_id;
	mr.string_value = NULL;
	mr.channel = NULL;
	mr.published_name = SOCKETID_PUBLISH_NAME;
	mr.value = SOCKETID_PUBLISH_VALUE;
	gettimeofday(&(mr.timestamp), NULL);
	
	repPublish(repoclient, NULL, NULL, &mr);

	// add check for new peers event to the loop
	struct timeval tv = {0 ,0};
	event_base_once(eventbase, -1, EV_TIMEOUT, &check_for_new_peers, NULL, &tv);
}

/* Called if the connection opening fails */
void conn_fail_cb(int connectionID, void *arg){
	error("Connection could not be established!\n");
}

int main(int argc, char *argv[]) {
	char *repository = "repository.napa-wine.eu:9832";
	char *stun_server = "stun.ekiga.net";
	char *bind_ip = NULL;
	int verbosity = 100;
	int port = 6666;

	if(argc % 2 != 1 && argc > 15) {
		printf("Usage:\n\t%s [-b <bindIp>] [-r <repository:port>] [-s <stunserver>] [-v <verbosity>] [-p <port>] [-C <cycle>] [-c <channel>]",argv[0]);
		exit(1);
	}

	int i;
	for (i = 1; i < argc; i += 2) {
		if (!strcmp("-r", argv[i])) {
			repository = argv[i+1];
		}
		else if (!strcmp("-s", argv[i])) {
			stun_server = argv[i+1];
		}
		else if (!strcmp("-b", argv[i])) {
			bind_ip = argv[i+1];
		}
		else if (!strcmp("-v", argv[i])) {
			verbosity = atoi(argv[i+1]);
		}
		else if (!strcmp("-p", argv[i])) {
			port = atoi(argv[i+1]);
		}
		else if (!strcmp("-C", argv[i])) {
			cycle = atoi(argv[i+1]);
		}
		else if (!strcmp("-c", argv[i])) {
			channel = argv[i+1];
		}
	}

	printf("Running conf:\nIP:\t\t%s\nSTUN:\t\t%s\nREPO:\t\t%s\nVERBOSITY:\t%d\nPORT:\t\t%d\nCYCLE:\t\t%d s\nCHANNEL:\t%s\n", bind_ip ? bind_ip:"auto", stun_server, repository, verbosity, port, cycle, channel);

	//Init grapes: log facility and libevent
	grapesInit(event_base_new());

	// Initialize logging
	grapesInitLog(verbosity, NULL, NULL);

	//Init monitoring layer
	monInit(eventbase, NULL);

	//Init repoclient
	repInit("");
	repoclient = repOpen(repository,0);
	if (repoclient == NULL) fatal("Unable to initialize repoclient");

	// Init messaging layer
	mlRegisterErrorConnectionCb(&conn_fail_cb);

	//register callback
	mlRegisterRecvDataCb(&rx_data_cb,MSG_TYPE);

	struct timeval timeout = {3,0};
	mlInit(true, timeout, port, bind_ip, 3478, stun_server,
			&receive_local_socketID_cb, (void*)eventbase);

	//Start everything
	event_base_dispatch(eventbase);
	stop_measures();
}
