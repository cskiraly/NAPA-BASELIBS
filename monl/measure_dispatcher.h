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

/* DestinationId Comparison function */
//TODO change it to a correct implemnetation
// struct socketIdCompare
// {
// 	bool operator()(const SocketId x, const SocketId y) const { 
// 		char *xptr, *yptr;
// 		xptr = (char *) x;
// 		yptr = (char *) y;
// 		if(memcmp(xptr, yptr, SOCKETID_SIZE) == 0)
// 			return true;
// 		return false;
// 	}
// };
struct socketIdCompare
{
	bool operator()(const SocketId x, const SocketId y) const { 
		if(mlCompareSocketIDs(x,y) == 0)
			return true;
		return false;
	}
};
/* DestinationId Hash function */
// struct socketIdHash
// {
// 	int operator()(const SocketId x) const {
// 		uint32_t hash_code = 0;
// 		uint32_t *intptr = (uint32_t*) x;
// 		for(int i=0; i < SOCKETID_SIZE / sizeof(uint32_t); i++)
// 			hash_code = hash_code + intptr[i];
// 		return hash_code;
// 	}
// };
struct socketIdHash
{
	int operator()(const SocketId x) const {
		return mlHashSocketID(x);
	}
};

typedef struct {
	ExecutionList el_local;
	ExecutionList el_remote;
	// elements in the execution list
	int32_t n_el_local;
	int32_t n_el_remote;
} DestinationMsgTypeData;

struct pkt_result{
	result r_rx_local[R_LAST_PKT];
	result r_rx_remote[R_LAST_PKT];
	result r_tx_local[R_LAST_PKT];
	result r_tx_remote[R_LAST_PKT];
	uint32_t tx_seq_num;
	//TODO store here destination specific data
};

struct data_result {
	result r_rx_local[R_LAST_DATA];
	result r_rx_remote[R_LAST_DATA];
	result r_tx_local[R_LAST_DATA];
	result r_tx_remote[R_LAST_DATA];
	uint32_t tx_seq_num;
	//TODO store here destination specific data
};


typedef std::tr1::unordered_map<MsgType,struct pkt_result *> ResultPktListMsgType;
typedef std::tr1::unordered_map<MsgType,struct data_result *> ResultDataListMsgType;
typedef std::tr1::unordered_map<MsgType,DestinationMsgTypeData*> DispatcherListMsgType;

typedef struct {
	DispatcherListMsgType rx_pkt;
	DispatcherListMsgType tx_pkt;
	DispatcherListMsgType rx_data;
	DispatcherListMsgType tx_data;

	ResultPktListMsgType r_pkt;
	ResultDataListMsgType r_data;

	char *sid;

} DestinationSocketIdData;

typedef std::tr1::unordered_map<SocketId,DestinationSocketIdData*,socketIdHash,socketIdCompare> DispatcherListSocketId;


class MeasureDispatcher {

	/* Execution lists for the message layer hooks */
	DispatcherListSocketId dispatcherList;
	
	int addMeasureToExecList(DispatcherListMsgType &dl, class MonMeasure *m, MsgType mt);
	int delMeasureFromExecList(DispatcherListMsgType &dl, class MonMeasure *m, MsgType mt);

	class MonMeasure* findDep(DispatcherListMsgType &dl, MeasurementCapabilities flags, MsgType mt, MeasurementId mid);
	class MonMeasure* findMeasureFromId(DestinationSocketIdData *dsid, MeasurementCapabilities flags, MeasurementId mid, MsgType mt);
	int checkDeps(DestinationSocketIdData *dsid, MeasurementCapabilities flags, MeasurementId mid, MsgType mt);

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

	int activateMeasure(class MonMeasure *m, SocketId dst, MsgType mt,int autoload = 1);
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
