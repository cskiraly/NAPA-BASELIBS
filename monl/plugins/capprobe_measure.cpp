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

#include "capprobe_measure.h"
#include "napa_log.h"
#include <sys/time.h>
#include <math.h>
#include <string>

#ifdef _WIN32
#define random(x) rand(x)
#endif

//defualt values only! can be changed using the changeParam() function
#define LOWER_LAYER_OVERHEAD 8+20+18+8+12
#define CAPPROBE_MTU 1300
#define CAPPROBE_SAMPLE_WINDOW 100

bool double_sort_function (double i,double j) { return (i<j); }


CapprobeMeasure::CapprobeMeasure(class MeasurePlugin *m, MeasurementCapabilities mc, class MeasureDispatcher *md): MonMeasure(m,mc,md) {
	pkt = NULL;
	max_size = 0;
}

CapprobeMeasure::~CapprobeMeasure() {
	if(pkt != NULL)
		delete[] pkt;
}

void CapprobeMeasure::init() {
	samples.clear();
	samples.reserve((int) param_values[P_CAPPROBE_PKT_TH]);

	if(pkt != NULL)
		delete[] pkt;
	pkt = new char[(int) param_values[P_CAPPROBE_PAYLOAD_SIZE]];

	avg_pause = param_values[P_CAPPROBE_PAYLOAD_SIZE] / param_values[P_CAPPROBE_RATE] * 8000.0;// [us]
	pair_num = 0;
	min_delay = NAN;
	min_ipd = NAN;
	last_rx = NAN;
	last_rx_pn = NAN;
	max_ipd = NAN;
	last_size = NAN;
}

void CapprobeMeasure::stop() {
	r_rx_list[R_CAPACITY_CAPPROBE] = r_tx_list[R_CAPACITY_CAPPROBE] = NAN;
	if(pkt != NULL)
		delete[] pkt;
	pkt = NULL;
}


int CapprobeMeasure::paramChange(MonParameterType ph, MonParameterValue p){
	switch(ph) {
		case P_CAPPROBE_RATE:
			param_values[P_CAPPROBE_RATE] = p;
			avg_pause = param_values[P_CAPPROBE_PAYLOAD_SIZE] / param_values[P_CAPPROBE_RATE] * 8000.0;
			break;
		case P_CAPPROBE_PAYLOAD_SIZE:
			param_values[P_CAPPROBE_PAYLOAD_SIZE] = p;
			if(pkt != NULL)
				delete[] pkt;
			pkt = new char[(int) param_values[P_CAPPROBE_PAYLOAD_SIZE]];
			avg_pause = param_values[P_CAPPROBE_PAYLOAD_SIZE] / param_values[P_CAPPROBE_RATE] * 8000.0;
			break;
		case P_CAPPROBE_PKT_TH:
			param_values[P_CAPPROBE_PKT_TH] = p;
			samples.clear();
			samples.reserve((int) param_values[P_CAPPROBE_PKT_TH]);
			break;
		case P_CAPPROBE_DELAY_TH:
			param_values[P_CAPPROBE_DELAY_TH] = p;
			break;
		case P_CAPPROBE_IPD_TH:
			param_values[P_CAPPROBE_IPD_TH] = p;
			break;
		case P_CAPPROBE_HEADER_SIZE:
			param_values[P_CAPPROBE_HEADER_SIZE] = p;
			break;
	}
	return EOK;
}

double CapprobeMeasure::gen_pause_exp() {
	double p = random();
	/*turn this into a random number between 0 and 1...*/
	double f = p / (double) INT_MAX;
	return -1 * avg_pause * log(1-f);
}

void CapprobeMeasure::Run() {
	double pause;
	uint32_t *pn = (uint32_t*) pkt;

	if(flags & REMOTE)
		return;

	*pn = pair_num++;
	sendOobData(pkt, CAPPROBE_MTU);
	sendOobData(pkt, CAPPROBE_MTU);

	struct timeval next;

	pause = gen_pause_exp();
	next.tv_sec = (unsigned long)pause / 1000000;
	next.tv_usec = (unsigned long)fmod(pause, 100000.0);

	scheduleNextIn(&next);
}

void CapprobeMeasure::receiveOobData(char *buf, int buf_len, result *r) {
	uint32_t *pn = (uint32_t*) pkt;

	if(flags & REMOTE) { //remote
		result cap;
		//look for min delay
		if(min_delay > r[R_CORRECTED_DELAY] || isnan(min_delay))
			min_delay = r[P_CAPPROBE_FILTER];
	
		//take only consecutive packets
		if(*pn == last_rx_pn) {
			cap = computeCapacity(last_rx_delay, r[R_CORRECTED_DELAY], r[R_RECEIVE_TIME] - last_rx, r[R_SIZE], r);
			if(!isnan(cap))
				sendOobData((char *) &cap, sizeof(cap));
		}

		last_rx = r[R_RECEIVE_TIME];
		last_rx_delay = r[R_CORRECTED_DELAY];
		last_rx_pn = *pn;

	} else  {
		result *cap = (result *) pkt;
		newSample(*cap);
	}
}

