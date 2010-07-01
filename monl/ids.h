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

#ifndef _IDS_H_
#define _IDS_H_

/* Measures IDs */
/* Add new Measure Ids here */

enum eMeasureId {
	EXAMPLE=0,
	HOPCOUNT,
	BYTE,
	COUNTER,
	BULK_TRANSFER,
	RTT,
	SEQWIN,
	LOSS,
	LOSS_BURST,
	CLOCKDRIFT,
	CORRECTED_DELAY,
	CAPACITY_CAPPROBE,
	AVAILABLE_BW_FORECASTER,
	GENERIC,
	LAST_ID //do not remove, must be last
};

/* Result IDs */
/* Add new Result Ids here */
enum eResultIdBoth {
	R_SEQNUM = 0,
	R_SIZE,
	R_SEND_TIME,
	R_REPLY_TIME,
	R_RECEIVE_TIME,
	R_RTT,
	R_SEQWIN,
	R_LOSS,
	R_LOSS_BURST,
	R_CLOCKDRIFT,
	R_CORRECTED_DELAY,
	R_CAPACITY_CAPPROBE,
	R_THROUGHPUT,
	R_AVAILABLE_BW_FORECASTER,
	R_LAST //do not remove, must be last
};


enum eResultIdPkt { /* Result availbale only at PKT level */
	R_INITIAL_PKT = R_LAST - 1, //do not remove
	R_INITIAL_TTL,
	R_TTL,
	R_HOPCOUNT,
	R_DATA_ID,
	R_DATA_OFFSET,
	R_LAST_PKT //do not remove, must be last
};

enum eResultIdData { /* Result availbale only at DATA level */
	R_INITIAL_DATA = R_LAST - 1, //do not remove
	R_LAST_DATA //do not remove, must be last
};

/* Control Messages IDS */
enum eControlId {
	REMOTEMEASURERESPONSE=0,
	INITREMOTEMEASURE,
	DEINITREMOTEMEASURE,
	OOBDATA,
	REMOTERESULTS
};

/* Statistical types */
enum stat_types {
	LAST = 0,
	AVG,
	WIN_AVG,
	VAR,
	WIN_VAR,
	MIN,
	WIN_MIN,
	MAX,
	WIN_MAX,
	SUM,
	WIN_SUM,
	RATE,
	LAST_STAT_TYPE
};

/* Default parameter ids */
enum eDefaultParam {
	P_WINDOW_SIZE = 0,
	P_PUBLISHING_PKT_TIME_BASED,
	P_PUBLISHING_RATE,
	P_PUBLISHING_TIME_SPREAD,
	P_PUBLISHING_NEW_ALL,
	P_INIT_NAN_ZERO,
	P_DEBUG_FILE,
	P_OOB_FREQUENCY,
	P_LAST_DEFAULT_PARAM
};

/* 
 * Plugin Parameters 
 */
/* SeqWin Plugin */
#define P_SEQN_WIN_SIZE P_LAST_DEFAULT_PARAM
#define P_SEQN_WIN_OVERFLOW_TH P_LAST_DEFAULT_PARAM+1
/* end Seqwin */

/* ClockDrift Plugin */
#define P_CLOCKDRIFT_ALGORITHM P_LAST_DEFAULT_PARAM
#define P_CLOCKDRIFT_PKT_TH P_LAST_DEFAULT_PARAM+1
#define P_CLOCKDRIFT_WIN_SIZE P_LAST_DEFAULT_PARAM+2
/* end ClockDrift */

/* Forecaster */
#define P_CAPPROBE_RATE P_LAST_DEFAULT_PARAM
#define P_CAPPROBE_PKT_TH P_LAST_DEFAULT_PARAM+1
#define P_CAPPROBE_DELAY_TH P_LAST_DEFAULT_PARAM+2
#define P_CAPPROBE_IPD_TH P_LAST_DEFAULT_PARAM+3
#define P_CAPPROBE_HEADER_SIZE P_LAST_DEFAULT_PARAM+4
#define P_CAPPROBE_PAYLOAD_SIZE P_LAST_DEFAULT_PARAM+5
#define P_CAPPROBE_FILTER P_LAST_DEFAULT_PARAM+6
#define P_CAPPROBE_NUM_INTERVALS P_LAST_DEFAULT_PARAM+7
/* end forecaster */

/* Forecaster */
#define P_FORECASTER_RATE P_LAST_DEFAULT_PARAM
#define P_FORECASTER_PKT_TH P_LAST_DEFAULT_PARAM+1
#define P_FORECASTER_DELAY_TH P_LAST_DEFAULT_PARAM+2
#define P_FORECASTER_PAYLOAD_SIZE P_LAST_DEFAULT_PARAM+3
#define P_FORECASTER_RELATIVE P_LAST_DEFAULT_PARAM+4
/* end forecaster */

#endif /* _IDS_HH_ */
