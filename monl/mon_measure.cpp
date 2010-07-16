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
#include "mon_measure.h"
#include "measure_dispatcher.h"
#include "measure_manager.h"
#include "mon_event.h"
#include	"grapes_log.h"

MonMeasure::MonMeasure(class MeasurePlugin *mp, MeasurementCapabilities mc, class MeasureDispatcher *ptrDisp) {
	measure_plugin = mp;
	flags = mc;
	used_counter = 0;
	auto_loaded = false;
	mh_remote = -1;
	ptrDispatcher = ptrDisp;

	/* Initialise default values */
	int i;
	status = STOPPED;
	r_rx_list = r_tx_list = NULL;
	param_values = new MonParameterValue[mp->params.size()];

	for( i = 0; i < mp->params.size(); i++)
		param_values[i] =  mp->params[i]->dval;

	if(!(flags & REMOTE))
		rb = new ResultBuffer((int)param_values[P_WINDOW_SIZE], mp->name.c_str(), ptrDispatcher->mm->peer_name, this);
	else
		rb = NULL;
};

int MonMeasure::sendOobData(char* buf, int buf_len) {
	return ptrDispatcher->oobDataTx(this, buf, buf_len);
}

int MonMeasure::scheduleNextIn(struct timeval *tv) {
	return schedule_measure(tv, this);
}

void MonMeasure::debugInit(const char *name) {
	char sid_string[SOCKETID_STRING_SIZE];
	SocketId sid;
	int errorstatus;

	std::string output_name;
	
	if(output_file.is_open())
		output_file.close();

	output_name = name;

	switch(flags & (TXONLY | RXONLY | TXRXUNI | TXRXBI)) {
	case TXONLY:
			output_name += "TXONLY";
			break;
	case RXONLY:
			output_name += "RXONLY";
			break;
	case TXRXBI:
			output_name += "TXRXBI";
			break;
	case TXRXUNI:
			output_name += "TXRXUNI";
			break;
	}

	output_name += "-MT:" + msg_type;

	sid = mlGetLocalSocketID(&errorstatus);
	if(errorstatus == 0) {
		if(mlSocketIDToString(sid, sid_string, sizeof(sid_string)) == 0)
			output_name += "-A:";
			output_name += sid_string;
	}

	if(dst_socketid != NULL) {
		if(mlSocketIDToString((SocketId) dst_socketid, sid_string, sizeof(sid_string)) == 0) {
			output_name += "-B:";
			output_name += sid_string;
		}
	}

	output_file.open (output_name.c_str(), std::fstream::out | std::fstream::app);
	output_file.setf(std::ios::fixed, std::ios::floatfield);
	output_file.precision(6);
}

void MonMeasure::debugStop() {
	output_file.close();
}

void MonMeasure::debugOutput(char *out) {
	switch((int)param_values[P_DEBUG_FILE]) {
	case	1:
			output_file << out << std::endl;
			break;
	case 	3:
			output_file << out << std::endl;
	case	2:
			debug("%s",out);
			break;
	}
}
