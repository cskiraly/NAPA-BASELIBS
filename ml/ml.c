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

#include <arpa/inet.h>
#ifndef WIN32
#include <netinet/in.h>
#include <sys/socket.h>
#endif
#include <fcntl.h>
#include <event2/event.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <errno.h>
#include <time.h>
#include <math.h>
#include <assert.h>

#include "util/udpSocket.h"
#include "util/stun.h"
#include "transmissionHandler.h"

#define LOG_MODULE "[ml] "
#include "ml_log.h"

/**************************** START OF INTERNALS ***********************/


/*
 * reserved message type for internal puposes
 */
#define MSG_TYPE_ML_KEEPALIVE 0x126	//TODO: check that it is really interpreted as internal

/*
 * a pointer to a libevent instance
 */
struct event_base *base;

/*
 * define the nr of connections the messaging layer can handle
 */
#define CONNECTBUFSIZE 10000
/*
 * define the nr of data that can be received parallel
 */
#define RECVDATABUFSIZE 10000
/*
 * define an array for message multiplexing
 */
#define MSGMULTIPLEXSIZE 127


/*
 * timeout before thinking that the STUN server can't be connected
 */
#define NAT_TRAVERSAL_TIMEOUT { 1, 0 }

/*
 * timeout before thinking of an mtu problem (check MAX_TRIALS as well)
 */
#define PMTU_TIMEOUT { 0, 500000 }

/*
 * retry sending connection messages this many times before reducing pmtu
 */
#define MAX_TRIALS 3

/*
 * default timeout value between the first and the last received packet of a message
 */
#define RECV_TIMEOUT_DEFAULT { 2, 0 }

/*
 * global variables
 */
/*
 * define a buffer of pointers to connect structures
 */
connect_data *connectbuf[CONNECTBUFSIZE];

/*
 * define a pointer buffer with pointers to recv_data structures
 */
recvdata *recvdatabuf[RECVDATABUFSIZE];

/*
 * define a pointer buffer for message multiplexing
 */
receive_data_cb recvcbbuf[MSGMULTIPLEXSIZE];

/*
 * stun server address
 */
struct sockaddr_in stun_server;

/*
 * receive timeout
 */
static struct timeval recv_timeout = RECV_TIMEOUT_DEFAULT;

/*
 * boolean NAT traversal successful if true
 */
boolean NAT_traversal;

/*
 * file descriptor for local socket
 */
evutil_socket_t socketfd;

/*
 * local socketID
 */
socket_ID local_socketID;

socketID_handle loc_socketID = &local_socketID;

/*
 * callback function pointers
 */
/*
 * monitoring module callbacks
 */
get_recv_pkt_inf_cb get_Recv_pkt_inf_cb = NULL;
get_send_pkt_inf_cb get_Send_pkt_inf_cb = NULL;
set_monitoring_header_pkt_cb set_Monitoring_header_pkt_cb = NULL;
get_recv_data_inf_cb get_Recv_data_inf_cb = NULL;
get_send_data_inf_cb get_Send_data_inf_cb = NULL;
set_monitoring_header_data_cb set_Monitoring_header_data_cb = NULL;
/*
 * connection callbacks
 */
receive_connection_cb receive_Connection_cb = NULL;
connection_failed_cb failed_Connection_cb = NULL;
/*
 * local socketID callback
 */
receive_localsocketID_cb receive_SocketID_cb;

/*
 * boolean that defines if received data is transmitted to the upper layer
 * via callback or via upper layer polling
 */
boolean recv_data_callback;

/*
 * helper function to get rid of a warning
 */
int min(int a, int b) {
	if (a > b) return b;
	return a;
}

/*
 * convert a socketID to a string. It uses a static buffer, so either strdup is needed, or the string will get lost!
 */
const char *conid_to_string(int con_id)
{
	static char s[INET_ADDRSTRLEN+1+5+1+INET_ADDRSTRLEN+1+5+1];
	mlSocketIDToString(&connectbuf[con_id]->external_socketID, s, sizeof(s));
	return s;
}

void register_recv_localsocketID_cb(receive_localsocketID_cb local_socketID_cb)
{
	if (local_socketID_cb == NULL) {
		error("ML : Register receive_localsocketID_cb: NULL ptr \n");
	} else {
		receive_SocketID_cb = local_socketID_cb;
	}
}


//void keep_connection_alive(const int connectionID)
//{
//
//    // to be done with the NAT traversal
//    // send a message over the wire
//    printf("\n");
//
//}

void unsetStunServer()
{
	stun_server.sin_addr.s_addr = INADDR_NONE;
}

bool isStunDefined()
{
	return stun_server.sin_addr.s_addr != INADDR_NONE;
}

void send_msg(int con_id, int msg_type, char* msg, int msg_len, bool truncable, send_params * sParams) {
	socketaddrgen udpgen;
	bool retry;
	int pkt_len, offset;
	struct iovec iov[4];

	char h_pkt[MON_PKT_HEADER_SPACE];
	char h_data[MON_DATA_HEADER_SPACE];

	struct msg_header msg_h;

	debug("ML: send_msg to %s conID:%d extID:%d\n", conid_to_string(con_id), con_id, connectbuf[con_id]->external_connectionID);

	iov[0].iov_base = &msg_h;
	iov[0].iov_len = MSG_HEADER_SIZE;

	msg_h.local_con_id = htonl(con_id);
	msg_h.remote_con_id = htonl(connectbuf[con_id]->external_connectionID);
	msg_h.msg_type = msg_type;
	msg_h.msg_seq_num = htonl(connectbuf[con_id]->seqnr++);


	iov[1].iov_len = iov[2].iov_len = 0;
	iov[1].iov_base = h_pkt;
	iov[2].iov_base = h_data;


	if (connectbuf[con_id]->internal_connect)
		udpgen = connectbuf[con_id]->external_socketID.internal_addr;
	else
		udpgen = connectbuf[con_id]->external_socketID.external_addr;

	do{
		offset = 0;
		retry = false;
		// Monitoring layer hook
		if(set_Monitoring_header_data_cb != NULL) {
			iov[2].iov_len = ((set_Monitoring_header_data_cb) (&(connectbuf[con_id]->external_socketID), msg_type));
		}
		msg_h.len_mon_data_hdr = iov[2].iov_len;

		if(get_Send_data_inf_cb != NULL && iov[2].iov_len != 0) {
			mon_data_inf sd_data_inf;

			memset(h_data, 0, MON_DATA_HEADER_SPACE);

			sd_data_inf.remote_socketID = &(connectbuf[con_id]->external_socketID);
			sd_data_inf.buffer = msg;
			sd_data_inf.bufSize = msg_len;
			sd_data_inf.msgtype = msg_type;
			sd_data_inf.monitoringDataHeader = iov[2].iov_base;
			sd_data_inf.monitoringDataHeaderLen = iov[2].iov_len;
			sd_data_inf.priority = sParams->priority;
			sd_data_inf.padding = sParams->padding;
			sd_data_inf.confirmation = sParams->confirmation;
			sd_data_inf.reliable = sParams->reliable;
			memset(&sd_data_inf.arrival_time, 0, sizeof(struct timeval));

			(get_Send_data_inf_cb) ((void *) &sd_data_inf);
		}

		do {
			if(set_Monitoring_header_pkt_cb != NULL) {
				iov[1].iov_len = (set_Monitoring_header_pkt_cb) (&(connectbuf[con_id]->external_socketID), msg_type);
			}
			pkt_len = min(connectbuf[con_id]->pmtusize - iov[2].iov_len - iov[1].iov_len - iov[0].iov_len, msg_len - offset) ;

			iov[3].iov_len = pkt_len;
			iov[3].iov_base = msg + offset;

			//fill header
			msg_h.len_mon_packet_hdr = iov[1].iov_len;
			msg_h.offset = htonl(offset);
			msg_h.msg_length = htonl(truncable ? pkt_len : msg_len);

			//monitoring layer hook
			if(get_Send_pkt_inf_cb != NULL && iov[1].iov_len) {
				mon_pkt_inf pkt_info;

				memset(h_pkt,0,MON_PKT_HEADER_SPACE);

				pkt_info.remote_socketID = &(connectbuf[con_id]->external_socketID);
				pkt_info.buffer = msg + offset;
				pkt_info.bufSize = pkt_len;
				pkt_info.msgtype = msg_type;
				pkt_info.dataID = connectbuf[con_id]->seqnr;
				pkt_info.offset = offset;
				pkt_info.datasize = msg_len;
				pkt_info.monitoringHeaderLen = iov[1].iov_len;
				pkt_info.monitoringHeader = iov[1].iov_base;
				pkt_info.ttl = -1;
				memset(&(pkt_info.arrival_time),0,sizeof(struct timeval));

				(get_Send_pkt_inf_cb) ((void *) &pkt_info);
			}

			debug("ML: sending packet to %s with rconID:%d lconID:%d\n", conid_to_string(con_id), ntohl(msg_h.remote_con_id), ntohl(msg_h.local_con_id));
			switch(sendPacket(socketfd, iov, 4, &udpgen.udpaddr)) {
				case MSGLEN:
					info("ML: sending message failed, reducing MTU from %d to %d (to:%s conID:%d lconID:%d msgsize:%d offset:%d)\n", connectbuf[con_id]->pmtusize, pmtu_decrement(connectbuf[con_id]->pmtusize), conid_to_string(con_id), ntohl(msg_h.remote_con_id), ntohl(msg_h.local_con_id), msg_len, offset);
					// TODO: pmtu decremented here, but not in the "truncable" packet. That is currently resent without changing the claimed pmtu. Might need to be changed.
					connectbuf[con_id]->pmtusize = pmtu_decrement(connectbuf[con_id]->pmtusize);
					if (connectbuf[con_id]->pmtusize > 0) {
						connectbuf[con_id]->delay = true;
						retry = true;
					}
					offset = msg_len; // exit the while
					break;
				case FAILURE:
					info("ML: sending message failed (to:%s conID:%d lconID:%d msgsize:%d offset:%d)\n", conid_to_string(con_id), ntohl(msg_h.remote_con_id), ntohl(msg_h.local_con_id), msg_len, offset);
					offset = msg_len; // exit the while
					break;
                                case THROTTLE:
                                    //    debug("THROTTLE on output"); 
					offset = msg_len; // exit the while
					break;
				case OK:
					//update
					offset += pkt_len;
					//transmit data header only in the first packet
					iov[2].iov_len = 0;
					break;
			}
		} while(offset != msg_len && !truncable);
	} while(retry);
}

