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

#ifndef _RTT_MEASURE_H_
#define _RTT_MEASURE_H_

#include <string>
#include "measure_plugin.h"
#include "mon_measure.h"


//TODO:
//- add window size as parameter

class RttMeasure : public MonMeasure {
	result last_time_rx;
	result last_time_rem_tx;

public:
	RttMeasure(class MeasurePlugin *m, MeasurementCapabilities mc, class MeasureDispatcher *md);
	virtual void init();
	virtual void stop();
	virtual ~RttMeasure();

	virtual result RxPkt(result *r,ExecutionList *el);

	virtual result RxData(result *r,ExecutionList *el);

	virtual result TxPkt(result *r,ExecutionList *el);

	virtual result TxData(result *r,ExecutionList *el);

	virtual void Run();
	virtual void receiveOobData(char *buf, int buf_len, result *r);

};

class RttMeasurePlugin : public MeasurePlugin {
	virtual ~RttMeasurePlugin() {};


	virtual MeasurementCapabilities getCaps() {
		return IN_BAND | OUT_OF_BAND | PACKET | DATA | TXRXBI;
	};

	virtual MonMeasure* createMeasure (MeasurementCapabilities mc, class MeasureDispatcher *md) {
		return new RttMeasure(this, mc, md);
	};

public:
	RttMeasurePlugin();
};



#endif /* _RTT_MEASURE_H_ */

