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

#include "../ml_all.h"

#ifdef __linux__
#include <linux/types.h>
#include <linux/errqueue.h>
#include <linux/if.h>
#include <ifaddrs.h>
#else
#define MSG_ERRQUEUE 0
#endif



/* debug varible: set to 1 if you want debug output  */
int verbose = 0;

int createSocket(const int port,const char *ipaddr)
{
  /* variables needed */
  struct sockaddr_in udpsrc, udpdst;

  int returnStatus = 0;
  debug("X.CreateSock %s %d\n",ipaddr, port);

  bzero((char *)&udpsrc, sizeof udpsrc);
  bzero((char *)&udpdst, sizeof udpsrc);

  //int udpSocket = 0;
  /*libevent2*/
  evutil_socket_t udpSocket;

  /* create a socket */
  udpSocket = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);

  /*libevent2*/
  evutil_make_socket_nonblocking(udpSocket);

  //This is debug code that checks if the socket was created
  if (udpSocket == -1)
    {
      error("Could not create an UDP socket! ERRNO %d\n",errno);
      return -1;
    }

  /* sets the standard server address and port */
  udpsrc.sin_family = AF_INET;
  udpsrc.sin_addr.s_addr = htonl(INADDR_ANY);
  udpsrc.sin_port = 0;

  /* sets a destinantion address  */
  udpdst.sin_family = AF_INET;
  udpdst.sin_addr.s_addr = htonl(INADDR_ANY);
  udpdst.sin_port = 0;

  /* set the address  */
  if (ipaddr == NULL)
    {
      udpsrc.sin_port = htons(port);
    }else{
      udpsrc.sin_addr.s_addr = inet_addr(ipaddr);
      udpsrc.sin_port = htons(port);
    }

  /* bind to the socket */
  returnStatus = bind(udpSocket,(struct sockaddr *)&udpsrc,sizeof(udpsrc));

  /* This is debug code */
  if (returnStatus) {
    error("Could not bind socketID to address! ERRNO %d\n",errno);
    return -2;
  }
#ifdef IP_MTU_DISCOVER

  int yes = 1;
  int size = sizeof(int);
  int pmtuopt = IP_PMTUDISC_DO;

  /*Setting the automatic PMTU discoery function from the socket
   * This sets the DON'T FRAGMENT bit on the IP HEADER
   */
  if(setsockopt(udpSocket,IPPROTO_IP,IP_MTU_DISCOVER,&pmtuopt ,size) == -1){
    error("setsockopt: set IP_DONTFRAG did not work. ERRNO %d\n",errno);

  }

  /* This option writes the IP_TTL field into ancillary data */
  if(setsockopt(udpSocket,IPPROTO_IP, IP_RECVTTL, &yes,size) < 0)
    {
      error("setsockopt: cannot set RECV_TTL. ERRNO %d\n",errno);
    }

  /* This option writes received internal and external(icmp) error messages into ancillary data */

  if(setsockopt(udpSocket,IPPROTO_IP, IP_RECVERR, &yes,size) < 0) {
	error("setsockopt: cannot set RECV_ERROR. ERRNO %d\n",errno);
    }

  /* Increase UDP receive buffer */

#endif

#ifdef _WIN32 //TODO: verify whether this fix is needed on other OSes. Verify why this is needed on Win (libevent?)
  int intval = 64*1024;
  if(setsockopt(udpSocket,SOL_SOCKET, SO_RCVBUF, (char *) &intval,sizeof(intval)) < 0) {
	error("setsockopt: cannot set SO_RECVBUF. ERRNO %d\n",errno);
    }

  if(setsockopt(udpSocket,SOL_SOCKET, SO_SNDBUF, (char *) &intval,sizeof(intval)) < 0) {
	error("setsockopt: cannot set SO_RECVBUF. ERRNO %d\n",errno);
    }
#endif

  debug("X.CreateSock\n");
  return udpSocket;

}

#ifndef _WIN32
/* Information: read the standard TTL from a socket  */
int getTTL(const int udpSocket,uint8_t *ttl){
#ifdef MAC_OS
	return 0;
#else

	unsigned int value;
	unsigned int size = sizeof(value);

	if(getsockopt(udpSocket,SOL_IP,IP_TTL,&value,&size) == -1){
		error("get TTL did not work\n");
		return 0;
	}

	*ttl = value;
	if(verbose == 1) debug("TTL is %i\n",value);

	return 1;
#endif
}

