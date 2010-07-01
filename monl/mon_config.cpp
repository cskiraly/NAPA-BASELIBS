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

#include <mon.h>
#include "measure_manager.h"
#include	<confuse.h>
#include <string.h>


cfg_opt_t cfg_ml[] = {
	CFG_STR("channel", NULL, CFGF_NONE),
	CFG_END()
};

void MeasureManager::monParseConfig(void *monl_config) {
#if 0
	cfg_t *monl_c = monl_config;
	if(!monl_c)
		return;

	/* defualt channel */
	char *channel = cfg_getstr(monl_c, "channel");
	default_channel = new char[strlen(channel)+1];
	memcpy(default_channel, channel, strlen(channel)+1);
#endif
}