void pmtu_timeout_cb(int fd, short event, void *arg);

void reschedule_conn_msg(int con_id)
{
	if (connectbuf[con_id]->timeout_event) {
		/* delete old timout */	
		event_del(connectbuf[con_id]->timeout_event);
		event_free(connectbuf[con_id]->timeout_event);
	}
	connectbuf[con_id]->timeout_event = event_new(base, -1, EV_TIMEOUT, &pmtu_timeout_cb, (void *) (long)con_id);
	evtimer_add(connectbuf[con_id]->timeout_event, &connectbuf[con_id]->timeout_value);
}

void send_conn_msg(int con_id, int buf_size, int command_type)
{
	if (buf_size < sizeof(struct conn_msg)) {
		error("ML: requested connection message size is too small\n");
		return;
	}

	if(connectbuf[con_id]->ctrl_msg_buf == NULL) {
		connectbuf[con_id]->ctrl_msg_buf = malloc(buf_size);
		memset(connectbuf[con_id]->ctrl_msg_buf, 0, buf_size);
	}

	if(connectbuf[con_id]->ctrl_msg_buf == NULL) {
		error("ML: can not allocate memory for connection message\n");
		return;
	}

	struct conn_msg *msg_header = (struct conn_msg*) connectbuf[con_id]->ctrl_msg_buf;

	msg_header->comand_type = command_type;
	msg_header->pmtu_size = connectbuf[con_id]->pmtusize;

	memcpy(&(msg_header->sock_id), loc_socketID, sizeof(socket_ID));

	send_msg(con_id, ML_CON_MSG, connectbuf[con_id]->ctrl_msg_buf, buf_size, true, &(connectbuf[con_id]->defaultSendParams));
}

void send_conn_msg_with_pmtu_discovery(int con_id, int buf_size, int command_type)
{
	struct timeval tout = PMTU_TIMEOUT;
	connectbuf[con_id]->timeout_value = tout;
	connectbuf[con_id]->trials = 1;
	send_conn_msg(con_id, buf_size, command_type);
	reschedule_conn_msg(con_id);
}

void resend_conn_msg(int con_id)
{
	connectbuf[con_id]->trials++;
	send_conn_msg(con_id, connectbuf[con_id]->pmtusize, connectbuf[con_id]->status);
	reschedule_conn_msg(con_id);
}

void recv_conn_msg(struct msg_header *msg_h, char *msgbuf, int msg_size, struct sockaddr_in *recv_addr)
{
	struct conn_msg *con_msg;
	int free_con_id, con_id;

	time_t now = time(NULL);
	double timediff = 0.0;
	char sock_id_str[1000];
	
	msgbuf += msg_h->len_mon_data_hdr;
	msg_size -= msg_h->len_mon_data_hdr;
	con_msg = (struct conn_msg *)msgbuf;
	
	//verify message validity
	if (msg_size < sizeof(struct conn_msg)) {
		char recv_addr_str[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(recv_addr->sin_addr.s_addr), recv_addr_str, INET_ADDRSTRLEN);
		info("Invalid conn_msg received from %s\n", recv_addr_str);
		return;
	}

	//decode sock_id for debug messages
	mlSocketIDToString(&con_msg->sock_id,sock_id_str,999);

	if (con_msg->sock_id.internal_addr.udpaddr.sin_addr.s_addr != recv_addr->sin_addr.s_addr &&
            con_msg->sock_id.external_addr.udpaddr.sin_addr.s_addr != recv_addr->sin_addr.s_addr   ) {
		char recv_addr_str[INET_ADDRSTRLEN];
		inet_ntop(AF_INET, &(recv_addr->sin_addr.s_addr), recv_addr_str, INET_ADDRSTRLEN);
		info("Conn msg received from %s, but claims to be from %s", recv_addr_str, sock_id_str);
		return;
	}

	// Monitoring layer hook
	if(get_Recv_data_inf_cb != NULL) {
		// update pointer to the real data
		mon_data_inf recv_data_inf;
		recv_data_inf.remote_socketID = &(con_msg->sock_id);
		recv_data_inf.buffer = msgbuf;
		recv_data_inf.bufSize = msg_size;
		recv_data_inf.msgtype = msg_h->msg_type;
		recv_data_inf.monitoringDataHeaderLen = msg_h->len_mon_data_hdr;
		recv_data_inf.monitoringDataHeader = msg_h->len_mon_data_hdr ? msgbuf : NULL;
		gettimeofday(&recv_data_inf.arrival_time, NULL);
		recv_data_inf.firstPacketArrived = true;
		recv_data_inf.recvFragments = 1;
		recv_data_inf.priority = false;
		recv_data_inf.padding = false;
		recv_data_inf.confirmation = false;
		recv_data_inf.reliable = false;

		// send data recv callback to monitoring module
		(get_Recv_data_inf_cb) ((void *) &recv_data_inf);
	}

	// check the connection command type
	switch (con_msg->comand_type) {
		/*
		* if INVITE: enter a new socket make new entry in connect array
		* send an ok
		*/
		case INVITE:
			info("ML: received INVITE from %s (size:%d)\n", sock_id_str, msg_size);
			/*
			* check if another connection for the external connectionID exist
			* that was established within the last 2 seconds
			*/
			free_con_id = -1;
			for (con_id = 0; con_id < CONNECTBUFSIZE; con_id++) {
				if (connectbuf[con_id] != NULL) {
					if (mlCompareSocketIDs(&(connectbuf[con_id]->external_socketID), &(con_msg->sock_id)) == 0) {
						//timediff = difftime(now, connectbuf[con_id]->starttime);	//TODO: why this timeout? Shouldn't the connection be closed instead if there is a timeout?
						//if (timediff < 2)
						//update remote connection ID
						if (connectbuf[con_id]->external_connectionID != msg_h->local_con_id) {
							warn("ML: updating remote connection ID for %s: from %d to %d\n",sock_id_str, connectbuf[con_id]->external_connectionID, msg_h->local_con_id);
							connectbuf[con_id]->external_connectionID = msg_h->local_con_id;
						}
						break;
					}
				} else if(free_con_id == -1)
					free_con_id = con_id;
			}

			if (con_id == CONNECTBUFSIZE) {
				// create an entry in the connecttrybuf
				if(free_con_id == -1) {
					error("ML: no new connect_buf available\n");
					return;
				}
				connectbuf[free_con_id] = (connect_data *) malloc(sizeof(connect_data));
				memset(connectbuf[free_con_id],0,sizeof(connect_data));
				connectbuf[free_con_id]->connection_head = connectbuf[free_con_id]->connection_last = NULL;
				connectbuf[free_con_id]->starttime = time(NULL);
				memcpy(&(connectbuf[free_con_id]->external_socketID), &(con_msg->sock_id), sizeof(socket_ID));
				connectbuf[free_con_id]->pmtusize = con_msg->pmtu_size;	// bootstrap pmtu from the other's size. Not strictly needed, but a good hint
				connectbuf[free_con_id]->timeout_event = NULL;
				connectbuf[free_con_id]->external_connectionID = msg_h->local_con_id;
				connectbuf[free_con_id]->internal_connect =
					!compare_external_address_socketIDs(&(con_msg->sock_id), loc_socketID);
				con_id = free_con_id;
			}

			//if(connectbuf[con_id]->status <= CONNECT) { //TODO: anwer anyway. Why the outher would invite otherwise?
				//update status and send back answer
				connectbuf[con_id]->status = CONNECT;
				send_conn_msg_with_pmtu_discovery(con_id, con_msg->pmtu_size, CONNECT);
			//}
			break;
		case CONNECT:
			info("ML: received CONNECT from %s (size:%d)\n", sock_id_str, msg_size);

			if(msg_h->remote_con_id != -1 && connectbuf[msg_h->remote_con_id] == NULL) {
				error("ML: received CONNECT for inexistent connection rconID:%d\n",msg_h->remote_con_id);
				return;
			}

			/*
			* check if the connection status is not already 1 or 2
			*/
			if (connectbuf[msg_h->remote_con_id]->status == INVITE) {
				// set the external connectionID
				connectbuf[msg_h->remote_con_id]->external_connectionID = msg_h->local_con_id;
				// change status con_msg the connection_data
				connectbuf[msg_h->remote_con_id]->status = READY;
				// change pmtusize in the connection_data: not needed. receiving a CONNECT means our INVITE went through. So why change pmtu?
				//connectbuf[msg_h->remote_con_id]->pmtusize = con_msg->pmtu_size;

				// send the READY
				send_conn_msg_with_pmtu_discovery(msg_h->remote_con_id, con_msg->pmtu_size, READY);

				if (receive_Connection_cb != NULL)
					(receive_Connection_cb) (msg_h->remote_con_id, NULL);

				// call all registered callbacks
				while(connectbuf[msg_h->remote_con_id]->connection_head != NULL) {
					struct receive_connection_cb_list *temp;
					temp = connectbuf[msg_h->remote_con_id]->connection_head;
					(temp->connection_cb) (msg_h->remote_con_id, temp->arg);
					connectbuf[msg_h->remote_con_id]->connection_head = temp->next;
					free(temp);
				}
				connectbuf[msg_h->remote_con_id]->connection_head =
					connectbuf[msg_h->remote_con_id]->connection_last = NULL;
			} else {
				// send the READY
				send_conn_msg_with_pmtu_discovery(msg_h->remote_con_id, con_msg->pmtu_size, READY);
			}

			debug("ML: active connection established\n");
			break;

			/*
			* if READY: find the entry in the connection array set the
			* connection active change the pmtu size
			*/
		case READY:
			info("ML: received READY from %s (size:%d)\n", sock_id_str, msg_size);
			if(connectbuf[msg_h->remote_con_id] == NULL) {
				error("ML: received READY for inexistent connection\n");
				return;
			}
			/*
			* checks if the connection is not already established
			*/
			if (connectbuf[msg_h->remote_con_id]->status == CONNECT) {
				// change status of the connection
				connectbuf[msg_h->remote_con_id]->status = READY;
				// change pmtusize: not needed. pmtu doesn't have to be symmetric
				//connectbuf[msg_h->remote_con_id]->pmtusize = con_msg->pmtu_size;

				if (receive_Connection_cb != NULL)
					(receive_Connection_cb) (msg_h->remote_con_id, NULL);

				while(connectbuf[msg_h->remote_con_id]->connection_head != NULL) {
					struct receive_connection_cb_list *temp;
					temp = connectbuf[msg_h->remote_con_id]->connection_head;
					(temp->connection_cb) (msg_h->remote_con_id, temp->arg);
					connectbuf[msg_h->remote_con_id]->connection_head = temp->next;
					free(temp);
				}
				connectbuf[msg_h->remote_con_id]->connection_head =
					connectbuf[msg_h->remote_con_id]->connection_last = NULL;
				debug("ML: passive connection established\n");
			}
			break;
	}
}

