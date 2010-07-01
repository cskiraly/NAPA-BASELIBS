/*
 * @note Policy Management
 *
 *          NEC Europe Ltd. PROPRIETARY INFORMATION
 *
 * This software is supplied under the terms of a license agreement
 * or nondisclosure agreement with NEC Europe Ltd. and may not be
 * copied or disclosed except in accordance with the terms of that
 * agreement.
 *
 *      Copyright (c) 2009 NEC Europe Ltd. All Rights Reserved.
 *
 * Authors: Kristian Beckers  <beckers@nw.neclab.eu>
 *    Sebastian Kiesel  <kiesel@nw.neclab.eu>
 *          
 *
 * NEC Europe Ltd. DISCLAIMS ALL WARRANTIES, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE AND THE WARRANTY AGAINST LATENT
 * DEFECTS, WITH RESPECT TO THE PROGRAM AND THE ACCOMPANYING
 * DOCUMENTATION.
 *
 * No Liability For Consequential Damages IN NO EVENT SHALL NEC Europe
 * Ltd., NEC Corporation OR ANY OF ITS SUBSIDIARIES BE LIABLE FOR ANY
 * DAMAGES WHATSOEVER (INCLUDING, WITHOUT LIMITATION, DAMAGES FOR LOSS
 * OF BUSINESS PROFITS, BUSINESS INTERRUPTION, LOSS OF INFORMATION, OR
 * OTHER PECUNIARY LOSS AND INDIRECT, CONSEQUENTIAL, INCIDENTAL,
 * ECONOMIC OR PUNITIVE DAMAGES) ARISING OUT OF THE USE OF OR INABILITY
 * TO USE THIS PROGRAM, EVEN IF NEC Europe Ltd. HAS BEEN ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES.
 *
 *     THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */

/**
 * @file echoServer.c
 * @brief A simple demo application for the messaging layer.   
 *
 * @author Kristian Beckers  <beckers@nw.neclab.eu>                             
 * @author Sebastian Kiesel  <kiesel@nw.neclab.eu> 
 *                                                                              
 * @date 7/28/2009 
 */


#include <fcntl.h>
#include <event2/event.h>
#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "ml.h"
#include "transmissionHandler.h"

/**
  * A pointer to a libevent 
  */
struct event_base *base;

/**
  * A peerID provided at application start
  */
int peerID;

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

/**
  * A local socketID handle 
  */
socketID_handle my_socketID;

/**
  * A remote socketID handle
  */
socketID_handle rm_socketID;

/**
 * An integer that is passed as an argument in open_connection
 */
int connection_argument;

/**
  * create a callback for the periodic printouts on the screen
  * @param fd A file descriptor
  * @param event A libevent event 
  * @param arg An arg pointer
  */
void print_peerID_cb(int fd, short event,void *arg){

  ///print the peerID
  printf("--echoServer: This peers ID is %d \n",peerID);
  ///Add a timeout event to the event list
  struct timeval timeout_value_print = {3,0};
  struct event *ev;
  ev = evtimer_new(base,print_peerID_cb,NULL);
  event_add(ev,&timeout_value_print);

}


/**
  * Send data to another peer. This function is implemented as timeout callback in the libevent
  * @param fd A file descriptor
  * @param event A libevent event 
  * @param arg An arg pointer
  */
void send_data_to_peer(int fd, short event,void *arg){

  printf("--echoServer: Send data to peer 2 \n");
  printf("               ++sending 10.000 'a'  \n");
      /// transmit data to peer 2  
      int size = 10000;
      char buffer[size];
      memset(buffer,'a',size);
      unsigned char msgtype = 12;
      int connectionID = 0;
      //strcpy(buffer, "Message for peer 2 \n");
  
      mlSendData(connectionID,buffer,size,msgtype,NULL);

}

/**
  * Send a file to a peer. This function is implemented as timeout callback in the libevent
  * @param fd A file descriptor
  * @param event A libevent event 
  * @param arg An arg pointer
  */

