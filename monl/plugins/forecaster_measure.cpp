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
#include "forecaster_measure.h"
#include "napa_log.h"
#include <sys/time.h>
#include <limits.h>

#ifdef WIN32
#define random(x) rand(x)
#endif



ForecasterMeasure::ForecasterMeasure(class MeasurePlugin *m, MeasurementCapabilities mc, class MeasureDispatcher *md): MonMeasure(m,mc,md) {
}

ForecasterMeasure::~ForecasterMeasure() {
}

void ForecasterMeasure::init() {
	if(pkt != NULL)
		delete[] pkt;
	pkt = new char[(int) param_values[P_FORECASTER_PAYLOAD_SIZE]];

	p_num = 0;
	p_below = 0;
	p_above = 0;
	min_delay = NAN;
	avg_pause = FORECASTER_MTU / param_values[P_FORECASTER_RATE] * 8000.0;// [us]
}

void ForecasterMeasure::stop() {
	r_tx_list[R_AVAILABLE_BW_FORECASTER] = r_rx_list[R_AVAILABLE_BW_FORECASTER] = NAN;
}

int ForecasterMeasure::paramChange(MonParameterType ph, MonParameterValue p){
	switch(ph) {
		case P_FORECASTER_PAYLOAD_SIZE:
			param_values[P_FORECASTER_PAYLOAD_SIZE] = p;
			if(pkt != NULL)
				delete[] pkt;
			pkt = new char[(int) param_values[P_FORECASTER_PAYLOAD_SIZE]];
			avg_pause = param_values[P_FORECASTER_PAYLOAD_SIZE] / param_values[P_FORECASTER_RATE] * 8000.0;
			break;
		case P_FORECASTER_RATE:
			param_values[P_FORECASTER_RATE] = p;
			avg_pause = param_values[P_FORECASTER_PAYLOAD_SIZE] / param_values[P_FORECASTER_RATE] * 8000.0;
			break;
		case P_FORECASTER_PKT_TH:
			param_values[P_FORECASTER_PKT_TH] = p;
			break;
		case P_CAPPROBE_DELAY_TH:
			param_values[P_CAPPROBE_DELAY_TH] = p;
			break;
		case P_FORECASTER_RELATIVE:
			param_values[P_FORECASTER_RELATIVE] = p;
			break;
		default:
			return -EINVAL;
	}
	return EOK;
}

double ForecasterMeasure::gen_pause_exp(double avg_pause){
	double p = random();
	/*turn this into a random number between 0 and 1...*/
	double f = p / (double) INT_MAX;

	return -1 * avg_pause * log(1-f);
}

void ForecasterMeasure::Run() {
	double pause;
	if(flags & REMOTE)
		return;

	struct timeval *tv = (struct timeval*) pkt;
	gettimeofday(tv,NULL);
	sendOobData(pkt, param_values[P_FORECASTER_PAYLOAD_SIZE]);

	struct timeval next = {0,0};

	pause = gen_pause_exp(avg_pause);
	next.tv_sec = (unsigned long)pause / 1000000;
	next.tv_usec = (unsigned long)fmod(pause, 100000.0);

	scheduleNextIn(&next);
}

void ForecasterMeasure::receiveOobData(char *buf, int buf_len, result *r) {
	if(flags & REMOTE) { //remote
		result ab;
		ab = computeAvailableBandwidth(r);
		if(!isnan(ab))
			sendOobData((char *) &ab, sizeof(ab));
	} else { //local
		result *ab = (result *) pkt;
		newSample(*ab);
	}
}

result ForecasterMeasure::RxPkt(result *r,ExecutionList *el) {
	if(!(flags & REMOTE)) //local: nothing to do
		return NAN;
	
	return computeAvailableBandwidth(r);
}

result ForecasterMeasure::computeAvailableBandwidth(result *r) {
	p_num++;

	if(r[R_CORRECTED_DELAY] < min_delay || isnan(min_delay)) {
		min_delay = r[R_CORRECTED_DELAY];
	}
	
	if (r[R_CORRECTED_DELAY] < min_delay +  param_values[P_FORECASTER_DELAY_TH] / 1000000.0) {
		p_below++;
	}

	if(p_num >= param_values[P_FORECASTER_PKT_TH]) {

		if(param_values[P_FORECASTER_RELATIVE])
			r_rx_list[R_AVAILABLE_BW_FORECASTER] = (double) p_below / (double) p_num * 100;
		else
			r_rx_list[R_AVAILABLE_BW_FORECASTER] = (double) p_below / (double) p_num * r_rx_list[R_CAPACITY_CAPPROBE];

		p_num = p_below = 0;
		min_delay = NAN;

		char dbg[512];
		snprintf(dbg, sizeof(dbg), "Ts: %f Ab: %f", r[R_RECEIVE_TIME], r_rx_list[R_AVAILABLE_BW_FORECASTER]);
		debugOutput(dbg);

		return r_rx_list[R_AVAILABLE_BW_FORECASTER];
	}
	return NAN;
}

ForecasterMeasurePlugin::ForecasterMeasurePlugin() {
	/* Initialise properties: MANDATORY! */
	name = "Available Bandwidth (forecaster)";
	desc = "The available bandwidth in kbit/s computed using forecaster";
	id = AVAILABLE_BW_FORECASTER;
	/* end of mandatory properties */
	addParameter(new MinParameter("Rate","The rate at which we probe the path in kbit/s",0,1000), P_FORECASTER_RATE);
	addParameter(new MinParameter("Packet Threshold","Update available bandwidth each # pkts",0,100), P_FORECASTER_PKT_TH);
	addParameter(new MinParameter("Delay Threshold","Delay threshold tolerance [us]",0,100), P_FORECASTER_DELAY_TH);
	addParameter(new MinParameter("Payload","The size of the payload for injected packets [bytes]",sizeof(uint32_t), FORECASTER_MTU), P_FORECASTER_PAYLOAD_SIZE);
	addParameter(new MinMaxParameter("Relative","Chooses between absolute (0) [kbit/s] and relative (1) [%]", 0, 1, 0), P_FORECASTER_RELATIVE);
}
