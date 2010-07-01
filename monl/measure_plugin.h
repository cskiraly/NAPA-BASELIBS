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

#ifndef _MEASURE_PLUGIN_H
#define _MEASURE_PLUGIN_H

#include "ids.h"
#include "mon.h"
#include "mon_parameter.h"
#include <vector>

class MeasurePlugin {
protected:
	eMeasureId id;
	std::string name;
	std::string desc;

	std::vector<MeasurementId> deps;

	void addParameter(class MonParameter *mp, MonParameterType ph) {
		mp->ph = ph;
		params.insert(params.begin() + ph, mp);
	};

	void addDependency(eMeasureId mid) {
		deps.insert(deps.end(), mid);
	};

	/* Contains all the parameters defined for this measure e.g. the size of
	* the sliding window */
	std::vector<class MonParameter*> params;
	friend class MonMeasure;
	friend class MeasureDispatcher;

public:
	MeasurePlugin() {
		//Add default parameters
		addParameter(new MinParameter("Window Size","The size of sliding window over which to computer statistics",2,100), P_WINDOW_SIZE);
		addParameter(new MinMaxParameter("Publishing packet or time based","Publishing based on packets [#] (0) or on time [s] (1)",0,1,1), P_PUBLISHING_PKT_TIME_BASED);
		addParameter(new MinParameter("Publishing Rate","Rate in # of samples or [s] to publish results",1,120), P_PUBLISHING_RATE);
		addParameter(new MinMaxParameter("Publishing Rate Time-spread","Spread factor to avoid result publish syncronisation", 0, 100, 10), P_PUBLISHING_TIME_SPREAD);
		addParameter(new MinMaxParameter("Publishing new or all","Publish only new results or everything", 0, 1, 0), P_PUBLISHING_NEW_ALL);
		addParameter(new MinMaxParameter("Init NAN or 0","Initialize stats to NAN or to 0", 0, 1, 0), P_INIT_NAN_ZERO);
		addParameter(new MinMaxParameter("Debug Output","Activate/deactive debug output to file (1), to log (2) or both (3)", 0, 3, 0), P_DEBUG_FILE);
		addParameter(new MonParameter("Exectuion Frequency","How often should it be run"), P_OOB_FREQUENCY);
	};

	virtual ~MeasurePlugin() {
		std::vector<class MonParameter*>::iterator it;
		for (it = params.begin(); it != params.end(); it++)
			delete (*it);
	};

	/* Contains the ids on which this element depends on (if any) */
	const std::vector<MeasurementId> &getDeps() {return deps;};
	/* Get the name of the measure e.g. Round Trip Time */
	const std::string &getName() {return name;};
	 /* Short description of the measure */
	const std::string &getDesc() {return desc;};
	/* List of paramters */
	const std::vector<class MonParameter *> &getParams() {return params;};
	/* Id of the measure. Must be unique. Defines also the position within
	* the execution list. Lower Ids are computed first higher Ids later. Important for
	* dependency on other measures */
	MeasurementId getId() {return id;};
	/* Bitmask defining the capabilities of the measure */
	virtual MeasurementCapabilities getCaps() = 0;
	/* Used to create the real instances of the measure */
	virtual class MonMeasure* createMeasure (MeasurementCapabilities, class MeasureDispatcher *md) = 0;
};

#endif
