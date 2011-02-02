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

#include "byte_measure.h"

ByteMeasure::ByteMeasure(class MeasurePlugin *m, MeasurementCapabilities mc, class MeasureDispatcher *md): MonMeasure(m,mc,md) {
};

ByteMeasure::~ByteMeasure() {
}

result ByteMeasure::RxPkt(result *r,ExecutionList *el) {
	char dbg[512];
	snprintf(dbg, sizeof(dbg), "Ts: %f S: %f", r[R_RECEIVE_TIME], r[R_SIZE]);
	debugOutput(dbg);
	return r[R_SIZE];
}

result ByteMeasure::RxData(result *r, ExecutionList *el) {
	return RxPkt(r,el);
}

result ByteMeasure::TxPkt(result *r,ExecutionList *el) {
	char dbg[512];
	snprintf(dbg, sizeof(dbg), "Ts: %f S: %f", r[R_SEND_TIME], r[R_SIZE]);
	debugOutput(dbg);
	return r[R_SIZE];
}

result ByteMeasure::TxData(result *r, ExecutionList *el) {
	return TxPkt(r,el);
}


RxByteMeasurePlugin::RxByteMeasurePlugin() {
	/* Initialise properties: MANDATORY! */
	name = "Rx Byte";
	desc = "Bytes received [bytes]";
	id = RX_BYTE;
	/* end of mandatory properties */
}

TxByteMeasurePlugin::TxByteMeasurePlugin() {
	/* Initialise properties: MANDATORY! */
	name = "Tx Byte";
	desc = "Bytes transmitted [bytes]";
	id = TX_BYTE;
	/* end of mandatory properties */
}
