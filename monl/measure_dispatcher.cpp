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

#include "measure_dispatcher.h"
#include "measure_manager.h"
#include "errors.h"
#include "ctrl_msg.h"
#include "grapes_log.h"


#include <arpa/inet.h>
#include <string.h>
#include <sstream>
#include <stdint.h>
#include <sys/time.h>

//TODO: possible improvements:
// - make buffer a ptr and don't make an explicit copy
// - implement a better hash and compare for SocketId
// - use activate/deactivate for automatic loading/unloading (done has to be checked)


void open_con_cb (int con_id, void *arg) {
	Message *msg = (Message*) arg;

	mlSendData(con_id,msg->msg,msg->length,msg->mt,NULL);

	delete[] msg->msg;
	delete msg;
}

int MeasureDispatcher::addMeasureToExecList(DispatcherListMsgType &dl, class MonMeasure *m, MsgType mt) {
	class MeasurePlugin *mp = m->measure_plugin;
	ExecutionList *el;
	int32_t *n_el;

	/* Create an execution list if necessary */
	if(dl.find(mt) == dl.end()) {
		dl[mt] = new DestinationMsgTypeData;
		dl[mt]->n_el_local = 0;
		dl[mt]->n_el_remote = 0;
	}

	if(m->flags & REMOTE) {
		n_el = &(dl[mt]->n_el_remote);
		el = &(dl[mt]->el_remote);
	} else {
		n_el = &(dl[mt]->n_el_local);
		el = &(dl[mt]->el_local);
	}
	/* Check if this measure is already in the list */
	if(el->find(mp->getId()) != el->end())
		return -EEXISTS;

	/* Check for all dependencies */
	std::vector<MeasurementId>::iterator it;
	std::vector<MeasurementId> deps = mp->getDeps();
	for(it = deps.begin(); it != deps.end(); it++) {
		if(el->find(*it) == el->end())
			return -EUNRESOLVEDDEP;
	}

	/* update counters */
	for(it = deps.begin(); it != deps.end(); it++) {
		(*el)[*it]->used_counter++;
	}

	/* Add measure to execution list */
	(*el)[mp->getId()] = m;
	(*n_el)++;

	return EOK;
}

int MeasureDispatcher::delMeasureFromExecList(DispatcherListMsgType &dl, class MonMeasure *m, MsgType mt) {
	class MeasurePlugin *mp = m->measure_plugin;
	ExecutionList *el;
	int32_t *n_el;

	/* Create an execution list if necessary */
	if(dl.find(mt) == dl.end())
		return -EINVAL;

	if(m->flags & REMOTE) {
		n_el = &(dl[mt]->n_el_remote);
		el = &(dl[mt]->el_remote);
	} else {
		n_el = &(dl[mt]->n_el_local);
		el = &(dl[mt]->el_local);
	}

	/* update counters */
	/* Check for all dependencies */
	std::vector<MeasurementId>::iterator it;
	std::vector<MeasurementId> deps = mp->getDeps();
	for(it = deps.begin(); it != deps.end(); it++) {
		(*el)[*it]->used_counter--;
	}

	el->erase(el->find(mp->getId()));
	*(n_el)--;

	if(dl[mt]->n_el_local == 0 && dl[mt]->n_el_remote == 0) {
		delete dl[mt];
		dl.erase(dl.find(mt));
	}

	return EOK;
}

int MeasureDispatcher::sendCtrlMsg(SocketId dst, Buffer &buffer)
{
	int con_id,res;
	Message *msg;
	send_params sparam;
	struct MonHeader *mheader = (MonHeader*)&buffer[0];
	mheader->length = buffer.size();

	if(dst == NULL)
		return EOK;

	// if connection exists, send data
	con_id = mlConnectionExist(dst, false);

	if(con_id >= 0) {
		if(mlGetConnectionStatus(con_id) == 1) {
			mlSendData(con_id,&buffer[0],buffer.size(),MSG_TYPE_MONL,NULL);
			return EOK;
		}
	}

	memset(&sparam, 0, sizeof(send_params));

	//otherwise open connection and delay sending data
	msg = new Message;
	msg->length = buffer.size();
	msg->msg = new char[msg->length];
	memcpy(msg->msg, &buffer[0], buffer.size());
	memcpy(msg->sid, dst, SOCKETID_SIZE);
	msg->mt = MSG_TYPE_MONL;
	msg->sparam = sparam;
	con_id = mlOpenConnection(dst,&open_con_cb,msg, sparam);

	if(con_id < 0) {
		delete msg->msg;
		delete msg;
		return -ENOMEM;
	}

	return EOK;
}


int MeasureDispatcher::receiveCtrlMsg(SocketId sid, MsgType mt, char *cbuf, int length)
{
	struct MonHeader *mheader = (MonHeader*) cbuf;

	if(length < mheader->length)
		return EINVAL;

	cbuf += MONHDR_SIZE;
	length -= MONHDR_SIZE;

	switch (mheader->type) {
	case INITREMOTEMEASURE:
			return initRemoteMeasureRx(sid, mt, cbuf);
	case REMOTEMEASURERESPONSE:
			return remoteMeasureResponseRx(sid, mt, cbuf);
	case DEINITREMOTEMEASURE:
			return deinitRemoteMeasureRx(sid, mt, cbuf);
	case REMOTERESULTS:
			return remoteResultsRx(sid, mt, cbuf);
	case OOBDATA:
			return oobDataRx(sid, mt, cbuf, length);
	}
	return EINVAL;
}

void MeasureDispatcher::headerSetup(eControlId type, Buffer &msg) {
	struct MonHeader mheader;
	mheader.length = 0;
	mheader.type = type;
	msg.insert(msg.end(), (char*) &mheader, ((char*) &mheader) + MONHDR_SIZE );
}