int sendPacketFinal(const int udpSocket, struct iovec *iov, int len, struct sockaddr_in *socketaddr)
{
	int error, ret;
	struct msghdr msgh;

	msgh.msg_name = socketaddr;
	msgh.msg_namelen = sizeof(struct sockaddr_in);
	msgh.msg_iov = iov;
	msgh.msg_iovlen = len;
	msgh.msg_flags = 0;

	/* we do not send ancilliary data  */
	msgh.msg_control = 0;
	msgh.msg_controllen = 0;

	/* send the message  */
	ret = sendmsg(udpSocket,&msgh,0);
	if (ret  < 0){
		error = errno;
		info("ML: sendmsg failed errno %d: %s\n", error, strerror(error));
		switch(error) {
			case EMSGSIZE:
				return MSGLEN;
			default:
				return FAILURE;
		}
	}
	return OK;
}

/* A general error handling function on socket operations
 * that is called when sendmsg or recvmsg report an Error
 *
 */

//This function has to deal with what to do when an icmp is received
//--check the connection array, look to which connection-establishment the icmp belongs
//--invoke a retransmission
int handleSocketError(const int udpSocket,const int iofunc,char *buf,int *bufsize,struct sockaddr_in *addr,icmp_error_cb icmpcb_value,int *ttl){


	if(verbose == 1) debug("handle Socket error is called\n");

	/* variables  */
	struct msghdr msgh;
	struct cmsghdr *cmsg;
	struct sock_extended_err *errptr;
	struct iovec iov;
	struct sockaddr_in sender_addr;
	int returnStatus;
	char errbuf[CMSG_SPACE(1024)];
	int recvbufsize = 1500;
	char recvbuf[recvbufsize];
	int icmp = 0;

	/* initialize recvmsg data  */
	msgh.msg_name = &sender_addr;
	msgh.msg_namelen = sizeof(struct sockaddr_in);
	msgh.msg_iov = &iov;
	msgh.msg_iovlen = 1;
	msgh.msg_iov->iov_base = recvbuf;
	msgh.msg_iov->iov_len = recvbufsize;
	msgh.msg_flags = 0;

	//set the size of the control data
	msgh.msg_control = errbuf;
	msgh.msg_controllen = sizeof(errbuf);
	//initialize pointer
	errptr = NULL;

	/* get the error from the error que:  */
	returnStatus = recvmsg(udpSocket,&msgh,MSG_ERRQUEUE|MSG_DONTWAIT);
	if (returnStatus <= 0) {
		return -1;
	}
	for (cmsg = CMSG_FIRSTHDR(&msgh); cmsg != NULL; cmsg = CMSG_NXTHDR(&msgh,cmsg)){
#ifdef __linux__
		//fill the error pointer
		if (cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_RECVERR) {
			errptr = (struct sock_extended_err *)CMSG_DATA(cmsg);
//			if (errptr == NULL){
//			if(verbose == 1)
//				error("no acillary error data \n");
//	     return -1;
//		}
		/* check if the error originated locally */
			if (errptr->ee_origin == SO_EE_ORIGIN_LOCAL){
				if (errptr->ee_errno != EMSGSIZE) {
					if(verbose == 1)
						error("local error: %s \n", strerror(errptr->ee_errno));
				}
			}
			/* check if the error originated from an icmp message  */
			if (errptr->ee_origin == SO_EE_ORIGIN_ICMP){
				char sender_addr_str[INET_ADDRSTRLEN];
				if(verbose == 1)
					debug("icmp error message received\n");

				int type = errptr->ee_type;
				int code = errptr->ee_code;
				icmp = 1;
				inet_ntop(AF_INET, &(sender_addr.sin_addr.s_addr), sender_addr_str, INET_ADDRSTRLEN);
				warn("icmp error message from %s:%d is type: %d code %d\n",
					sender_addr_str,ntohs(sender_addr.sin_port),
					errptr->ee_type,errptr->ee_code);

				/* raise the pmtu callback when an pmtu error occurred
				*  -> icmp message type 3 code 4 icmp
				*/

				if (type == 3 && code == 4){
					if(verbose == 1)
						debug("pmtu error message received\n");

					int mtusize = *bufsize;
					(icmpcb_value)(buf,mtusize);
				}
			}
		}
#endif
            ;
	} //end of for

	/* after the error is read from the socket error queue the
	* socket operation that was interrupeted by reading the error queue
	* has to be carried out
	*/

	//error("socketErrorHandle: before iofunc loop \n");
/* @TODO commented the next section because some errors are not rcoverable. ie error code EAGAIN means no dtata is available and even if you retry the eroor most probably persists (no dtata arrived in the mena time) causing recorsive callings and subsequent seg fault probably due to stack overflow
better error handling might be needed */
#if 0
	int transbuf;
	memcpy(&transbuf,bufsize,4);

	if(iofunc == 1) {
		sendPacket(udpSocket,buf,transbuf,addr,icmpcb_value);
		if(verbose == 1)
			error("handle socket error: packetsize %i \n ",*bufsize );
	} else {
		if(iofunc == 2 && icmp == 1){
			if(verbose == 1)
				error("handleSOcketError: recvPacket called \n ");
			recvPacket(udpSocket,buf,bufsize,addr,icmpcb_value,ttl);
		} else {
		/* this is the case the socket has just an error message not related to anything of the messaging layer */
			if(verbose == 1)
				error("handleSocketError: unrelated error \n");
			*ttl = -1;
		}
	}
#endif
	return 0;
}



