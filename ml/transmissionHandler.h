/*
 *          Policy Management
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
 *          Sebastian Kiesel  <kiesel@nw.neclab.eu>
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
 * @file transmissionHandler.h
 * @brief The transmissionHandler takes care of connecting peers and exchanging data between them.
 *
 * @author Kristian Beckers  <beckers@nw.neclab.eu>                             
 * @author Sebastian Kiesel  <kiesel@nw.neclab.eu> 
 *
 *                                                                              
 * @date 7/28/2009 
 */

#ifndef TRANSMISSIONHANDLER_H
#define TRANSMISSIONHANDLER_H
#include <sys/time.h>
#ifndef WIN32
#include <netinet/in.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <sys/uio.h>
#endif
#include <fcntl.h>
#include <event2/event.h>
#include <sys/types.h>
#include "util/udpSocket.h"
#include "util/stun.h"
#include "ml.h"

#ifndef WIN32
#ifndef boolean
typedef bool boolean;
#endif
#endif

#ifndef TRUE
#define TRUE ((bool)1)
#endif
#ifndef FALSE
#define FALSE ((bool)0)
#endif

#ifdef RTX
/**
 * This is the maximum number of gaps RTX can keep track of inside one message
 */
#define RTX_MAX_GAPS 25

#define ML_NACK_MSG 128
#endif
/**
 * This is the maximum size of the monitoring module header that can be added to the messaging layer header
 */
#define MON_DATA_HEADER_SPACE 32
#define MON_PKT_HEADER_SPACE 32

/**
  * Define the size of the messaging layer header 
  */
#define MSGL_HEADER_SIZE 18

/**
  * Define a type that is a pointer to a libevent event 
  */
typedef struct event *sock_event_ptr;

/**
 * Defined a mtu size - IP size - UDP size
 */
typedef enum {MAX = 1472, DSL = 1464, DSLMEDIUM = 1422, DSLSLIM = 1372, BELOWDSL = 1172, MIN = 472, P_ERROR = -1} pmtu;

/**
 * Define connection command types
 */
typedef enum {INVITE = 0, CONNECT, READY} con_msg_types;

/**
 * Define receiver buffer status
 */

typedef enum {ACTIVE = 0, INACTIVE, COMPLETE} recvbuf_status_types;

/**
 * A union that considers IPv4 and IPv6 addresses 
 */
typedef union{

  struct sockaddr_in udpaddr; ///< ipv4 address
  struct sockaddr_in6 udpaddr6; ///< ipv6 address

} socketaddrgen;

/**
  the socketID contains the internal and the external IP+Port 
 */
typedef struct _socket_ID {

  socketaddrgen internal_addr; ///< internal address
  socketaddrgen external_addr; ///< external or reflexive address
} socket_ID;

#ifdef RTX
struct gap {
	int offsetFrom;
	int offsetTo;
};
#endif

/**
  * A struct that contains information about data that is being received
  */
typedef struct {

  int recvID; ///< the receive ID
  int connectionID; ///< The connectionID
  int seqnr; ///< The sequence number
  char *recvbuf; ///< A pointer to the receive buffer
  int bufsize; ///< The  buffersize
  int status; ///< The receive status. 1 if receiving finished
  char msgtype; ///< The message type
  char firstPacketArrived; ///< did the first packet arrive 
  int recvFragments; ///< the number of received framgents
  int arrivedBytes; ///< the number of received Bytes
  int monitoringDataHeaderLen; ///< size of the monitoring data header (0 == no header)
  struct event *timeout_event; ///< a timeout event
  struct timeval timeout_value; ///< the value for a libevent timeout
  time_t starttime; ///< the start time
#ifdef RTX
  struct event* last_pkt_timeout_event;
  int txConnectionID;
  int expectedOffset;
  int gapCounter; //index of the first "free slot"
  int firstGap;	//first gap which hasn't been handled yet (for which the NACK hasn't been sent yet)
  struct gap gapArray[RTX_MAX_GAPS];
#endif
} recvdata;

