/*
 *          
 *
 *
 * 
 *	Agnieszka Witczak & Szymon Kuc
 *     
 */


#include <stdio.h>
#include <stdlib.h>

#include <unistd.h>

#ifndef _WIN32
#include <netinet/in.h>
#else
#include <winsock2.h>
#endif

typedef struct PktContainer {
	int udpSocket; 
	struct iovec *iov; 
	int iovlen; 
	struct sockaddr_in *socketaddr;

	int pktLen;		//kB
	struct timeval timeStamp;
	struct PktContainer *next;
	unsigned char priority;
} PacketContainer;


typedef struct PktQueue { 
	PacketContainer *head;
	PacketContainer *tail;

	int size;		//kB

} PacketQueue;


PacketContainer* createPacketContainer (const int uSoc,struct iovec *ioVector,int iovlen,struct sockaddr_in *sockAddress, unsigned char prior);

int addPacketTXqueue(PacketContainer *packet);

PacketContainer* takePacketToSend();

int removeOldestPacket() ;

int isQueueEmpty();

int getFirstPacketSize();

void setQueuesParams (int TXsize, int RTXsize, double maxTimeToHold); //in  bytes, bytes, seconds

#ifdef RTX
void addPacketRTXqueue(PacketContainer *packet);

int rtxPacketsFromTo(int connID, int msgSeqNum, int offsetFrom, int offsetTo);
#endif