void recvPacket(const int udpSocket,char *buffer,int *recvSize,struct sockaddr_in *udpdst,icmp_error_cb icmpcb_value,int *ttl)
{

	/* variables  */
	struct msghdr msgh; 
	struct cmsghdr *cmsg;
	int *ttlptr;
	int received_ttl;
	struct iovec iov;
	//struct sockaddr_in sender_addr;
	//unsigned int sender_len;
	int returnStatus;
	
	/* initialize recvmsg data  */
	msgh.msg_name = udpdst;
	msgh.msg_namelen = sizeof(struct sockaddr_in);
	msgh.msg_iov = &iov;
	msgh.msg_iovlen = 1;
	msgh.msg_iov->iov_base = buffer;
	msgh.msg_iov->iov_len = *recvSize;
	msgh.msg_flags = 0;
	
	/*
	*  This shows the receiving of TTL within the ancillary data of recvmsg
	*/
	
	// ancilliary data buffer 
	char ttlbuf[CMSG_SPACE(sizeof(int))];
	
	//set the size of the control data
	msgh.msg_control = ttlbuf;
	msgh.msg_controllen = sizeof(ttlbuf);

	returnStatus = recvmsg(udpSocket,&msgh,0);
	msgh.msg_iov->iov_len = returnStatus;

	*recvSize = returnStatus;
	/* receive the message */
	if (returnStatus < 0) {
		if(verbose == 1) {
			error("udpSocket:recvPacket: Read the error queue \n ");
			error("recvmsg failed. errno %d \n",errno);
		}
		// TODO debug code: delete afterwards start
		if(errno == 11) {	//TODO: What is this 11, and why is it here with a NOP?
			int a;
			a++;
		};
		// end
		handleSocketError(udpSocket,2,buffer,recvSize,udpdst,icmpcb_value,ttl);

	} else {
		/* debug code  */
		if(verbose == 1)
			debug("udpSocket_recvPacket: Message received.\n");

		for (cmsg = CMSG_FIRSTHDR(&msgh); cmsg != NULL; cmsg = CMSG_NXTHDR(&msgh,cmsg)) {
			if (cmsg->cmsg_level == IPPROTO_IP && cmsg->cmsg_type == IP_TTL) {
				ttlptr = (int *) CMSG_DATA(cmsg);
				received_ttl = *ttlptr;
				memcpy(ttl,ttlptr,4);
				if(verbose == 1)
					debug("received ttl true: %i  \n ",received_ttl);
				break;
			}
		}
	}
}


int closeSocket(int udpSocket)
{

  /*cleanup */
  close(udpSocket);
  return 0;

}