void send_file_to_peer(int fd, short event,void *arg){


  printf("--echoServer: Send file to peer 3 \n");

  char buf[20000];
  FILE *testfileread;
  testfileread  = fopen("testfile.txt","r");

  if (testfileread == NULL)
    {
      fprintf(stderr,"Could not open file for reading!\n");
    }

  /// open a file and write it into a buffer 
  fseek (testfileread , 0 , SEEK_END);
  long lSize = ftell (testfileread);
  rewind (testfileread);
  int result = fread(buf,1,lSize,testfileread);
  fclose(testfileread);

  int bufsize = sizeof(buf);
  unsigned char msgtype = 13;
  int connectionID = 0;

  fprintf(stderr,"The buffersize is %i\n",bufsize);
  if (result != lSize){ fprintf(stderr,"Could not read file!\n");  }
  else { printf("The size of the file is %ld \n",lSize); }
  
  mlSendData(connectionID,buf,(int)lSize,msgtype,NULL);

}

/**
  * A callback function that tells a connection has been established. 
  * @param connectionID The connection ID
  * @param *arg An argument for data about the connection 
  */
void receive_conn_cb (int connectionID, void *arg){

  printf("--echoServer: Received incoming connection connectionID %d  \n",connectionID );

  connections[nr_connections] = connectionID;
  nr_connections++;

  if(peerID == 1){

  struct timeval timeout_value = {5,0};
  struct event *ev;
  ev = evtimer_new(base,send_data_to_peer,NULL);
  event_add(ev,&timeout_value);
  
  }else 
    if(peerID == 3){

      /* send a file to peer 1 */
      struct timeval timeout_value_send_file = {12,0};
      struct event *ev3;
      ev3 = evtimer_new(base,send_file_to_peer,NULL);
      event_add(ev3,&timeout_value_send_file);
    
    }

} 


/**
  * A connection is poened callback
  * @param fd A file descriptor
  * @param event A libevent event 
  * @param arg An arg pointer
  */
void open_connection_cb(int fd, short event,void *arg){

  send_params defaultSendParams;
  int con_id = mlOpenConnection(rm_socketID,&receive_conn_cb,&connection_argument, defaultSendParams); ///< try to open a connection to another peer.
  ///If the result is zero the event is resheduled into the event list.
  if(con_id < 0){

    printf("open_connection failed : resheduling event\n");

    /* ReTry to open a connection after 3 seconds */
    struct timeval timeout_value_open_conn = {3,0};
    struct event *ev;
    ev = evtimer_new(base,open_connection_cb,NULL);
    event_add(ev,&timeout_value_open_conn);

  }

}

/**
  * The peer reveives data per polling. Implemented as a callback that is preiodically resheduled.
  * @param fd A file descriptor
  * @param event A libevent event 
  * @param arg An arg pointer
  */
void recv_data_from_peer_poll(int fd, short event,void *arg){

  printf("--echoServer: Polling for recv data \n" );
  
  char buffer[20000];
  int size = 0;
  //unsigned char msgtype = 12;
  //int connectionID = 0;
  int returnValue = 0;
  recv_params recvp;
  int i = 0;
  int connectionID;

  printf("--echoServer: before recv_Data \n");
  
  printf("nr_connections %d \n ",nr_connections );

  //iterate through all connections
  for(i=0; i <  nr_connections;i++){

    connectionID = connections[i];
    

  returnValue = mlRecvData(connectionID,buffer,&size,&recvp);

  //printf("--echoServer: after mlRecvData \n");

  
  if (returnValue == 1){

    //printf("--echoServer: after mlRecvData  \n");

    unsigned char msgtype  = recvp.msgtype;
    int connectionID = recvp.connectionID;

    printf("--echoServer: msgtype is %d \n",msgtype);
    printf("--echoServer: size is %d \n",size);
    printf("--echoServer: connectionID is %d \n",connectionID);

    if(msgtype == 12){
    
    char recv_buf[size];
    memcpy(recv_buf,buffer,size);

    printf("--echoServer: Received data from polling: \n\t %s \n\t buflen %d \n",recv_buf,size);
    
    // set the event again
    struct timeval timeout_value_recv = {10,0};
    struct event *ev;
    ev = evtimer_new(base,recv_data_from_peer_poll,NULL);
    event_add(ev,&timeout_value_recv);
    }else 
      if(msgtype == 13){

	printf("received testfile: msgtype 13 \n");

	//printf("%s \n" , buffer );
	/*
	FILE *testfile1 = NULL;
	
	printf("received testfile: msgtype 13 \n");

	testfile1 = fopen("recv.txt","r");

	printf("received testfile: msgtype 13 \n");

	if (testfile1 == NULL) {fprintf(stderr,"Could not open file for writing!\n"); }

	int cur_char;
	for(cur_char = 0; cur_char < size;++cur_char)
	  {
	    fputc(buffer[cur_char],testfile1);
	  }
	
	fclose(testfile1);
	
	printf(" testfile fully worked  \n ");
        */
      }
 
  }else{

    printf("--echoServer: polling ...no data arrived \n");
    // set the event again
  struct timeval timeout_value_recv = {8,0};
  struct event *ev;
  ev = evtimer_new(base,recv_data_from_peer_poll,NULL);
  event_add(ev,&timeout_value_recv);

    }
  }
}

