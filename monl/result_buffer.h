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

#ifndef _RESULT_BUFFER_H_
#define _RESULT_BUFFER_H_

#include "ids.h"
#include <string.h>
#include <string>
#include <math.h>
#include "ml.h"
#include "mon.h"

#include "grapes_log.h"

class ResultBuffer {
private:
	class MonMeasure *m;
	static const char *stat_suffixes[];
	result *circular_buffer;
	result *circular_buffer_var;
	int size,pos;

	bool new_data;
	result last_publish;

	int samples;

	int n_samples;
	result sum_samples;
	result sum_win_samples;
	result rate_sum_samples;

	result sum_var_samples;
	result sum_win_var_samples;

	char **originator_name;

	std::string publish_name;
	std::string cnl;
	const char *default_name;
	int name_size;
	enum stat_types *publish;
	int publish_length;

	void *repo_client;
	int newSampleWin(result r);

public:
	result stats[LAST_STAT_TYPE];

	ResultBuffer(int s, const char *pname, char** oname, MonMeasure *ptr_m): size(s), publish(NULL), publish_length(0), new_data(false), m(ptr_m) {
		last_publish = 0.0;
		circular_buffer = new result[s];
		circular_buffer_var = new result[s];
		default_name = pname;
		originator_name = oname;
		publish_name += default_name;
		name_size = publish_name.size();
		init();
	};

	~ResultBuffer() {
		delete[] circular_buffer;
		delete[] circular_buffer_var;
		if(publish) { delete[] publish;}
	};

	int init();

	int setPublishStatisticalType(const char *publishing_name, const char *channel, enum stat_types *st, int length, void *rc) {
		publish_name.clear();

		if(publishing_name != NULL)
			publish_name = publishing_name;
		else
			publish_name = default_name;
		name_size = publish_name.size();

		if(channel != NULL)
			cnl = channel;
		else
			cnl = "";

		if(publish)
			delete[] publish;
		publish = new stat_types [length];
		memcpy(publish,st, length * sizeof(enum stat_types));
		publish_length = length;
		repo_client = rc;

		return publishResults();
	}

	int publishResults(void);
	int resizeBuffer(int s);
	int newSample(result r);
};

#endif
