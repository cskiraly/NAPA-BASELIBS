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

#include "clockdrift_measure.h"
#include "napa_log.h"
#include <math.h>


double ClockdriftMeasure::x_axis_inter(double a1, double b1, double a2, double b2) {
	return((b2-b1)/(a1-a2));
}

double ClockdriftMeasure::y_axis_inter(double a1, double b1, double a2, double b2) {
	return a1 * x_axis_inter(a1, b1, a2, b2) + b1;
}

bool clockdrift_sort_function (struct clockdrift_sample i,struct clockdrift_sample j)
{ return (i.tx_time<j.tx_time); }

ClockdriftMeasure::ClockdriftMeasure(class MeasurePlugin *m, MeasurementCapabilities mc, class MeasureDispatcher *md): MonMeasure(m,mc,md) {
	if(param_values[P_CLOCKDRIFT_ALGORITHM] != 1) {
		n = new int[(int)param_values[P_CLOCKDRIFT_WIN_SIZE]];
		samples.reserve((int)param_values[P_CLOCKDRIFT_WIN_SIZE]);
	} else
		n = NULL;
	a = 0;
	b = 0;
	first_tx = 0;
}

ClockdriftMeasure::~ClockdriftMeasure() {
	if(n != NULL) {
		delete[] n;
		n = NULL;
	}
}


void ClockdriftMeasure::init() {
	samples.clear();
	first_tx = NAN;
	pos = 0;
	pnum = 0;
	sum_t = sum_d = sum_t_v = sum_d_v = sum_td_v = 0;
}

void ClockdriftMeasure::stop() {
	r_tx_list[R_CLOCKDRIFT] = r_rx_list[R_CLOCKDRIFT] = NAN;
}

int ClockdriftMeasure::paramChange(MonParameterType ph, MonParameterValue p){
		switch(ph) {
		case P_CLOCKDRIFT_ALGORITHM:
			if(p != 1) {
				if(n == NULL) {
					n = new int[(int)param_values[P_CLOCKDRIFT_WIN_SIZE]];
					samples.reserve((int)param_values[P_CLOCKDRIFT_WIN_SIZE]);
					//TODO: possibly? keep old data
					samples.clear();
					first_tx = NAN;
					pos = 0;
					pnum = 0;
					sum_t = sum_d = sum_t_v = sum_d_v = sum_td_v = 0;
				}
			} else {
				if(n != NULL) {
					delete[] n;
					n = NULL;
				}
			}
			return EOK;
		case P_CLOCKDRIFT_WIN_SIZE:
			if(n != NULL) {
				delete[] n;
				n = new int[(int)p];
				samples.reserve((int)p);
			}
			//TODO: possibly? keep old data
			samples.clear();
			first_tx = NAN;
			pos = 0;
			pnum = 0;
			sum_t = sum_d = sum_t_v = sum_d_v = sum_td_v = 0;
			return EOK;
		default:
			return EOK;
	}
}