/**
  * The peer receives data per callback from the messaging layer. 
  * @param *buffer A pointer to the buffer
  * @param buflen The length of the buffer
  * @param msgtype The message type 
  * @param *arg An argument that receives metadata about the received data
  */
void recv_data_from_peer_cb(char *buffer,int buflen,unsigned char msgtype,void *arg){

  printf("--echoServer: Received data from callback: \n\t %s \t msgtype %d \t buflen %d \n",buffer,msgtype,buflen ); 

  if(peerID == 2){

    printf("--echoServer: Send data to peer 1 \n");
    
    /* transmit data to peer 1  */

    int size = 10000;
    char buffer[size];
    memset(buffer,'a',size);
    int connectionID = 0;

    mlSendData(connectionID,buffer,size,msgtype,NULL);

  }
  
}

/**
  * A funtion that prints a connection establishment has failed
  * @param connectionID The connection ID
  * @param *arg An argument for data about the connection
  */
void conn_fail_cb(int connectionID,void *arg){

  printf("--echoServer: ConnectionID %d  could not be established \n ",connectionID);

}

/**
  * callback for a received packet notification from the messaging layer
  * @param *arg An argument for a recv packet infromation struct
  */
void received_a_packet_monitoring_cb(void *arg){

  printf("--Monitoring module: received a packet notification \n");
  
}

/**
 * A funtion that is a callback for the messaging layer when a new socketId is ready to be used
 * SocketIDhandle is a handle for the local socketID
 * Errorstatus is set to -1 if an socket error occurred and the socket is not usable and 1 if the NAT traversal failed and 0 if no error occurred
*/
void receive_local_socketID_cb(socketID_handle local_socketID,int errorstatus){

  printf("--Monitoring module: received a packet notification \n");

}

/**
  * callback before a message is send: set the monitoring module packet header 
  * @param *arg An pointer to an packet inf struct
  * @param send_pkt_inf A send_pkt_type_and_dst_inf struct that holds information about the data destination
  * @return 1 because a monitoring data header is set 
  */
int set_monitoring_module_packet_header(void *arg,send_pkt_type_and_dst_inf *send_pkt_inf){

  printf("--Monitoring module: set monitoring module packet header \n");

  char monitoring_module_packet_header[16];
  memset(monitoring_module_packet_header,'1',16);

  arg = (void *)monitoring_module_packet_header;

  return 1;

}


/**
  * callback when data is send 
  * @param *arg An pointer to an packet inf struct 
  */
void received_data_monitoring_cb(void *arg){

  printf("--Monitoring module: received data notification \n");

}

/**
  * callback when data is send
  * @param *arg A pointer to a monitoring data information struct
  */
void send_data_monitoring_cb(void *arg){

  printf("--Monitoring module: send data notification  \n");

}

/**
  * callback before data is send: monitoring module data header
  * @param arg A monitoring data information struct
  * @param send_pkt_inf A send_pkt_type_and_dst_inf struct that holds information about the data destination
  * @return returns 0 for no header set and 1 for header set
  */
int set_monitoring_module_data_header(void *arg, send_pkt_type_and_dst_inf *send_pkt_inf){

  printf("--Monitoring module: set monitoring module data header \n");

  char monitoring_module_data_header[16];
  memset(monitoring_module_data_header,'1',16);

  arg = (void *)monitoring_module_data_header;
  return 1;
}

/**
  * This is just a dummy funtion that creates a socketID
  * @param peerID
  * @return A socketID handle
  */
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
  udpaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  udpaddr.sin_port = htons(port);
  
  socketaddrgen udpgen;
  udpgen.udpaddr = udpaddr;
  remote_socketID->internal_addr = udpgen;
  remote_socketID->external_addr = udpgen;

  return remote_socketID;

}


