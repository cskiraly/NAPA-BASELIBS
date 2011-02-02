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

#define LOG_MODULE "[monl] "
#include	"grapes_log.h"
#include "ml.h"
#include "mon.h"
#include "measure_manager.h"
#include <iostream>
#include <stdlib.h>
#include <string.h>
#include <errors.h>
#include "mon_event.h"



/* Manager class Initialised by monInit()*/
class MeasureManager * man;
/* Array of available measurements Initialised by monInit()*/
struct MeasurementPlugin *measure_array;
int measure_array_len;

/* callback wrappers */
/*
  * callback for a received packet notification from the messaging layer
  * @param *arg An argument for a recv packet information struct
  */

extern "C" {
void monPktRxcb(void *arg);
void monPktTxcb(void *arg);
int monPktHdrSetcb(void *arg,send_pkt_type_and_dst_inf *send_pkt_inf);
void monDataRxcb(void *arg);
void monDataTxcb(void *arg);
int monDataHdrSetcb(void *arg,send_pkt_type_and_dst_inf *send_pkt_inf);
void dataRxcb(char *buffer,int buflen, unsigned char msgtype, recv_params *arg);
}

void monPktRxcb(void *arg){
	mon_pkt_inf *pkt_info;
	pkt_info = (mon_pkt_inf *)arg;
	debug("received a RX packet notification");
	man->cbRxPkt(arg);
}

void monPktTxcb(void *arg){
	mon_pkt_inf *pkt_info;
	pkt_info = (mon_pkt_inf *)arg;
	debug("received a TX packet notification");
	man->cbTxPkt(arg);
}

int monPktHdrSetcb(socketID_handle remote_socketID, uint8_t msgtype){
	debug("set monitoring module packet header");
	return man->cbHdrPkt(remote_socketID, msgtype);
}

void monDataRxcb(void *arg){
	mon_data_inf *data_info;
	data_info = (mon_data_inf *)arg;
	debug("received a RX data notification");
	man->cbRxData(arg);
}

void monDataTxcb(void *arg){
	mon_data_inf *data_info;
	data_info = (mon_data_inf *)arg;
	debug("received a TX data notification");
	man->cbTxData(arg);
}

int monDataHdrSetcb(socketID_handle remote_socketID, uint8_t msgtype){
	debug("set monitoring module data header");
	return man->cbHdrData(remote_socketID, msgtype);
}

/*
  * The peer receives data per callback from the messaging layer. 
  * @param *buffer A pointer to the buffer
  * @param buflen The length of the buffer
  * @param msgtype The message type 
  * @param *arg An argument that receives metadata about the received data
  */
void dataRxcb(char *buffer,int buflen, MsgType msgtype, recv_params *rparams){
	debug("Received data from callback: msgtype %d buflen %d",msgtype,buflen);
	man->receiveCtrlMsg(rparams->remote_socketID, msgtype, buffer, buflen);
}


