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

#include "packet_measure.h"

PacketMeasure::PacketMeasure(class MeasurePlugin *m, MeasurementCapabilities mc, class MeasureDispatcher *md): MonMeasure(m,mc,md) {
};

PacketMeasure::~PacketMeasure() {
}


result PacketMeasure::RxPkt(result *r, ExecutionList *el) {
	char dbg[512];
	snprintf(dbg, sizeof(dbg), "Ts: %f", r[R_RECEIVE_TIME]);
	debugOutput(dbg);
	return 1;
}

result PacketMeasure::RxData(result *r, ExecutionList *el) {
	return RxPkt(r,el);
}

result PacketMeasure::TxPkt(result *r, ExecutionList *el) {
	char dbg[512];
	snprintf(dbg, sizeof(dbg), "Ts: %f", r[R_SEND_TIME]);
	debugOutput(dbg);
	return 1;
}

result PacketMeasure::TxData(result *r, ExecutionList *el) {
	return TxPkt(r,el);
}

RxPacketMeasurePlugin::RxPacketMeasurePlugin() {
	/* Initialise properties: MANDATORY! */
	name = "Rx Packet";
	desc = "Received pkts/chunks";
	id = RX_PACKET;
	/* end of mandatory properties */
}

TxPacketMeasurePlugin::TxPacketMeasurePlugin() {
	/* Initialise properties: MANDATORY! */
	name = "Tx Packet";
	desc = "Transmitted pkts/chunks";
	id = TX_PACKET;
	/* end of mandatory properties */
}