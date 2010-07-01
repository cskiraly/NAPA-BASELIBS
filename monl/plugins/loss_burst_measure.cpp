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

#include "loss_burst_measure.h"
#include "grapes_log.h"

LossBurstMeasure::LossBurstMeasure(class MeasurePlugin *m, MeasurementCapabilities mc, class MeasureDispatcher *md): MonMeasure(m,mc,md) {
}

void LossBurstMeasure::init() {
	burst = 0;
}

void LossBurstMeasure::stop() {
	r_tx_list[R_LOSS_BURST] = r_rx_list[R_LOSS_BURST] = NAN;
}

result LossBurstMeasure::RxPkt(result *r,ExecutionList *el) {
	if(isnan(r[R_LOSS]))
		return NAN;

	if(r[R_LOSS] != 0)
		burst++;
	else {
		r[R_LOSS_BURST] = burst;
		burst = 0;
		char dbg[512];
		snprintf(dbg, sizeof(dbg), "Ts: %f Lb: %f", r[R_RECEIVE_TIME], r[R_LOSS_BURST]);
		debugOutput(dbg);
	}

	return r[R_LOSS_BURST];
}

result LossBurstMeasure::RxData(result *r,ExecutionList *el) {
	return RxPkt(r,el);
}

LossBurstMeasurePlugin::LossBurstMeasurePlugin() {
	/* Initialise properties: MANDATORY! */
	name = "Loss Burst";
	desc = "The loss burst length";
	id = LOSS_BURST;
	/* end of mandatory properties */
	addDependency(LOSS);
}