int MeasureDispatcher::oobDataTx(class MonMeasure *m, char *buf, int buf_len) {
	Buffer buffer;
	struct OobData oobdata;

	buffer.reserve(MONHDR_SIZE + OOBDATA_SIZE + buf_len);

	headerSetup(OOBDATA, buffer);

	oobdata.mh_local = m->mh_local;
	oobdata.mh_remote = m->mh_remote;

	buffer.insert(buffer.end(), (char*)&oobdata, ((char*)&oobdata) + OOBDATA_SIZE);

	buffer.insert(buffer.end(), buf, buf + buf_len);

	return sendCtrlMsg(m->dst_socketid, buffer);
}

int MeasureDispatcher::oobDataRx(SocketId sid, MsgType mt, char *cbuf, int length) {
	struct OobData *oobdata = (struct OobData*) cbuf;
	result *r = NULL;

	if(mm->isValidMonHandler(oobdata->mh_remote))
		if(mm->mMeasureInstances[oobdata->mh_remote]->status == RUNNING &&
 				mlCompareSocketIDs(mm->mMeasureInstances[oobdata->mh_remote]->dst_socketid, sid) == 0) {
			if(mm->mMeasureInstances[oobdata->mh_remote]->flags & DATA) {
				if(mm->mMeasureInstances[oobdata->mh_remote]->flags & REMOTE)
					r = dispatcherList[sid]->r_data[MSG_TYPE_MONL]->r_rx_remote;
				else
					r = dispatcherList[sid]->r_data[MSG_TYPE_MONL]->r_rx_local;
			} else if (mm->mMeasureInstances[oobdata->mh_remote]->flags & PACKET){
				if(mm->mMeasureInstances[oobdata->mh_remote]->flags & REMOTE)
					r = dispatcherList[sid]->r_pkt[MSG_TYPE_MONL]->r_rx_remote;
				else
					r = dispatcherList[sid]->r_pkt[MSG_TYPE_MONL]->r_rx_local;
			}
			if(!r)
				fatal("MONL: r is not set");
			mm->mMeasureInstances[oobdata->mh_remote]->receiveOobData(cbuf + OOBDATA_SIZE,
					length - OOBDATA_SIZE, r);
		}
	return EOK;
}

int MeasureDispatcher::remoteResultsTx(SocketId dst, struct res_mh_pair *rmp, int length, MsgType mt) {
	Buffer buffer;
	struct RemoteResults rresults;

	buffer.reserve(MONHDR_SIZE + REMOTERESULTS_SIZE + length * sizeof(struct res_mh_pair));
	
	headerSetup(REMOTERESULTS, buffer);

	rresults.length = length;
	rresults.msg_type = mt;
	
	buffer.insert(buffer.end(), (char*)&rresults, ((char*)&rresults) + REMOTERESULTS_SIZE);

	buffer.insert(buffer.end(), (char*) rmp, ((char*) rmp) + length * sizeof(struct res_mh_pair));

	return sendCtrlMsg(dst, buffer);
}

int MeasureDispatcher::remoteResultsRx(SocketId sid, MsgType mt, char *cbuf) {
	struct RemoteResults *rresults = (struct RemoteResults *) cbuf;
	struct res_mh_pair *rmp;

	rmp = (struct res_mh_pair *) (cbuf + REMOTERESULTS_SIZE);

	for(int i = 0; i < rresults->length; i++) {
		if(mm->isValidMonHandler(rmp[i].mh)) {
			if(mm->mMeasureInstances[rmp[i].mh]->status == RUNNING &&
			 mm->mMeasureInstances[rmp[i].mh]->msg_type == rresults->msg_type &&
 			 mlCompareSocketIDs(mm->mMeasureInstances[rmp[i].mh]->dst_socketid, sid) == 0) {
				if(mm->mMeasureInstances[rmp[i].mh]->rb != NULL) {
						mm->mMeasureInstances[rmp[i].mh]->rb->newSample(rmp[i].res);
				}
			}
		}
	}
	return EOK;
};

int MeasureDispatcher::remoteMeasureResponseTx(SocketId dst, MonHandler mhr, MonHandler mh, int32_t cid, int32_t status) {
	Buffer buffer;
	struct MeasureResponse mresponse;

	buffer.reserve(MONHDR_SIZE + MRESPONSE_SIZE);

	headerSetup(REMOTEMEASURERESPONSE, buffer);

	mresponse.mh_remote	= mhr;
	mresponse.mh_local	= mh;
	mresponse.command		= cid;
	mresponse.status		= status;

	buffer.insert(buffer.end(), (char*)&mresponse, ((char*)&mresponse) + MRESPONSE_SIZE);

	return sendCtrlMsg(dst, buffer);
}

int MeasureDispatcher::remoteMeasureResponseRx(SocketId src, MsgType mt, char* cbuf) {
	struct MeasureResponse *mresponse = (MeasureResponse*) cbuf;

	if(! mm->isValidMonHandler(mresponse->mh_remote)) {	//might not exist if the measure had been deleted in the meantime
		return -EINVAL;
	}

	MonMeasure *m = mm->mMeasureInstances[mresponse->mh_remote];
	switch(mresponse->command) {
		case INITREMOTEMEASURE:
			m->mh_remote = mresponse->mh_local;
			if(mresponse->status == EOK) {
				if(m->flags & OUT_OF_BAND) {
					struct timeval tv = {0,0};
					if(m->scheduleNextIn(&tv) != EOK)
						m->status = FAILED;
					else
						m->status = RUNNING;
				} else
					m->status = RUNNING;
			} else
				m->status = RFAILED;
			break;
		case DEINITREMOTEMEASURE:
			if(mresponse->status == EOK) {
				if(m->status == STOPPING && m->auto_loaded == true) {
					if(mm->monDestroyMeasure(m->mh_local) != EOK)
						m->status = FAILED;
					if(dispatcherList.find(src) != dispatcherList.end()) {
						if(dispatcherList[src]->rx_pkt.empty() && dispatcherList[src]->tx_pkt.empty() &&
								dispatcherList[src]->rx_data.empty() && dispatcherList[src]->tx_data.empty())
							delete dispatcherList[src];
					}
				} else
					m->status = STOPPED;
			}
			else
				m->status = RFAILED;
			break;
		}
	return EOK;
}

