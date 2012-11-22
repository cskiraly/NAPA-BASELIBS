/*
 * Copyright (c) Agnieszka Witczak & Szymon Kuc
 *
 * This file is part of the Messaging Library.
 *
 * The Messaging Library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * The Messaging Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with the Messaging Library.  If not, see <http://www.gnu.org/licenses/>.
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

	int pktLen;		//bytes
	struct timeval timeStamp;
	struct PktContainer *next;
	unsigned char priority;
} PacketContainer;


typedef struct PktQueue { 
	PacketContainer *head;
	PacketContainer *tail;

	int size;		//bytes

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
