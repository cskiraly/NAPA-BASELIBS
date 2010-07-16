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

#ifndef _MEASURE_DISPATCHER_H_
#define _MEASURE_DISPATCHER_H_

#include "mon_measure.h"
#include "measure_plugin.h"

#include <tr1/unordered_map>

//#include <iostream>

#include <vector>
#include <string.h>

#include "ml.h"

typedef std::vector<char> Buffer;

typedef struct {
	char *msg;
	int32_t length;
	char sid[SOCKETID_SIZE];
	MsgType mt;
	send_params sparam;
} Message;

void open_con_cb (int con_id, void *arg);

struct SocketIdMt {
	SocketId sid;
	MsgType mt;
};

struct socketIdMtCompare
{
	bool operator()(struct SocketIdMt x, struct SocketIdMt y) const {
		if(mlCompareSocketIDs(x.sid,y.sid) == 0 && x.mt == y.mt)
			return true;
		return false;
	}
};

struct socketIdMtHash
{
	int operator()(struct SocketIdMt x) const {
		return mlHashSocketID(x.sid) + x.mt;
	}
};

typedef struct {
	ExecutionList el_rx_pkt_local;
	ExecutionList el_rx_pkt_remote;
	ExecutionList el_tx_pkt_local;
	ExecutionList el_tx_pkt_remote;
	ExecutionList el_rx_data_local;
	ExecutionList el_rx_data_remote;
	ExecutionList el_tx_data_local;
	ExecutionList el_tx_data_remote;

	result pkt_r_rx_local[R_LAST_PKT];
	result pkt_r_rx_remote[R_LAST_PKT];
	result pkt_r_tx_local[R_LAST_PKT];
	result pkt_r_tx_remote[R_LAST_PKT];
	uint32_t pkt_tx_seq_num;

	result data_r_rx_local[R_LAST_DATA];
	result data_r_rx_remote[R_LAST_DATA];
	result data_r_tx_local[R_LAST_DATA];
	result data_r_tx_remote[R_LAST_DATA];
	uint32_t data_tx_seq_num;

	uint8_t sid[SOCKETID_SIZE];
	MsgType mt;
	SocketIdMt h_dst;

	ExecutionList mids_local;
	ExecutionList mids_remote;
} DestinationSocketIdMtData;


typedef std::tr1::unordered_map<struct SocketIdMt, DestinationSocketIdMtData*, socketIdMtHash, socketIdMtCompare> DispatcherListSocketIdMt;

class MeasureDispatcher {

	/* Execution lists for the message layer hooks */
	DispatcherListSocketIdMt dispatcherList;
	
	void addMeasureToExecLists(SocketIdMt h_dst, class MonMeasure *m);
	void delMeasureFromExecLists(MonMeasure *m);
	int stopMeasure(class MonMeasure *m);

	void createDestinationSocketIdMtData(struct SocketIdMt h_dst);
	void destroyDestinationSocketIdMtData(SocketId dst, MsgType mt);

	class MonMeasure* findMeasureFromId(DestinationSocketIdMtData *dd, MeasurementCapabilities flags, MeasurementId mid);

	int sendCtrlMsg(SocketId dst, Buffer &buffer);
	int receiveCtrlMsg(SocketId sid, MsgType mt, char *cbuf, int length);

	int remoteMeasureResponseRx(SocketId src, MsgType mt, char* cbuf);
	int remoteMeasureResponseTx(SocketId dst, MonHandler mhr, MonHandler mh, int32_t cid, int32_t status);

	int remoteResultsTx(SocketId dst, struct res_mh_pair *rmp, int length, MsgType mt);
	int remoteResultsRx(SocketId src, MsgType mt, char *cbuf);

	int initRemoteMeasureTx(class MonMeasure *m, SocketId dst, MsgType mt);
	int initRemoteMeasureRx(SocketId src, MsgType mt, char *cbuf);

	int deinitRemoteMeasureTx(class MonMeasure *m);
	int deinitRemoteMeasureRx(SocketId src, MsgType mt, char* cbuff);

	int oobDataRx(SocketId sid, MsgType mt, char *cbuf, int length);

	void headerSetup(eControlId type, Buffer &msg);
protected:
	class MeasureManager *mm;

	uint8_t initial_ttl;

	friend class MeasureManager;
	friend class MonMeasure;
public:
	MeasureDispatcher(): initial_ttl(0) {
		int error1, error2;
		uint8_t ttl;
		error2 = mlGetStandardTTL(mlGetLocalSocketID(&error1), &initial_ttl);
		if(error1 == 1 || error2 != 1)
			initial_ttl = 0;
	};

	int activateMeasure(class MonMeasure *m, SocketId dst, MsgType mt, int autoload = 1);
	int deactivateMeasure(class MonMeasure *m);

	int oobDataTx(class MonMeasure *m, char *buf, int buf_len);

	int scheduleMeasure(MonHandler mh);
	int schedulePublish(MonHandler mh);

	/* CAllback Functions */
	void cbRxPkt(void *arg);
	void cbRxData(void *arg);
	void cbTxPkt(void *arg);
	void cbTxData(void *arg);
	int cbHdrPkt(SocketId sid, MsgType mt);
	int cbHdrData(SocketId sid, MsgType mt);
};

#endif /* _MEASURE_DISPATCHER_H_ */