struct receive_connection_cb_list{
	struct receive_connection_cb_list *next;
	receive_connection_cb connection_cb;
	void *arg;
};

/**
 * A struct with information about a connection that exist or a connection that is being established
 */
typedef struct {

  int connectionID; ///< the connection ID
  socket_ID external_socketID; ///< the external socketID
  int external_connectionID; ///< the internal connectionID
  pmtu pmtusize; ///< the pmtu size
  bool delay;
  int trials;
  char *ctrl_msg_buf;
  int status; ///< the status of the connection. status has the following encoding: 0: INVITE send, 1: CONNECT send, 2: connection established
  time_t starttime; ///< the time when the first connection attempt was made
  uint32_t seqnr; ///< sequence number for connections that have been tried
  struct event *timeout_event;
  struct timeval timeout_value; ///< the timeout value for the connection establishment
  bool internal_connect; ///< set to true if a connection to the internal address could be established
  struct receive_connection_cb_list *connection_head;
  struct receive_connection_cb_list *connection_last;
  send_params defaultSendParams;
  uint32_t keepalive_seq; 
} connect_data;

#define ML_CON_MSG 127


/**
 * A struct with the messaging layer header for connection handling messages
 */
#pragma pack(push)  /* push current alignment to stack */
#pragma pack(1)     /* set alignment to 1 byte boundary */
struct conn_msg {
	uint32_t comand_type; ///< see con_msg_types
	int32_t pmtu_size;	/// the pmtu size 
	socket_ID sock_id;	/// the socketId of the sender
} __attribute__((packed));

#ifdef RTX
/************modifications-START************/

struct nack_msg {
	int32_t con_id;		///local connectionID of the transmitter
	int32_t msg_seq_num;
	uint32_t offsetFrom;
	uint32_t offsetTo;
} __attribute__((packed));

/************modifications-END**************/
#endif

struct msg_header {
	uint32_t offset;
	uint32_t msg_length;
	int32_t local_con_id;	///> the local connection id
	int32_t remote_con_id;	///> the remote connection id
	int32_t msg_seq_num;
	uint8_t msg_type; ///< set to ML_CON_MSG
	uint8_t len_mon_data_hdr;
	uint8_t len_mon_packet_hdr;
};
#define MSG_HEADER_SIZE (sizeof(struct msg_header))
#pragma pack(pop)   /* restore original alignment from stack */

int sendPacket(const int udpSocket, struct iovec *iov, int len, struct sockaddr_in *socketaddr);

