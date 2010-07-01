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

#ifndef _CLOCKDRIFT_MEASURE_H_
#define _CLOCKDRIFT_MEASURE_H_

#include "measure_plugin.h"
#include "mon_measure.h"
#include <algorithm>

struct clockdrift_sample {
	double tx_time;
	double delay;
};

class ClockdriftMeasure : public MonMeasure {
	double x_axis_inter(double a1, double b1, double a2, double b2);
	double y_axis_inter(double a1, double b1, double a2, double b2);

	int pos;
	int *n;
	int pnum;

	double sum_t, sum_d, sum_t_v, sum_d_v, sum_td_v;

public:
	double a, b, first_tx;

	std::vector<struct clockdrift_sample> samples;

	ClockdriftMeasure(class MeasurePlugin *m, MeasurementCapabilities mc, class MeasureDispatcher *md);
	virtual ~ClockdriftMeasure();
	virtual result RxPkt(result *r, ExecutionList *el);
	virtual result RxData(result *r, ExecutionList *el);
	virtual void stop();
	virtual void init();
	virtual int paramChange(MonParameterType ph, MonParameterValue p);
};

class ClockdriftMeasurePlugin : public MeasurePlugin {
	virtual ~ClockdriftMeasurePlugin() {};

	virtual MeasurementCapabilities getCaps() {
		return IN_BAND | PACKET | DATA | TXRXUNI;
	};


	virtual MonMeasure* createMeasure (MeasurementCapabilities mc, class MeasureDispatcher *md) {
		return new ClockdriftMeasure(this, mc, md);
	};
public:
	ClockdriftMeasurePlugin();
};

#endif /* _CLOCKDRIFT_MEASURE_H_ */

