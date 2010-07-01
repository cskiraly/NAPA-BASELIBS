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

#ifndef _FORECASTER_MEASURE_H_
#define _FORECASTER_MEASURE_H_

#include <string>
#include "measure_plugin.h"
#include "mon_measure.h"
	
#define FORECASTER_MTU 1300

//TODO:
//- add window size as parameter

struct timestamps {
	struct timeval tx;
	struct timeval rx;
};

class ForecasterMeasure : public MonMeasure {
	double avg_pause;
	char* pkt;

	double min_delay;
	int p_num;
	int p_below;
	int p_above;
	
	double gen_pause_exp(double);
	result computeAvailableBandwidth(result *r);

public:
	ForecasterMeasure(class MeasurePlugin *m, MeasurementCapabilities mc, class MeasureDispatcher *md);
	virtual void init();
	virtual void stop();

	virtual ~ForecasterMeasure();
	virtual int paramChange(MonParameterType ph, MonParameterValue p);

	virtual void Run();
	virtual void receiveOobData(char *buf, int buf_len, result *r);

	virtual result RxPkt(result *r,ExecutionList *el);
};

class ForecasterMeasurePlugin : public MeasurePlugin {
	virtual ~ForecasterMeasurePlugin() {};


	virtual MeasurementCapabilities getCaps() {
		return OUT_OF_BAND | PACKET | DATA | TXRXUNI;
	};

	virtual MonMeasure* createMeasure (MeasurementCapabilities mc, class MeasureDispatcher *md) {
		return new ForecasterMeasure(this, mc, md);
	};

public:
	ForecasterMeasurePlugin();
};



#endif /* _FORECASTER_MEASURE_H__ */

