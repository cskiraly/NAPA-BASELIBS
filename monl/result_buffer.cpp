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

#include "result_buffer.h"
#include "errors.h"
#include	"grapes_log.h"
#include "repoclient.h"

#include "mon_event.h"

#include <arpa/inet.h>

#include <sys/time.h>
#include <stdlib.h>
#include <math.h>

//TODO:
//- check if pubblishing was succesfull


const char* ResultBuffer::stat_suffixes[] = {
	"_last",
	"_avg",
	"_win_avg",
	"_var",
	"_win_var",
	"_min",
	"_win_min",
	"_max",
	"_win_max",
	"_sum",
	"_win_sum",
	"_rate"
};

int ResultBuffer::publishResults(void){
	MeasurementRecord mr;
	SocketId sid;
	char sidA_string[SOCKETID_STRING_SIZE] = "";
	char sidB_string[SOCKETID_STRING_SIZE] = "";
	int errorstatus;
	result ts;

	if(m->param_values[P_PUBLISHING_PKT_TIME_BASED] == 1.0) {//time based
		struct timeval tv;
		float delta = m->param_values[P_PUBLISHING_RATE] * m->param_values[P_PUBLISHING_TIME_SPREAD] / 100.0; 
		tv.tv_sec = m->param_values[P_PUBLISHING_RATE] - delta +  2.0 * delta * (float) rand() / (float) RAND_MAX;
		tv.tv_usec = 0;
		schedule_publish(&tv, m);
	}

	if(m->status != RUNNING)
		return EOK;

	if(!new_data && m->param_values[P_PUBLISHING_NEW_ALL] == 0.0)
		return EOK;
	new_data = false;

	info("MONL: publishResults called: %s value: %f (%s)", publish_name.c_str(), stats[LAST], default_name);
	if(publish_length == 0)
		return EOK;
	/* Get local ID */
	sid = mlGetLocalSocketID(&errorstatus);
	if(errorstatus != 0)
		return -EFAILED;
	if(mlSocketIDToString(sid, sidA_string, sizeof(sidA_string)) != 0)
		return -EFAILED;
	if(m->dst_socketid != NULL) {
		if(mlSocketIDToString((SocketId) m->dst_socketid, sidB_string, sizeof(sidB_string)) != 0)
			return -EFAILED;
	}
	if(*originator_name == NULL)
		mr.originator = sidA_string;
	else
		mr.originator = *originator_name;
	mr.targetA = sidA_string;
	mr.targetB = sidB_string;
	mr.string_value = NULL;
	mr.channel = cnl.c_str();
	gettimeofday(&mr.timestamp, NULL);
	
	/* rate */
	ts =  mr.timestamp.tv_usec / 1000000.0;
	ts += mr.timestamp.tv_sec;
	if(last_publish != 0.0) {
		stats[RATE] = rate_sum_samples / (ts - last_publish);
	}
	last_publish = ts;
	rate_sum_samples = 0.0;
	
	for(int i = 0; i < publish_length; i++) {
		mr.value = stats[publish[i]];
		if(isnan(mr.value))
			continue;
		publish_name += stat_suffixes[publish[i]];
		mr.published_name = publish_name.c_str();
		debug("MONL: publishResults called: %s value: %f", mr.published_name, mr.value);
		repPublish(repo_client, NULL, NULL, &mr);
		publish_name.erase(name_size);
	}
	return EOK;
};

int ResultBuffer::init() {
	int i;
	n_samples = 0; samples = 0; sum_samples = 0;
	sum_win_samples = 0; rate_sum_samples = 0;
	pos = 0;
	sum_var_samples = sum_win_var_samples = 0;
	for(i = 0; i < LAST_STAT_TYPE; i++)
		stats[i] = (m->param_values[P_INIT_NAN_ZERO] == 0.0 ? NAN : 0);
	for(i = 0; i < size; i++)
		circular_buffer[i] = NAN;
	return EOK;
}

