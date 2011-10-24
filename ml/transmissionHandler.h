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
#ifndef _WIN32
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

#ifndef _WIN32
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

#endif