int MeasureDispatcher::initRemoteMeasureRx(SocketId src, MsgType mt, char *cbuf) {
	MonHandler mh;
	MonParameterValue *param_vector;
	MonMeasure *m = NULL;

	struct InitRemoteMeasure *initmeasure = (InitRemoteMeasure*) cbuf;

	/* Check if a previous instance is running */
	if(dispatcherList.find(src) != dispatcherList.end())
		m = findMeasureFromId(dispatcherList[src], initmeasure->mc, initmeasure->mid, initmeasure->msg_type);

	if(m == NULL)
		mh = mm->monCreateMeasureId(initmeasure->mid, initmeasure->mc);
	else
		mh = m->mh_local;

	if(mh < 0)
		return remoteMeasureResponseTx(src, initmeasure->mh_local, -1, INITREMOTEMEASURE, EINVAL);

	mm->mMeasureInstances[mh]->mh_remote = initmeasure->mh_local;

	param_vector = (MonParameterValue*)(cbuf + INITREMOTEMEASURE_SIZE);

	for(int i=0; i < initmeasure->n_params; i++) {
		if(mm->monSetParameter(mh,i,param_vector[i]) != EOK)	//TODO: init might be called inside this, which might use paramerets (e.g. src) that are set only below, in activateMeasure
			goto error;
	}

	if(m == NULL) {
		if(activateMeasure(mm->mMeasureInstances[mh], src, initmeasure->msg_type) != EOK)
			goto error;
	} else
		m->defaultInit();

	return remoteMeasureResponseTx(src, initmeasure->mh_local, mh, INITREMOTEMEASURE, EOK);

error:
	return remoteMeasureResponseTx(src, initmeasure->mh_local, mh, INITREMOTEMEASURE, EINVAL);
}

int MeasureDispatcher::initRemoteMeasureTx(class MonMeasure *m, SocketId dst, MsgType mt) {
	Buffer buffer;

	struct InitRemoteMeasure initmeasure;

	buffer.reserve(MONHDR_SIZE + INITREMOTEMEASURE_SIZE + sizeof(MonParameterValue) * m->measure_plugin->params.size());

	headerSetup(INITREMOTEMEASURE, buffer);

	initmeasure.mid = m->measure_plugin->getId();
	/* setup flags for REMOTE party */
	initmeasure.mc = (m->getFlags() | REMOTE) & ~(TXLOC | RXLOC | TXREM | RXREM);
	if(m->getFlags() & TXREM)
		initmeasure.mc |= TXLOC;
	if(m->getFlags() & RXREM)
		initmeasure.mc |= RXLOC;

	initmeasure.mh_local = m->mh_local;
	initmeasure.msg_type = mt;
	initmeasure.n_params = m->measure_plugin->params.size();

	buffer.insert(buffer.end(),(char*)&initmeasure,((char*)&initmeasure) + INITREMOTEMEASURE_SIZE);

	for(int i=0; i < initmeasure.n_params; i++)
		buffer.insert(buffer.end(), (char*)&(m->param_values[i]), ((char*)&(m->param_values[i])) + sizeof(MonParameterValue));

	return sendCtrlMsg(dst, buffer);
}

int MeasureDispatcher::deinitRemoteMeasureTx(class MonMeasure *m) {
	Buffer buffer;
	struct DeInitRemoteMeasure deinitmeasure;

	buffer.reserve(MONHDR_SIZE + DEINITREMOTEMEASURE_SIZE);

	headerSetup(DEINITREMOTEMEASURE, buffer);

	deinitmeasure.mh_local = m->mh_local;
	deinitmeasure.mh_remote = m->mh_remote;

	buffer.insert(buffer.end(),(char*)&deinitmeasure, ((char*)&deinitmeasure) + DEINITREMOTEMEASURE_SIZE);

	return sendCtrlMsg(m->dst_socketid, buffer);
};

int MeasureDispatcher::deinitRemoteMeasureRx(SocketId src, MsgType mt, char* cbuf) {
	int ret;

	struct DeInitRemoteMeasure *deinitmeasure = (DeInitRemoteMeasure*) cbuf;

	if(! mm->isValidMonHandler(deinitmeasure->mh_remote)) {	//might not exist if the measure had been deleted in the meantime
		return -EINVAL;
	}

	ret = deactivateMeasure(mm->mMeasureInstances[deinitmeasure->mh_remote]);
	if (ret != EOK )
		return remoteMeasureResponseTx(src, deinitmeasure->mh_local, deinitmeasure->mh_remote, DEINITREMOTEMEASURE, ret);

	ret = mm->monDestroyMeasure(deinitmeasure->mh_remote);	//TODO: this mh should not be reused! Packets might still arrive referencig this
	if (ret != EOK )
		return remoteMeasureResponseTx(src, deinitmeasure->mh_local, deinitmeasure->mh_remote, DEINITREMOTEMEASURE, ret);

	return remoteMeasureResponseTx(src,  deinitmeasure->mh_local, deinitmeasure->mh_remote, DEINITREMOTEMEASURE, EOK);
}

class MonMeasure* MeasureDispatcher::findDep(DispatcherListMsgType &dl, MeasurementCapabilities flags, MsgType mt, MeasurementId mid) {
	ExecutionList *el;

	if(dl.find(mt) == dl.end()) {
		return NULL;
	}

	if(flags & REMOTE) {
		el = &(dl[mt]->el_remote);
	} else {
		el = &(dl[mt]->el_local);
	}

	/* Check if this measure is already in the list */
	if(el->find(mid) != el->end())
		return (*el)[mid];

	return NULL;
}

