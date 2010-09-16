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

#include <cmath>

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

void MeasureDispatcher::addMeasureToExecLists(SocketIdMt h_dst, class MonMeasure *m) {
	class MeasurePlugin *mp = m->measure_plugin;

	/* IN_BAND  measurments added to hook executionlists */
	if(m->flags & IN_BAND) {
		/* TXLOC */
		if(m->flags & TXLOC) {
			if(m->flags & PACKET) {
				if(m->flags & REMOTE)
					dispatcherList[h_dst]->el_tx_pkt_remote[mp->getId()] = m;
				else
					dispatcherList[h_dst]->el_tx_pkt_local[mp->getId()] = m;
			}
			if(m->flags & DATA) {
				if(m->flags & REMOTE)
					dispatcherList[h_dst]->el_tx_data_remote[mp->getId()] = m;
				else
					dispatcherList[h_dst]->el_tx_data_local[mp->getId()] = m;
			}
		}
		/* RXLOC */
		if(m->flags & RXLOC) {
			if(m->flags & PACKET)  {
				if(m->flags & REMOTE)
					dispatcherList[h_dst]->el_rx_pkt_remote[mp->getId()] = m;
				else
					dispatcherList[h_dst]->el_rx_pkt_local[mp->getId()] = m;
			}
			if(m->flags & DATA) {
				if(m->flags & REMOTE)
					dispatcherList[h_dst]->el_rx_data_remote[mp->getId()] = m;
				else
					dispatcherList[h_dst]->el_rx_data_local[mp->getId()] = m;
			}
		}
	}
}

void MeasureDispatcher::delMeasureFromExecLists(MonMeasure *m) {
	SocketIdMt h_dst;
	class MeasurePlugin *mp = m->measure_plugin;
	ExecutionList::iterator it;

	h_dst.sid = (SocketId) m->dst_socketid;
	h_dst.mt = m->msg_type;

	/* IN_BAND  measurments added to hook executionlists */
	if(m->flags & IN_BAND) {
		/* TXLOC */
		if(m->flags & TXLOC) {
			if(m->flags & PACKET) {
				if(m->flags & REMOTE) {
					it = dispatcherList[h_dst]->el_tx_pkt_remote.find(mp->getId());
					if(it != dispatcherList[h_dst]->el_tx_pkt_remote.end())
						dispatcherList[h_dst]->el_tx_pkt_remote.erase(it);
				} else {
					it = dispatcherList[h_dst]->el_tx_pkt_local.find(mp->getId());
					if(it != dispatcherList[h_dst]->el_tx_pkt_local.end())
						dispatcherList[h_dst]->el_tx_pkt_local.erase(it);
				}
			}
			if(m->flags & DATA) {
				if(m->flags & REMOTE) {
					it = dispatcherList[h_dst]->el_tx_data_remote.find(mp->getId());
					if(it != dispatcherList[h_dst]->el_tx_data_remote.end())
						dispatcherList[h_dst]->el_tx_data_remote.erase(it);
				} else {
					it = dispatcherList[h_dst]->el_tx_data_local.find(mp->getId());
					if(it != dispatcherList[h_dst]->el_tx_data_local.end())
						dispatcherList[h_dst]->el_tx_data_local.erase(it);
				}
			}
		}
		/* RXLOC */
		if(m->flags & RXLOC) {
			if(m->flags & PACKET) {
				if(m->flags & REMOTE) {
					it = dispatcherList[h_dst]->el_rx_pkt_remote.find(mp->getId());
					if(it != dispatcherList[h_dst]->el_rx_pkt_remote.end())
						dispatcherList[h_dst]->el_rx_pkt_remote.erase(it);
				} else {
					it = dispatcherList[h_dst]->el_rx_pkt_local.find(mp->getId());
					if(it != dispatcherList[h_dst]->el_rx_pkt_local.end())
						dispatcherList[h_dst]->el_rx_pkt_local.erase(it);
				}
			}
			if(m->flags & DATA) {
				if(m->flags & REMOTE) {
					it = dispatcherList[h_dst]->el_rx_data_remote.find(mp->getId());
					if(it != dispatcherList[h_dst]->el_rx_data_remote.end())
						dispatcherList[h_dst]->el_rx_data_remote.erase(it);
				} else {
					it = dispatcherList[h_dst]->el_rx_data_local.find(mp->getId());
					if(it != dispatcherList[h_dst]->el_rx_data_local.end())
						dispatcherList[h_dst]->el_rx_data_local.erase(it);
				}
			}
		}
	}
}

