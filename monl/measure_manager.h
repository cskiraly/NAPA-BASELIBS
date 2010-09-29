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

#ifndef _MEASURE_MANAGER_H
#define _MEASURE_MANAGER_H

#include <string>
#include <map>
#include "mon_measure.h"
#include "measure_plugin.h"
#include "measure_dispatcher.h"

#include "grapes_log.h"


#define MAX_NUM_MEASURES_INSTANTIATED 1000000

typedef std::map<MeasurementId, class MeasurePlugin *> MonMeasureMapId;
typedef std::map<std::string, class MeasurePlugin *> MonMeasureMapName;
typedef std::map<MonHandler,class MonMeasure *> MeasureMap;

class MeasureManager {
protected:
	/* List of available measures */
	MonMeasureMapId mMonMeasuresId;
	MonMeasureMapName mMonMeasuresName;

	/* defualts */
	void *default_repo_client;
	char* default_channel;
	char** peer_name;

	class MeasureDispatcher cDispatcher;
	
	/* Currently installed instances */
	MeasureMap mMeasureInstances;
	int next_measure_handler;
	
	/* Used to load a MeasurePlugin. If the Id or name has been already used it returns a negative value, *
	 * 0 otherwise. */
	int loadMeasurePlugin(MeasurePlugin *plugin) {
		if(mMonMeasuresId.find(plugin->getId()) == mMonMeasuresId.end() &&
			mMonMeasuresName.find(plugin->getName()) == mMonMeasuresName.end())
		{
			mMonMeasuresId[plugin->getId()] = plugin;
			mMonMeasuresName[plugin->getName()] = plugin;
			return EOK;
		} else
			return -EINVAL;
	};

	int loadMeasurePlugins(void);

	/* Inserts new instance. Returns handler */
	 MonHandler insertNewInstance(MonMeasure *m) {
		int new_mh;
		/* For memory efficiency we use a hash container and a circular buffer */
		/* check if next_measure_handler is not in use (went around once) */
		if(mMeasureInstances.find(next_measure_handler) != mMeasureInstances.end())
			return -ENOMEM;
		new_mh = next_measure_handler;
		mMeasureInstances[new_mh] = m;

		/* update next_measure_handler */
		next_measure_handler++;
		if(next_measure_handler == MAX_NUM_MEASURES_INSTANTIATED)
			next_measure_handler = 0;

		return new_mh;
	}
	/* checks handler */
	bool isValidMonHandler(MonHandler mh) {
		if(mMeasureInstances.find(mh) != mMeasureInstances.end())
			return true;
		else
			return false;
	}
	
	friend class MonMeasure;
	friend class MeasureDispatcher;
public:
	/* Constructor */
	MeasureManager() {
		default_repo_client = NULL;
		default_channel = NULL;
		next_measure_handler = 0;
		cDispatcher.mm = this;
		peer_name = new char*;
		*peer_name =  NULL;
		loadMeasurePlugins();
	};

	MeasureManager(void *repo) {
		default_repo_client = repo;
		default_channel = NULL;
		next_measure_handler = 0;
		cDispatcher.mm = this;
		peer_name = new char*;
		*peer_name =  NULL;
		loadMeasurePlugins();
	};

	/* Destructor */
	~MeasureManager() {
		/* Free list of available measures */
		for(MonMeasureMapId::reverse_iterator it = mMonMeasuresId.rbegin();
			it !=mMonMeasuresId.rend(); ++it)
			delete it->second;
		/* Free instances */
		for(MeasureMap::iterator it = mMeasureInstances.begin();
			it != mMeasureInstances.end(); ++it)
			monDestroyMeasure(it->first);
		if(default_channel)
			delete[] default_channel;
		if(*peer_name != NULL)
			delete[] *peer_name;
		delete peer_name;
	}

	/* Returns a map container with all the measures which can be instantiated and used */
	MonMeasureMapId& monListMeasure() {
		return mMonMeasuresId;
	};

	/* Creates an instance of a measure specified through the name of the measure
	 * returns handler of the new instance to be used subsequently with the functions below */
	int monCreateMeasureName(MeasurementName name, MeasurementCapabilities mc) {
		std::string t(name);
		MonMeasure *m;
		int ret;
		MonHandler mh;

		if(mMonMeasuresName.find(t) == mMonMeasuresName.end())
			return -EINVAL;

		if(!(mc & REMOTE))
			mc |= (mMonMeasuresName[t]->getCaps() & TXRXBI);

		m = mMonMeasuresName[t]->createMeasure(mc, &cDispatcher);

		ret = m->setFlags(mc);
		if(ret != EOK)
			goto error;

		ret = mh = insertNewInstance(m);
		if(mh < 0)
			goto error;

		m->mh_local = mh;
		return mh;

	error:
		delete m;
		return ret;
	};

