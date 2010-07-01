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

#include "bulktransfer_measure.h"

//TODO add timer parameter

BulktransferMeasure::BulktransferMeasure(class MeasurePlugin *m, MeasurementCapabilities mc, class MeasureDispatcher *md): MonMeasure(m,mc,md) {
	last_reset_time = 0.0;
	sum = 0.0;
};

BulktransferMeasure::~BulktransferMeasure() {
}

void BulktransferMeasure::Run() {
	struct timeval ts;
	result last_reset_time_old;

	last_reset_time_old = last_reset_time;

	gettimeofday(&ts,NULL);
	last_reset_time = ts.tv_usec / 1000000.0;
	last_reset_time = last_reset_time + ts.tv_sec;


	if(last_reset_time_old == 0.0) {
		sum = 0.0;
		return;
	}

	r_rx_list[R_THROUGHPUT] = r_tx_list[R_THROUGHPUT] = sum / (last_reset_time - last_reset_time_old) * 8.0 / 1000.0;
	newSample(r_rx_list[R_THROUGHPUT]);

	sum = 0.0;

	struct timeval next = {1, 0};

	scheduleNextIn(&next);
}

void BulktransferMeasure::init() {
	last_reset_time = 0.0;
	sum = 0.0;
}

void BulktransferMeasure::stop() {
	r_rx_list[R_THROUGHPUT] = r_tx_list[R_THROUGHPUT] = NAN;
}

result BulktransferMeasure::RxPkt(result *r,ExecutionList *el) {
	sum += r[R_SIZE];
	char dbg[512];
	snprintf(dbg, sizeof(dbg), "Ts: %f Sum: %f", r[R_RECEIVE_TIME], sum);
	debugOutput(dbg);
	return NAN;
}

result BulktransferMeasure::RxData(result *r, ExecutionList *el) {
	return RxPkt(r,el);
}

result BulktransferMeasure::TxPkt(result *r,ExecutionList *el) {
	sum += r[R_SIZE];
	char dbg[512];
	snprintf(dbg, sizeof(dbg), "Ts: %f Sum: %f", r[R_SEND_TIME], sum);
	debugOutput(dbg);
	return NAN;
}

result BulktransferMeasure::TxData(result *r, ExecutionList *el) {
	return TxPkt(r,el);
}


BulktransferMeasurePlugin::BulktransferMeasurePlugin() {
	/* Initialise properties: MANDATORY! */
	name = "Bulk Transfer";
	desc = "Traffic volume in [kbit/s]";
	id = BULK_TRANSFER;
	/* end of mandatory properties */
}
