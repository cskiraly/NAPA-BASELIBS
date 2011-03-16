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

#include "loss_measure.h"
#include "napa_log.h"

LossMeasure::LossMeasure(class MeasurePlugin *m, MeasurementCapabilities mc, class MeasureDispatcher *md): MonMeasure(m,mc,md) {
	mSeqWin = NULL;
	tx_every = 10;
}

void LossMeasure::init() {
	mSeqWin = NULL;
}

void LossMeasure::stop() {
	r_tx_list[R_LOSS] = r_rx_list[R_LOSS] = NAN;
	mSeqWin = NULL;
}

result LossMeasure::RxPkt(result *r,ExecutionList *el) {
	if(isnan(r[R_SEQWIN]))
		return NAN;

//	if(mSeqWin == NULL) { dirty trick to test bug
		if(el->find(SEQWIN) == el->end()) {
			error("MONL: LOSS missing dependency");
			return NAN;
		}
		mSeqWin = (*el)[SEQWIN];
//	}

	if(r[R_SEQWIN] > mSeqWin->getParameter(P_SEQN_WIN_SIZE) && 
		r[R_SEQWIN] < mSeqWin->getParameter(P_SEQN_WIN_OVERFLOW_TH)) {
		r[R_LOSS] = 1;
	}
	else if (r[R_SEQWIN] == mSeqWin->getParameter(P_SEQN_WIN_SIZE))
		r[R_LOSS] = 0;
	char dbg[512];
	snprintf(dbg, sizeof(dbg), "Ts: %f Loss: %f", r[R_RECEIVE_TIME],  r[R_LOSS]);
	debugOutput(dbg);
	return every(r[R_LOSS]);
}

result LossMeasure::RxData(result *r,ExecutionList *el) {
	return RxPkt(r,el);
}
/*
//	if(delta_sn_win == 0)
//		dups++;
//	if(p_sn != largest_sn +1)
//		ooo++;

	
	} else {
		if(delta_sn_win >= param_values[LOSS_WIN_OVERFLOW_TH]) {
			int late; late++;}
		else if(delta_sn_win > param_values[LOSS_WIN_SIZE]) {
			r[LOSS_PKT] = 1;
			sn_win[ind] = p_sn;
		} else if(delta_sn_win == param_values[LOSS_WIN_SIZE]) {
			r[LOSS_PKT] = 0;
			sn_win[ind] = p_sn;
		}
	}
}*/


LossMeasurePlugin::LossMeasurePlugin() {
	/* Initialise properties: MANDATORY! */
	name = "Loss";
	desc = "The loss probability between two peers";
	id = LOSS;
	/* end of mandatory properties */
	addDependency(SEQWIN);
}