const char *mlAutodetectIPAddress() {
	static char addr[128] = "";
#ifdef __linux__

	FILE *r = fopen("/proc/net/route", "r");
	if (!r) return NULL;

	char iface[IFNAMSIZ] = "";
	char line[128] = "x";
	while (1) {
		char dst[32];
		char ifc[IFNAMSIZ];

		char *dummy = fgets(line, 127, r);
		if (feof(r)) break;
		if ((sscanf(line, "%s\t%s", iface, dst) == 2) && !strcpy(dst, "00000000")) {
			strcpy(iface, ifc);
		 	break;
		}
	}
	if (iface[0] == 0) return NULL;

	struct ifaddrs *ifAddrStruct=NULL;
	if (getifaddrs(&ifAddrStruct)) return NULL;

	while (ifAddrStruct) {
		if (ifAddrStruct->ifa_addr && ifAddrStruct->ifa_addr->sa_family == AF_INET &&
			ifAddrStruct->ifa_name && !strcmp(ifAddrStruct->ifa_name, iface))  {
			void *tmpAddrPtr=&((struct sockaddr_in *)ifAddrStruct->ifa_addr)->sin_addr;
			return inet_ntop(AF_INET, tmpAddrPtr, addr, 127);
		}
	ifAddrStruct=ifAddrStruct->ifa_next;
	}
#else
       {
                char namebuf[256];
                if(gethostname(namebuf, sizeof(namebuf))) {
                  perror("Error getting hostname");
                  return NULL;
                }
printf("Hostname is %s\n", namebuf);
                struct hostent *he = gethostbyname(namebuf);
                if(he == NULL) return NULL;
printf("Hostaddr: %x\n", *((unsigned int *)(he->h_addr)));
		return inet_ntop(AF_INET, he->h_addr, addr, 127);
       }
#endif
	return NULL;
}
#else  // WINDOWS

int sendPacketFinal(const int udpSocket, struct iovec *iov, int len, struct sockaddr_in *socketaddr)
{
  char stack_buffer[1600];
  char *buf = stack_buffer;
  int i;
  int total_len = 0;
  int ret;
  for(i = 0; i < len; i++) total_len += iov[i].iov_len;

  if(outputRateControl(total_len) != OK) return THROTTLE;

  if(total_len > sizeof(stack_buffer)) {
     warn("sendPacket total_length %d requires dynamically allocated buffer", total_len);
     buf = malloc(total_len);
  }
  total_len = 0;
  for(i = 0; i < len; i++) {
     memcpy(buf+total_len, iov[i].iov_base, iov[i].iov_len);
     total_len += iov[i].iov_len;
  } 
  ret = sendto(udpSocket, buf, total_len, 0, (struct sockaddr *)socketaddr, sizeof(*socketaddr));
debug("Sent %d bytes (%d) to %d\n",ret, WSAGetLastError(), udpSocket);
  if(buf != stack_buffer) free(buf);
  return ret == total_len ? OK:FAILURE; 
}

void recvPacket(const int udpSocket,char *buffer,int *recvSize,struct sockaddr_in *udpdst,icmp_error_cb icmpcb_value,int *ttl)
{
  debug("recvPacket");
  int salen = sizeof(struct sockaddr_in);
  int ret;
  ret = recvfrom(udpSocket, buffer, *recvSize, 0, (struct sockaddr *)udpdst, &salen);
  *recvSize = ret;
  if(ret == SOCKET_ERROR) {
      int err = WSAGetLastError();
      if(err = WSAECONNRESET) {
           warn("RECVPACKET detected ICMP Unreachable  for %s:%d", inet_ntoa(udpdst->sin_addr), udpdst->sin_port);
      }
      else {
           warn("RECVPACKET unclassified error %d", err);
      }
      *recvSize = -1;
      return;
  }
  *ttl=10;
}

int getTTL(const int udpSocket,uint8_t *ttl){
  return 64;
}

const char *mlAutodetectIPAddress() {
  return NULL;
}


const char *inet_ntop(int af, const void *src,
       char *dst, size_t size) {
    char *c = inet_ntoa(*(struct in_addr *)src);
    if(strlen(c) >= size) return NULL;
    return strcpy(dst, c);
}
int inet_pton(int af, const char * src, void *dst) {
    unsigned long l = inet_addr(src);
    if(l == INADDR_NONE) return 0;
    *(unsigned long *)dst = l;
    return 1;
}
#endif