	int monCreateMeasureId(MeasurementId id, MeasurementCapabilities mc) {
		MonMeasure *m;
		int ret;
		MonHandler mh;

		if(mMonMeasuresId.find(id) == mMonMeasuresId.end())
			return -EINVAL;

		if(!(mc & REMOTE))
			mc |= (mMonMeasuresId[id]->getCaps() & TXRXBI);

		m = mMonMeasuresId[id]->createMeasure(mc, &cDispatcher);

		ret = m->setFlags(mc);
		if(ret != EOK)
			goto error;

		ret = mh = insertNewInstance(m);
		if(mh < 0)
			goto error;

		m->mh_local = mh;

		return mh;

	error:
		delete m;
		return ret;
	};

	/* Destroys a specific measure instance */
	int monDestroyMeasure(MonHandler mh) {
		/* find it */
		if(!isValidMonHandler(mh))
			return -EINVAL;

		/* deactivate it */
		cDispatcher.deactivateMeasure(mMeasureInstances[mh]);

		delete mMeasureInstances[mh];
		mMeasureInstances.erase(mh);

		return EOK;
	};

	MeasurementId monMeasureHandlerToId (MonHandler mh) {
		/* find it */
		if(!isValidMonHandler(mh))
			return -EINVAL;
		
		return mMeasureInstances[mh]->measure_plugin->getId();
	}

	std::vector<class MonParameter *>* monListParameters(MonHandler mh) {
		if(!isValidMonHandler(mh))
			return NULL;
		return &(mMeasureInstances[mh]->listParameters());
	};

	int monSetParameter(MonHandler mh, int pid, MonParameterValue p) {
		if(!isValidMonHandler(mh))
			return -EINVAL;
		return mMeasureInstances[mh]->setParameter(pid,p);
	};

	MonParameterValue monGetParameter(MonHandler mh, size_t pid) {
		if(!isValidMonHandler(mh))
			return NAN;
		return mMeasureInstances[mh]->getParameter(pid);
	};

	MeasurementStatus monGetStatus(MonHandler mh) {
		if(!isValidMonHandler(mh))
			return MS_ERROR;
		return mMeasureInstances[mh]->getStatus();
	};

	 MeasurementCapabilities monGetMeasurementCaps(MonHandler mh) {
		if(!isValidMonHandler(mh))
			return -EINVAL;
		return mMeasureInstances[mh]->getFlags();
	}

	int monActivateMeasure(MonHandler mh, SocketId sid, MsgType mt) {
		if(!isValidMonHandler(mh))
			return -EINVAL;
		return cDispatcher.activateMeasure(mMeasureInstances[mh], sid, mt);
	}

	int monDeactivateMeasure(MonHandler mh) {
		if(!isValidMonHandler(mh))
			return -EINVAL;
		return cDispatcher.deactivateMeasure(mMeasureInstances[mh]);	
	}

	size_t getMeasuresCount(void) {
		return mMonMeasuresId.size();
	};

	void cbRxPkt(void *arg) {
		cDispatcher.cbRxPkt(arg);
	};

	void cbRxData(void *arg) {
		cDispatcher.cbRxData(arg);
	};

	void cbTxPkt(void *arg) {
		cDispatcher.cbTxPkt(arg);
	};

	void cbTxData(void *arg) {
		cDispatcher.cbTxData(arg);
	};

	int cbHdrPkt(SocketId sid, MsgType mt) {
		return cDispatcher.cbHdrPkt(sid, mt);
	};

	int cbHdrData(SocketId sid, MsgType mt) {
		return cDispatcher.cbHdrData(sid, mt);
	};

	void receiveCtrlMsg(SocketId sid, MsgType mt, char *buffer, int buflen){
		cDispatcher.receiveCtrlMsg(sid, mt, buffer, buflen);
	};

	int monPublishStatisticalType(MonHandler mh, const char* publishing_name, const char *channel, enum stat_types st[], int length, void *repo_client) {
		if(repo_client == NULL)
			repo_client = default_repo_client;

		if(repo_client == NULL)
			return -EFAILED;

		if(channel == NULL)
			channel = default_channel;

		if(!isValidMonHandler(mh))
			return -EINVAL;

		return mMeasureInstances[mh]->setPublishStatisticalType(publishing_name, channel, st, length, repo_client);
	};

	result monRetrieveResult(MonHandler mh, enum stat_types st) {
		if(!isValidMonHandler(mh))
			return NAN;

		if(mMeasureInstances[mh]->rb == NULL)
			return NAN;

		mMeasureInstances[mh]->rb->updateStats();
		return mMeasureInstances[mh]->rb->stats[st];
	};

	int monNewSample(MonHandler mh, result r) {
		if(!isValidMonHandler(mh))
			return -EINVAL;

		if(mMeasureInstances[mh]->flags != 0)
			return -EINVAL;

		return mMeasureInstances[mh]->newSample(r);
	};

	void monParseConfig(void *);

	int monSetPeerName(const char *pn);
};

#endif /* _MEASURE_MANAGER_H */
