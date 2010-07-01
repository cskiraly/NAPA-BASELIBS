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

#include "corrected_delay_measure.h"
#include "clockdrift_measure.h"
#include "grapes_log.h"
#include <math.h>

CorrecteddelayMeasure::CorrecteddelayMeasure(class MeasurePlugin *m, MeasurementCapabilities mc, class MeasureDispatcher *md): MonMeasure(m,mc,md) {
}

CorrecteddelayMeasure::~CorrecteddelayMeasure() {
}

void CorrecteddelayMeasure::stop() {
	r_tx_list[R_CORRECTED_DELAY] = r_rx_list[R_CORRECTED_DELAY] = NAN;
	mClockdrift = NULL;
}

void CorrecteddelayMeasure::init() {
	mClockdrift = NULL;
}

result CorrecteddelayMeasure::RxPkt(result *r,ExecutionList *el) {
	if(mClockdrift == NULL) {
		if(el->find(CLOCKDRIFT) == el->end()) {
			error("MONL: Corrected Delay: dependencies not fullfilled");
			return NAN;
		}
		mClockdrift = (*el)[CLOCKDRIFT];
	}

	if(!isnan(r[R_CLOCKDRIFT]))
		r[R_CORRECTED_DELAY] = fabs(r[R_SEND_TIME] - r[R_RECEIVE_TIME]) - (r[R_SEND_TIME] - ((ClockdriftMeasure*)mClockdrift)->first_tx) * ((ClockdriftMeasure*)mClockdrift)->b - ((ClockdriftMeasure*)mClockdrift)->a;
	else
		r[R_CORRECTED_DELAY] = NAN;

	char dbg[512];
	snprintf(dbg, sizeof(dbg), "Ts: %f OrgD: %f CorD: %f Tx: %f Rx: %f a: %f b: %f Ftx: %f", r[R_RECEIVE_TIME], fabs(r[R_SEND_TIME] - r[R_RECEIVE_TIME]), r[R_CORRECTED_DELAY], r[R_SEND_TIME], r[R_RECEIVE_TIME], ((ClockdriftMeasure*)mClockdrift)->a, ((ClockdriftMeasure*)mClockdrift)->b, ((ClockdriftMeasure*)mClockdrift)->first_tx);
	debugOutput(dbg);

	return r[R_CORRECTED_DELAY];
}

result CorrecteddelayMeasure::RxData(result *r,ExecutionList *el) {
	return RxPkt(r, el);
}


CorrecteddelayMeasurePlugin::CorrecteddelayMeasurePlugin() {
	/* Initialise properties: MANDATORY! */
	name = "Corrected delay";
	desc = "Forward delay with clock drift correction";
	id = CORRECTED_DELAY;
	/* end of mandatory properties */
	addDependency(CLOCKDRIFT);
	//TODO add dependency
}
