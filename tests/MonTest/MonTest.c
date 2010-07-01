/***************************************************************************
 *   Copyright (C) 2009 by Robert Birke   *
 *   robert.birke@polito.it   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#include <event2/event.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#include "ml.h"
#include "mon.h"
#include "grapes_log.h"
#include "grapes.h"
#include "repoclient.h"

int active = 0; // active or passive?

#define SOCKETID_PUBLISH_NAME "SocketId"
#define SOCKETID_PUBLISH_VALUE 0
char request[] = "APingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPingPing";

char reply[] = "BPong";
//Note first characters identifies Request form Replies and must be maintaned (it is not printed out)

HANDLE repoclient;

socketID_handle rem_peer = NULL; // the remote SocketId

/** Helper to print a string list */
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

void get_peers_cb(HANDLE client, HANDLE id, void *cbarg, char **result, int n) {
	info("GetPeers done: %d Peers. List:\n%s", n, print_list(result, n, 1));
}

/* Event to send periodically traffic after the connection has been established */
void send_data_cb(int fd, short event,void *arg){
	int *con_id = arg;
	struct timeval t = {0,10000};

	mlSendData(*con_id, request, strlen(request) + 1, 3, NULL);

	//reschedule
	event_base_once(eventbase, -1, EV_TIMEOUT, &send_data_cb, arg, &t);
}

/* Called once the connection has been established  */
void open_conn_cb_active (int connectionID, void *arg){
	struct timeval t = {0,0};
	char str[SOCKETID_STRING_SIZE];
	
	int *con_id = malloc(sizeof(int));
	*con_id = connectionID;

	mlSocketIDToString(rem_peer,str, sizeof(str));
	info("Opened connection to %s.",str);
	info("Start sending requests");

	// peer 1 sends data to peer 2
	if(active) {
		int ret;
		MonHandler mh;
		enum stat_types st[] = {AVG};
		enum stat_types st2[] = {SUM};

// 		/* HopCount */
// 		mh = monCreateMeasure(HOPCOUNT, TXRXUNI | PACKET | IN_BAND);
// 		monPublishStatisticalType(mh, NULL, NULL, st, sizeof(st)/sizeof(enum stat_types), repoclient);
// 		ret = monActivateMeasure(mh, rem_peer,0);
// 
// 		/* RX Bytes */
// 		mh = monCreateMeasure(BYTE, RXONLY | PACKET | IN_BAND);
// 		monPublishStatisticalType(mh, NULL, NULL, st2, sizeof(st2)/sizeof(enum stat_types), repoclient);
// 		ret = monActivateMeasure(mh, rem_peer,0);
// 
// 		/* RX Pkts */
// 		mh = monCreateMeasure(COUNTER, RXONLY | PACKET | IN_BAND);
// 		monPublishStatisticalType(mh, NULL, NULL, st2, sizeof(st2)/sizeof(enum stat_types), repoclient);
// 		ret = monActivateMeasure(mh, NULL, 0);
// 
// 		/* Round Trip Time */
// 		mh = monCreateMeasure(RTT, TXRXBI | DATA | IN_BAND);
// 		monPublishStatisticalType(mh, NULL, st , sizeof(st)/sizeof(enum stat_types), repoclient);
// 		ret = monActivateMeasure(mh,rem_peer,3);
// 
// 		/* Clockdrift and capacity (Note: out of band measures: injects test traffic) */
		mh = monCreateMeasure(CLOCKDRIFT, TXRXUNI | PACKET | IN_BAND);
		monSetParameter (mh, P_CLOCKDRIFT_ALGORITHM, 1);
		ret = monActivateMeasure(mh,rem_peer, 3);
//

		mh = monCreateMeasure(CORRECTED_DELAY, TXRXUNI | PACKET | IN_BAND);
		ret = monActivateMeasure(mh,rem_peer, 3);

		mh = monCreateMeasure(CAPACITY_CAPPROBE, TXRXUNI | PACKET | IN_BAND);
		monSetParameter (mh, P_DEBUG_FILE, 1);
		monSetParameter (mh, P_CAPPROBE_DELAY_TH, -1);
// 		monSetParameter (mh, P_CAPPROBE_PKT_TH, 100);
// 		monSetParameter (mh, P_CAPPROBE_IPD_TH, 60);
// 		monPublishStatisticalType(mh, NULL, st , sizeof(st)/sizeof(enum stat_types), repoclient);
 		ret = monActivateMeasure(mh,rem_peer, 3);

		event_base_once(eventbase, -1, EV_TIMEOUT, &send_data_cb, con_id , &t);
	}
}