//int monInit(cfg_opt_t *cfg_mon) {
int monInit(void *eb, void *repo_client) {
	MonMeasureMapId c_measure_map_id;
	
        debug("X.moninit\n");
	init_mon_event(eb);
	
	int i,k,len;
   /* Create instance of the monitoring layer */
	man = new MeasureManager(repo_client);
	

	/* Fill array of possible measurements */
	c_measure_map_id = man->monListMeasure();
	len = c_measure_map_id.size();

	measure_array_len=0;
	measure_array = (struct MeasurementPlugin*) malloc(len * sizeof(struct MeasurementPlugin));
	if(measure_array == NULL)
		return -ENOMEM;
	memset(measure_array, 0, len * sizeof(struct MeasurementPlugin));
	
        debug("X.moninit\n");
	
        MonMeasureMapId::iterator it;
	for(it = c_measure_map_id.begin(), i = 0; it != c_measure_map_id.end(); it++,i++) {
		measure_array[i].name = const_cast<char *>((it->second->getName()).c_str());
		measure_array[i].id = it->second->getId();
		measure_array[i].desc = const_cast<char *>((it->second->getDesc()).c_str());

		len = (it->second->getDeps()).size();
		if (len > 0) {
			measure_array[i].deps_id = (MeasurementId*) malloc(len * sizeof(MeasurementId));
			if(measure_array[i].deps_id == NULL)
				goto out_of_mem;

			for(k = 0; k < len; k++) {
				measure_array[i].deps_id[k] =
					c_measure_map_id[(it->second->getDeps()).at(k)]->getId();
			}
			measure_array[i].deps_length = k;
		}

		len = (it->second->getParams()).size();
		if (len > 0) {
			measure_array[i].params = (MeasurementParameter *) malloc(len * sizeof(MeasurementParameter));
			if(measure_array[i].params == NULL)
				goto out_of_mem;

	
			for(k = 0; k < len; k++) {
				measure_array[i].params[k].ph = ((it->second->getParams()).at(k))->ph;
				measure_array[i].params[k].name = const_cast<char *>
					(((it->second->getParams()).at(k))->name.c_str());
				measure_array[i].params[k].desc = const_cast<char *>
					(((it->second->getParams()).at(k))->desc.c_str());
				measure_array[i].params[k].dval = ((it->second->getParams()).at(k))->dval;
			}
			measure_array[i].params_length = k;
		}

		measure_array[i].caps = it->second->getCaps();
	}

        debug("X.moninit\n");
	measure_array_len = i;

	/* register callback functions */
	/* measurement */
	mlRegisterGetRecvPktInf(&monPktRxcb);
	mlRegisterGetSendPktInf(&monPktTxcb);
	mlRegisterSetMonitoringHeaderPktCb(&monPktHdrSetcb);
	mlRegisterGetRecvDataInf(&monDataRxcb);
	mlRegisterSetMonitoringHeaderDataCb(&monDataHdrSetcb);
	mlRegisterGetSendDataInf(&monDataTxcb);

	/* control data */
	mlRegisterRecvDataCb(&dataRxcb,MSG_TYPE_MONL);


	return EOK;

out_of_mem:
	for(k=0; k=i; k++) {
		if(measure_array[k].deps_id != NULL)
			free(measure_array[k].deps_id);
		if(measure_array[k].params != NULL)
			free(measure_array[k].params);
	}
	return -ENOMEM;
}

const struct MeasurementPlugin *monListMeasure (int *length){
	*length = measure_array_len;
	return measure_array;
}

MonHandler monCreateMeasureName (const MeasurementName name, MeasurementCapabilities mc){
   return man->monCreateMeasureName(name, mc);
}

MonHandler monCreateMeasure (MeasurementId id, MeasurementCapabilities mc){
   return man->monCreateMeasureId(id, mc);
}

int monDestroyMeasure (MonHandler mh) {
	return man->monDestroyMeasure(mh);
}

struct MeasurementParameter *monListParametersById (MeasurementId mid, int *length) {
	int i;
	for(i = 0; i < measure_array_len; i++)
		if(measure_array[i].id == mid) {
			*length = measure_array[i].params_length;
			return measure_array[i].params;
		}
	*length = 0;
	return NULL;
}

struct MeasurementParameter *monListParametersByHandler (MonHandler mh, int *length) {
	return monListParametersById(man->monMeasureHandlerToId(mh), length);
}

int monSetParameter (MonHandler mh, MonParameterType ph, MonParameterValue p) {
	return man->monSetParameter(mh,ph,p);
}

MonParameterValue monGetParameter (MonHandler mh, MonParameterType ph) {
	return man->monGetParameter(mh,ph);
}

MeasurementStatus monGetStatus(MonHandler mh) {
	return man->monGetStatus(mh);
}

int monActivateMeasure (MonHandler mh, SocketId sid, MsgType mt) {
	return man->monActivateMeasure(mh ,sid ,mt);
}

int monDeactivateMeasure (MonHandler mh) {
	return man->monDeactivateMeasure(mh);
}

int monPublishStatisticalType(MonHandler mh, const char *publishing_name, const char *channel, stat_types *st, int length, void * repo_client){
	return man->monPublishStatisticalType(mh, publishing_name, channel, st, length, repo_client);
}

result monRetrieveResult(MonHandler mh, enum stat_types st){
	return man->monRetrieveResult(mh,st);
}

int monNewSample(MonHandler mh, result r){
	return man->monNewSample(mh,r);
}

int monSetPeerName(const char *name){
	return man->monSetPeerName(name);
}
