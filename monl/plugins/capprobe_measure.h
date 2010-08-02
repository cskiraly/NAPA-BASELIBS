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
 ***************************************************************************/

#ifndef _CAPPROBE_MEASURE_H_
#define _CAPPROBE_MEASURE_H_

#include <string>
#include "measure_plugin.h"
#include "mon_measure.h"
#include <limits.h>
#include <algorithm>


class CapprobeMeasure : public MonMeasure {
	double avg_pause;
	char *pkt;

	result min_delay;
	result min_ipd, max_ipd;
	result last_rx;
	result last_rx_delay;
	result last_rx_pn;
	result last_size;
	result max_size;

	uint32_t pair_num;

	double gen_pause_exp();

	std::vector<double> samples;

	result computeCapacity(result delay, result delay2, result ipg, result size, result *r);


public:
	CapprobeMeasure(class MeasurePlugin *m, MeasurementCapabilities mc, class MeasureDispatcher *md);
	virtual void init();
	virtual void stop();
	virtual ~CapprobeMeasure();
	virtual int paramChange(MonParameterType ph, MonParameterValue p);

	virtual void Run();
	virtual void receiveOobData(char *buf, int buf_len, result *r);

	virtual result RxPkt(result *r,ExecutionList *el);
};

class CapprobeMeasurePlugin : public MeasurePlugin {
	virtual ~CapprobeMeasurePlugin() {};


	virtual MeasurementCapabilities getCaps() {
		return OUT_OF_BAND | IN_BAND | PACKET | DATA  | TXRXUNI;
	};

	virtual MonMeasure* createMeasure (MeasurementCapabilities mc, class MeasureDispatcher *md) {
		return new CapprobeMeasure(this, mc, md);
	};

public:
	CapprobeMeasurePlugin();
};



#endif /* _CAPPROBE_MEASURE_H_ */