int MeasureDispatcher::sendCtrlMsg(SocketId dst, Buffer &buffer)
{
	int con_id,res;
	Message *msg;
	send_params sparam = {0,0,0,0,0};
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

	return sendCtrlMsg((SocketId) m->dst_socketid, buffer);
}

int MeasureDispatcher::oobDataRx(SocketId sid, MsgType mt, char *cbuf, int length) {
	struct SocketIdMt h_dst;
	h_dst.sid = sid;
	h_dst.mt = MSG_TYPE_MONL;

	struct OobData *oobdata = (struct OobData*) cbuf;
	result *r = NULL;

	if(mm->isValidMonHandler(oobdata->mh_remote))
		if(mm->mMeasureInstances[oobdata->mh_remote]->status == RUNNING &&
 				mlCompareSocketIDs((SocketId) mm->mMeasureInstances[oobdata->mh_remote]->dst_socketid, sid) == 0) {
			if(mm->mMeasureInstances[oobdata->mh_remote]->flags & DATA) {
				if(mm->mMeasureInstances[oobdata->mh_remote]->flags & REMOTE)
					r = dispatcherList[h_dst]->data_r_rx_remote;
				else
					r = dispatcherList[h_dst]->data_r_rx_local;
			} else if (mm->mMeasureInstances[oobdata->mh_remote]->flags & PACKET) {
				if(mm->mMeasureInstances[oobdata->mh_remote]->flags & REMOTE)
					r = dispatcherList[h_dst]->pkt_r_rx_remote;
				else
					r = dispatcherList[h_dst]->pkt_r_rx_local;
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
 			 mlCompareSocketIDs((SocketId) mm->mMeasureInstances[rmp[i].mh]->dst_socketid, sid) == 0) {
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
				if(m->status == STOPPING) {
					stopMeasure(m);
					if(mm->monDestroyMeasure(m->mh_local) != EOK)
						m->status = FAILED;
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
	struct SocketIdMt h_dst;
	struct InitRemoteMeasure *initmeasure = (InitRemoteMeasure*) cbuf;

	h_dst.sid = src;
	h_dst.mt = initmeasure->msg_type;

	/* Check if a previous instance is running */
	if(dispatcherList.find(h_dst) != dispatcherList.end())
		m = findMeasureFromId(dispatcherList[h_dst], initmeasure->mc, initmeasure->mid);

	//if so remove it, so that we can create a new one (needed if mc changed)
	if(m != NULL)
		mm->monDestroyMeasure(m->mh_local);

	mh = mm->monCreateMeasureId(initmeasure->mid, initmeasure->mc);

	if(mh < 0)
		return remoteMeasureResponseTx(src, initmeasure->mh_local, -1, INITREMOTEMEASURE, EINVAL);

	mm->mMeasureInstances[mh]->mh_remote = initmeasure->mh_local;

	param_vector = (MonParameterValue*)(cbuf + INITREMOTEMEASURE_SIZE);

	for(int i=0; i < initmeasure->n_params; i++) {
		if(mm->monSetParameter(mh,i,param_vector[i]) != EOK)	//TODO: init might be called inside this, which might use paramerets (e.g. src) that are set only below, in activateMeasure
			goto error;
	}

	if(activateMeasure(mm->mMeasureInstances[mh], src, initmeasure->msg_type) != EOK)
		goto error;

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

	return sendCtrlMsg((SocketId) m->dst_socketid, buffer);
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

class MonMeasure* MeasureDispatcher::findMeasureFromId(DestinationSocketIdMtData *dd,  MeasurementCapabilities flags, MeasurementId mid) {
	ExecutionList::iterator it;

	//check if loaded
	if(flags & REMOTE) {
		it = dd->mids_remote.find(mid);
		if(it == dd->mids_remote.end())
			return NULL;
	} else {
		it = dd->mids_local.find(mid);
		if(it == dd->mids_local.end())
			return NULL;
	}

	return it->second;
}

int MeasureDispatcher::activateMeasure(class MonMeasure *m, SocketId dst, MsgType mt, int auto_load) {
	MeasurementId mid;
	struct SocketIdMt h_dst;
	int ret;

	h_dst.sid = dst;
	h_dst.mt = mt;

	mid = m->measure_plugin->getId();
	m->msg_type = mt;

	if(dst != NULL) {
		memcpy(m->dst_socketid, (uint8_t *) dst, SOCKETID_SIZE);
		m->dst_socketid_publish = true;
	} else {
		if(m->flags != 0)
			return -EINVAL;
		m->dst_socketid_publish = false;
	}

	m->defaultInit();

	if(m->flags == 0) {
		m->status = RUNNING;
		return EOK;
	}

	//check if already present
	if(dispatcherList.find(h_dst) != dispatcherList.end()) {
		if(m->flags & REMOTE) {
			if(dispatcherList[h_dst]->mids_remote.find(mid) != dispatcherList[h_dst]->mids_remote.end())
				return -EEXISTS;
		} else {
			if(dispatcherList[h_dst]->mids_local.find(mid) != dispatcherList[h_dst]->mids_local.end())
				return -EEXISTS;
		}
	}


	if(dispatcherList.find(h_dst) == dispatcherList.end()) {
		createDestinationSocketIdMtData(h_dst);
	}

	if(m->flags & OUT_OF_BAND) {
		struct SocketIdMt h_dst2 = h_dst;
		h_dst2.mt = MSG_TYPE_MONL;
		if(dispatcherList.find(h_dst2) == dispatcherList.end())
			createDestinationSocketIdMtData(h_dst2);
	}

	if(m->flags & PACKET) {
		if(m->flags & REMOTE) {
			m->r_rx_list = dispatcherList[h_dst]->pkt_r_rx_remote;
			m->r_tx_list = dispatcherList[h_dst]->pkt_r_tx_remote;
		} else {
			m->r_rx_list = dispatcherList[h_dst]->pkt_r_rx_local;
			m->r_tx_list = dispatcherList[h_dst]->pkt_r_tx_local;
		}
	} else {
		if(m->flags & REMOTE) {
			m->r_rx_list = dispatcherList[h_dst]->data_r_rx_remote;
			m->r_tx_list = dispatcherList[h_dst]->data_r_tx_remote;
		} else {
			m->r_rx_list = dispatcherList[h_dst]->data_r_rx_local;
			m->r_tx_list = dispatcherList[h_dst]->data_r_tx_local;
		}
	}
	//TODO: check deps

	// add it to loaded measure list
	if(m->flags & REMOTE)
		dispatcherList[h_dst]->mids_remote[mid] = m;
	else
		dispatcherList[h_dst]->mids_local[mid] = m;

	//Handle IN_BAND measures
	addMeasureToExecLists(h_dst, m);

	//Handle OUT_OF_BAND measures
	if(m->flags & OUT_OF_BAND) {
		struct timeval tv = {0,0};
		//if only local
		if(!(m->flags & TXREM) && !(m->flags & RXREM))
			m->scheduleNextIn(&tv); //start it!
	}

	/* Remote counterparts ? */
	if(m->flags & TXREM || m->flags & RXREM) {
		/* Yes, initialise them */
		m->status = INITIALISING;
		ret = initRemoteMeasureTx(m,dst,mt);
		if(ret != EOK)
			goto error;
	} else {
		m->status = RUNNING;
	}

	if(m->dst_socketid == NULL)
		info("bla1");
	return EOK;

error:
	if(m->dst_socketid == NULL)
		info("bla2");

	stopMeasure(m);
	if(m->dst_socketid == NULL)
		info("bla3");

	return ret;
}

void MeasureDispatcher::createDestinationSocketIdMtData(struct SocketIdMt h_dst) {
	DestinationSocketIdMtData *dd;
	
	dd = new DestinationSocketIdMtData;

	memcpy(dd->sid, (uint8_t*) h_dst.sid, SOCKETID_SIZE);
	dd->h_dst.sid = (SocketId) &(dd->sid);
	dd->h_dst.mt = h_dst.mt;

	dispatcherList[dd->h_dst] = dd;

	//initialize
	for(int i = 0; i < R_LAST_PKT; i++) {
		dispatcherList[dd->h_dst]->pkt_r_rx_local[i] = NAN;
		dispatcherList[dd->h_dst]->pkt_r_rx_remote[i] = NAN;
		dispatcherList[dd->h_dst]->pkt_r_tx_local[i] = NAN;
		dispatcherList[dd->h_dst]->pkt_r_tx_remote[i] = NAN;
	}
	dispatcherList[dd->h_dst]->pkt_tx_seq_num = 0;

	for(int i = 0; i < R_LAST_DATA; i++) {
		dispatcherList[dd->h_dst]->data_r_rx_local[i] = NAN;
		dispatcherList[dd->h_dst]->data_r_rx_remote[i] = NAN;
		dispatcherList[dd->h_dst]->data_r_tx_local[i] = NAN;
		dispatcherList[dd->h_dst]->data_r_tx_remote[i] = NAN;
	}
	dispatcherList[dd->h_dst]->data_tx_seq_num = 0;
}

void MeasureDispatcher::destroyDestinationSocketIdMtData(SocketId dst, MsgType mt) {
	DestinationSocketIdMtData *dd;
	DispatcherListSocketIdMt::iterator it;
	struct SocketIdMt h_dst;

	h_dst.sid = dst;
	h_dst.mt = mt;

	it = dispatcherList.find(h_dst);
	if(it != dispatcherList.end()) {
		dd = it->second;
		dispatcherList.erase(it);
		delete dd;
	}
	else
		fatal("Trying to erase non existent DestinationSocketIdMtData");
}

int MeasureDispatcher::deactivateMeasure(class MonMeasure *m) {
	SocketIdMt h_dst;

	h_dst.sid = (SocketId) m->dst_socketid;
	h_dst.mt = m->msg_type;

	if(m->status == STOPPED)
		return EOK;

	if(m->used_counter > 0)
		return -EINUSE;

	if(m->flags == 0) {
		m->defaultStop();
		m->status = STOPPED;
		return EOK;
	}

	if(dispatcherList.find(h_dst) == dispatcherList.end())
		return EINVAL;

	//remove it from the loaded list
	if(m->flags & REMOTE) {
		if(dispatcherList[h_dst]->mids_remote.find(m->measure_plugin->getId()) == dispatcherList[h_dst]->mids_remote.end())
			return -EINVAL;
	} else {
		if(dispatcherList[h_dst]->mids_local.find(m->measure_plugin->getId()) == dispatcherList[h_dst]->mids_local.end())
			return -EINVAL;
	}

	return stopMeasure(m);
}

int MeasureDispatcher::stopMeasure(class MonMeasure *m) {
	SocketIdMt h_dst;

	h_dst.sid = (SocketId) m->dst_socketid;
	h_dst.mt = m->msg_type;

	m->defaultStop();

	//Handle IN_BAND measures
	delMeasureFromExecLists(m);
	//Handle OUT_OF:BAND measures
	//TODO: Anything to do at all?

	//Handle remote counterpart
	if(m->flags & TXREM || m->flags & RXREM) {
			m->status = STOPPING;
			deinitRemoteMeasureTx(m);
	} else
		m->status = STOPPED;
	m->r_tx_list = m->r_rx_list = NULL;

	//remove it from the loaded list
	if(m->flags & REMOTE)
		dispatcherList[h_dst]->mids_remote.erase(dispatcherList[h_dst]->mids_remote.find(m->measure_plugin->getId()));
	else
		dispatcherList[h_dst]->mids_local.erase(dispatcherList[h_dst]->mids_local.find(m->measure_plugin->getId()));

	//check if we can remove also the DistinationSocektIdMtData
	if(dispatcherList[h_dst]->mids_remote.empty() && dispatcherList[h_dst]->mids_local.empty())
		destroyDestinationSocketIdMtData((SocketId) m->dst_socketid, m->msg_type);
}


void MeasureDispatcher::cbRxPkt(void *arg) {
	ExecutionList::iterator it;
	result *r_loc, *r_rem;
	struct res_mh_pair rmp[R_LAST_PKT];
	int i,j;
	mon_pkt_inf *pkt_info = (mon_pkt_inf *)arg;
	struct MonPacketHeader *mph = (MonPacketHeader*) pkt_info->monitoringHeader;
	struct SocketIdMt h_dst;
	h_dst.sid = pkt_info->remote_socketID;
	h_dst.mt = pkt_info->msgtype;

	if(pkt_info->remote_socketID == NULL)
		return;

	/* something to do? */
	if(dispatcherList.size() == 0)
		return;

	if(dispatcherList.find(h_dst) == dispatcherList.end())
		return;

	//we have a result vector to fill
	r_loc = dispatcherList[h_dst]->pkt_r_rx_local;
	r_rem = dispatcherList[h_dst]->pkt_r_rx_remote;

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


	// are there local in band measures?
	if (dispatcherList[h_dst]->el_rx_pkt_local.size() > 0) {
		/* yes! */
		ExecutionList *el_ptr_loc = &(dispatcherList[h_dst]->el_rx_pkt_local);
	
		/* Call measures in order */
		for( it = el_ptr_loc->begin(); it != el_ptr_loc->end(); it++)
			if(it->second->status == RUNNING)
				it->second->RxPktLocal(el_ptr_loc);
	}

	if (dispatcherList[h_dst]->el_rx_pkt_remote.size() > 0) {
		/* And remotes */
		ExecutionList *el_ptr_rem = &(dispatcherList[h_dst]->el_rx_pkt_remote);
	
		/* Call measurees in order */
		j = 0;
		for( it = el_ptr_rem->begin(); it != el_ptr_rem->end(); it++)
			if(it->second->status == RUNNING)
				it->second->RxPktRemote(rmp,j,el_ptr_rem);
	
		/* send back results */
		if(j > 0)
		remoteResultsTx(h_dst.sid, rmp, j, h_dst.mt);
	}
}

void MeasureDispatcher::cbRxData(void *arg) {
	ExecutionList::iterator it;
	result *r_loc, *r_rem;
	struct res_mh_pair rmp[R_LAST_DATA];
	int i,j;
	mon_data_inf *data_info = (mon_data_inf *)arg;
	struct MonDataHeader *mdh = (MonDataHeader*) data_info->monitoringDataHeader;
	struct SocketIdMt h_dst;
	h_dst.sid = data_info->remote_socketID;
	h_dst.mt = data_info->msgtype;

	if(data_info->remote_socketID == NULL)
		return;

	/* something to do? */
	if(dispatcherList.size() == 0)
		return;

	if(dispatcherList.find(h_dst) == dispatcherList.end())
		return;

	//we have a result vector to fill
	r_loc = dispatcherList[h_dst]->data_r_rx_local;
	r_rem = dispatcherList[h_dst]->data_r_rx_remote;

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

	if (dispatcherList[h_dst]->el_rx_data_local.size() > 0) {
		/* yes! */
		/* Locals first */
		ExecutionList *el_ptr_loc = &(dispatcherList[h_dst]->el_rx_data_local);
	
	
		/* Call measurees in order */
		for( it = el_ptr_loc->begin(); it != el_ptr_loc->end(); it++){
			MonMeasure *m = it->second;
			if(it->second->status == RUNNING)
				it->second->RxDataLocal(el_ptr_loc);
		}
	}

	if (dispatcherList[h_dst]->el_rx_data_remote.size() > 0) {
		/* And remotes */
	
		ExecutionList *el_ptr_rem = &(dispatcherList[h_dst]->el_rx_data_remote);
	
		/* Call measurees in order */
		j = 0;
		for( it = el_ptr_rem->begin(); it != el_ptr_rem->end(); it++)
			if(it->second->status == RUNNING)
				it->second->RxDataRemote(rmp,j,el_ptr_rem);
	
		/* send back results */
		if(j > 0)
			remoteResultsTx(h_dst.sid, rmp, j, h_dst.mt);
	}
}

void MeasureDispatcher::cbTxPkt(void *arg) {
	ExecutionList::iterator it;
	result *r_loc, *r_rem;
	struct res_mh_pair rmp[R_LAST_PKT];
	int i,j;
	mon_pkt_inf *pkt_info = (mon_pkt_inf *)arg;
	struct MonPacketHeader *mph = (MonPacketHeader*) pkt_info->monitoringHeader;
	struct timeval ts;
	struct SocketIdMt h_dst;
	h_dst.sid = pkt_info->remote_socketID;
	h_dst.mt = pkt_info->msgtype;

	if(pkt_info->remote_socketID == NULL)
		return;

	/* something to do? */
	if(dispatcherList.size() == 0)
		return;

	if(dispatcherList.find(h_dst) == dispatcherList.end())
		return;

	/* yes! */

	r_loc = dispatcherList[h_dst]->pkt_r_tx_local;
	r_rem = dispatcherList[h_dst]->pkt_r_tx_remote;

	/* prepare initial result vector (based on header information) */
		
	r_rem[R_SEQNUM] = r_loc[R_SEQNUM] = ++(dispatcherList[h_dst]->pkt_tx_seq_num);
	r_rem[R_SIZE] = r_loc[R_SIZE] = pkt_info->bufSize;
	r_rem[R_INITIAL_TTL] = r_loc[R_INITIAL_TTL] = initial_ttl;
	gettimeofday(&ts,NULL);	
	r_rem[R_SEND_TIME] = r_loc[R_SEND_TIME] = ts.tv_usec / 1000000.0;
	r_rem[R_SEND_TIME] = r_loc[R_SEND_TIME] =  r_loc[R_SEND_TIME] + ts.tv_sec;

	if(dispatcherList[h_dst]->el_tx_pkt_local.size() > 0) {

		ExecutionList *el_ptr_loc = &(dispatcherList[h_dst]->el_tx_pkt_local);

		/* Call measurees in order */
		for( it = el_ptr_loc->begin(); it != el_ptr_loc->end(); it++)
			if(it->second->status == RUNNING)
				it->second->TxPktLocal(el_ptr_loc);
	}

	if(dispatcherList[h_dst]->el_tx_pkt_remote.size() > 0) {
		/* And remotes */
		ExecutionList *el_ptr_rem = &(dispatcherList[h_dst]->el_tx_pkt_remote);
	
		/* Call measurees in order */
		j = 0;
		for( it = el_ptr_rem->begin(); it != el_ptr_rem->end(); it++)
			if(it->second->status == RUNNING)
				it->second->TxPktRemote(rmp,j,el_ptr_rem);
	
		/* send back results */
		if(j > 0)
			remoteResultsTx(h_dst.sid, rmp, j, h_dst.mt);
	}

	if(mph != NULL) {
		mph->seq_num = htonl((uint32_t)r_rem[R_SEQNUM]);
		mph->initial_ttl = (uint32_t)r_rem[R_INITIAL_TTL];
		mph->ts_sec = htonl((uint32_t)floor(r_rem[R_SEND_TIME]));
		mph->ts_usec = htonl((uint32_t)((r_rem[R_SEND_TIME] - floor(r_rem[R_SEND_TIME])) * 1000000.0));
		if(!std::isnan(r_rem[R_REPLY_TIME])) {
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
	struct timeval ts;
	struct SocketIdMt h_dst;
	h_dst.sid = data_info->remote_socketID;
	h_dst.mt = data_info->msgtype;

	if(data_info->remote_socketID == NULL)
		return;

	/* something to do? */
	if(dispatcherList.size() == 0)
		return;

	if(dispatcherList.find(h_dst) == dispatcherList.end())
		return;
	
	/* yes! */
	r_loc = dispatcherList[h_dst]->data_r_tx_local;
	r_rem = dispatcherList[h_dst]->data_r_tx_remote;

	
	//TODO add fields
	r_rem[R_SIZE] = r_loc[R_SIZE] = data_info->bufSize;
	r_rem[R_SEQNUM] = r_loc[R_SEQNUM] = ++(dispatcherList[h_dst]->data_tx_seq_num);
	gettimeofday(&ts,NULL);
	r_rem[R_SEND_TIME] = r_loc[R_SEND_TIME] = ts.tv_usec / 1000000.0;
	r_rem[R_SEND_TIME] = r_loc[R_SEND_TIME] = r_loc[R_SEND_TIME] + ts.tv_sec;

	if(dispatcherList[h_dst]->el_tx_data_local.size() > 0) {
		ExecutionList *el_ptr_loc = &(dispatcherList[h_dst]->el_tx_data_local);

		/* Call measures in order */
		for( it = el_ptr_loc->begin(); it != el_ptr_loc->end(); it++)
			if(it->second->status == RUNNING)
				it->second->TxDataLocal(el_ptr_loc);
	}

	if(dispatcherList[h_dst]->el_tx_data_remote.size() > 0) {
		/* And remote */
		ExecutionList *el_ptr_rem = &(dispatcherList[h_dst]->el_tx_data_remote);
	
		/* Call measures in order */
		j = 0;
		for( it = el_ptr_rem->begin(); it != el_ptr_rem->end(); it++)
			if(it->second->status == RUNNING)
				it->second->TxDataRemote(rmp, j, el_ptr_rem);
	
		/* send back results */
		if(j > 0)
			remoteResultsTx(h_dst.sid, rmp, j, h_dst.mt);
	}

	if(mdh != NULL) {
		mdh->seq_num =  htonl((uint32_t)r_rem[R_SEQNUM]);
		mdh->ts_sec =  htonl((uint32_t)floor(r_rem[R_SEND_TIME]));
		mdh->ts_usec =  htonl((uint32_t)((r_rem[R_SEND_TIME] - floor(r_rem[R_SEND_TIME])) * 1000000.0));
		if(!std::isnan(r_rem[R_REPLY_TIME])) {
			mdh->ans_ts_sec = htonl((uint32_t)floor(r_rem[R_REPLY_TIME]));
			mdh->ans_ts_usec = htonl((uint32_t)((r_rem[R_REPLY_TIME] - floor(r_rem[R_REPLY_TIME])) * 1000000.0));
		} else {
			mdh->ans_ts_sec = 0;
			mdh->ans_ts_usec = 0;
		}
	}
}

int MeasureDispatcher::cbHdrPkt(SocketId sid, MsgType mt) {
	struct SocketIdMt h_dst;
	h_dst.sid = sid;
	h_dst.mt = mt;

	if(sid == NULL)
		return 0;

	/* something to do? */
	if(dispatcherList.size() == 0)
		return 0;

	if(dispatcherList.find(h_dst) == dispatcherList.end())
		return 0;

	/* yes! */
	return MON_PACKET_HEADER_SIZE;
}

int MeasureDispatcher::cbHdrData(SocketId sid, MsgType mt) {
	struct SocketIdMt h_dst;
	h_dst.sid = sid;
	h_dst.mt = mt;

	if(sid == NULL)
		return 0;

	/* something to do? */
	if(dispatcherList.size() == 0)
		return 0;

	if(dispatcherList.find(h_dst) == dispatcherList.end())
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
