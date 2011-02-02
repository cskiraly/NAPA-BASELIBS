/*
 *          
 *
 *	upgraded rateControl - token bucket
 * 
 *	Agnieszka Witczak & Szymon Kuc
 *     
 */

#include <ml_all.h>

extern struct event_base *base;
static long bucket_size = 0;
static int64_t drain_rate = 0;


static long bytes_in_bucket = 0;
struct timeval bib_then = { 0, 0};

void planFreeSpaceInBucketEvent();

void freeSpaceInBucket_cb (int fd, short event,void *arg) {

	/*struct timeval test;

	gettimeofday(&test,NULL);

	int us;

	if(test.tv_usec > (int) arg)
		us = test.tv_usec - (int) arg;
	else
		us = 1000000 + test.tv_usec - (int) arg;

	fprintf(stderr,"Event scheduled in: %d microseconds\n",us);*/

	while((!isQueueEmpty()) && (outputRateControl(getFirstPacketSize()) == OK)) {	


		PacketContainer* packet = takePacketToSend();

		if (packet == NULL) return;

		struct timeval now;
   		gettimeofday(&now, NULL);
		bib_then = now;

#ifdef RTX
		if (!(packet->priority & NO_RTX)) addPacketRTXqueue(packet);
#endif

		sendPacket(packet->udpSocket, packet->iov, 4, packet->socketaddr);
	}

	if (!isQueueEmpty()) planFreeSpaceInBucketEvent(getFirstPacketSize());
	
	return;
}

void planFreeSpaceInBucketEvent(int bytes) {		//plan the event for time when there will be free space in bucket (for the first packet from the TXqueue)
	struct timeval TXtime;
	struct event *ev;

	//time needed to send data = firstPacketFromQueue.size, will free space for this packet in the bucket
	TXtime.tv_sec = bytes / drain_rate; //seconds
	TXtime.tv_usec = (bytes - TXtime.tv_sec * drain_rate) * 1000000 / drain_rate; //microseconds

	ev = evtimer_new(base, freeSpaceInBucket_cb, NULL);
	event_add(ev, &TXtime);
}

int queueOrSendPacket(const int udpSocket, struct iovec *iov, int len, struct sockaddr_in *socketaddr, unsigned char priority)
{
	PacketContainer *newPacket = (PacketContainer*) createPacketContainer(udpSocket,iov,len,socketaddr,priority);

debug("QUUEO1");
debug("QUUEO1");
	if(!(priority & HP)) {
		if (!isQueueEmpty()) {						//some packets are already waiting, "I am for sure after them"
			return addPacketTXqueue(newPacket);
		}	
		else if(outputRateControl(newPacket->pktLen) != OK) {			//queue is empty, not enough space in bucket - "I will be first in the queue"
			planFreeSpaceInBucketEvent(newPacket->pktLen);		//when there will be enough space in the bucket for the first packet from the queue
			return addPacketTXqueue(newPacket);
		}
	}
debug("QUUEO1c\n");
#ifdef RTX
	if (!(priority & NO_RTX)) addPacketRTXqueue(newPacket);
#endif

debug("QUUEO2");
	return sendPacket(udpSocket, iov, 4, socketaddr);
}

void setOutputRateParams(int bucketsize, int drainrate) { //given in Bytes and Bits/s
     bucket_size = bucketsize;
     outputRateControl(0);
     drain_rate = drainrate >> 3; //now in bytes/s
}

int outputRateControl(int len) {
	struct timeval now;
	gettimeofday(&now, NULL);

	if(drain_rate <= 0) {
		bytes_in_bucket = 0;
		bib_then = now;
		return OK;
	} else {
		unsigned int leaked;
		int total_drain_secs = bytes_in_bucket / (drain_rate) + 1;

		if(now.tv_sec - bib_then.tv_sec - 1 > total_drain_secs) {
				bytes_in_bucket = 0;
		} else {
			leaked = drain_rate * (now.tv_sec - bib_then.tv_sec);
			leaked += drain_rate * (now.tv_usec - bib_then.tv_usec) / 1000000; 
			if(leaked > bytes_in_bucket) bytes_in_bucket = 0;
					else bytes_in_bucket -= leaked;
		}

		bib_then = now;
		if(bytes_in_bucket + len <= bucket_size) {
			bytes_in_bucket += len;
			return OK;
		} else {
			return THROTTLE;
		}
   }
}