int ResultBuffer::resizeBuffer(int s) {
	result *old_buffer = circular_buffer;
	result *old_buffer_var = circular_buffer_var;
	int i,j, n_sam, old_pos, old_size;
	
	if(s == size)
		return EOK;


	circular_buffer = new result[s];
	circular_buffer_var = new result[s];

	n_sam = fmin(s, fmin(size, n_samples));
	old_pos = pos;
	old_size = size;

	sum_win_samples = sum_win_var_samples = 0;
	n_samples = 0; pos = 0; size = s;

	for(i = 0; i < n_sam; i++) {
		j = old_pos - n_sam + i;
		if(j < 0)
			j = j + old_size;
		newSampleWin(old_buffer[j]);
	}

	
	delete[] old_buffer;
	delete[] old_buffer_var;
	return EOK;
};

int ResultBuffer::newSample(result r) {
	samples++;

	stats[LAST] = r;

	//TODO add statistical data like avg

	sum_samples += r;
	rate_sum_samples += r;

	stats[SUM] = sum_samples;

	/* Average  */
	if(isnan(stats[AVG]))
		stats[AVG] = 0;
	stats[AVG] = ((samples - 1) * stats[AVG] + r) / samples;

	/* Variance */
	sum_var_samples += pow(r - stats[AVG],2);

	if(samples < 2)
		stats[VAR] = 0;
	else
		stats[VAR] = stats[VAR] * (samples - 2)/(samples - 1) + samples / pow(samples - 1, 2) * pow(r - stats[AVG],2);


	/* Minimum maximum */
	if(r < stats[MIN] || isnan(stats[MIN]))
		stats[MIN] = r;

	if(r > stats[MAX] || isnan(stats[MAX]))
		stats[MAX] = r;

	// Update window based stats
	newSampleWin(r);

	new_data = true;

	/* is it time to publish ?*/
	if(m->param_values[P_PUBLISHING_PKT_TIME_BASED] == 0.0 && samples % (int)m->param_values[P_PUBLISHING_RATE] == 0) {
		return publishResults();
	}

	return EOK;
};

int ResultBuffer::newSampleWin(result r) {
	int i,j;
	result var_s;

	//TODO add statistical data like avg

	/* sum */
	if(n_samples < size)	{
		n_samples++;
	} else {
		sum_win_samples -= circular_buffer[pos];
		sum_win_var_samples -= circular_buffer_var[pos];
	}
	sum_win_samples += r;

	stats[WIN_SUM] = sum_win_samples;	

	stats[WIN_AVG] = sum_win_samples/n_samples;

	var_s = pow(r - stats[WIN_AVG],2);
	sum_win_var_samples += var_s;

	if(n_samples > 1)
		stats[WIN_VAR] = sum_win_var_samples/(n_samples - 1);

	/* Minimum maximum */
	if(isnan(stats[WIN_MIN]))
		stats[WIN_MIN] = r;
	if(isnan(stats[WIN_MAX]))
		stats[WIN_MAX] = r;

	if(stats[WIN_MIN] == circular_buffer[pos] || stats[WIN_MAX] == circular_buffer[pos]) {
		circular_buffer[pos] = stats[WIN_MAX] = stats[WIN_MIN] = r;
		for(i = 0; i <= n_samples - 1; i++) {
			j = pos - i; // pos already incremented and last sample is starting one
			if(j < 0)
				j += size;
			if(circular_buffer[j] < stats[WIN_MIN])
				stats[WIN_MIN] = circular_buffer[j];
			if(circular_buffer[j] > stats[WIN_MAX])
				stats[WIN_MAX] = circular_buffer[j];
		}
	} else {
		if(r < stats[WIN_MIN]) 
			stats[WIN_MIN] = r;
		if (r > stats[WIN_MAX])
			stats[WIN_MAX] = r;
	}

	circular_buffer[pos] = r;
	circular_buffer_var[pos] = var_s;
	pos++;
	if(pos >= size)
		pos -= size;

	return EOK;
};
