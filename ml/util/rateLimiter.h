/*
 * Copyright (c) Agnieszka Witczak & Szymon Kuc
/*
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

/*
 *	upgraded rateControl - token bucket
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