class MonMeasure* MeasureDispatcher::findMeasureFromId(DestinationSocketIdData *dsid,  MeasurementCapabilities flags, MeasurementId mid, MsgType mt) {
	if(flags & IN_BAND) {
		if(flags & TXLOC) {
			if(flags & PACKET)
				return findDep(dsid->tx_pkt, flags, mt, mid);
			if(flags & DATA)
				return findDep(dsid->tx_data, flags, mt, mid);
		}

		if(flags & RXLOC) {
			if(flags & PACKET)
				return findDep(dsid->rx_pkt, flags, mt, mid);
			if(flags & DATA)
				return findDep(dsid->rx_data, flags, mt, mid);
		}
	}

	return NULL;
}

int MeasureDispatcher::checkDeps(DestinationSocketIdData *dsid, MeasurementCapabilities flags, MeasurementId mid, MsgType mt) {
	int ret = 1;

	if(flags & IN_BAND) {
		if(flags & TXLOC) {
			if(flags & PACKET)
				ret &= (findDep(dsid->tx_pkt, flags, mt, mid) != NULL);
			if(flags & DATA)
				ret &= (findDep(dsid->tx_data, flags, mt, mid) != NULL);
		}

		if(flags & RXLOC) {
			if(flags & PACKET)
				ret &= (findDep(dsid->rx_pkt, flags, mt, mid) != NULL);
			if(flags & DATA)
				ret &= (findDep(dsid->rx_data, flags, mt, mid) != NULL);
		}
	}

	return ret;
}


int MeasureDispatcher::activateMeasure(class MonMeasure *m, SocketId dst, MsgType mt, int auto_load) {
	int ret;
	m->status = INITIALISING;
	m->msg_type = mt;

	if(dst != NULL) {
		if(m->dst_socketid == NULL)
			m->dst_socketid = (SocketId) new char[SOCKETID_SIZE];

		memcpy((char*) m->dst_socketid, (char*) dst, SOCKETID_SIZE);
	} else {
		if(m->flags != 0)
			return -EINVAL;
	}

	m->defaultInit();

	if(m->flags == 0) {
		m->status = RUNNING;
		return EOK;
	}

	if(dispatcherList.find(dst) == dispatcherList.end()) {
		char *c_sid = new char[SOCKETID_SIZE];
		memcpy(c_sid, (char*) dst, SOCKETID_SIZE);
		dst = (SocketId) c_sid;
		dispatcherList[dst] = new DestinationSocketIdData;
		dispatcherList[dst]->sid = c_sid;
	}

	if(m->flags & PACKET) {
		if(dispatcherList[dst]->r_pkt.find(mt) == dispatcherList[dst]->r_pkt.end()) {
			dispatcherList[dst]->r_pkt[mt] = new struct pkt_result;
			for(int i = 0; i < R_LAST_PKT; i++) {
				dispatcherList[dst]->r_pkt[mt]->r_rx_remote[i] = NAN;
				dispatcherList[dst]->r_pkt[mt]->r_tx_remote[i] = NAN;
				dispatcherList[dst]->r_pkt[mt]->r_rx_local[i] = NAN;
				dispatcherList[dst]->r_pkt[mt]->r_tx_local[i] = NAN;
			}
			dispatcherList[dst]->r_pkt[mt]->tx_seq_num = 0;
		}
		if(m->flags & OUT_OF_BAND) {
			if(dispatcherList[dst]->r_pkt.find(MSG_TYPE_MONL) == dispatcherList[dst]->r_pkt.end()) {
				dispatcherList[dst]->r_pkt[MSG_TYPE_MONL] = new struct pkt_result;
				for(int i = 0; i < R_LAST_PKT; i++) {
					dispatcherList[dst]->r_pkt[MSG_TYPE_MONL]->r_rx_remote[i] = NAN;
					dispatcherList[dst]->r_pkt[MSG_TYPE_MONL]->r_tx_remote[i] = NAN;
					dispatcherList[dst]->r_pkt[MSG_TYPE_MONL]->r_rx_local[i] = NAN;
					dispatcherList[dst]->r_pkt[MSG_TYPE_MONL]->r_tx_local[i] = NAN;
				}
				dispatcherList[dst]->r_pkt[MSG_TYPE_MONL]->tx_seq_num = 0;
			}
		}
		if(m->flags & REMOTE) {
			m->r_rx_list = dispatcherList[dst]->r_pkt[mt]->r_rx_remote;
			m->r_tx_list = dispatcherList[dst]->r_pkt[mt]->r_tx_remote;
		} else {
			m->r_rx_list = dispatcherList[dst]->r_pkt[mt]->r_rx_local;
			m->r_tx_list = dispatcherList[dst]->r_pkt[mt]->r_tx_local;
		}
	}
	if(m->flags & DATA) {
		if(dispatcherList[dst]->r_data.find(mt) == dispatcherList[dst]->r_data.end()) {
			dispatcherList[dst]->r_data[mt] = new struct data_result;
			for(int i = 0; i < R_LAST_DATA; i++) {
				dispatcherList[dst]->r_data[mt]->r_rx_remote[i] = NAN;
				dispatcherList[dst]->r_data[mt]->r_tx_remote[i] = NAN;
				dispatcherList[dst]->r_data[mt]->r_rx_local[i] = NAN;
				dispatcherList[dst]->r_data[mt]->r_tx_local[i] = NAN;
			}
			dispatcherList[dst]->r_data[mt]->tx_seq_num = 0;
		}
		if(m->flags & OUT_OF_BAND) {
			if(dispatcherList[dst]->r_data.find(MSG_TYPE_MONL) == dispatcherList[dst]->r_data.end()) {
				dispatcherList[dst]->r_data[MSG_TYPE_MONL] = new struct data_result;
				for(int i = 0; i < R_LAST_DATA; i++) {
					dispatcherList[dst]->r_data[MSG_TYPE_MONL]->r_rx_remote[i] = NAN;
					dispatcherList[dst]->r_data[MSG_TYPE_MONL]->r_tx_remote[i] = NAN;
					dispatcherList[dst]->r_data[MSG_TYPE_MONL]->r_rx_local[i] = NAN;
					dispatcherList[dst]->r_data[MSG_TYPE_MONL]->r_tx_local[i] = NAN;
				}
				dispatcherList[dst]->r_data[MSG_TYPE_MONL]->tx_seq_num = 0;
			}
		}
		if(m->flags & REMOTE) {
			m->r_rx_list = dispatcherList[dst]->r_data[mt]->r_rx_remote;
			m->r_tx_list = dispatcherList[dst]->r_data[mt]->r_tx_remote;
		} else {
			m->r_rx_list = dispatcherList[dst]->r_data[mt]->r_rx_local;
			m->r_tx_list = dispatcherList[dst]->r_data[mt]->r_tx_local;
		}
	}

	//try to automatically load dependencies
	if(auto_load) {
		class MeasurePlugin *mp = m->measure_plugin;
		/* Check for all dependencies */
		std::vector<MeasurementId>::iterator it;
		std::vector<MeasurementId> deps = mp->getDeps();
		for(it = deps.begin(); it != deps.end(); it++) {
			if(checkDeps(dispatcherList[dst], m->flags, *it, mt) == 0) {
				MonHandler mh;
				mh = mm->monCreateMeasureId(*it, m->flags);
				if(mh > 0){
					if(activateMeasure(mm->mMeasureInstances[mh], dst, mt) == EOK)
						mm->mMeasureInstances[mh]->auto_loaded = true;
					else {
						mm->monDestroyMeasure(mh);
						ret = -EUNRESOLVEDDEP;
						goto error;
					}
				} else
					ret = -EUNRESOLVEDDEP;
					goto error;
			}
		}
	}

	/* IN_BAND  measurmente added to hook executionlists */
	if(m->flags & IN_BAND) {
		/* TXLOC */
		if(m->flags & TXLOC) {
			if(m->flags & PACKET) {
				ret = addMeasureToExecList(dispatcherList[dst]->tx_pkt,m,mt);
				if(ret != EOK)
					goto error;
			}
			if(m->flags & DATA) {
				ret = addMeasureToExecList(dispatcherList[dst]->tx_data,m,mt);
				if(ret != EOK)
					goto error;
			}
		}
		/* RXLOC */
		if(m->flags & RXLOC) {
			if(m->flags & PACKET) {
				ret = addMeasureToExecList(dispatcherList[dst]->rx_pkt,m,mt);
				if(ret != EOK)
					goto error;
			}
			if(m->flags & DATA) {
				ret = addMeasureToExecList(dispatcherList[dst]->rx_data,m,mt);
				if(ret != EOK)
					goto error;
			}
		}
		/* Remote counterparts ? */
		if(m->flags & TXREM || m->flags & RXREM) {
			/* Yes, initialise them */
			ret = initRemoteMeasureTx(m,dst,mt);
			if(ret != EOK)
				goto error;
			else
				return ret;
		}
	}

	if(m->flags & OUT_OF_BAND) {
		struct timeval tv = {0,0};
		/* TXLOC need only a local instance */
		if(m->flags & TXLOC) {
			ret = m->scheduleNextIn(&tv);
			if(ret != EOK)
				goto error;
		}

		/* Remote counterparts ? */
		if(m->flags & TXREM || m->flags & RXREM) {
			/* Yes, initialise them */
			ret = initRemoteMeasureTx(m,dst,mt);
			if(ret != EOK)
				goto error;
			else
				return ret;
		} else {
			/* No, we are done */
			ret = m->scheduleNextIn(&tv);
			if(ret != EOK)
				goto error;
		}
	}

	m->status = RUNNING;
	return EOK;

error:
	m->status = FAILED;
	m->r_tx_list = m->r_rx_list = NULL;
	return ret;
};

