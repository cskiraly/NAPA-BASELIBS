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
 * @file udpSocket.h 
 * @brief This file contains a udp socket abstraction. 
 *
 * The udpSocket is in the current implementation an abstraction for the linux operating system and developed with ubuntu version 8.04. Should it be neccassy to run the messaging layer under another OS the udpSocket would have to be replaced by an implementation for that operating system. 
 *
 * @author Kristian Beckers  <beckers@nw.neclab.eu> 
 * @author Sebastian Kiesel  <kiesel@nw.neclab.eu>
 * 
 * @date 7/28/2009
 */



#ifndef UDPSOCKET_H
#define UDPSOCKET_H

#include <string.h>
#include <netinet/in.h>

/**
 * The maximum buffer size for a send or received packet.  
 * The value is set to the maximal pmtu size.  
 */
#define MAXBUF 2000

/// @{
/**
  * socket error variable: No error occurred  
  */
#define SO_EE_ORIGIN_NONE       0

/**
  * Socket error variable: local error
  */
#define SO_EE_ORIGIN_LOCAL      1

/**
  * Socket error variable: icmp message received  
  */
#define SO_EE_ORIGIN_ICMP       2

/**
  * Socket error variable: icmp version 6 message received  
  */
#define SO_EE_ORIGIN_ICMP6      3
/// @}

typedef enum {OK = 0, MSGLEN, FAILURE} error_codes;

/** 
 * A callback functions for received pmtu errors (icmp packets type 3 code 4) 
 * @param buf TODO
 * @param bufsize TODO
 */
typedef void(*icmp_error_cb)(char *buf,int bufsize);

/** 
 * Create a messaging layer socket 
 * @param port The port of the socket
 * @param ipaddr The ip address of the socket. If left NULL the socket is bound to all local interfaces (INADDR_ANY).
 * @return The udpSocket file descriptor that the OS assigned. <0 on error
 */
int createSocket(const int port,const char *ipaddr);

/** 
 * A function to get the standard TTL from the operating system. 
 * @param udpSocket The file descriptor of the udpSocket. 
 * @param *ttl A pointer to an int that is used to store the ttl information. 
 * @return 1 if the operation is successful and 0 if a mistake occured. 
 */
int getTTL(const int udpSocket,uint8_t *ttl);

/** 
 * Sends a udp packet.
 * @param udpSocket The udpSocket file descriptor.
 * @param *buffer A pointer to the send buffer. 
 * @param bufferSize The size of the send buffer. 
 * @param *socketaddr The address of the remote socket
 */
int sendPacket(const int udpSocket, struct iovec *iov, int len, struct sockaddr_in *socketaddr);

/**
 * Receive a udp packet
 * @param udpSocket The udpSocket file descriptor.
 * @param *buffer A pointer to the receive buffer.
 * @param *recvSize A pointer to an int that is used to store the size of the recv buffer. 
 * @param *udpdst The socket address from the remote socket that send the packet.
 * @param icmpcb_value A function pointer to a callback function from type icmp_error_cb.   
 * @param *ttl A pointer to an int that is used to store the ttl information.
 */
void recvPacket(const int udpSocket,char *buffer,int *recvSize,struct sockaddr_in *udpdst,icmp_error_cb icmpcb_value,int *ttl);

/** 
 * This function is used for Error Handling. If an icmp packet is received it processes the packet. 
 * @param udpSocket The udpSocket file descriptor
 * @param iofunc An integer that states if the function was called from sendPacket or recvPaket. The value for sendPacket is 1 and for recvPacket 2.
 * @param *buf A pointer to the send or receive buffer
 * @param *bufsize A pointer to an int that ha the size of buf
 * @param *addr The address of the remote socket
 * @param icmpcb_value A function pointer to a callback function from type icmp_error_cb
 * @param *ttl A pointer to an int that is used to store the ttl information
 * @return 0 if everything went well and -1 if an error occured
 */
int handleSocketError(const int udpSocket,const int iofunc,char *buf,int *bufsize,struct sockaddr_in *addr,icmp_error_cb icmpcb_value,int *ttl);

/** 
 * This function closes a socket  
 * @param udpSocket The file descriptor of the udp socket 
 * @return 0 if everything went well and -1 if an error occured 
 */
int closeSocket(const int udpSocket);

#endif