result ClockdriftMeasure::RxPkt(result *r,ExecutionList *el) {
	double tx_time, delay;
	result res = NAN;

	if(isnan(first_tx))
		first_tx = r[R_SEND_TIME];

	tx_time = r[R_SEND_TIME] - first_tx;
	delay = fabs(r[R_SEND_TIME] - r[R_RECEIVE_TIME]);
	pnum++;

	if(param_values[P_CLOCKDRIFT_ALGORITHM] == 1 || param_values[P_CLOCKDRIFT_ALGORITHM] == 0) {
		sum_t += tx_time;
		sum_t_v += pow(tx_time - sum_t/pnum, 2);
		sum_d += delay;
		sum_d_v += pow(delay - sum_d/pnum, 2);
		sum_td_v += (delay - sum_d/pnum) * (tx_time - sum_t/pnum);
	}

	if(param_values[P_CLOCKDRIFT_ALGORITHM] != 1) {
		samples[pos].tx_time = tx_time;
		samples[pos].delay = delay;
	}

	pos++;

	if(pos % (int)param_values[P_CLOCKDRIFT_PKT_TH] == 0) {
		int i,j,k,end;
		double m_t, m_d, v_t, v_d, v_td, sum;
		m_t = m_d = 0;

		if(param_values[P_CLOCKDRIFT_ALGORITHM] == 1 || param_values[P_CLOCKDRIFT_ALGORITHM] == 0) {
			r[R_CLOCKDRIFT] = b = sum_td_v / sum_t_v;
			a = sum_d/pnum - b * sum_t/pnum;

			char dbg[512];
			snprintf(dbg, sizeof(dbg), "Ts: %f Clockdrift 1 a: %f b: %f", r[R_RECEIVE_TIME], a, b);
			debugOutput(dbg);
		}

		end = pnum > (int)param_values[P_CLOCKDRIFT_WIN_SIZE] ? (int)param_values[P_CLOCKDRIFT_WIN_SIZE] : pos;

		if(param_values[P_CLOCKDRIFT_ALGORITHM] == 2 || param_values[P_CLOCKDRIFT_ALGORITHM] == 0) {
			for(i = 0; i < end; i++) {
				m_t += samples[i].tx_time;
				m_d += samples[i].delay;
			}
			m_t = m_t /i; m_d = m_d /i;
	
			v_t = v_d = v_td = 0;
			for(i = 0; i < end; i++) {
				v_t += pow(samples[i].tx_time - m_t, 2);
				v_d += pow(samples[i].delay - m_d, 2);
				v_td += (samples[i].tx_time - m_t) * (samples[i].delay - m_d);
			}
	
			r[R_CLOCKDRIFT] = b = v_td / v_t;
			a = m_d - b * m_t;

			char dbg[512];
			snprintf(dbg, sizeof(dbg), "Ts: %f Clockdrift 2 a: %f b: %f", r[R_RECEIVE_TIME], a, b);
			debugOutput(dbg);
		}

		if(param_values[P_CLOCKDRIFT_ALGORITHM] == 3 || param_values[P_CLOCKDRIFT_ALGORITHM] == 0) {
			std::sort(samples.begin(), samples.begin() + end, clockdrift_sort_function);
	
			/* start estimation */
			n[0] = 0; n[1] = 1; k = 1;
			for (i = 2; i < end; i++) {
				for (j = k; j >= 1; j--) {
					if (x_axis_inter(samples[i].tx_time, -samples[i].delay, samples[n[j]].tx_time, -samples[n[j]].delay)
							< x_axis_inter(samples[n[j]].tx_time, -samples[n[j]].delay,samples[n[j-1]].tx_time, -samples[n[j-1]].delay))
						break;
				}
				k = j + 1;
				n[k] = i;
			}
	
			sum = 0.0;
			for (i = 0; i < end; i++)
				sum += samples[i].tx_time;
	
			sum /= end;
			for (i = 0; i < k - 1; i++) {
				if (samples[n[i]].tx_time< sum && sum <= samples[n[i+1]].tx_time) {
					r[R_CLOCKDRIFT] = b = x_axis_inter(samples[n[i]].tx_time, -samples[n[i]].delay, samples[n[i+1]].tx_time, -samples[n[i+1]].delay);
					a = -y_axis_inter(samples[n[i]].tx_time, -samples[n[i]].delay, samples[n[i+1]].tx_time, -samples[n[i+1]].delay);
					break;
				}
			}

			char dbg[512];
			snprintf(dbg, sizeof(dbg), "Ts: %f Clockdrift 3 a: %f b: %f", r[R_RECEIVE_TIME], a, b);
			debugOutput(dbg);
		}
		res = r[R_CLOCKDRIFT];
	}

	if(pos == param_values[P_CLOCKDRIFT_WIN_SIZE])
		pos = 0;

	return res;
}

result ClockdriftMeasure::RxData(result *r,ExecutionList *el) {
	return RxPkt(r, el);
}

ClockdriftMeasurePlugin::ClockdriftMeasurePlugin() {
	/* Initialise properties: MANDATORY! */
	name = "Clock Drift";
	desc = "The drift between the clocks of the two systems";
	id = CLOCKDRIFT;
	/* end of mandatory properties */
	addParameter(new MinMaxParameter("Algorithm","1 - Linear regression (interactive), 2 - Linear regression, 3 - Linear programming",0,3,0), P_CLOCKDRIFT_ALGORITHM);
	addParameter(new MinParameter("Packet threshold","How often to compute the drift",100,100), P_CLOCKDRIFT_PKT_TH);
	addParameter(new MinParameter("Window size","The window size on which to compute",100,1000), P_CLOCKDRIFT_WIN_SIZE);
}
