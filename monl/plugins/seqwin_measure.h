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

#ifndef _SEQWIN_MEASURE_H_
#define _SEQWIN_MEASURE_H_

#include "measure_plugin.h"
#include "mon_measure.h"

class SeqWinMeasure : public MonMeasure {
	bool win_valid;
	uint32_t largest_sn;
	uint32_t initial_sn;
	uint32_t *sn_win;
	uint32_t pnum;

public:
	SeqWinMeasure(class MeasurePlugin *m, MeasurementCapabilities mc, class MeasureDispatcher *md);
	virtual ~SeqWinMeasure();

	virtual result RxPkt(result *r, ExecutionList *el);
	virtual result RxData(result *r, ExecutionList *el);
	virtual int paramChange(MonParameterType ph, MonParameterValue p);
	virtual void init();
	virtual void stop();
};

class SeqWinMeasurePlugin : public MeasurePlugin {
	virtual ~SeqWinMeasurePlugin() {};


	virtual MeasurementCapabilities getCaps() {
		return IN_BAND | PACKET | DATA | TXRXUNI;
	};

	virtual MonMeasure* createMeasure (MeasurementCapabilities mc, class MeasureDispatcher *md) {
		return new SeqWinMeasure(this, mc, md);
	};

public:
	SeqWinMeasurePlugin();
};

#endif /* _SEQWIN_MEASURE_H_ */