int MeasureDispatcher::deactivateMeasure(class MonMeasure *m) {
	int ret;
	SocketId dst = m->dst_socketid;
	
	class MeasurePlugin *mp = m->measure_plugin;
	std::vector<MeasurementId>::iterator it;
	std::vector<MeasurementId> deps = mp->getDeps();

	if(m->status == STOPPED)
		return EOK;
	
	if(m->used_counter > 0)
		return -EINUSE;

	if(m->status != FAILED)
		m->defaultStop();

	m->status = STOPPED;
	m->r_tx_list = m->r_rx_list = NULL;

	/* IN_BAND  measurments added to hook executionlists*/
	if(m->flags & IN_BAND) {
		/* TXLOC */
		if(m->flags & TXLOC) {
			if(m->flags & PACKET) {
				ret = delMeasureFromExecList(dispatcherList[dst]->tx_pkt,m,m->msg_type);
				if(ret != EOK)
					goto error;
			}
			if(m->flags & DATA) {
				ret = delMeasureFromExecList(dispatcherList[dst]->tx_data,m,m->msg_type);
				if(ret != EOK)
					goto error;
			}
		}

		/* RXLOC */
		if(m->flags & RXLOC) {
			if(m->flags & PACKET) {
				ret = delMeasureFromExecList(dispatcherList[dst]->rx_pkt,m,m->msg_type);
				if(ret != EOK)
					goto error;
			}
			if(m->flags & DATA) {
				ret = delMeasureFromExecList(dispatcherList[dst]->rx_data,m,m->msg_type);
				if(ret != EOK)
					goto error;
			}
		}

		/* Remote counterparts ? */
		if(m->flags & TXREM) {
				m->status = STOPPING;
				ret = deinitRemoteMeasureTx(m);
				if(ret != EOK)
						goto error;
		}
	}
	//TODO out of band measures check this!!!
	if(m->flags & OUT_OF_BAND) {
		m->status = STOPPING;
		if(m->flags & TXREM) {
				ret = deinitRemoteMeasureTx(m);
				if(ret != EOK)
						goto error;
		}
	}

	//remove automatically loaded ones
	for(it = deps.begin(); it != deps.end(); it++) {
		class MonMeasure *md = findMeasureFromId(dispatcherList[dst], m->flags, *it, m->msg_type);
		if(md != NULL) {
			if(md->used_counter == 0 && md->auto_loaded == true) {
				ret = deactivateMeasure(md);
				if(ret != EOK)
						goto error;
				if(md->status == STOPPED) {
					ret = mm->monDestroyMeasure(md->mh_local);
					if(ret != EOK)
							goto error;
				}
			}
		}
	}

	if(dispatcherList[dst]->rx_pkt.find(m->msg_type) == dispatcherList[dst]->rx_pkt.end() &&
		dispatcherList[dst]->tx_pkt.find(m->msg_type) == dispatcherList[dst]->tx_pkt.end()) {
			delete dispatcherList[dst]->r_pkt[m->msg_type];
			dispatcherList[dst]->r_pkt.erase(m->msg_type);
	}

	if(dispatcherList[dst]->rx_data.find(m->msg_type) == dispatcherList[dst]->rx_data.end() &&
		dispatcherList[dst]->tx_data.find(m->msg_type) == dispatcherList[dst]->tx_data.end()) {
			delete dispatcherList[dst]->r_data[m->msg_type];
			dispatcherList[dst]->r_data.erase(m->msg_type);
	}

	if(dispatcherList[dst]->rx_pkt.empty() && dispatcherList[dst]->tx_pkt.empty() &&
			dispatcherList[dst]->rx_data.empty() && dispatcherList[dst]->tx_data.empty()) {
		ResultPktListMsgType::iterator it;
		for(it = dispatcherList[dst]->r_pkt.begin(); it != dispatcherList[dst]->r_pkt.end(); it++)
			delete it->second;
		ResultDataListMsgType::iterator it2;
		for(it2 = dispatcherList[dst]->r_data.begin(); it2 != dispatcherList[dst]->r_data.end(); it2++)
			delete it2->second;
		delete[] dispatcherList[dst]->sid;
		delete dispatcherList[dst];
	}

	if(m->dst_socketid) {
		delete[] (char *) m->dst_socketid;
		m->dst_socketid = NULL;
	}
	return EOK;

error:
	m->status = FAILED;

};