/**
  * The main function of the echo server demo application for the messaging layer. 
  * @param argc The argument count
  * @param *argv[] The demo application needs a peer ID as argument. In the current version the peerID has to be either 1,2,or 3
  * @return 1 if an error occurred 0 if everything went allright
  */
int main(int argc, char *argv[]){

 
  if (argc < 2){

    fprintf(stderr, "Usage: %s <peerID>\n", argv[0]);
    exit(1);

  }

  if (atoi(argv[1]) > 3 || atoi(argv[1]) < 1){

    fprintf(stderr, "Usage: <peerID> has to be 1,2 or 3\n");
    exit(1);
  }

  /// register the monitoring module callbacks 
  /// packet level callbacks 
  mlRegisterGetRecvPktInf(&received_a_packet_monitoring_cb);
  mlRegisterSetMonitoringHeaderPktCb(&set_monitoring_module_packet_header);
  /// data level callbacks 
  mlRegisterGetRecvDataInf(&received_data_monitoring_cb);
  mlRegisterGetSendDataInf(&send_data_monitoring_cb);
  mlRegisterSetMonitoringHeaderDataCb(&set_monitoring_module_data_header);

  /// register transmission callbacks
  mlRegisterRecvConnectionCb(&receive_conn_cb);
  mlRegisterErrorConnectionCb(&conn_fail_cb);

  /// register a stun server on localhost with port 3478 
  //mlSetStunServer(3478,NULL);

  //int peerID;
  peerID = atoi(argv[1]);

  /* initiate a libevent instance  */
  //event_init();

  /// initiate a libevent vers.2 
  //struct event_base *base;
  base = event_base_new();
  
  /// set a recv timeout value of 2 seconds and 5 miliseconds
  struct timeval timeout_value;
  timeout_value.tv_sec = 3;
  timeout_value.tv_usec = 5;
  //set_Recv_Timeout(timeout_value);

  /// create an event that prints the peerID on the screen periodically 
  struct timeval timeout_value_print = {8,0};
  struct event *ev;
  ev = evtimer_new(base,print_peerID_cb,NULL);
  event_add(ev,&timeout_value_print);

  switch(peerID){

    case 1:

      printf("--echoServer: This is peer 1 \n ");

      /// call the init method and set the recv_data method to polling 
      boolean recv_data_cb1 = false;
      mlInit(recv_data_cb1,timeout_value,9001,NULL,3478,NULL,&receive_local_socketID_cb,(void*)base);

      /// create a remote socketID --> implemented as a hack right now. In the final application a more clean solution of how to get the nodeIDs is required
      rm_socketID = getRemoteSocketID(2);

      /// poll the messaging layer every 2 seconds if data has arrived. Start after 5 seconds.
      struct timeval timeout_value_recv = {5,0};
      struct event *ev;    
      ev = evtimer_new(base,recv_data_from_peer_poll,NULL);
      event_add(ev,&timeout_value_recv);      

      /// Try to open a connection after 4 seconds
      struct timeval timeout_value_open_conn = {4,0};
      struct event *ev1;
      ev1 = evtimer_new(base,open_connection_cb,NULL);
      event_add(ev1,&timeout_value_open_conn);

    break;

    case 2: 

      printf("--echoServer: This is peer 2 \n ");

      /// call the init method and set the recv_data method to callback 
      boolean recv_data_cb2 = true;
      mlInit(recv_data_cb2,timeout_value,9002,NULL,3478,NULL,&receive_local_socketID_cb,(void*)base);

      /// register a recv data callback for the  
      mlRegisterRecvDataCb(&recv_data_from_peer_cb,12);

    break; 


    case 3:
      
      printf("--echoServer: This is peer 3 \n ");

      /// call the init method and set the recv_data method to callback
      boolean recv_data_cb3 = false;
      mlInit(recv_data_cb3,timeout_value,9003,NULL,3478,NULL,&receive_local_socketID_cb,(void*)base);

      /// create a remote socketID --> implemented as a hack right now. In the final application a more clean solution of how to get the nodeIDs is required
      rm_socketID = getRemoteSocketID(1);

      /// Try to open a connection after 4 seconds 
      struct timeval timeout_value_open_conn_1 = {6,0};
      struct event *ev3;
      ev3 = evtimer_new(base,open_connection_cb,NULL);
      event_add(ev3,&timeout_value_open_conn_1);

    break;
    
  }

  printf("event base dispatch \n");
  event_base_dispatch(base);

  return 0;

}