void recv_stun_msg(char *msgbuf, int recvSize)
{
	/*
	* create empty stun message struct
	*/
	StunMessage resp;
	memset(&resp, 0, sizeof(StunMessage));
	/*
	* parse the message
	*/
	int returnValue = 0;
	returnValue = recv_stun_message(msgbuf, recvSize, &resp);

	if (returnValue == 0) {
		/*
		* read the reflexive Address into the local_socketID
		*/
		struct sockaddr_in reflexiveAddr = {0};
		reflexiveAddr.sin_family = AF_INET;
		reflexiveAddr.sin_addr.s_addr = htonl(resp.mappedAddress.ipv4.addr);
		reflexiveAddr.sin_port = htons(resp.mappedAddress.ipv4.port);
		socketaddrgen reflexiveAddres = {0};
		reflexiveAddres.udpaddr = reflexiveAddr;
		local_socketID.external_addr = reflexiveAddres;
		NAT_traversal = true;
		// callback to the upper layer indicating that the socketID is now
		// ready to use
		(receive_SocketID_cb) (&local_socketID, 0);
	}
}

//done
void recv_timeout_cb(int fd, short event, void *arg)
{
	int recv_id = (long) arg;
	debug("ML: recv_timeout_cb called. Timeout for id:%d\n",recv_id);

	if (recvdatabuf[recv_id] == NULL) {
		return;
	}


/*	if(recvdatabuf[recv_id]->status == ACTIVE) {
		//TODO make timeout at least a DEFINE
		struct timeval timeout = { 4, 0 };
		recvdatabuf[recv_id]->status = INACTIVE;
		event_base_once(base, -1, EV_TIMEOUT, recv_timeout_cb,
			arg, &timeout);
		return;
	}
*/

	if(recvdatabuf[recv_id]->status == ACTIVE) {
		// Monitoring layer hook
		if(get_Recv_data_inf_cb != NULL) {
			mon_data_inf recv_data_inf;

			recv_data_inf.remote_socketID =
					&(connectbuf[recvdatabuf[recv_id]->connectionID]->external_socketID);
			recv_data_inf.buffer = recvdatabuf[recv_id]->recvbuf;
			recv_data_inf.bufSize = recvdatabuf[recv_id]->bufsize;
			recv_data_inf.msgtype = recvdatabuf[recv_id]->msgtype;
			recv_data_inf.monitoringDataHeaderLen = recvdatabuf[recv_id]->monitoringDataHeaderLen;
			recv_data_inf.monitoringDataHeader = recvdatabuf[recv_id]->monitoringDataHeaderLen ?
				recvdatabuf[recv_id]->recvbuf : NULL;
			gettimeofday(&recv_data_inf.arrival_time, NULL);
			recv_data_inf.firstPacketArrived = recvdatabuf[recv_id]->firstPacketArrived;
			recv_data_inf.recvFragments = recvdatabuf[recv_id]->recvFragments;
			recv_data_inf.priority = false;
			recv_data_inf.padding = false;
			recv_data_inf.confirmation = false;
			recv_data_inf.reliable = false;

			// send data recv callback to monitoring module

//			(get_Recv_data_inf_cb) ((void *) &recv_data_inf);
		}

		// Get the right callback
		receive_data_cb receive_data_callback = recvcbbuf[recvdatabuf[recv_id]->msgtype];

		recv_params rParams;

		rParams.nrMissingBytes = recvdatabuf[recv_id]->bufsize - recvdatabuf[recv_id]->arrivedBytes;
		rParams.recvFragments = recvdatabuf[recv_id]->recvFragments;
		rParams.msgtype = recvdatabuf[recv_id]->msgtype;
		rParams.connectionID = recvdatabuf[recv_id]->connectionID;
		rParams.remote_socketID =
			&(connectbuf[recvdatabuf[recv_id]->connectionID]->external_socketID);
		rParams.firstPacketArrived = recvdatabuf[recv_id]->firstPacketArrived;

// 		(receive_data_callback) (recvdatabuf[recv_id]->recvbuf + recvdatabuf[recv_id]->monitoringDataHeaderLen, recvdatabuf[recv_id]->bufsize - recvdatabuf[recv_id]->monitoringDataHeaderLen,
// 			recvdatabuf[recv_id]->msgtype, &rParams);

		//clean up
		if (recvdatabuf[recv_id]->timeout_event) {
			event_del(recvdatabuf[recv_id]->timeout_event);
			event_free(recvdatabuf[recv_id]->timeout_event);
			recvdatabuf[recv_id]->timeout_event = NULL;
		}
		free(recvdatabuf[recv_id]->recvbuf);
		free(recvdatabuf[recv_id]);
		recvdatabuf[recv_id] = NULL;
	}
}

