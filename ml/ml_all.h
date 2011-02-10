#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <time.h>
#include <math.h>
#include <assert.h>
#include <errno.h>

#ifndef _WIN32
#include <arpa/inet.h>
#include <netdb.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <fcntl.h>
#else

#include <winsock2.h>
#include <ws2tcpip.h>
#endif

#include "util/udpSocket.h"
#include "util/stun.h"
#include "transmissionHandler.h"
#include "util/rateLimiter.h"
#include "util/queueManagement.h"

#define LOG_MODULE "[ml] "
#include "ml_log.h"

