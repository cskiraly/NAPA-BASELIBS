#include <event2/event.h>
#include <unistd.h>
#include <string.h>
#include <math.h>

#include "ml.h"
#include "napa_log.h"
#include "napa.h"
#include "transmissionHandler.h"

int peerID;

int numOfBytes;

char local_id[SOCKETID_STRING_SIZE];

/**
  * define a maximum number of connections
  */
#define maxConnections 10000

/**
  * an connections array
  */
int connections[maxConnections];

/**
  * the initial number of connections
  */
int nr_connections = 0;

int connection_argument;

int timeInterval;
int numOfMsg;
int msgLen;

#define EXECMAX 400

void send_data_to_peer(int fd, short event,void *arg){
	
	static int execNum = 0;
	static char letter = 0x41;	
	int connectionID = 0;	
	
	if (execNum == EXECMAX) {
		//mlCloseConnection(connectionID);
		//info("[MLTest} - Connection closed.");	
		return;
	}

	info("%d.[MLTest} - Sending data to peer 2...", execNum);

	char* buffer;
	buffer = (char*) malloc(msgLen);
	unsigned char msgtype = 12;
	int i;

	for (i=0; i < numOfMsg; i++) {	
		memset(buffer,letter, msgLen);
		buffer[msgLen-1] = 0;
		mlSendData(connectionID,buffer,msgLen,msgtype,NULL);
		info("[MLTest} - Message '%c' has been sent.",letter);
		letter++;
		if (letter > 0x7d ) letter = 0x41;		
	}
	
      	free(buffer);

	info("[MLTest} - Sending next data in %d seconds...", timeInterval);
	struct timeval timeout_value = {timeInterval, 0};
	struct event *ev;
	ev = evtimer_new(eventbase,send_data_to_peer,NULL);
	event_add(ev,&timeout_value);

	execNum++;
}

void send_data_to_peer2(int fd, short event,void *arg){
      info("[MLTest} - Sending data to peer 2...");
      /// transmit data to peer 2  
      //int size = 30;
      char* buffer;
      buffer = (char*) malloc(numOfBytes);

      memset(buffer,'a',numOfBytes);
      buffer[numOfBytes-1] = 0;
      unsigned char msgtype = 12;
      int connectionID = 0;
      //strcpy(buffer, "Test message to peer 2");
//      mlSetThrottle(10, 1);
      mlSendData(connectionID,buffer,numOfBytes,msgtype,NULL);
      info("[MLTest} - Data has been sent.");
      mlCloseConnection(connectionID);
      info("[MLTest} - Connection closed.");
      free(buffer);
      //exit(0);
}


void receive_conn_cb (int connectionID, void *arg){

  info("[MLTest} - Received connection from callback: connectionID:%d", connectionID);

  connections[nr_connections] = connectionID;
  nr_connections++;

//  mlSetThrottle(8, 8);			//kB,kB/s

  if(peerID == 1){
	  info("[MLTest} - Sending data in 3 seconds...");
	  struct timeval timeout_value = {3,0};
	  struct event *ev;
	  ev = evtimer_new(eventbase,send_data_to_peer,NULL);
	  event_add(ev,&timeout_value);
  }

} 

socketID_handle getRemoteSocketID(int peerID){

  socketID_handle remote_socketID = malloc(SOCKETID_SIZE);
  int port;

  switch (peerID){

  case 1:
    port = 9001;
    break;

  case 2:
    port = 9002;
    break;
    
  case 3:
    port = 9003;
    break;

  }

  struct sockaddr_in udpaddr;
  udpaddr.sin_family = AF_INET;
  udpaddr.sin_addr.s_addr = inet_addr("127.0.0.1");
  udpaddr.sin_port = htons(port);
  
  socketaddrgen udpgen;
  udpgen.udpaddr = udpaddr;
  remote_socketID->internal_addr = udpgen;
  remote_socketID->external_addr = udpgen;

  return remote_socketID;

}

void open_connection_cb(int fd, short event,void *arg){

	send_params defaultSendParams;
	char remote_id[SOCKETID_STRING_SIZE];	
	
	socketID_handle rm_socketID = getRemoteSocketID(2);
	mlSocketIDToString(rm_socketID, remote_id, sizeof(remote_id));	
	
	info("[MLTest} - Openning a connection to %s", remote_id);	
	int con_id = mlOpenConnection(rm_socketID, &receive_conn_cb, &connection_argument, defaultSendParams); 
  	
	///If the result is zero the event is resheduled into the event list.
	if(con_id < 0){

		error("[MLTest} - Openning connection failed. Openning next connection in 4 seconds...");
		/* ReTry to open a connection after 4 seconds */
		struct timeval timeout_value_open_conn = {4,0};
		struct event *ev;
		ev = evtimer_new(eventbase,open_connection_cb,NULL);
		event_add(ev,&timeout_value_open_conn);

  	}

}


/* Called after the ML finished initialising */
void receive_local_socketID_cb(socketID_handle local_socketID, int status){
	int res, conID;
	char str[SOCKETID_STRING_SIZE];
	
	if(status) {
		info("[MLTest] - Still trying to do NAT traversal");
		return;
	}

	info("[MLTest] - Nat traversal completed");

	mlSocketIDToString(local_socketID, local_id, sizeof(str));
	info("[MLTest] - My local SocketId is: %s", local_id);

	
	if (peerID == 1) {
		/// Try to open a connection after 4 seconds
		info("[MLTest} - Openning connection in 4 seconds...");		
		struct timeval timeout_value_open_conn = {4,0};
		struct event *ev1;
		ev1 = evtimer_new(eventbase,open_connection_cb,NULL);
		event_add(ev1,&timeout_value_open_conn);	
	}
	
}

void recv_data_from_peer_cb(char *buffer,int buflen,unsigned char msgtype,void *arg){
  buffer[20] = 0;
  info("[MLTest] - Received data: %s\n",buffer);


}

/* Called if the connection opening fails */
void conn_fail_cb(int connectionID, void *arg){
	error("[MLTest] - Connection ID:%d could not be established!",connectionID);
}

int main(int argc, char *argv[]) {
	char *stun_server = "stun.rnktel.com";
	char *bind_ip = NULL;
	int verbosity = 100;

	if (argc < 2){

		printf("Usage: %s <peerID>\n", argv[0]);
		exit(1);

	}

	peerID = atoi(argv[1]);
	
	if (peerID < 1 || peerID > 2){

		printf("Usage: <peerID> has to be 1 or 2\n");
		exit(1);
	}
	
	int inputRate = 10 *1024;		//kB/s

	msgLen = 2500;
	timeInterval = 1;

	numOfMsg = inputRate / ((msgLen/1349 + 1)*23 + msgLen)*timeInterval;	

	//Init napa: log facility and libevent
	napaInit(event_base_new());

	// Initialize logging
	napaInitLog(verbosity, NULL, NULL);

	//initRateLimiter(eventbase);

	info("Running peer ID: %d", peerID);

	// Init messaging layer
	mlRegisterErrorConnectionCb(&conn_fail_cb);

	// Register a recv data callback  
	mlRegisterRecvDataCb(&recv_data_from_peer_cb,12);

	struct timeval timeout = {3,0};
	mlInit(true, timeout, 9000+atoi(argv[1]), bind_ip, 3478, stun_server,
			&receive_local_socketID_cb, (void*)eventbase);

	//Start everything
	event_base_dispatch(eventbase);
}
