/*
 *          Policy Management
 *
 *
 * This software was created by arpad.bakay@netvisor.hu
 *
 *     THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */

#include <sys/time.h>

#include "udpSocket.h"

static long bucket_size = 0;
static int drain_rate = 0;

static long bytes_in_bucket = 0;
struct timeval bib_when = { 0, 0};

int outputRateControl(int len) {
   struct timeval now;
   gettimeofday(&now, NULL);
   if(drain_rate <= 0) {
      bytes_in_bucket = 0;
      bib_when = now;
      return OK;
   }
   else {
       int total_drain_secs = bytes_in_bucket / drain_rate + 1; 
       if(now.tv_sec - bib_when.tv_sec - 1 < total_drain_secs) {
           bytes_in_bucket = 0;
       }
       else {
          long leaked = drain_rate * 1024 * (now.tv_sec - bib_when.tv_sec);
          leaked += drain_rate * (now.tv_usec - bib_when.tv_usec) / (1000000 / 1024); 
	  if(leaked > bytes_in_bucket) bytes_in_bucket = 0;
          else bytes_in_bucket -= leaked;
       }
       bib_when = now;
       if(bytes_in_bucket + len <= bucket_size) {
              bytes_in_bucket += len;
              return OK;
       }
       else  return THROTTLE;
   }
}

void setOutputRateParams(int bucketsize, int drainrate) {
     bucket_size = bucketsize * 1024;
     outputRateControl(0);
     drain_rate = drainrate; 
}


