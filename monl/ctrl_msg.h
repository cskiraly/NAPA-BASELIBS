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
#ifndef _CTRL_MSG_H_
#define _CTRL_MSG_H_

#include "mon.h"

//TODO: check sizeof with structure ... probably not working properly
#pragma pack(push)  /* push current alignment to stack */
#pragma pack(1)     /* set alignment to 1 byte boundary */

// Header
struct MonHeader {
	uint16_t length;
	uint16_t type;
};
#define MONHDR_SIZE (sizeof(struct MonHeader))

//MeasureResposne
struct MeasureResponse {
	MonHandler 	mh_remote;
	MonHandler 	mh_local;
	uint32_t		command;
	uint32_t		status;
};
#define MRESPONSE_SIZE (sizeof(struct MeasureResponse))

struct InitRemoteMeasure {
	MeasurementId 					mid;
	MeasurementCapabilities		mc;
	MonHandler						mh_local;
	MsgType							msg_type;
	uint16_t							n_params;
};
#define INITREMOTEMEASURE_SIZE (sizeof(struct InitRemoteMeasure))

struct DeInitRemoteMeasure {
	MonHandler						mh_local;
	MonHandler						mh_remote;
};
#define DEINITREMOTEMEASURE_SIZE (sizeof(struct DeInitRemoteMeasure))

struct RemoteResults {
	uint32_t							length;
	MsgType							msg_type;
};
#define REMOTERESULTS_SIZE (sizeof(struct RemoteResults))

struct OobData {
	MonHandler						mh_local;
	MonHandler						mh_remote;
};
#define OOBDATA_SIZE (sizeof(struct OobData))

#pragma pack(pop)   /* restore original alignment from stack */

#endif