void MeasureDispatcher::cbRxPkt(void *arg) {
	ExecutionList::iterator it;
	result *r_loc, *r_rem;
	struct res_mh_pair rmp[R_LAST_PKT];
	int i,j;
	mon_pkt_inf *pkt_info = (mon_pkt_inf *)arg;
	struct MonPacketHeader *mph = (MonPacketHeader*) pkt_info->monitoringHeader;
	SocketId sid = pkt_info->remote_socketID;
	MsgType mt = pkt_info->msgtype;

	/* something to do? */
	if(dispatcherList.size() == 0)
		return;

	if(dispatcherList.find(sid) == dispatcherList.end())
		return;

	if(dispatcherList[sid]->r_pkt.find(mt) == dispatcherList[sid]->r_pkt.end()) {
		return;
	}

	//we have a result vector to fill
	r_loc = dispatcherList[sid]->r_pkt[mt]->r_rx_local;
	r_rem = dispatcherList[sid]->r_pkt[mt]->r_rx_remote;

	//TODO: add standard fields
	if(mph != NULL) {
		r_rem[R_SEQNUM] = r_loc[R_SEQNUM] = ntohl(mph->seq_num);
		r_rem[R_INITIAL_TTL] = r_loc[R_INITIAL_TTL] = mph->initial_ttl;
		r_rem[R_SEND_TIME] = r_loc[R_SEND_TIME] = ntohl(mph->ts_usec) / 1000000.0;
		r_rem[R_SEND_TIME] = r_loc[R_SEND_TIME] = r_loc[R_SEND_TIME] + ntohl(mph->ts_sec);
		r_rem[R_REPLY_TIME] = r_loc[R_REPLY_TIME] = ntohl(mph->ans_ts_usec) / 1000000.0;
		r_rem[R_REPLY_TIME] = r_loc[R_REPLY_TIME] = r_loc[R_REPLY_TIME] + ntohl(mph->ans_ts_sec);
	}

	r_rem[R_SIZE] = r_loc[R_SIZE] = pkt_info->bufSize;
	r_rem[R_TTL] = r_loc[R_TTL] = pkt_info->ttl;
	r_rem[R_DATA_ID] = r_loc[R_DATA_ID] = pkt_info->dataID;
	r_rem[R_DATA_OFFSET] = r_loc[R_DATA_OFFSET] = pkt_info->offset;
	r_rem[R_RECEIVE_TIME] = r_loc[R_RECEIVE_TIME] = pkt_info->arrival_time.tv_usec / 1000000.0;
	r_rem[R_RECEIVE_TIME] = r_loc[R_RECEIVE_TIME] += pkt_info->arrival_time.tv_sec;


	// are there in band measures?
	if (dispatcherList[sid]->rx_pkt.size() == 0)
		return;

	if(dispatcherList[sid]->rx_pkt.find(mt) == dispatcherList[sid]->rx_pkt.end())
		return;

	/* yes! */
	/* Locals first */
	ExecutionList *el_ptr_loc = &(dispatcherList[sid]->rx_pkt[mt]->el_local);

	/* Call measures in order */
	for( it = el_ptr_loc->begin(); it != el_ptr_loc->end(); it++)
			if(it->second->status == RUNNING)
					it->second->RxPktLocal(el_ptr_loc);

	/* And remotes */
	ExecutionList *el_ptr_rem = &(dispatcherList[sid]->rx_pkt[mt]->el_remote);

	/* Call measurees in order */
	j = 0;
	for( it = el_ptr_rem->begin(); it != el_ptr_rem->end(); it++)
		if(it->second->status == RUNNING)
			it->second->RxPktRemote(rmp,j,el_ptr_rem);

	/* send back results */
	if(j > 0)
		remoteResultsTx(sid, rmp, j, mt);
}