//
///**
// * The init method needs to be called to initialise the transmissionHandler
// * @param recv_data_cb This is a boolean that decides how an upper layer receives data. When the boolean is set to true data is received via callback. When it is set to false the upper layer has to poll with the recv_data function.
// * @param timeout_value A timeout value for the data receival
// * @param port A port that will be used to create a new UDP socket
// * @param ipaddr An ipaddress that will be used to create a new UDP socket. If left NULL th
//e messaging layer binds it to localhost.
// * @param stun_port The port of the stun server.
// * @param stun_ipaddr The ip address of the stun server.
// * @param local_socketID_cb A callback from the type receive_localsocketID_cb that is envoke once the socketID is ready to use.
// * @param arg A void pointer that is used in this implementation to handover the libevent pointer.
// */
//void init_transmissionHandler(bool recv_data_cb,struct timeval timeout_value,const int port,const char *ipaddr,const int stun_port,const char *stun_ipaddr,receive_localsocketID_cb local_socketID_cb,void *arg);
//
///**
// * create a socket: returns a socketID handle
// * A socketID handle is a pointer to the opague data structre
// * @param port A port that will be used to create a new UDP socket
// * @param ipaddr An ipaddress that will be used to create a new UDP socket. If left NULL the messaging layer binds it to localhost.
// */
//void create_socket(const int port,const char *ipaddr);
//
///**
// * destroys a socket
// * @param socketID The socketID handle to the socket ID that shall be destroyed.
// */
//void close_socket(socketID_handle socketID);
//
///**
// * opens a connection between two socketIDs
// * @param local_socketID The local socketID
// * @param external_socketID The remote socketID
// * @param connectionID A pointer to an int. The connectionID will be stored in that int.
// * @param connection_cb  A function pointer to a callback function from the type receive_connection_cb
// * @param arg A void pointer that is returned via the callback.
// * @return returnValue the connectionId, < 0 for a failure.
// */
//int open_connection(socketID_handle external_socketID,receive_connection_cb connection_cb,void *arg);
//
///**
// * destroys a connections
// * @param connectionID
// */
//void close_connection(const int connectionID);
//
//
///**
//  * Send a NAT traversal keep alive
//  * @param connectionID
//  */
//void keep_connection_alive(const int connectionID);
//
///**
// * send all data
// * @param connectionID The connection the data should be send to.
// * @param container A container for several buffer pointer and length pairs from type send_all_data_container
// * @param nr_entries The number of buffer pointer and length pairs in the container. The maximum nr is 5.
// * @param msgtype The message type.
// * @param sParams A pointer to a send_params struct.
// * @return 0 if a problem occured, 1 if everything was alright.
// */
//int send_all_data(const int connectionID,send_all_data_container *container,int nr_entries,unsigned char msgtype,send_params *sParams);
//
///**
// * send data
// * @param connectionID The connection the data should be send to.
// * @param sendbuf A pointer to the send buffer.
// * @param bufsize The buffersize of the send buffer.
// * @param msgtype The message type.
// * @param sParams A pointer to a send_params struct.
// */
//void send_data(const int connectionID,char *sendbuf,int bufsize,int msgtype,send_params *sParams);
//
//void send_msg(int con_id, int msg_type, char* msg, int msg_len, bool truncable, send_params * sparam);
///**
// * receive data
// * @param connectionID The connection the data came from.
// * @param recvbuf A pointer to a buffer the data will be stored in.
// * @param bufsize A pointer to an int that tells the size of the received data.
// * @param rParams A pointer to a recv_params struct.
// */
//int recv_data(const int connectionID,char *recvbuf,int *bufsize,recv_params *rParams);
//
///**
// * set the stun server
// * @param port The port of the stun server.
// * @param ipaddr The ip address of the stun server.
// */
//void setStunServer(const int port,const char *ipaddr);
//
///**
// * unset the stun server
// */
//void unsetStunServer();
//
///**
// * checks whether a stun server was defined
// * @returns true if a stun serve rwas defined
// */
//bool isStunDefined();
//
///**
// * set recv timeout
// * @param timeout_value The timeout value for receiving data.
// */
//void setRecv1Timeout(struct timeval timeout_value);
//
///**
//  * create a message for the connection establishment
//  * @param *msgbuf A buffer for the message
//  * @param bufsize The buffersize
//  * @param local_connectionID The local connectionID
//  * @param local_socketID The local socketID
//  * @param command_type The command type
//  * @return -1 for failure otherwise the size of the allocated control message buffer
//  */
//
//int
//create_conn_msg(int pmtusize,int local_connectionID, socketID_handle local_socketID, con_msg_types command_type);
//
///**
//  * send a connect message to another peer
//  * @param connectionID the connectionID
//  * @param *msgbuf A pointer to the message buffer
//  * @param bufsize The buffersize
//  */
//void send_conn_msg(const int connectionID,int bufsize, int command);
//
///**
//  * process an incoming conenction message
//  * @param local_socketID
//  * @param bufsize
//  */
//void recv_conn_msg(struct msg_header *msg_h,char *msgbuf,int bufsize);
//
///**
//  * process an incoming message
//  * @param fd A file descriptor
//  * @param event A libevent event
//  * @param arg An arg pointer
//  */
//void recv_pkg(int fd, short event, void *arg);
//
///**
//  * receive a stun message
//  * @param *msgbuf A pointer to a message buffer
//  * @param recvSize The buffersize
//  */
//void recv_stun_msg(char *msgbuf,int recvSize);
//
///**
//  * process an incoming data message
//  * @param *msgbuf A pointer to a message buffer
//  * @param bufsize The buffersize
//  * @param ttl The ttl value of the packet
//  */
//void recv_data_msg(struct msg_header *msg_h, char *msgbuf, int bufsize);
//
///**
//  * A receive timeout callback for the connections
//  * @param fd A file descriptor
//  * @param event A libevent event
//  * @param arg An arg pointer
//  */
//void recv_timeout_cb(int fd,short event,void *arg);
//
///**
//  * A pmtu timeout callback
//  * @param fd A file descriptor
//  * @param event A libevent event
//  * @param arg An arg pointer
//  */
//void pmtu_timeout_cb(int fd,short event,void *arg);
//
///**
//  * A function to decrement the pmtu value
//  * @param pmtusize A pmtusize
//  */
//pmtu pmtu_decrement(pmtu pmtusize);
//
///**
//  * A pmtu error callback
//  * @param *msg A pointer to a message buffer
//  * @param msglen The buffersize
//  */
//void pmtu_error_cb_th(char *msg,int msglen);
//
///**
// * Compare two socket IDs and check if the external IPaddresses match
// * @param sock1 The first socketID
// * @param sock2 The second socketID
// * @return 0 if they match, otherwise 1
// */
//int compare_external_address_socketIDs(socketID_handle sock1,socketID_handle sock2);
//
///**
//  * Compare two socket IDs
//  * @param sock1 The first socketID
//  * @param sock2 The second socketID
//  * @return 0 if they match, otherwise 1
//  */
//int mlCompareSocketIDs(socketID_handle sock1,socketID_handle sock2);
//
///**
//  * A NAT traversal timeout callback
//  * @param fd A file descriptor
//  * @param event A libevent event
//  * @param arg An arg pointer
//  */
//void nat_traversal_timeout(int fd, short event, void *arg);
//
///**
// * A function that returns the standard TTL value the operating system places i
//n every IP packet
// * @param socketID A local socketID
// * @param ttl A pointer to an integer
// * @return 0 if everything went well and -1 if an error occured
// */
//int get_standard_ttl(socketID_handle socketID,uint8_t *ttl);
//
///**
// * A funtion that returns a handle for the local socketID
// * @param errorstatus Returns 0 if the socketID is operational and 2 if the NAT traversal failed
// * @return A socketID handle
// */
//socketID_handle get_local_socketID(int *errorstatus);
//
///**
// * A funtion that returns the external IP from the local socketID
// * @param external_addr A pointer to a char buffer that holds the externall address
// * @return -1 if an error occurred and 0 if it succeded
// */
//int get_external_IP(char* external_addr);
//
///**
// * A funtion that returns a string from a socketID handler
// * @param socketID
// * @param socketID_string
// * @param len size of socketID_string buffer
// * @return -1 if an error occurred and 0 if it succeded
// */
//int mlSoc2ketIDToString(socketID_handle socketID,char* socketID_string, size_t len);
//
///**
// * A funtion that returns a socketID handler from a string
// * @param socketID A socketID in string format
// * @return a pointer to a socketID
// */
//int string_to_socketID(const char* socketID_string, socketID_handle socketID);
//
///**
// * A funtion that returns the whenever a connection is ready to use or not.
// * @param connectionID The connectionID of the connection that is queried.
// * @return the status or -1 if the connectionID does not exist
// */
//int get_connection_status(int connectionID);
//
///**
// * A funtion that returns the whenever a connection is ready to use or not.
// * @param socketID A remote socketID to which the connection has been established.
// * @param ready if true,return the connID for READY connections only
// * @return -1 if the connection does not exist and the connection ID if it exists.
// */
//int connection_exist(socketID_handle socketID,bool ready);
//
///**
// * Resolve a hostname to an unsigned long ready to be loaded into struct in_addr.s_addr
// * @param ipaddr the ip addr in a textual format
// * @return the resolved address, in network byte ordering
// */
//unsigned long resolve(const char *ipaddr);
//
//int get_path_mtu(int ConnectionId);


#endif
