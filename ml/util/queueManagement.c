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
#include <math.h>

#include <unistd.h>
#include <string.h>
#ifndef WIN32
#include <sys/uio.h>
#include <netinet/in.h>
#endif

#include "queueManagement.h"
#include "udpSocket.h"

	FILE *file = NULL;

PacketQueue TXqueue;
PacketQueue RTXqueue;

int TXmaxSize = 6000*1500;			//bytes
int RTXmaxSize = 6000*1500;			//bytes

struct timeval maxTimeToHold = {5,0};

PacketContainer* createPacketContainer(const int uSoc,struct iovec *ioVector,int iovlen,struct sockaddr_in *sockAddress, unsigned char prior) {
	
	PacketContainer *packet = malloc(sizeof(PacketContainer));
	packet->udpSocket = uSoc;
	packet->iovlen = iovlen;
	packet->next = NULL;
	packet->pktLen = ioVector[0].iov_len + ioVector[1].iov_len + ioVector[2].iov_len + ioVector[3].iov_len;

	packet->priority = prior;

 	int i;
        packet->iov = malloc(sizeof(struct iovec) * iovlen);
        for (i=0; i<iovlen; i++){
                packet->iov[i].iov_len = ioVector[i].iov_len;
                packet->iov[i].iov_base = malloc(packet->iov[i].iov_len);
                memcpy(packet->iov[i].iov_base, ioVector[i].iov_base, packet->iov[i].iov_len);
        }

        packet->socketaddr = malloc(sizeof(struct sockaddr_in));
        memcpy(packet->socketaddr, sockAddress, sizeof(struct sockaddr_in));

	int packet_len = ioVector[0].iov_len + ioVector[1].iov_len + ioVector[2].iov_len + ioVector[3].iov_len;
	return packet;
}

void destroyPacketContainer(PacketContainer* pktContainer){

        int i;
        for (i=0; i < pktContainer->iovlen; i++) free(pktContainer->iov[i].iov_base);
        free(pktContainer->iov);
        free(pktContainer->socketaddr);
        free(pktContainer);
}

int addPacketTXqueue(PacketContainer *packet) {
	if ((TXqueue.size + packet->pktLen) > TXmaxSize) {
		fprintf(stderr,"[queueManagement::addPacketTXqueue] -- Sorry! Max size for TX queue. Packet will be discarded. \n");
		destroyPacketContainer(packet);
		return THROTTLE;
	}
	TXqueue.size += packet->pktLen;	

	if (TXqueue.head == NULL) {			//adding first element
		TXqueue.head = packet;
		TXqueue.tail = packet;		
	} else {					//adding at the end of the queue
		TXqueue.tail->next = packet;
		TXqueue.tail = packet;
	}
	
	//logQueueOccupancy();
	return OK;
}

PacketContainer* takePacketToSend() {			//returns pointer to packet or NULL if queue is empty
	if (TXqueue.head != NULL) {
		PacketContainer *packet = TXqueue.head;
		
		TXqueue.size -= packet->pktLen;
		TXqueue.head = TXqueue.head->next;
		packet->next = NULL;
					
		return packet;
	}
	else return NULL;	
}


void addPacketRTXqueue(PacketContainer *packet) {
	//removing old packets - because of maxTimeToHold
	struct timeval now, age;
	gettimeofday(&now, NULL);
 
	PacketContainer *tmp = RTXqueue.head;
	while (tmp != NULL) {
		age.tv_sec = now.tv_sec - tmp->timeStamp.tv_sec;
		age.tv_usec = now.tv_usec - tmp->timeStamp.tv_usec;

		if (age.tv_sec > maxTimeToHold.tv_sec) {

			tmp = tmp->next;
			removeOldestPacket();
		}
		else break;
	}


	if ((RTXqueue.size + packet->pktLen) > RTXmaxSize) {
		removeOldestPacket();
	}

	//adding timeStamp
	gettimeofday(&(packet->timeStamp), NULL);

	//finally - adding packet
	RTXqueue.size += packet->pktLen;

	if (RTXqueue.head == NULL) {			//adding first element
		RTXqueue.head = packet;
		RTXqueue.tail = packet;
		return;	
	} else {					//adding at the end of the queue
		RTXqueue.tail->next = packet;
		RTXqueue.tail = packet;
		return;
	}
}

int removeOldestPacket() {			//return 0 if success, else (queue empty) return 1
	if (RTXqueue.head != NULL) {
		RTXqueue.size -= RTXqueue.head->pktLen;
		PacketContainer *pointer = RTXqueue.head;		
		RTXqueue.head = RTXqueue.head->next;
		destroyPacketContainer(pointer);
		return 0;
	}
	return 1;	
}

void setQueuesParams (int TXsize, int RTXsize, double maxTTHold) {
	TXmaxSize = TXsize;
	RTXmaxSize = RTXsize;
	maxTimeToHold.tv_sec = (int)floor(maxTTHold);
	maxTimeToHold.tv_usec = (int)(1000000.0 * fmod(maxTTHold, 1.0));
}

int isQueueEmpty() {
	
	if (TXqueue.head == NULL) return 1;
	return 0;
}

int getFirstPacketSize() {

	if (TXqueue.head != NULL) return TXqueue.head->pktLen;
	return 0;
}

void logQueueOccupancy() {
	struct timeval now;
	gettimeofday(&now, NULL);
	double time = now.tv_sec + ((double) now.tv_usec)/1000000;
	file = fopen("./queue.txt", "a+");
	//char *str;
	//sprintf(str,"%f",time); 
	//fputs(str, p);
	fprintf(file,"%f \t %d \n",time, TXqueue.size);	
	fprintf(stderr,"[queueManagement::logQueueOccupancy] -- Time: %f \t queueOccupancy: %d \n", time, TXqueue.size);
	fclose(file);
}
