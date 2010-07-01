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

#include "hopcount_measure.h"

HopcountMeasure::HopcountMeasure(class MeasurePlugin *m, MeasurementCapabilities mc, class MeasureDispatcher *md): MonMeasure(m,mc,md) {
};

HopcountMeasure::~HopcountMeasure() {
}

void HopcountMeasure::stop() {
	r_tx_list[R_HOPCOUNT] = r_rx_list[R_HOPCOUNT] = NAN;
}

result HopcountMeasure::RxPkt(result *r,ExecutionList *el) {
	if(r[R_INITIAL_TTL] != 0.0 && !isnan(r[R_INITIAL_TTL]))
		r[R_HOPCOUNT] = r[R_INITIAL_TTL]-r[R_TTL];
	else {
		//use heuristic
		if(r[R_TTL] <= 32)
			r[R_HOPCOUNT] = 32 - r[R_TTL];
		else if(r[R_TTL] <= 64)
			r[R_HOPCOUNT] = 64 - r[R_TTL];
		else if(r[R_TTL] <= 128)
			r[R_HOPCOUNT] = 128 - r[R_TTL];
		else
			r[R_HOPCOUNT] = 256 - r[R_TTL];
	}
	char dbg[512];
	snprintf(dbg, sizeof(dbg), "Ts: %f Hops: %f (%f-%f)", r[R_SEND_TIME], r[R_HOPCOUNT], r[R_TTL], r[R_INITIAL_TTL]);
	debugOutput(dbg);
	return r[R_HOPCOUNT];
}


HopcountMeasurePlugin::HopcountMeasurePlugin() {
	/* Initialise properties: MANDATORY! */
	name = "Hopcount";
	desc = "The distance between two peers in terms of hopcount";
	id = HOPCOUNT;
	/* end of mandatory properties */
}
