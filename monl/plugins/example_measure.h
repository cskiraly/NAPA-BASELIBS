/***************************************************************************
 *   Copyright (C) 2009 by Robert Birke   *
 *   robert.birke@polito.it   *
 *                                                                         *
 *   This program is free software; you can redistribute it and/or modify  *
 *   it under the terms of the GNU General Public License as published by  *
 *   the Free Software Foundation; either version 2 of the License, or     *
 *   (at your option) any later version.                                   *
 *                                                                         *
 *   This program is distributed in the hope that it will be useful,       *
 *   but WITHOUT ANY WARRANTY; without even the implied warranty of        *
 *   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the         *
 *   GNU General Public License for more details.                          *
 *                                                                         *
 *   You should have received a copy of the GNU General Public License     *
 *   along with this program; if not, write to the                         *
 *   Free Software Foundation, Inc.,                                       *
 *   59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.             *
 ***************************************************************************/

#ifndef _EXAMPLE_MEASURE_H_
#define _EXAMPLE_MEASURE_H_

#include <string>
#include "measure_plugin.h"
#include "mon_measure.h"

class ExampleMeasure : public MonMeasure {

public:
	ExampleMeasure(class MeasurePlugin *m, MeasurementCapabilities mc, class MeasureDispatcher *md): MonMeasure(m,mc,md) {};
	virtual result RxPkt(result *r,ExecutionList *el) {
		//add code or remove function
		return NAN;
	};
	virtual result RxData(result *r,ExecutionList *el) {
		//add code or remove function
		return NAN;
	};
	virtual result TxPkt(result *r,ExecutionList *el) {
		//add code or remove function
		return NAN;
	};
	virtual result TxData(result *r,ExecutionList *el) {
		//add code or remove function
		return NAN;
	};

	virtual void stop() {
	};
};

class ExampleMeasurePlugin : public MeasurePlugin {
	virtual ~ExampleMeasurePlugin() {};


	virtual MeasurementCapabilities getCaps() {
		return IN_BAND | PACKET | DATA;
	};

	virtual MonMeasure* createMeasure (MeasurementCapabilities mc, class MeasureDispatcher *md) {
		return new ExampleMeasure(this, mc | RXONLY, md);
	};
public:
	ExampleMeasurePlugin(): MeasurePlugin() {
		/* Initialise properties: MANDATORY! */
		name = "Example Measure";
		desc = "This is a simple sample code for creating a new measure";
		id = EXAMPLE;
		/* end of mandatory properties */
		/* Create parameters */
		addParameter(new MonParameter ("Param1","Example Parameter 1"),0);
		addParameter(new MonParameter ("Param2","Example Parameter 2",10),1);
		/* Set dependencies (if any) */
	};
};



#endif /* _EXAMPLE_MEASURE_H_ */