/* Called after the ML finished initialising. Used to open the initial connection*/
void receive_local_socketID_cb(socketID_handle local_socketID, int status){
	int res, conID;
	char str[SOCKETID_STRING_SIZE];
	
	if(status ) {
		info("Still trying to do NAT traversal");
		return;
	}

	info("Nat traversal completed");

	mlSocketIDToString(local_socketID,str, sizeof(str));
	info("My local SocketId is: %s",str);

	//publish our Id in the repository
	MeasurementRecord mr;
	mr.originator = mr.targetA = mr.targetB = str;
	mr.string_value = NULL;
	mr.channel = NULL;
	mr.published_name = SOCKETID_PUBLISH_NAME;
	mr.value = SOCKETID_PUBLISH_VALUE;
	gettimeofday(&(mr.timestamp), NULL);
	
	repPublish(repoclient, NULL, NULL, &mr);

	/* Get a peer list (not used as of now) */
	Constraint cons;
	cons.published_name = SOCKETID_PUBLISH_NAME;
	cons.strValue = NULL;
	cons.minValue = SOCKETID_PUBLISH_VALUE;
	cons.maxValue = SOCKETID_PUBLISH_VALUE;

	repGetPeers(repoclient, get_peers_cb, NULL, 10, &cons, 1, NULL, 0, NULL);

	send_params sParams;
	if(active) {
		char str[SOCKETID_STRING_SIZE];

		mlSocketIDToString(rem_peer,str, sizeof(str));
		info("We are active!");
		info("Open connection to %s ...", str);

		mlOpenConnection(rem_peer, &open_conn_cb_active, NULL, sParams);
	} else
		info("Waiting for incomig requests");
}

/* Called if the connection opening fails */
void conn_fail_cb(int connectionID, void *arg){
	error("Connection could not be established!\n");
}

void send_reply(int c_id, socketID_handle sock) {
	char str[SOCKETID_STRING_SIZE];
	MsgType mt = 3;

	mlSocketIDToString(sock,str, sizeof(str));
	info("Sending reply to %s msg_type: %d", str, mt);

	mlSendData(c_id, reply, strlen(reply) + 1, 3, NULL);
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


int main(int argc, char *argv[]) {
	char *repository = "repository.napa-wine.eu:9832";
	char *stun_server = "stun.ekiga.net";
	char *bind_ip = NULL;
	int verbosity = 100;

	if((argc % 2 != 1 && argc > 9) || (argc == 2 && (!strcmp(argv[1], "-h") || !strcmp(argv[1], "--help")))) {
		printf("Usage:\n\t%s [-b <bindIp>] [-a <SocketId>] [-r <repository:port>] [-s <stunserver>]\n", argv[0]);
		exit(1);
	}

	int i;
	for (i = 1; i < argc; i += 2) {
		/* Are we active or passive? */
		if (!strcmp("-a", argv[i])) {
			char str[SOCKETID_STRING_SIZE];
			active = 1;
			rem_peer = malloc(SOCKETID_SIZE);
			mlStringToSocketID(argv[i+1], rem_peer);

			mlSocketIDToString(rem_peer, str, sizeof(str));
			info("Remote SocketId: %s", str);
			printf("\n");
			}
		else if (!strcmp("-r", argv[i])) {
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
	}

	printf("Running conf:\nIP: %s\nSTUN: %s\nREPO: %s\nACTIVE: %s\nVERBOSITY: %d\n", bind_ip ? bind_ip:"auto", stun_server, repository, active ? "active" : "passive", verbosity);

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

	//register callback
	mlRegisterRecvDataCb(&rx_data_cb,3);

	// Init messaging layer
	mlRegisterErrorConnectionCb(&conn_fail_cb);
	

	struct timeval timeout = {3,0};
	mlInit(true, timeout, 9000+active, bind_ip, 3478, stun_server,
			&receive_local_socketID_cb, (void*)eventbase);
	//mlInit(true,timeout,9000+active,NULL,3478,NULL,&receive_local_socketID_cb,(void*)eventbase);


	//Start everything
	event_base_dispatch(eventbase);
	free(rem_peer);
}
