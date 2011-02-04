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

#ifndef _MON_MEASURE_HH_
#define _MON_MEASURE_HH_

#include <vector>
#include <bitset>
#include <map>
#include "mon_parameter.h"
#include "measure_plugin.h"
#include "ids.h"
#include "mon.h"
#include <math.h>
#include "result_buffer.h"
#include "ml.h"

#include "napa_log.h"

#include <fstream>

/************************** monMeasure ********************************/

/* Measures can be:
 * - IN_BAND, OUT_OF_BAND, BOTH
 * - PACKET,DATA, BOTH
 * This is stored in a bitset called caps. If the bit is set the
 * corresponding type is true. flags contians how a particular instance
 * is used */

#pragma pack(push)  /* push current alignment to stack */
#pragma pack(1)     /* set alignment to 1 byte boundary */
struct res_mh_pair {
	result res;
	MonHandler mh;
};
#pragma pack(pop)   /* restore original alignment from stack */

typedef std::map<MeasurementId, class MonMeasure*> ExecutionList;

class MonMeasure {
	int rx_cnt;
	result meas_sum;
	int meas_cnt;
protected:
	bool auto_loaded;

	/* Bitmask defining the capabilities of this instance of the measure */
	MeasurementCapabilities flags;

	/* Pointer to the MeasurePlugin class describing this measure */
	class MeasurePlugin *measure_plugin;

	/* Status of the measurement instance */
	MeasurementStatus status;

	MonHandler mh_remote;

	uint8_t dst_socketid[SOCKETID_SIZE];
	bool dst_socketid_publish;
	MsgType  msg_type;

	int used_counter;

	ResultBuffer *rb;

	/* Parameter values */
	MonParameterValue *param_values;

	int tx_every;

	result every(result &m);

	void debugInit(const char *);
	void debugStop();
	std::fstream output_file;

	int paramChangeDefault(MonParameterType ph, MonParameterValue p) {
		if(ph == P_WINDOW_SIZE && rb != NULL)
			return rb->resizeBuffer((int)p);
		if(ph == P_INIT_NAN_ZERO && rb != NULL)
			return rb->init();
		if(ph == P_DEBUG_FILE) {
			if(param_values[P_DEBUG_FILE] == p)
				return EOK;
			if(p == 1)
				debugInit(measure_plugin->name.c_str());
			if(p == 0)
				debugStop();
			return EOK;
		}
		if(ph < P_LAST_DEFAULT_PARAM)
			return EOK;
		return paramChange(ph,p);
	}

	result *r_rx_list;
	result *r_tx_list;

	/* called when the measure is put on an execution list */
	/* should be used to reinitialise the measurement in subsequent uses */
	virtual void init() {};
	virtual void stop() {};


	friend class MeasureManager;
	friend class MeasureDispatcher;
	friend class ResultBuffer;
public:

	void debugOutput(char *out);

	class MeasureDispatcher *ptrDispatcher;
	MonHandler mh_local;

	/* Functions */
	/* Constructor: MUST BE CALLED BY DERIVED CLASSES*/
	MonMeasure(class MeasurePlugin *mp, MeasurementCapabilities mc, class MeasureDispatcher *ptrDisp);

	virtual ~MonMeasure() {
		delete[] param_values;
		if(rb != NULL)
			delete rb;
	};

	/* Get vector of MonParameterValues (pointers to the MonParameterValue instances) */
	std::vector<class MonParameter* >& listParameters(void) {
		return measure_plugin->params;
	};

	/* Set MonParameterValue value */
	int setParameter(size_t pid, MonParameterValue p) {
		if(pid < measure_plugin->params.size() && measure_plugin->params[pid]->validator(p) == EOK) {
			MonParameterValue pt;
			pt = param_values[pid];
			param_values[pid] = p;
			if(paramChangeDefault(pid,p) == EOK) {
				return EOK;
			}
			param_values[pid] = pt;
		}
		return -EINVAL;
	};

	/* Get MonParameterValue value */
	MonParameterValue getParameter(size_t pid) {
		if(pid >= measure_plugin->params.size())
			return NAN;
		return param_values[pid];
	};

	MeasurementStatus getStatus() {
	return status;
	}

	/* Get flags */
	MeasurementCapabilities getFlags() {
		return flags;
	}