void MeasureDispatcher::cbRxData(void *arg) {
	ExecutionList::iterator it;
	result *r_loc, *r_rem;
	struct res_mh_pair rmp[R_LAST_DATA];
	int i,j;
	mon_data_inf *data_info = (mon_data_inf *)arg;
	struct MonDataHeader *mdh = (MonDataHeader*) data_info->monitoringDataHeader;
	SocketId sid = data_info->remote_socketID;
	MsgType mt = data_info->msgtype;

	/* something to do? */
	if(dispatcherList.size() == 0)
		return;

	if(dispatcherList.find(sid) == dispatcherList.end())
		return;

	if(dispatcherList[sid]->r_data.find(mt) == dispatcherList[sid]->r_data.end()) {
		return;
	}

	r_loc = dispatcherList[sid]->r_data[mt]->r_rx_local;
	r_rem = dispatcherList[sid]->r_data[mt]->r_rx_remote;

	//TODO: add standard fields
	if(mdh != NULL) {
		r_rem[R_SEQNUM] = r_loc[R_SEQNUM] = ntohl(mdh->seq_num);
		r_rem[R_SEND_TIME] = r_loc[R_SEND_TIME] = ntohl(mdh->ts_usec) / 1000000.0;
		r_rem[R_SEND_TIME] = r_loc[R_SEND_TIME] = r_loc[R_SEND_TIME] + ntohl(mdh->ts_sec);
		r_rem[R_REPLY_TIME] = r_loc[R_REPLY_TIME] = ntohl(mdh->ans_ts_usec) / 1000000.0;
		r_rem[R_REPLY_TIME] = r_loc[R_REPLY_TIME] = r_loc[R_REPLY_TIME] + ntohl(mdh->ans_ts_sec);
	}
	r_rem[R_SIZE] = r_loc[R_SIZE] = data_info->bufSize;
	r_rem[R_RECEIVE_TIME] = r_loc[R_RECEIVE_TIME] = data_info->arrival_time.tv_usec / 1000000.0;
	r_rem[R_RECEIVE_TIME] = r_loc[R_RECEIVE_TIME] += data_info->arrival_time.tv_sec;

	if (dispatcherList[sid]->rx_data.size() == 0)
		return;

	if(dispatcherList[sid]->rx_data.find(mt) == dispatcherList[sid]->rx_data.end())
		return;

	/* yes! */
	/* Locals first */
	ExecutionList *el_ptr_loc = &(dispatcherList[sid]->rx_data[mt]->el_local);


	/* Call measurees in order */
	for( it = el_ptr_loc->begin(); it != el_ptr_loc->end(); it++){
		MonMeasure *m = it->second;
		if(it->second->status == RUNNING)
			it->second->RxDataLocal(el_ptr_loc);
	}

	/* And remotes */

	ExecutionList *el_ptr_rem = &(dispatcherList[sid]->rx_data[mt]->el_remote);

	/* Call measurees in order */
	j = 0;
	for( it = el_ptr_rem->begin(); it != el_ptr_rem->end(); it++)
		if(it->second->status == RUNNING)
			it->second->RxDataRemote(rmp,j,el_ptr_rem);

	/* send back results */
	if(j > 0)
		remoteResultsTx(sid, rmp, j, mt);
}

void MeasureDispatcher::cbTxPkt(void *arg) {
	ExecutionList::iterator it;
	result *r_loc, *r_rem;
	struct res_mh_pair rmp[R_LAST_PKT];
	int i,j;
	mon_pkt_inf *pkt_info = (mon_pkt_inf *)arg;
	struct MonPacketHeader *mph = (MonPacketHeader*) pkt_info->monitoringHeader;
	SocketId sid = pkt_info->remote_socketID;
	MsgType mt = pkt_info->msgtype;
	struct timeval ts;

	
	/* something to do? */
	if(dispatcherList.size() == 0)
		return;

	if(dispatcherList.find(sid) == dispatcherList.end())
		return;

	if(dispatcherList[sid]->r_pkt.find(mt) == dispatcherList[sid]->r_pkt.end()) {
		return;
	}

	/* yes! */

	r_loc = dispatcherList[sid]->r_pkt[mt]->r_tx_local;
	r_rem = dispatcherList[sid]->r_pkt[mt]->r_tx_remote;

	/* prepare initial result vector (based on header information) */
		
	r_rem[R_SEQNUM] = r_loc[R_SEQNUM] = ++dispatcherList[sid]->r_pkt[mt]->tx_seq_num;
	r_rem[R_SIZE] = r_loc[R_SIZE] = pkt_info->bufSize;
	r_rem[R_INITIAL_TTL] = r_loc[R_INITIAL_TTL] = initial_ttl;
	gettimeofday(&ts,NULL);	
	r_rem[R_SEND_TIME] = r_loc[R_SEND_TIME] = ts.tv_usec / 1000000.0;
	r_rem[R_SEND_TIME] = r_loc[R_SEND_TIME] =  r_loc[R_SEND_TIME] + ts.tv_sec;

	if(dispatcherList[sid]->tx_pkt.find(mt) != dispatcherList[sid]->tx_pkt.end()) {

		ExecutionList *el_ptr_loc = &(dispatcherList[sid]->tx_pkt[mt]->el_local);
	
		/* Call measurees in order */
		for( it = el_ptr_loc->begin(); it != el_ptr_loc->end(); it++)
			if(it->second->status == RUNNING)
				it->second->TxPktLocal(el_ptr_loc);
	
		/* And remotes */
		ExecutionList *el_ptr_rem = &(dispatcherList[sid]->tx_pkt[mt]->el_remote);
	
		/* Call measurees in order */
		j = 0;
		for( it = el_ptr_rem->begin(); it != el_ptr_rem->end(); it++)
			if(it->second->status == RUNNING)
				it->second->TxPktRemote(rmp,j,el_ptr_rem);
	
		/* send back results */
		if(j > 0)
			remoteResultsTx(sid, rmp, j, mt);
	}

	if(mph != NULL) {
		mph->seq_num = htonl((uint32_t)r_rem[R_SEQNUM]);
		mph->initial_ttl = (uint32_t)r_rem[R_INITIAL_TTL];
		mph->ts_sec = htonl((uint32_t)floor(r_rem[R_SEND_TIME]));
		mph->ts_usec = htonl((uint32_t)((r_rem[R_SEND_TIME] - floor(r_rem[R_SEND_TIME])) * 1000000.0));
		if(!isnan(r_rem[R_REPLY_TIME])) {
			mph->ans_ts_sec = htonl((uint32_t)floor(r_rem[R_REPLY_TIME]));
			mph->ans_ts_usec = htonl((uint32_t)((r_rem[R_REPLY_TIME] - floor(r_rem[R_REPLY_TIME])) * 1000000.0));
		} else {
			mph->ans_ts_sec = 0;
			mph->ans_ts_usec = 0;
		}
	}
}