// process a single recv data message
void recv_data_msg(struct msg_header *msg_h, char *msgbuf, int bufsize)
{
	debug("ML: received packet of size %d with rconID:%d lconID:%d type:%d offset:%d\n",bufsize,msg_h->remote_con_id,msg_h->local_con_id,msg_h->msg_type,msg_h->offset);

	int recv_id, free_recv_id = -1;

	if(connectbuf[msg_h->remote_con_id] == NULL) {
		debug("ML: Received a message not related to any opened connection!\n");
		return;
	}

	// check if a recv_data exist and enter data
	for (recv_id = 0; recv_id < RECVDATABUFSIZE; recv_id++)
		if (recvdatabuf[recv_id] != NULL) {
			if (msg_h->remote_con_id == recvdatabuf[recv_id]->connectionID &&
					msg_h->msg_seq_num == recvdatabuf[recv_id]->seqnr)
						break;
		} else
			if(free_recv_id == -1)
				free_recv_id = recv_id;


	if(recv_id == RECVDATABUFSIZE) {
		//no recv_data found: create one
		recv_id = free_recv_id;
		recvdatabuf[recv_id] = (recvdata *) malloc(sizeof(recvdata));
		memset(recvdatabuf[recv_id], 0, sizeof(recvdata));
		recvdatabuf[recv_id]->connectionID = msg_h->remote_con_id;
		recvdatabuf[recv_id]->seqnr = msg_h->msg_seq_num;
		recvdatabuf[recv_id]->monitoringDataHeaderLen = msg_h->len_mon_data_hdr;
		recvdatabuf[recv_id]->bufsize = msg_h->msg_length + msg_h->len_mon_data_hdr;
		recvdatabuf[recv_id]->recvbuf = (char *) malloc(recvdatabuf[recv_id]->bufsize);
		recvdatabuf[recv_id]->arrivedBytes = 0;	//count this without the Mon headers
		/*
		* read the timeout data and set it
		*/
		recvdatabuf[recv_id]->timeout_value = recv_timeout;
		recvdatabuf[recv_id]->timeout_event = NULL;
		recvdatabuf[recv_id]->recvID = recv_id;
		recvdatabuf[recv_id]->starttime = time(NULL);
		recvdatabuf[recv_id]->msgtype = msg_h->msg_type;

		// fill the buffer with zeros
		memset(recvdatabuf[recv_id]->recvbuf, 0, recvdatabuf[recv_id]->bufsize);
		debug(" new @ id:%d\n",recv_id);
	} else {	//message structure already exists, no need to create new
		debug(" found @ id:%d (arrived before this packet: bytes:%d fragments%d\n",recv_id, recvdatabuf[recv_id]->arrivedBytes, recvdatabuf[recv_id]->recvFragments);
	}

	//if first packet extract mon data header and advance pointer
	if (msg_h->offset == 0) {
		memcpy(recvdatabuf[recv_id]->recvbuf, msgbuf, msg_h->len_mon_data_hdr);
		msgbuf += msg_h->len_mon_data_hdr;
		bufsize -= msg_h->len_mon_data_hdr;
		recvdatabuf[recv_id]->firstPacketArrived = 1;
	}


	// increment fragmentnr
	recvdatabuf[recv_id]->recvFragments++;
	// increment the arrivedBytes
	recvdatabuf[recv_id]->arrivedBytes += bufsize;

	// enter the data into the buffer
	memcpy(recvdatabuf[recv_id]->recvbuf + msg_h->len_mon_data_hdr + msg_h->offset, msgbuf, bufsize);

	//TODO very basic checkif all fragments arrived: has to be reviewed
	if(recvdatabuf[recv_id]->arrivedBytes == recvdatabuf[recv_id]->bufsize - recvdatabuf[recv_id]->monitoringDataHeaderLen)
		recvdatabuf[recv_id]->status = COMPLETE; //buffer full -> msg completly arrived
	else
		recvdatabuf[recv_id]->status = ACTIVE;

	if (recv_data_callback) {
		if(recvdatabuf[recv_id]->status == COMPLETE) {
			// Monitoring layer hook
			if(get_Recv_data_inf_cb != NULL) {
				mon_data_inf recv_data_inf;

				recv_data_inf.remote_socketID =
					 &(connectbuf[recvdatabuf[recv_id]->connectionID]->external_socketID);
				recv_data_inf.buffer = recvdatabuf[recv_id]->recvbuf;
				recv_data_inf.bufSize = recvdatabuf[recv_id]->bufsize;
				recv_data_inf.msgtype = recvdatabuf[recv_id]->msgtype;
				recv_data_inf.monitoringDataHeaderLen = recvdatabuf[recv_id]->monitoringDataHeaderLen;
				recv_data_inf.monitoringDataHeader = recvdatabuf[recv_id]->monitoringDataHeaderLen ?
					recvdatabuf[recv_id]->recvbuf : NULL;
				gettimeofday(&recv_data_inf.arrival_time, NULL);
				recv_data_inf.firstPacketArrived = recvdatabuf[recv_id]->firstPacketArrived;
				recv_data_inf.recvFragments = recvdatabuf[recv_id]->recvFragments;
				recv_data_inf.priority = false;
				recv_data_inf.padding = false;
				recv_data_inf.confirmation = false;
				recv_data_inf.reliable = false;

				// send data recv callback to monitoring module

				(get_Recv_data_inf_cb) ((void *) &recv_data_inf);
			}

			// Get the right callback
			receive_data_cb receive_data_callback = recvcbbuf[msg_h->msg_type];
			if (receive_data_callback) {

				recv_params rParams;

				rParams.nrMissingBytes = recvdatabuf[recv_id]->bufsize - recvdatabuf[recv_id]->monitoringDataHeaderLen - recvdatabuf[recv_id]->arrivedBytes;
				rParams.recvFragments = recvdatabuf[recv_id]->recvFragments;
				rParams.msgtype = recvdatabuf[recv_id]->msgtype;
				rParams.connectionID = recvdatabuf[recv_id]->connectionID;
				rParams.remote_socketID =
					&(connectbuf[recvdatabuf[recv_id]->connectionID]->external_socketID);

				char str[1000];
				mlSocketIDToString(rParams.remote_socketID,str,999);
				debug("ML: received message from conID:%d, %s\n",recvdatabuf[recv_id]->connectionID,str);
				rParams.firstPacketArrived = recvdatabuf[recv_id]->firstPacketArrived;

				(receive_data_callback) (recvdatabuf[recv_id]->recvbuf + recvdatabuf[recv_id]->monitoringDataHeaderLen, recvdatabuf[recv_id]->bufsize - recvdatabuf[recv_id]->monitoringDataHeaderLen,
					recvdatabuf[recv_id]->msgtype, (void *) &rParams);
			} else {
			    warn("ML: callback not initialized for this message type: %d!\n",msg_h->msg_type);
			}

			//clean up
			if (recvdatabuf[recv_id]->timeout_event) {
				debug("ML: freeing timeout for %d",recv_id);
				event_del(recvdatabuf[recv_id]->timeout_event);
				event_free(recvdatabuf[recv_id]->timeout_event);
				recvdatabuf[recv_id]->timeout_event = NULL;
			} else {
				debug("ML: received in 1 packet\n",recv_id);
			}
			free(recvdatabuf[recv_id]->recvbuf);
			free(recvdatabuf[recv_id]);
			recvdatabuf[recv_id] = NULL;
		} else { // not COMPLETE
			if (!recvdatabuf[recv_id]->timeout_event) {
				//start time out
				//TODO make timeout at least a DEFINE
				recvdatabuf[recv_id]->timeout_event = event_new(base, -1, EV_TIMEOUT, &recv_timeout_cb, (void *) (long)recv_id);
				evtimer_add(recvdatabuf[recv_id]->timeout_event, &recv_timeout);
			}
		}
	}
}

//done
void pmtu_timeout_cb(int fd, short event, void *arg)
{

	int con_id = (long) arg;
	pmtu new_pmtusize;

	debug("ML: pmtu timeout called (lcon:%d)\n",con_id);

	if(connectbuf[con_id] == NULL) {
		error("ML: pmtu timeout called on non existing con_id\n");
		return;
	}

	if(connectbuf[con_id]->status == READY) {
		// nothing to do anymore
		event_del(connectbuf[con_id]->timeout_event);
		event_free(connectbuf[con_id]->timeout_event);
		connectbuf[con_id]->timeout_event = NULL;
		return;
	}

	info("ML: pmtu timeout while connecting(to:%s lcon:%d status:%d size:%d trial:%d tout:%ld.%06ld)\n",conid_to_string(con_id), con_id, connectbuf[con_id]->status, connectbuf[con_id]->pmtusize, connectbuf[con_id]->trials, connectbuf[con_id]->timeout_value.tv_sec, connectbuf[con_id]->timeout_value.tv_usec);

	if(connectbuf[con_id]->delay || connectbuf[con_id]->trials == MAX_TRIALS - 1) {
		double delay = connectbuf[con_id]->timeout_value.tv_sec + connectbuf[con_id]->timeout_value.tv_usec / 1000000.0;
		delay = delay * 2;
		info("\tML: increasing pmtu timeout to %f sec\n", delay);
		connectbuf[con_id]->timeout_value.tv_sec = floor(delay);
		connectbuf[con_id]->timeout_value.tv_usec = fmod(delay, 1.0) * 1000000.0;
		if(connectbuf[con_id]->delay) {
			connectbuf[con_id]->delay = false;
			reschedule_conn_msg(con_id);
		}
	}

	if(connectbuf[con_id]->trials == MAX_TRIALS) {
		// decrement the pmtu size
		struct timeval tout = PMTU_TIMEOUT;
		info("\tML: decreasing pmtu estimate from %d to %d\n", connectbuf[con_id]->pmtusize, pmtu_decrement(connectbuf[con_id]->pmtusize));
		connectbuf[con_id]->pmtusize = pmtu_decrement(connectbuf[con_id]->pmtusize);
		connectbuf[con_id]->timeout_value = tout; 
		connectbuf[con_id]->trials = 0;
	}

	//error in PMTU discovery?
	if (connectbuf[con_id]->pmtusize == ERROR) {
		if (connectbuf[con_id]->internal_connect == true) {
			//as of now we tried directly connecting, now let's try trough the NAT
			connectbuf[con_id]->internal_connect = false;
			connectbuf[con_id]->pmtusize = DSLSLIM;
		} else {
			//nothing to do we have to give up
			error("ML: Could not create connection with connectionID %i!\n",con_id);
			// envoke the callback for failed connection establishment
			if(failed_Connection_cb != NULL)
				(failed_Connection_cb) (con_id, NULL);
			// delete the connection entry
			mlCloseConnection(con_id);
			return;
		}
	}

	//retry
	resend_conn_msg(con_id);
}


int schedule_pmtu_timeout(int con_id)
{
	if (! connectbuf[con_id]->timeout_event) {
		struct timeval tout = PMTU_TIMEOUT;
		connectbuf[con_id]->timeout_value = tout;
		connectbuf[con_id]->trials = 1;
		connectbuf[con_id]->timeout_event = event_new(base, -1, EV_TIMEOUT, &pmtu_timeout_cb, (void *) (long)con_id);
		evtimer_add(connectbuf[con_id]->timeout_event, &connectbuf[con_id]->timeout_value);
	}
}

/*
 * decrements the mtu size
 */
