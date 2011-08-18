/***************************************************************************
 *   Copyright (C) 2009 by Robert Birke
 *   robert.birke@polito.it
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.

 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.

 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301  USA 
 ***********************************************************************/
#include "rtt_measure.h"
#include <sys/time.h>

RttMeasure::RttMeasure(class MeasurePlugin *m, MeasurementCapabilities mc, class MeasureDispatcher *md): MonMeasure(m,mc,md,10) {
}

RttMeasure::~RttMeasure() {
}

void RttMeasure::init() {
	last_time_rx = 0.0;
	last_time_rem_tx = 0.0;
}

void RttMeasure::stop() {
	r_tx_list[R_RTT] = r_rx_list[R_RTT] = NAN;
	r_tx_list[R_RECEIVE_TIME] = r_rx_list[R_RECEIVE_TIME] = NAN;
}

result RttMeasure::RxPkt(result *r,ExecutionList *el) {
	char dbg[512];
	if(flags & REMOTE) {
		last_time_rx = r[R_RECEIVE_TIME];
		last_time_rem_tx = r[R_SEND_TIME];

		snprintf(dbg, sizeof(dbg), "RX rem: Ts: %f seq#: %f", r[R_RECEIVE_TIME], r[R_SEQNUM]);
		debugOutput(dbg);

		return NAN;
	} else {
		if(r[R_REPLY_TIME] != 0.0) {
			r[R_RTT] = r[R_RECEIVE_TIME] - r[R_REPLY_TIME];
			snprintf(dbg, sizeof(dbg), "RX: Ts: %f seq#: %f Rtt: %f Replyt: %f", r[R_RECEIVE_TIME], r[R_SEQNUM], r[R_RTT], r[R_REPLY_TIME]);
			debugOutput(dbg);

			return every(r[R_RTT]);
		}

		snprintf(dbg, sizeof(dbg), "RX: Ts: %f seq#: %f Rtt: nan Replyt: %f", r[R_RECEIVE_TIME], r[R_SEQNUM], r[R_RTT], r[R_REPLY_TIME]);
		debugOutput(dbg);
	}
	return NAN;
}

result RttMeasure::RxData(result *r,ExecutionList *el) {
	return RxPkt(r,el);
}

result RttMeasure::TxPkt(result *r,ExecutionList *el) {
	char dbg[512];
	if(flags & REMOTE) {
		if(last_time_rem_tx != 0.0 && last_time_rx != 0.0) {
			r[R_REPLY_TIME] = last_time_rem_tx + r[R_SEND_TIME] - last_time_rx;

			snprintf(dbg, sizeof(dbg), "TX rem: Ts: %f seq#: %f Last: %f Replyt: %f", r[R_SEND_TIME], r[R_SEQNUM], last_time_rx, r[R_REPLY_TIME]);
			debugOutput(dbg);
		}
		else
			r[R_REPLY_TIME] = NAN;
	} else {
		snprintf(dbg, sizeof(dbg), "TX: Ts: %f seq#: %f", r[R_SEND_TIME], r[R_SEQNUM]);
		debugOutput(dbg);
	}
	return NAN;
}

result RttMeasure::TxData(result *r,ExecutionList *el) {
	return TxPkt(r,el);
}

void RttMeasure::Run() {
	if(flags & REMOTE)
		return;

	struct timeval tv;
	gettimeofday(&tv,NULL);
	sendOobData((char*) (&tv),sizeof(struct timeval));

	//Schedule next
	struct timeval next = {1,0};
	scheduleNextIn(&next);
}

void RttMeasure::receiveOobData(char *buf, int buf_len, result *r) {
	if(flags & REMOTE) {
		sendOobData(buf, buf_len);
	} else {
		result start, end;
		struct timeval tv_end;
		struct timeval *tv_start;

		tv_start = (struct timeval *) buf;

		start = tv_start->tv_sec + tv_start->tv_usec / 1000000.0;
		end = r[R_RECEIVE_TIME];

		newSample(end-start);
	}
}

RttMeasurePlugin::RttMeasurePlugin() {
	/* Initialise properties: MANDATORY! */
	name = "Round trip time";
	desc = "The round trip time between two peers in [s]";
	id = RTT;
	/* end of mandatory properties */
}