	/* Set flags */
	int setFlags(MeasurementCapabilities f) {
		if(f == measure_plugin->getCaps() && f == 0)
			return EOK;
		/*check that f is a subset of Caps */
		if(f & ~(measure_plugin->getCaps() | TXRXBI | REMOTE))
			return -ERANGE;
		/* Check packet vs chunk */
		if(!((f & PACKET)^(f & DATA)))
			return -ERANGE;
		/* check inband vs out-of-band */
		if(f & TIMER_BASED)
			flags |= TIMER_BASED | IN_BAND | OUT_OF_BAND;
		else 
			if(!((f & IN_BAND)^(f & OUT_OF_BAND)))
				return -ERANGE;
		return EOK;
	}

	int setPublishStatisticalType(const char * publishing_name, const char *channel, enum stat_types st[], int length, void *repo_client) {
		if(rb == NULL)
		{
			info("rb == NULL");
			return -EINVAL;
		}

		int result = rb->setPublishStatisticalType(publishing_name, channel, st, length, repo_client);

		return result;
	};

	void defaultInit() {
		init();
		if(param_values[P_DEBUG_FILE] == 1.0)
			debugInit(measure_plugin->name.c_str());
	};

	void defaultStop() {
		if(rb)
			rb->publishResults();
		stop();
		if(param_values[P_DEBUG_FILE] == 1.0)
			debugStop();
	};

	/* called when  changes in parameter values happen */
	virtual int paramChange(MonParameterType ph, MonParameterValue p) {return EOK;};


	void RxPktLocal(ExecutionList *el) {
		if(r_rx_list == NULL)
			return;
		newSample(RxPkt(r_rx_list, el));
	};
	void RxPktRemote(struct res_mh_pair *rmp, int &i, ExecutionList *el) {
		if(r_rx_list == NULL)
			return;
		result r = RxPkt(r_rx_list, el);
		if(!isnan(r)) {
			rmp[i].res = r;
			rmp[i].mh = mh_remote;
			i++;
		}
	};

	void RxDataLocal(ExecutionList *el) {
		if(r_rx_list == NULL)
			return;
		newSample(RxData(r_rx_list,el));
	};
	void RxDataRemote(struct res_mh_pair *rmp, int &i, ExecutionList *el) {
		if(r_rx_list == NULL)
			return;
		result r = RxData(r_rx_list, el);
		if(!isnan(r)) {
			rmp[i].res = r;
			rmp[i].mh = mh_remote;
			i++;
		}
	};

	void TxPktLocal(ExecutionList *el) {
		if(r_tx_list == NULL)
			return;
		newSample(TxPkt(r_tx_list, el));
	};
	void TxPktRemote(struct res_mh_pair *rmp, int &i, ExecutionList *el) {
		if(r_tx_list == NULL)
			return;
		result r = TxPkt(r_tx_list, el);
		if(!isnan(r)) {
			rmp[i].res = r;
			rmp[i].mh = mh_remote;
			i++;
		}
	};

	void TxDataLocal(ExecutionList *el)  {
		if(r_tx_list == NULL)
			return;
		newSample(TxData(r_tx_list, el));
	};
	void TxDataRemote(struct res_mh_pair *rmp, int &i, ExecutionList *el) {
		if(r_tx_list == NULL)
			return;
		result r = TxData(r_tx_list, el);
		if(!isnan(r)) {
			rmp[i].res = r;
			rmp[i].mh = mh_remote;
			i++;
		}
	};

	int newSample(result r) {
		if(!isnan(r) && rb != NULL) {
			rb->newSample(r);
			return EOK;
		}
		else
			return -EINVAL;
	};

	//If OOB this function is called upon scheduled event
	virtual void Run() {return;};
	virtual void receiveOobData(char *buf, int buf_len, result *r) {return;};
	int scheduleNextIn(struct timeval *tv);
	

	int sendOobData(char* buf, int buf_len);

	/* Functions processing a message (packet and data) and extracting the measure to be stored */
	virtual result RxPkt(result *r, ExecutionList *) {return NAN;};
	virtual result RxData(result *r, ExecutionList *) {return NAN;};
	virtual result TxPkt(result *r, ExecutionList *) {return NAN;};
	virtual result TxData(result *r, ExecutionList *) {return NAN;};
	
	//void OutOfBandcb(HANDLE *event_handle, void *arg) {return;};
};

#endif  /* _MON_MEASURE_HH_ */