pmtu pmtu_decrement(pmtu pmtusize)
{
	pmtu pmtu_return_size;
	switch(pmtusize) {
	case MAX:
		//return DSL;
		return DSLSLIM;	//shortcut to use less vales
	case DSL:
		return DSLMEDIUM;
	case DSLMEDIUM:
		return DSLSLIM;
	case DSLSLIM:
		//return BELOWDSL;
		return MIN;	//shortcut to use less vales
	case BELOWDSL:
		return MIN;
	case MIN:
		return ERROR;
	default:
		warn("ML: strange pmtu size encountered:%d, changing to some safe value:%d\n", pmtusize, MIN);
		return MIN;
	}
}

// called when an ICMP pmtu error message (type 3, code 4) is received
void pmtu_error_cb_th(char *msg, int msglen)
{
	debug("ML: pmtu_error callback called msg_size: %d\n",msglen);
	//TODO debug
	return;

    char *msgbufptr = NULL;
    int msgtype;
    int connectionID;
    pmtu pmtusize;
    pmtu new_pmtusize;
    int dead = 0;

    // check the packettype
    msgbufptr = &msg[0];

    // check the msgtype
    msgbufptr = &msg[1];
    memcpy(&msgtype, msgbufptr, 4);

    if (msgtype == 0) {

	// get the connectionID
	msgbufptr = &msg[5];
	memcpy(&connectionID, msgbufptr, 4);

	int msgtype_c = connectbuf[connectionID]->status;
//	pmtusize = connectbuf[connectionID]->pmtutrysize;

	if (msgtype_c != msgtype) {
	    dead = 1;
	}


    } else if (msgtype == 1) {

	// read the connectionID
	msgbufptr = &msg[9];
	memcpy(&connectionID, msgbufptr, 4);

	int msgtype_c = connectbuf[connectionID]->status;
//	pmtusize = connectbuf[connectionID]->pmtutrysize;

	if (msgtype_c != msgtype) {
	    dead = 1;
	}

    }
    // decrement the pmtu size
    new_pmtusize = pmtu_decrement(pmtusize);

//    connectbuf[connectionID]->pmtutrysize = new_pmtusize;

    if (new_pmtusize == ERROR) {
		error("ML:  Could not create connection with connectionID %i !\n",
			connectionID);

		if(failed_Connection_cb != NULL)
			(failed_Connection_cb) (connectionID, NULL);
		// set the message type to a non existent message
		msgtype = 2;
		// delete the connection entry
		 mlCloseConnection(connectionID);
	}

    if (msgtype == 0 && dead != 1) {

	// stop the timeout event
	// timeout_del(connectbuf[connectionID]->timeout);
	/*
	 * libevent2
	 */

	// event_del(connectbuf[connectionID]->timeout);


	// create and send a connection message
// 	create_conn_msg(new_pmtusize, connectionID,
// 			&local_socketID, INVITE);

//	send_conn_msg(connectionID, new_pmtusize);

	// set a timeout event for the pmtu discovery
	// timeout_set(connectbuf[connectionID]->timeout,pmtu_timeout_cb,(void
	// *)&connectionID);

	// timeout_add(connectbuf[connectionID]->timeout,&connectbuf[connectionID]->timeout_value);

	/*
	 * libevent2
	 */

	struct event *ev;
	ev = evtimer_new(base, pmtu_timeout_cb,
			 (void *) connectbuf[connectionID]);

	// connectbuf[connectionID]->timeout = ev;

	event_add(ev, &connectbuf[connectionID]->timeout_value);

    } else if (msgtype == 1 && dead != 1) {

	// stop the timeout event
	// timeout_del(connectbuf[connectionID]->timeout);

	/*
	 * libevent2
	 */
	// info("still here 11 \n");
	// printf("ev %d \n",connectbuf[connectionID]->timeout);
	// event_del(connectbuf[connectionID]->timeout );
	// evtimer_del(connectbuf[connectionID]->timeout );


// 	// create and send a connection message
// 	create_conn_msg(new_pmtusize,
// 			connectbuf[connectionID]->connectionID,
// 			NULL, CONNECT);

	//send_conn_msg(connectionID, new_pmtusize);

	// set a timeout event for the pmtu discovery
	// timeout_set(connectbuf[connectionID]->timeout,pmtu_timeout_cb,(void
	// *)&connectionID);
	// timeout_add(connectbuf[connectionID]->timeout,&connectbuf[connectionID]->timeout_value);

	/*
	 * libevent2
	 */
	// struct event *ev;
	// ev = evtimer_new(base,pmtu_timeout_cb, (void
	// *)connectbuf[connectionID]);
	// connectbuf[connectionID]->timeout = ev;
	// event_add(ev,&connectbuf[connectionID]->timeout_value);

    }
}

/*
 * what to do once a packet arrived if it is a conn packet send it to
 * recv_conn handler if it is a data packet send it to the recv_data
 * handler
 */

//done --
void recv_pkg(int fd, short event, void *arg)
{
	debug("ML: recv_pkg called\n");

	struct msg_header *msg_h;
	char msgbuf[MAX];
	pmtu recvSize = MAX;
	char *bufptr = msgbuf;
	int ttl;
	struct sockaddr_in recv_addr;
	int msg_size;

	recvPacket(fd, msgbuf, &recvSize, &recv_addr, pmtu_error_cb_th, &ttl);


	// check if it is not just an ERROR message
	if(recvSize < 0)
		return;

	// @TODO check if this simplistic STUN message recognition really always works, probably not
	unsigned short stun_bind_response = 0x0101;
	unsigned short * msgspot = (unsigned short *) msgbuf;
	if (*msgspot == stun_bind_response) {
		debug("ML: recv_pkg: parse stun message called\n");
		recv_stun_msg(msgbuf, recvSize);
		return;
	}

	msg_h = (struct msg_header *) msgbuf;

	/* convert header from network to host order */
	msg_h->offset = ntohl(msg_h->offset);
	msg_h->msg_length = ntohl(msg_h->msg_length);
	msg_h->local_con_id = ntohl(msg_h->local_con_id);
	msg_h->remote_con_id = ntohl(msg_h->remote_con_id);
	msg_h->msg_seq_num = ntohl(msg_h->msg_seq_num);

	//verify minimum size
	if (recvSize < sizeof(struct msg_header)) {
	  info("UDP packet too small, can't be an ML packet");
	  return;
	}

	//TODO add more verifications

	bufptr += MSG_HEADER_SIZE + msg_h->len_mon_packet_hdr;
	msg_size = recvSize - MSG_HEADER_SIZE - msg_h->len_mon_packet_hdr;

	//verify more fields
	if (msg_size < 0) {
	  info("Corrupted UDP packet received");
	  return;
	}

	if(get_Recv_pkt_inf_cb != NULL) {
		mon_pkt_inf msginfNow;
		msginfNow.monitoringHeaderLen = msg_h->len_mon_packet_hdr;
		msginfNow.monitoringHeader = msg_h->len_mon_packet_hdr ? &msgbuf[0] + MSG_HEADER_SIZE : NULL;
		//TODO rethink this ...
		if(msg_h->msg_type == ML_CON_MSG) {
			struct conn_msg *c_msg = (struct conn_msg *) bufptr;
			msginfNow.remote_socketID = &(c_msg->sock_id);
		}
		else if(connectbuf[msg_h->remote_con_id] == NULL) {
			error("ML: received pkg called with non existent connection\n");
			return;
		} else
			msginfNow.remote_socketID = &(connectbuf[msg_h->remote_con_id]->external_socketID);
		msginfNow.buffer = bufptr;
		msginfNow.bufSize = recvSize;
		msginfNow.msgtype = msg_h->msg_type;
		msginfNow.ttl = ttl;
		msginfNow.dataID = msg_h->msg_seq_num;
		msginfNow.offset = msg_h->offset;
		msginfNow.datasize = msg_h->msg_length;
		gettimeofday(&msginfNow.arrival_time, NULL);
		(get_Recv_pkt_inf_cb) ((void *) &msginfNow);
	}


	switch(msg_h->msg_type) {
		case ML_CON_MSG:
			debug("ML: received conn pkg\n");
			recv_conn_msg(msg_h, bufptr, msg_size, &recv_addr);
			break;
		default:
			if(msg_h->msg_type < 127) {
				debug("ML: received data pkg\n");
				recv_data_msg(msg_h, bufptr, msg_size);
				break;
			}
			debug("ML: unrecognised msg_type\n");
			break;
	}
}

/*
 * compare the external IP address of two socketIDs
 */
int
compare_external_address_socketIDs(socketID_handle sock1, socketID_handle sock2)
{
	if( sock1->external_addr.udpaddr.sin_addr.s_addr == sock2->external_addr.udpaddr.sin_addr.s_addr)
		return 0;
	return 1;
}

void try_stun();

/*
 * the timeout of the NAT traversal
 */
void nat_traversal_timeout(int fd, short event, void *arg)
{
	if (NAT_traversal == false) {
		debug("ML: NAT traversal request re-send\n");
		if(receive_SocketID_cb)
			(receive_SocketID_cb) (&local_socketID, 2);
		try_stun();
	}
}

//return IP address, or INADDR_NONE if can't resolve
unsigned long resolve(const char *ipaddr)
{
	struct hostent *h = gethostbyname(ipaddr);
	if (!h) {
		error("ML: Unable to resolve host name %s\n", ipaddr);
		return INADDR_NONE;
	}
	unsigned long *addr = (unsigned long *) (h->h_addr);
	return *addr;
}


