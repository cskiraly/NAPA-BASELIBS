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

#include "seqwin_measure.h"

SeqWinMeasure::SeqWinMeasure(class MeasurePlugin *m, MeasurementCapabilities mc, class MeasureDispatcher *md): MonMeasure(m,mc,md), win_valid(false),
		largest_sn(0), initial_sn(0)
{
	sn_win = new uint32_t[(int) param_values[P_SEQN_WIN_SIZE]];
}

SeqWinMeasure::~SeqWinMeasure() {
	delete[] sn_win;
}

int SeqWinMeasure::paramChange(MonParameterType ph, MonParameterValue p) {
	switch(ph) {
		case P_SEQN_WIN_SIZE:
			delete[] sn_win;
			sn_win = new uint32_t[(int) p];
			//TODO: possibly improve to keep previous values
			pnum = 0;
			return EOK;
		default:
			return EOK;
	}
}

void SeqWinMeasure::init() {
	pnum = 0;
}

void SeqWinMeasure::stop() {
	r_tx_list[R_SEQWIN] = r_rx_list[R_SEQWIN] = NAN;
}

result SeqWinMeasure::RxPkt(result *r, ExecutionList *el) {
	uint32_t ind;
	uint32_t delta_sn_win;
	uint32_t p_sn = (uint32_t)r[R_SEQNUM];

	//first packet?
	if(pnum == 0) {
		initial_sn = largest_sn = p_sn;
		//TODO: check this
		for(ind = 0; ind < (int) param_values[P_SEQN_WIN_SIZE]; ind++)
			sn_win[ind] = p_sn;
		win_valid = false;
		pnum = 1;
		return NAN;
	}

	ind = p_sn - initial_sn;
	if(ind >= param_values[P_SEQN_WIN_SIZE])
		win_valid = true; //ew filled at least 1 window
	ind = ind % (uint32_t) param_values[P_SEQN_WIN_SIZE];
	delta_sn_win = p_sn - sn_win[ind];

	if(p_sn - largest_sn < param_values[P_SEQN_WIN_OVERFLOW_TH])
		largest_sn = p_sn;

	if(!win_valid) {
		sn_win[ind] = p_sn;
		//check for overflow (TODO: needed ? ask marco)
	} else if(delta_sn_win < param_values[P_SEQN_WIN_OVERFLOW_TH])
		sn_win[ind] = p_sn;

	if(delta_sn_win == 0)
		r[R_SEQWIN] = 0;
	else if(win_valid)
		r[R_SEQWIN] = delta_sn_win;

	return r[R_SEQWIN];
}

result SeqWinMeasure::RxData(result *r, ExecutionList *el) {
	return RxPkt(r, el);
}

SeqWinMeasurePlugin::SeqWinMeasurePlugin() {
	/* Initialise properties: MANDATORY! */
	name = "Sequence Number Window";
	desc = "A sequence number window handler as support to loss, late, dup, burst measures";
	id = SEQWIN;
	/* end of mandatory properties */
	addParameter(new MinParameter ("SeqnumWinSize","Sequence number window size",1,5),P_SEQN_WIN_SIZE);
	addParameter(new MinParameter ("SeqnumWinOverflow","Sequence number window overflow detection threshold",1,500),P_SEQN_WIN_OVERFLOW_TH);
}