void MeasureDispatcher::cbTxData(void *arg) {
	ExecutionList::iterator it;
	result *r_loc,*r_rem;
	struct res_mh_pair rmp[R_LAST_DATA];
	int i,j;
	mon_data_inf *data_info = (mon_data_inf *)arg;
	struct MonDataHeader *mdh = (MonDataHeader*) data_info->monitoringDataHeader;
	SocketId sid = data_info->remote_socketID;
	MsgType mt = data_info->msgtype;
	struct timeval ts;

	/* something to do? */
	if(dispatcherList.size() == 0)
		return;

	if(dispatcherList.find(sid) == dispatcherList.end())
		return;

	if(dispatcherList[sid]->r_data.find(mt) == dispatcherList[sid]->r_data.end()) {
		return;
	}

	
	/* yes! */
	r_loc = dispatcherList[sid]->r_data[mt]->r_tx_local;
	r_rem = dispatcherList[sid]->r_data[mt]->r_tx_remote;

	
	//TODO add fields
	r_rem[R_SIZE] = r_loc[R_SIZE] = data_info->bufSize;
	r_rem[R_SEQNUM] = r_loc[R_SEQNUM] = ++(dispatcherList[sid]->r_data[mt]->tx_seq_num);
	gettimeofday(&ts,NULL);
	r_rem[R_SEND_TIME] = r_loc[R_SEND_TIME] = ts.tv_usec / 1000000.0;
	r_rem[R_SEND_TIME] = r_loc[R_SEND_TIME] = r_loc[R_SEND_TIME] + ts.tv_sec;


	if(dispatcherList[sid]->tx_data.find(mt) != dispatcherList[sid]->tx_data.end()) {
		ExecutionList *el_ptr_loc = &(dispatcherList[sid]->tx_data[mt]->el_local);
		

		/* Call measures in order */
		for( it = el_ptr_loc->begin(); it != el_ptr_loc->end(); it++)
			if(it->second->status == RUNNING)
				it->second->TxDataLocal(el_ptr_loc);
	
		/* And remote */
		ExecutionList *el_ptr_rem = &(dispatcherList[sid]->tx_data[mt]->el_remote);
	
		/* Call measures in order */
		j = 0;
		for( it = el_ptr_rem->begin(); it != el_ptr_rem->end(); it++)
			if(it->second->status == RUNNING)
				it->second->TxDataRemote(rmp, j, el_ptr_rem);
	
		/* send back results */
		if(j > 0)
			remoteResultsTx(sid, rmp, j, mt);
	}

	if(mdh != NULL) {
		mdh->seq_num =  htonl((uint32_t)r_rem[R_SEQNUM]);
		mdh->ts_sec =  htonl((uint32_t)floor(r_rem[R_SEND_TIME]));
		mdh->ts_usec =  htonl((uint32_t)((r_rem[R_SEND_TIME] - floor(r_rem[R_SEND_TIME])) * 1000000.0));
		if(!isnan(r_rem[R_REPLY_TIME])) {
			mdh->ans_ts_sec = htonl((uint32_t)floor(r_rem[R_REPLY_TIME]));
			mdh->ans_ts_usec = htonl((uint32_t)((r_rem[R_REPLY_TIME] - floor(r_rem[R_REPLY_TIME])) * 1000000.0));
		} else {
			mdh->ans_ts_sec = 0;
			mdh->ans_ts_usec = 0;
		}
	}
}

int MeasureDispatcher::cbHdrPkt(SocketId sid, MsgType mt) {
	/* something to do? */
	if(dispatcherList.size() == 0)
		return 0;

	if(dispatcherList.find(sid) == dispatcherList.end())
		return 0;

	if(dispatcherList[sid]->tx_pkt.find(mt) == dispatcherList[sid]->tx_pkt.end() &&
		dispatcherList[sid]->r_pkt.find(mt) == dispatcherList[sid]->r_pkt.end())
		return 0;

	/* yes! */
	return MON_PACKET_HEADER_SIZE;
}

int MeasureDispatcher::cbHdrData(SocketId sid, MsgType mt) {
	/* something to do? */
	if(dispatcherList.size() == 0)
		return 0;

	if(dispatcherList.find(sid) == dispatcherList.end())
		return 0;

	if(dispatcherList[sid]->tx_data.find(mt) == dispatcherList[sid]->tx_data.end() &&
		dispatcherList[sid]->r_data.find(mt) == dispatcherList[sid]->r_data.end())
		return 0;

	/* yes! return the space we need */
	return MON_DATA_HEADER_SIZE;
}
int MeasureDispatcher::scheduleMeasure(MonHandler mh) {
	if(mm->isValidMonHandler(mh))
		mm->mMeasureInstances[mh]->Run();
	return EOK;
}
int MeasureDispatcher::schedulePublish(MonHandler mh) {
	if(mm->isValidMonHandler(mh))
		if(mm->mMeasureInstances[mh]->rb != NULL)
			mm->mMeasureInstances[mh]->rb->publishResults();
	return EOK;
}

