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

#include "measure_manager.h"
#include "string.h"

//Start of plugins
#include "plugins/example_measure.h"
#include "plugins/hopcount_measure.h"
#include "plugins/rtt_measure.h"
#include "plugins/seqwin_measure.h"
#include "plugins/loss_measure.h"
#include "plugins/loss_burst_measure.h"
#include "plugins/forecaster_measure.h"
#include "plugins/capprobe_measure.h"
#include "plugins/clockdrift_measure.h"
#include "plugins/corrected_delay_measure.h"
#include "plugins/counter_measure.h"
#include "plugins/byte_measure.h"
#include "plugins/generic_measure.h"
#include "plugins/bulktransfer_measure.h"
//End of plugins

int MeasureManager::loadMeasurePlugins() {
	/* Load all measures. You need to add new measures here *
	 */
	loadMeasurePlugin(new ExampleMeasurePlugin);
	loadMeasurePlugin(new HopcountMeasurePlugin);
	loadMeasurePlugin(new RttMeasurePlugin);
	loadMeasurePlugin(new SeqWinMeasurePlugin);
	loadMeasurePlugin(new LossMeasurePlugin);
	loadMeasurePlugin(new LossBurstMeasurePlugin);
	loadMeasurePlugin(new ForecasterMeasurePlugin);
	loadMeasurePlugin(new CapprobeMeasurePlugin);
	loadMeasurePlugin(new ClockdriftMeasurePlugin);
	loadMeasurePlugin(new CorrecteddelayMeasurePlugin);
	loadMeasurePlugin(new ByteMeasurePlugin);
	loadMeasurePlugin(new CounterMeasurePlugin);
	loadMeasurePlugin(new GenericMeasurePlugin);
	loadMeasurePlugin(new BulktransferMeasurePlugin);
	return EOK;
};

int MeasureManager::monSetPeerName(char *pn) {
	int s;	
	if(pn == NULL)
		return -EINVAL;

	if(*peer_name != NULL)
		delete [] *peer_name;

	s = strlen(pn)+1; //include '\0'
	*peer_name = new char[s];
	memcpy(*peer_name, pn, s);
};