/*
 * returns the file descriptor, or <0 on error. The ipaddr can be a null
 * pointer. Then all available ipaddr on the machine are choosen.
 */
int create_socket(const int port, const char *ipaddr)
{
	struct sockaddr_in udpaddr = {0};
	udpaddr.sin_family = AF_INET;
	if (ipaddr == NULL) {
		/*
		* try to guess the local IP address
		*/
		const char *ipaddr_iface = mlAutodetectIPAddress();
		if (ipaddr_iface) {
			udpaddr.sin_addr.s_addr = inet_addr(ipaddr_iface);
		} else {
			udpaddr.sin_addr.s_addr = INADDR_ANY;
		}
	} else {
		udpaddr.sin_addr.s_addr = inet_addr(ipaddr);
	}
	udpaddr.sin_port = htons(port);

	socketaddrgen udpgen;
	memset(&udpgen,0,sizeof(socketaddrgen));	//this will be sent over the net, so set it to 0
	udpgen.udpaddr = udpaddr;
	local_socketID.internal_addr = udpgen;

	socketfd = createSocket(port, ipaddr);
	if (socketfd < 0){
		return socketfd;
        }

	struct event *ev;
	ev = event_new(base, socketfd, EV_READ | EV_PERSIST, recv_pkg, NULL);

	event_add(ev, NULL);

	try_stun();

	return socketfd;
}

/*
 * try to figure out external IP using STUN, if defined
 */
void try_stun()
{
	if (isStunDefined()) {
		/*
		* send the NAT traversal STUN request
		*/
		 send_stun_request(socketfd, &stun_server);

		/*
		* enter a NAT traversal timeout that takes care of retransmission
		*/
		struct event *ev1;
		struct timeval timeout_value_NAT_traversal = NAT_TRAVERSAL_TIMEOUT;
		ev1 = evtimer_new(base, nat_traversal_timeout, NULL);
		event_add(ev1, &timeout_value_NAT_traversal);

		NAT_traversal = false;
	} else {
		/*
		* Assume we have accessibility and copy internal address to external one
		*/
		local_socketID.external_addr = local_socketID.internal_addr;
		NAT_traversal = true; // @TODO: this is not really NAT traversal, but a flag that init is over
		// callback to the upper layer indicating that the socketID is now
		// ready to use
		if(receive_SocketID_cb)
			(receive_SocketID_cb) (&local_socketID, 0); //success
	}
}

/**************************** END OF INTERNAL ***********************/

/**************************** MONL functions *************************/

int mlInit(bool recv_data_cb,struct timeval timeout_value,const int port,const char *ipaddr,const int stun_port,const char *stun_ipaddr,receive_localsocketID_cb local_socketID_cb,void *arg){

	base = (struct event_base *) arg;
	recv_data_callback = recv_data_cb;
	mlSetRecvTimeout(timeout_value);
	if (stun_ipaddr) {
		 mlSetStunServer(stun_port, stun_ipaddr);
	} else {

	}
	register_recv_localsocketID_cb(local_socketID_cb);
	return create_socket(port, ipaddr);
}

void mlSetThrottle(int bucketsize, int drainrate) {
        setOutputRateParams(bucketsize, drainrate);
}
     

/* register callbacks  */
void mlRegisterGetRecvPktInf(get_recv_pkt_inf_cb recv_pkt_inf_cb){

	if (recv_pkt_inf_cb == NULL) {
		error("ML: Register get_recv_pkt_inf_cb failed: NULL ptr  \n");
	} else {
		get_Recv_pkt_inf_cb = recv_pkt_inf_cb;
	}
}

void mlRegisterGetSendPktInf(get_send_pkt_inf_cb  send_pkt_inf_cb){

	if (send_pkt_inf_cb == NULL) {
		error("ML: Register get_send_pkt_inf_cb: NULL ptr  \n");
	} else {
		get_Send_pkt_inf_cb = send_pkt_inf_cb;
	}
}


void mlRegisterSetMonitoringHeaderPktCb(set_monitoring_header_pkt_cb monitoring_header_pkt_cb ){

	if (monitoring_header_pkt_cb == NULL) {
		error("ML: Register set_monitoring_header_pkt_cb: NULL ptr  \n");
	} else {
		set_Monitoring_header_pkt_cb = monitoring_header_pkt_cb;
	}
}

void mlRegisterGetRecvDataInf(get_recv_data_inf_cb recv_data_inf_cb){

	if (recv_data_inf_cb == NULL) {
		error("ML: Register get_recv_data_inf_cb: NULL ptr  \n");
	} else {
		get_Recv_data_inf_cb = recv_data_inf_cb;
	}
}

void mlRegisterGetSendDataInf(get_send_data_inf_cb  send_data_inf_cb){

	if (send_data_inf_cb == NULL) {
		error("ML: Register get_send_data_inf_cb: NULL ptr  \n");
	} else {
		get_Send_data_inf_cb = send_data_inf_cb;
	}
}

void mlRegisterSetMonitoringHeaderDataCb(set_monitoring_header_data_cb monitoring_header_data_cb){

	if (monitoring_header_data_cb == NULL) {
		error("ML: Register set_monitoring_header_data_cb : NULL ptr  \n");
	} else {
		set_Monitoring_header_data_cb = monitoring_header_data_cb;
	}
}

void mlSetRecvTimeout(struct timeval timeout_value){

	recv_timeout = timeout_value;

}

int mlGetStandardTTL(socketID_handle socketID,uint8_t *ttl){

	return getTTL(socketfd, ttl);

}

socketID_handle mlGetLocalSocketID(int *errorstatus){

	if (NAT_traversal == false) {
		*errorstatus = 2;
		return NULL;
	}

	*errorstatus = 0;
	return &local_socketID;

}


/**************************** END of MONL functions *************************/

/**************************** GENERAL functions *************************/

void mlRegisterRecvConnectionCb(receive_connection_cb recv_conn_cb){

	if (recv_conn_cb == NULL) {
		error("ML: Register receive_connection_cb: NULL ptr  \n");
	}else {
		receive_Connection_cb = recv_conn_cb;
	}
}

void mlRegisterErrorConnectionCb(connection_failed_cb conn_failed){

	if (conn_failed == NULL) {
		error("ML: Register connection_failed_cb: NULL ptr  \n");
	} else {
		failed_Connection_cb = conn_failed;
	}
}

void mlRegisterRecvDataCb(receive_data_cb data_cb,unsigned char msgtype){

    if (msgtype > 126) {

    	error
	    ("ML: Could not register recv_data callback. Msgtype is greater then 126 \n");

    }

    if (data_cb == NULL) {

    	error("ML: Register receive data callback: NUll ptr \n ");

    } else {

	recvcbbuf[msgtype] = data_cb;

    }

}

void mlCloseSocket(socketID_handle socketID){

	free(socketID);

}

void keepalive_fn(evutil_socket_t fd, short what, void *arg) {
	socketID_handle peer = arg;

	int con_id = mlConnectionExist(peer, false);
	if (con_id < 0 || connectbuf[con_id]->defaultSendParams.keepalive <= 0) {
		/* Connection fell from under us or keepalive was disabled */
		free(arg);
		return;
	}

	/* do what we gotta do */
	if ( connectbuf[con_id]->status == READY) {
		char keepaliveMsg[32] = "";
		sprintf(keepaliveMsg, "KEEPALIVE %d", connectbuf[con_id]->keepalive_seq++);
		send_msg(con_id, MSG_TYPE_ML_KEEPALIVE, keepaliveMsg, 1 + strlen(keepaliveMsg), false, 
			&(connectbuf[con_id]->defaultSendParams));
	}

	/* re-schedule */
	struct timeval t = { 0,0 };
	t.tv_sec = connectbuf[con_id]->defaultSendParams.keepalive;
	if (connectbuf[con_id]->defaultSendParams.keepalive) 
		event_base_once(base, -1, EV_TIMEOUT, keepalive_fn, peer, &t);
}

void setupKeepalive(int conn_id) {
	/* Save the peer's address for us */
	socketID_handle peer = malloc(sizeof(socket_ID));
	memcpy(peer, &connectbuf[conn_id]->external_socketID, sizeof(socket_ID));

	struct timeval t = { 0,0 };
	t.tv_sec = connectbuf[conn_id]->defaultSendParams.keepalive;

	if (connectbuf[conn_id]->defaultSendParams.keepalive) 
		event_base_once(base, -1, EV_TIMEOUT, keepalive_fn, peer, &t);
}

