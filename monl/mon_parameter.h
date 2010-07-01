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

#ifndef _MON_PARAMETER_H_
#define _MON_PARAMETER_H_

#include <string>
#include "mon.h"
#include "errors.h"

#define DEFAULT_DEFAULT_VALUE 0

class MeasurePlugin;

/* This class describes the basic MonParameterValue */
class MonParameter {

public:
	/* Constructor */
	MonParameter(std::string n,std::string d, MonParameterValue dv = 0): name(n), desc(d), dval(dv) {}

	/* The name of the measure e.g. window size*/
	const std::string name;

	/* Short description of the measure */
	const std::string desc;

	/* Default value */
	const MonParameterValue dval;

	/* Parameter handler */
	MonParameterType ph;

	/* Validation function */
	virtual int validator (MonParameterValue) {return EOK;};
};

/* A MonParameterValue with minimal value check v must be >= min */

class MinParameter : public MonParameter {
	/* the minimum acceptable value */
	MonParameterValue min;
public:
	/* Constructor */
	MinParameter(std::string n,std::string d, MonParameterValue m, MonParameterValue dv = DEFAULT_DEFAULT_VALUE):MonParameter(n,d,dv),min(m) {}

	/* Validation function */
	virtual int validator (MonParameterValue p) {
		if (p >= min)
			return EOK;
	return -EINVAL;
	};
};

/* A MonParameterValue with maximum value check: v must be <= max */

class MaxParameter : public MonParameter {
	/* the maximum acceptable value */
	MonParameterValue max;
public:
	/* Constructor */
	MaxParameter(std::string n,std::string d, MonParameterValue M, MonParameterValue dv = DEFAULT_DEFAULT_VALUE):MonParameter(n,d,dv),max(M) {}

	/* Validation function */
	virtual int validator (MonParameterValue p) {
		if (p <= max)
			return EOK;
	return -EINVAL;
	};
};


/* A MonParameterValue with interval value check: v must be >= min and <= max*/

class MinMaxParameter : public MonParameter {
	/* the minimum acceptable value */
	MonParameterValue min;
	/* the maximum acceptable value */
	MonParameterValue max;
public:
	/* Constructor */
	MinMaxParameter(std::string n,std::string d, MonParameterValue m, MonParameterValue M, MonParameterValue dv = DEFAULT_DEFAULT_VALUE):MonParameter(n,d,dv),min(m),max(M) {}

	/* Validation function */
	virtual int validator (MonParameterValue p) {
		if (p <= max && p >= min)
			return EOK;
	return -EINVAL;
	};
};
#endif /* _MON_PARAMETER_HH_ */
