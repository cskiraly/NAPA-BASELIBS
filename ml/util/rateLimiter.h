/*
 *          
 *
 *	upgraded rateControl - token bucket
 * 
 *	Agnieszka Witczak & Szymon Kuc
 *     
 */

//#include <sys/time.h>

#include <stdio.h>
#include <stdlib.h>
#include <event2/event.h>
#include <errno.h>


#define HP 1
#define NO_RTX 2

void planFreeSpaceInBucketEvent();

void freeSpaceInBucket_cb (int fd, short event,void *arg);

int queueOrSendPacket(const int udpSocket, struct iovec *iov, int len, struct sockaddr_in *socketaddr, unsigned char priority);