/* connection functions */
int mlOpenConnection(socketID_handle external_socketID,receive_connection_cb connection_cb,void *arg, const send_params defaultSendParams){

	int con_id;
	if (external_socketID == NULL) {
		error("ML: cannot open connection: one of the socketIDs is NULL\n");
		return -1;
	}
	if (NAT_traversal == false) {
		error("ML: cannot open connection: NAT traversal for socketID still in progress\n");
		return -1;
	}
	if (connection_cb == NULL) {
		error("ML: cannot open connection: connection_cb is NULL\n");
		return -1;
	}

	// check if that connection already exist

	con_id = mlConnectionExist(external_socketID, false);
	if (con_id >= 0) {
		// overwrite defaultSendParams
		bool newKeepalive = 
			connectbuf[con_id]->defaultSendParams.keepalive == 0 && defaultSendParams.keepalive != 0;
		connectbuf[con_id]->defaultSendParams = defaultSendParams;
		if (newKeepalive) setupKeepalive(con_id);
		// if so check if it is ready to use
		if (connectbuf[con_id]->status == READY) {
				// if so use the callback immediately
				(connection_cb) (con_id, arg);

		// otherwise just write the connection cb and the arg pointer
		// into the connection struct
		} else {
			struct receive_connection_cb_list *temp;
			temp = malloc(sizeof(struct receive_connection_cb_list));
			temp->next = NULL;
			temp->connection_cb = connection_cb;
			temp->arg = arg;
			if(connectbuf[con_id]->connection_last != NULL) {
				connectbuf[con_id]->connection_last->next = temp;
				connectbuf[con_id]->connection_last = temp;
			} else
				connectbuf[con_id]->connection_last = connectbuf[con_id]->connection_head = temp;
		}
		return con_id;
	}
	// make entry in connection_establishment array
	for (con_id = 0; con_id < CONNECTBUFSIZE; con_id++) {
		if (connectbuf[con_id] == NULL) {
			connectbuf[con_id] = (connect_data *) malloc(sizeof(connect_data));
			memset(connectbuf[con_id],0,sizeof(connect_data));
			connectbuf[con_id]->starttime = time(NULL);
			memcpy(&connectbuf[con_id]->external_socketID, external_socketID, sizeof(socket_ID));
			connectbuf[con_id]->pmtusize = DSLSLIM;
			connectbuf[con_id]->timeout_event = NULL;
			connectbuf[con_id]->status = INVITE;
			connectbuf[con_id]->seqnr = 0;
			connectbuf[con_id]->internal_connect = !compare_external_address_socketIDs(external_socketID, &local_socketID);
			connectbuf[con_id]->connectionID = con_id;

			connectbuf[con_id]->connection_head = connectbuf[con_id]->connection_last = malloc(sizeof(struct receive_connection_cb_list));
			connectbuf[con_id]->connection_last->next = NULL;
			connectbuf[con_id]->connection_last->connection_cb = connection_cb;
			connectbuf[con_id]->connection_last->arg = arg;
			connectbuf[con_id]->external_connectionID = -1;

			connectbuf[con_id]->defaultSendParams = defaultSendParams;
			if (defaultSendParams.keepalive) setupKeepalive(con_id);
			break;
		}
	} //end of for

	if (con_id == CONNECTBUFSIZE) {
		error("ML: Could not open connection: connection buffer full\n");
		return -1;
	}

	// create and send a connection message
	info("ML:Sending INVITE to %s (lconn:%d)\n",conid_to_string(con_id), con_id);
	send_conn_msg_with_pmtu_discovery(con_id, connectbuf[con_id]->pmtusize, INVITE);

	return con_id;

}

void mlCloseConnection(const int connectionID){

	// remove it from the connection array
	if(connectbuf[connectionID]) {
		if(connectbuf[connectionID]->ctrl_msg_buf) {
			free(connectbuf[connectionID]->ctrl_msg_buf);
		}
		// remove related events
		if (connectbuf[connectionID]->timeout_event) {
			event_del(connectbuf[connectionID]->timeout_event);
			event_free(connectbuf[connectionID]->timeout_event);
			connectbuf[connectionID]->timeout_event = NULL;
		}
		free(connectbuf[connectionID]);
		connectbuf[connectionID] = NULL;
	}

}

void mlSendData(const int connectionID,char *sendbuf,int bufsize,unsigned char msgtype,send_params *sParams){

	if (connectionID < 0) {
		error("ML: send data failed: connectionID does not exist\n");
		return;
	}

	if (connectbuf[connectionID] == NULL) {
		error("ML: send data failed: connectionID does not exist\n");
		return;
	}
	if (connectbuf[connectionID]->status != READY) {
	    error("ML: send data failed: connection is not active\n");
	    return;
	}

	if (sParams == NULL) {
		sParams = &(connectbuf[connectionID]->defaultSendParams);
	}

	send_msg(connectionID, msgtype, sendbuf, bufsize, false, sParams);

}

/* transmit data functions  */
int mlSendAllData(const int connectionID,send_all_data_container *container,int nr_entries,unsigned char msgtype,send_params *sParams){

    if (nr_entries < 1 || nr_entries > 5) {

	error
	    ("ML : sendALlData : nr_enties is not between 1 and 5 \n ");
	return 0;

    } else {

	if (nr_entries == 1) {

		mlSendData(connectionID, container->buffer_1,
		      container->length_1, msgtype, sParams);

	    return 1;

	} else if (nr_entries == 2) {

	    int buflen = container->length_1 + container->length_2;
	    char buf[buflen];
	    memcpy(buf, container->buffer_1, container->length_1);
	    memcpy(&buf[container->length_1], container->buffer_2,
		   container->length_2);
	    mlSendData(connectionID, buf, buflen, msgtype, sParams);

	    return 1;

	} else if (nr_entries == 3) {

	    int buflen =
		container->length_1 + container->length_2 +
		container->length_3;
	    char buf[buflen];
	    memcpy(buf, container->buffer_1, container->length_1);
	    memcpy(&buf[container->length_1], container->buffer_2,
		   container->length_2);
	    memcpy(&buf[container->length_2], container->buffer_3,
		   container->length_3);
	    mlSendData(connectionID, buf, buflen, msgtype, sParams);


	    return 1;

	} else if (nr_entries == 4) {

	    int buflen =
		container->length_1 + container->length_2 +
		container->length_3 + container->length_4;
	    char buf[buflen];
	    memcpy(buf, container->buffer_1, container->length_1);
	    memcpy(&buf[container->length_1], container->buffer_2,
		   container->length_2);
	    memcpy(&buf[container->length_2], container->buffer_3,
		   container->length_3);
	    memcpy(&buf[container->length_3], container->buffer_4,
		   container->length_4);
	    mlSendData(connectionID, buf, buflen, msgtype, sParams);

	    return 1;

	} else {

	    int buflen =
		container->length_1 + container->length_2 +
		container->length_3 + container->length_4 +
		container->length_5;
	    char buf[buflen];
	    memcpy(buf, container->buffer_1, container->length_1);
	    memcpy(&buf[container->length_1], container->buffer_2,
		   container->length_2);
	    memcpy(&buf[container->length_2], container->buffer_3,
		   container->length_3);
	    memcpy(&buf[container->length_3], container->buffer_4,
		   container->length_4);
	    memcpy(&buf[container->length_4], container->buffer_5,
		   container->length_5);
	    mlSendData(connectionID, buf, buflen, msgtype, sParams);

	    return 1;
	}

    }

}