result CapprobeMeasure::RxPkt(result *r,ExecutionList *el) {
	result cap = NAN;

	if(!(flags & REMOTE)) //local: nothing to do
		return NAN;

	//look for min delay
	if(min_delay > r[R_CORRECTED_DELAY] || isnan(min_delay))
		min_delay = r[P_CAPPROBE_FILTER];

	//take only consecutive packets
	if(r[R_DATA_ID] == last_rx_pn)
	// &&  next_offset == r[R_DATA_OFFSET]) TODO: should also check in order
		cap = computeCapacity(last_rx_delay, r[R_CORRECTED_DELAY], r[R_RECEIVE_TIME] - last_rx, last_size, r);

	last_rx = r[R_RECEIVE_TIME];
	last_rx_delay = r[R_CORRECTED_DELAY];
	last_rx_pn = r[R_DATA_ID];
	last_size = r[R_SIZE];
	
	return cap;
}


result CapprobeMeasure::computeCapacity(result delay1, result delay2, result ipg, result size, result *r) {

	//filter on min_delay
	if((delay1 < min_delay + param_values[P_CAPPROBE_DELAY_TH] / 1000000.0 &&
		delay2 < min_delay + param_values[P_CAPPROBE_DELAY_TH] / 1000000.0) ||
		param_values[P_CAPPROBE_DELAY_TH] < 0)
	{
		if(!isnan(ipg)) {
				samples.push_back(ipg);
				if(ipg < min_ipd || isnan(min_ipd))
					min_ipd = ipg;
				if(ipg > max_ipd || isnan(max_ipd))
					max_ipd = ipg;
		}
	}

	if(samples.size() >= param_values[P_CAPPROBE_PKT_TH]) {// when collected enough data try to make an estimate
		int j,i,c, max_c, i_max;
		result threshold, interval;

		if(max_size < size || isnan(max_size))
			max_size = size;

		std::sort(samples.begin(), samples.end(), double_sort_function);
		
		switch((int) param_values[P_CAPPROBE_FILTER]) {
		case 0: //pdf based
				c = 0; max_c = -1; i_max = 0;
				interval = (max_ipd - min_ipd) / param_values[P_CAPPROBE_NUM_INTERVALS];
				threshold = min_ipd + interval;
				for(i = 0; i < samples.size(); i++) {
					if(samples[i] >= threshold) {
						if(c > max_c) {
							max_c = c;
							i_max = i - c;
						}
						while(samples[i] >= threshold)
							threshold += interval;
						c = 1;
					} else
						c++;
				}
				j = (i_max + max_c / 2);
				break;
		case 1: //median based
				for(j=0; j < samples.size(); j++) {
						if(samples[j] > min_ipd * (1 + param_values[P_CAPPROBE_IPD_TH] / 100.0))
							break;
					}
				j = (int)(samples.size() * 0.1) / 2;
				break;
		}

		r_rx_list[R_CAPACITY_CAPPROBE] = r_tx_list[R_CAPACITY_CAPPROBE] =  (max_size + param_values[P_CAPPROBE_HEADER_SIZE]) * 8.0 / samples[j];

		char dbg[512];
		snprintf(dbg, sizeof(dbg), "Ts: %f Size: %f Cap: %f", r[R_SEND_TIME], max_size, r_rx_list[R_CAPACITY_CAPPROBE]);
		debugOutput(dbg);
		for(j=0; j < samples.size(); j++) {
			snprintf(dbg, sizeof(dbg), "Ipg: %f", samples[j]);
			debugOutput(dbg);
		}

		samples.clear();
		min_delay = NAN;
		min_ipd = NAN;
		max_ipd = NAN;
		max_size = NAN;
		return r_rx_list[R_CAPACITY_CAPPROBE];
	}
	return NAN;
}

CapprobeMeasurePlugin::CapprobeMeasurePlugin() {
	/* Initialise properties: MANDATORY! */
	name = "Capacity (capprobe)";
	desc = "The capacity in kbit/s computed using capprobe";
	id = CAPACITY_CAPPROBE;
	/* end of mandatory properties */
	addParameter(new MinParameter("Rate","The rate at which we probe the path in kbit/s",0,100), P_CAPPROBE_RATE);
	addParameter(new MinParameter("Packet Threshold","The minimum number of packets to process before considering the value somehow attendible [#]",0,CAPPROBE_SAMPLE_WINDOW), P_CAPPROBE_PKT_TH);
	addParameter(new MinParameter("Delay Threshold","The tolerance which is added to the minimum delay [us]",-1,100), P_CAPPROBE_DELAY_TH);
	addParameter(new MinParameter("Inter-pair delay threshold","The tolerance to the inter-pair delay [%]",0,60), P_CAPPROBE_IPD_TH);
	addParameter(new MinParameter("Header size","The size of the lower layer headers [bytes]",0, LOWER_LAYER_OVERHEAD), P_CAPPROBE_HEADER_SIZE);
	addParameter(new MinParameter("Payload","The size of the payload for injected packets [bytes]",sizeof(uint32_t), CAPPROBE_MTU), P_CAPPROBE_PAYLOAD_SIZE);
	addParameter(new MinMaxParameter("Filter","The filter algorithm used: 0 - PDF 1 - Median",0,1,0), P_CAPPROBE_FILTER);
	addParameter(new MinParameter("Intervals","The number of intervals for the PDF [#]",0, 1000), P_CAPPROBE_NUM_INTERVALS);
	//TODO addDependency(CORRECTED_DELAY);
}