int mlRecvData(const int connectionID,char *recvbuf,int *bufsize,recv_params *rParams){

	//TODO yet to be converted
	return 0;
#if 0
	if (rParams == NULL) {
		error("ML: recv_data failed: recv_params is a NULL ptr\n");
		return 0;
    } else {

	info("ML: recv data called \n");

	int i = 0;
	int returnValue = 0;
	double timeout = (double) recv_timeout.tv_sec;
	time_t endtime = time(NULL);

	for (i = 0; i < RECVDATABUFSIZE; i++) {

	    if (recvdatabuf[i] != NULL) {

		if (recvdatabuf[i]->connectionID == connectionID) {

		    info("ML: recv data has entry  \n");

		    double timepass = difftime(endtime, recvdatabuf[i]->starttime);

		    // check if the specified connection has data and it
		    // is complete
		    // check the data seqnr
		    // if(connectionID == recvdatabuf[i]->connectionID &&
		    // 1 == recvdatabuf[i]->status){

		    if (1 == recvdatabuf[i]->status) {

			// info("transmissionHandler: recv_data set is
			// complete \n" );

			// debug("debud \n");

			// exchange the pointers
			int buffersize = 0;
			buffersize = recvdatabuf[i]->bufsize;
			*bufsize = buffersize;
			// recvbuf = recvdatabuf[i]->recvbuf;

			// info("buffersize %d \n",buffersize);
			memcpy(recvbuf, recvdatabuf[i]->recvbuf,
			       buffersize);
			// debug(" recvbuf %s \n",recvbuf );

// 			double nrMissFrags =
// 			    (double) recvdatabuf[i]->nrFragments /
// 			    (double) recvdatabuf[i]->recvFragments;
// 			int nrMissingFragments = (int) ceil(nrMissFrags);

//			rParams->nrMissingFragments = nrMissingFragments;
// 			rParams->nrFragments = recvdatabuf[i]->nrFragments;
			rParams->msgtype = recvdatabuf[i]->msgtype;
			rParams->connectionID =
			    recvdatabuf[i]->connectionID;

			// break from the loop
			// debug(" recvbuf %s \n ",recvbuf);

			// double nrMissFrags =
			// (double)recvdatabuf[i]->nrFragments /
			// (double)recvdatabuf[i]->recvFragments;
			// int nrMissingFragments =
			// (int)ceil(nrMissFrags);

			if(get_Recv_data_inf_cb != NULL) {
				mon_data_inf recv_data_inf;

				recv_data_inf.remote_socketID = &(connectbuf[connectionID]->external_socketID);
				recv_data_inf.buffer = recvdatabuf[i]->recvbuf;
				recv_data_inf.bufSize = recvdatabuf[i]->bufsize;
				recv_data_inf.msgtype = recvdatabuf[i]->msgtype;
// 				recv_data_inf.monitoringHeaderType = recvdatabuf[i]->monitoringHeaderType;
// 				recv_data_inf.monitoringDataHeader = recvdatabuf[i]->monitoringDataHeader;
				gettimeofday(&recv_data_inf.arrival_time, NULL);
				recv_data_inf.firstPacketArrived = recvdatabuf[i]->firstPacketArrived;
				recv_data_inf.nrMissingFragments = nrMissingFragments;
				recv_data_inf.nrFragments = recvdatabuf[i]->nrFragments;
				recv_data_inf.priority = false;
				recv_data_inf.padding = false;
				recv_data_inf.confirmation = false;
				recv_data_inf.reliable = false;

				// send data recv callback to monitoring module

				(get_Recv_data_inf_cb) ((void *) &recv_data_inf);
			}


			// free the allocated memory
			free(recvdatabuf[i]);
			recvdatabuf[i] = NULL;

			returnValue = 1;
			break;

		    }

		    if (recvdatabuf[i] != NULL) {

			if (timepass > timeout) {

			    info("ML: recv_data timeout called  \n");

			    // some data about the missing chunks should
			    // be added here
			    // exchange the pointers
			    int buffersize = 0;
			    buffersize = recvdatabuf[i]->bufsize;
			    *bufsize = buffersize;
			    // recvbuf = recvdatabuf[i]->recvbuf;

			    double nrMissFrags =
				(double) recvdatabuf[i]->nrFragments /
				(double) recvdatabuf[i]->recvFragments;
			    int nrMissingFragments =
				(int) ceil(nrMissFrags);

			    // debug(" recvbuf %s \n",recvbuf );

			    memcpy(recvbuf, recvdatabuf[i]->recvbuf,
				   buffersize);

			    rParams->nrMissingFragments =
				nrMissingFragments;
			    rParams->nrFragments =
				recvdatabuf[i]->nrFragments;
			    rParams->msgtype = recvdatabuf[i]->msgtype;
			    rParams->connectionID =
				recvdatabuf[i]->connectionID;

				if(get_Recv_data_inf_cb != NULL) {
					mon_data_inf recv_data_inf;

					recv_data_inf.remote_socketID = &(connectbuf[connectionID]->external_socketID);
					recv_data_inf.buffer = recvdatabuf[i]->recvbuf;
					recv_data_inf.bufSize = recvdatabuf[i]->bufsize;
					recv_data_inf.msgtype = recvdatabuf[i]->msgtype;
					recv_data_inf.monitoringHeaderType = recvdatabuf[i]->monitoringHeaderType;
					recv_data_inf.monitoringDataHeader = recvdatabuf[i]->monitoringDataHeader;
					gettimeofday(&recv_data_inf.arrival_time, NULL);
					recv_data_inf.firstPacketArrived = recvdatabuf[i]->firstPacketArrived;
					recv_data_inf.nrMissingFragments = nrMissingFragments;
					recv_data_inf.nrFragments = recvdatabuf[i]->nrFragments;
					recv_data_inf.priority = false;
					recv_data_inf.padding = false;
					recv_data_inf.confirmation = false;
					recv_data_inf.reliable = false;

					// send data recv callback to monitoring module

					(get_Recv_data_inf_cb) ((void *) &recv_data_inf);
				}

			    // free the allocated memory
			    free(recvdatabuf[i]);
			    recvdatabuf[i] = NULL;

			    returnValue = 1;
			    break;

			}
		    }

		}

	    }
	    // debug("2 recvbuf %s \n ",recvbuf);
	}
	return returnValue;
    }
#endif

}

int mlSocketIDToString(socketID_handle socketID,char* socketID_string, size_t len){

	char internal_addr[INET_ADDRSTRLEN];
	char external_addr[INET_ADDRSTRLEN];

	assert(socketID);

	inet_ntop(AF_INET, &(socketID->internal_addr.udpaddr.sin_addr.s_addr), internal_addr, INET_ADDRSTRLEN);
	inet_ntop(AF_INET, &(socketID->external_addr.udpaddr.sin_addr.s_addr), external_addr, INET_ADDRSTRLEN);

	snprintf(socketID_string,len,"%s:%d-%s:%d", internal_addr, ntohs(socketID->internal_addr.udpaddr.sin_port),
		external_addr,	ntohs(socketID->external_addr.udpaddr.sin_port));
	return 0;

}

int mlStringToSocketID(const char* socketID_string, socketID_handle socketID){

	//@TODO add checks against malformed string
	char external_addr[INET_ADDRSTRLEN];
	int external_port;
	char internal_addr[INET_ADDRSTRLEN];
	int internal_port;

	char *pch;
	char *s = strdup(socketID_string);

	//replace ':' with a blank
	pch=strchr(s,':');
	while (pch!=NULL){
				*pch = ' ';
		pch=strchr(pch+1,':');
	}
	pch=strchr(s,'-');
	if(pch) *pch = ' ';

	sscanf(s,"%s %d %s %d", internal_addr, &internal_port,
		external_addr, &external_port);

	//set structure to 0, we initialize each byte, since it will be sent on the net later
	memset(socketID, 0, sizeof(struct _socket_ID));

	if(inet_pton(AF_INET, internal_addr, &(socketID->internal_addr.udpaddr.sin_addr)) == 0)
		return EINVAL;
	socketID->internal_addr.udpaddr.sin_family = AF_INET;
	socketID->internal_addr.udpaddr.sin_port = htons(internal_port);


	if(inet_pton(AF_INET, external_addr, &(socketID->external_addr.udpaddr.sin_addr)) ==0)
		return EINVAL;
	socketID->external_addr.udpaddr.sin_family = AF_INET;
	socketID->external_addr.udpaddr.sin_port = htons(external_port);

	free(s);
	return 0;

}

int mlGetConnectionStatus(int connectionID){

	if(connectbuf[connectionID])
		return connectbuf[connectionID]->status == READY;
	return -1;
    
}


int mlConnectionExist(socketID_handle socketID, bool ready){

    /*
     * check if another connection for the external connectionID exist
     * that was established \ within the last 2 seconds
     */
	int i;
	for (i = 0; i < CONNECTBUFSIZE; i++)
		if (connectbuf[i] != NULL)
			if (mlCompareSocketIDs(&(connectbuf[i]->external_socketID), socketID) == 0) {
				if (ready) return (connectbuf[i]->status == READY ? i : -1);;
				return i;
				}

    return -1;

}

//Added by Robert Birke as comodity functions

//int mlPrintSocketID(socketID_handle socketID) {
//	char str[SOCKETID_STRING_SIZE];
//	mlSocketIDToString(socketID, str, sizeof(str));
//	printf(stderr,"int->%s<-ext\n",str);
//}

/*
 * hash code of a socketID
 * TODO might think of a better way
 */
int mlHashSocketID(socketID_handle sock) {
	//assert(sock);
   return sock->internal_addr.udpaddr.sin_port +
			sock->external_addr.udpaddr.sin_port;
}

int mlCompareSocketIDs(socketID_handle sock1, socketID_handle sock2) {

	assert(sock1 && sock2);

	/*
	* compare internal addr
	*/
	if(sock1 == NULL || sock2 == NULL)
		return 1;

	if (sock1->internal_addr.udpaddr.sin_addr.s_addr !=
	    sock2->internal_addr.udpaddr.sin_addr.s_addr)
			return 1;

	if (sock1->internal_addr.udpaddr.sin_port !=
		 sock2->internal_addr.udpaddr.sin_port)
			return 1;

	/*
	* compare external addr
	*/
	if (sock1->external_addr.udpaddr.sin_addr.s_addr !=
	    sock2->external_addr.udpaddr.sin_addr.s_addr)
			return 1;

	if (sock1->external_addr.udpaddr.sin_port !=
		 sock2->external_addr.udpaddr.sin_port)
			return 1;

	return 0;
}

int mlCompareSocketIDsByPort(socketID_handle sock1, socketID_handle sock2)
{
	if(sock1 == NULL || sock2 == NULL)
		return 1;
 
	if (sock1->internal_addr.udpaddr.sin_port !=
		 sock2->internal_addr.udpaddr.sin_port)
			return 1;

	if (sock1->external_addr.udpaddr.sin_port !=
		 sock2->external_addr.udpaddr.sin_port)
			return 1;
	return 0;
}

int mlGetPathMTU(int ConnectionId) {
	if(ConnectionId < 0 || ConnectionId >= CONNECTBUFSIZE)
		return -1;
	if (connectbuf[ConnectionId] != NULL)
		return connectbuf[ConnectionId]->pmtusize;
	return -1;
}

/**************************** END of GENERAL functions *************************/

/**************************** NAT functions *************************/

/* setter  */
void mlSetStunServer(const int port,const char *ipaddr){

	stun_server.sin_family = AF_INET;
	if (ipaddr == NULL)
		stun_server.sin_addr.s_addr = htonl(INADDR_NONE);
	else
		stun_server.sin_addr.s_addr = resolve(ipaddr);
	stun_server.sin_port = htons(port);

}

int mlGetExternalIP(char* external_addr){

	socketaddrgen udpgen;
	struct sockaddr_in udpaddr;

	udpgen = local_socketID.external_addr;
	udpaddr = udpgen.udpaddr;

	inet_ntop(AF_INET, &(udpaddr.sin_addr), external_addr,
			INET_ADDRSTRLEN);

	if (external_addr == NULL) {

	return -1;

	} else {

	return 0;

	}

}

/**************************** END of NAT functions *************************/