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

#ifndef _MON_H
#define _MON_H

/** @file mon.h
 *
 * <b>Definitions for the Monitoring Layer (MonL) API v0.9</b>
 *
 * <b>Architecture</b>: The Monitoring Box is a management layer which governs the execution of 
 * <i>Monitoring Plugins</i>.
 * Each plugin implements one specific measurement type. Plugins are either scheduled on hook events
 * (in-band measurements) or based on time (out-of-band measurements). Scheduling and triggering (both event and
 * periodic ones) of plugins is done by the Monitoring Layer, and the plugins MUST NOT rely on any external 
 * scheduler service in order to protect control flow.
 * The MonL is responsible for the registration and setting up OS and framework-dependent timing and triggering
 * services (i.e. communication callback hooks and timeouts).
 *
 * The typical sequence to start a measurement should be:
 * -# (optional) <i>monListMeasures()</i> to get a list of available measurements and their capabilities
 * -# <i>mh = monCreateMeasure(t,c)</i> to create an instance of a measurement of type <i>t</i> and capabilities <i>c</i>. A <i>handler</i> is returned, which can be used to interact with this instance.
 *   -# (optional) get list of parameters using <i>monListParameters()</i>
 *   -# (optional) check current values of parameters with <i>monGetParameter()</i>
 *   -# (optional) adjust values of parameters with <i>monSetParameter()</i>
 * -# Schedule the measurement instance for execution specifying which messages should be analysed 
 * (remote peer and message type) using <i>monActivateMeasure()</i>
 * -# (optional) check status of measurement instance with <i>monGetStatus()</i>
 *
 * The typical sequence to stop a measurement should be:
 * -# (optional) remove an instance from the execution list using <i>monDeactivateMeasure()</i>
 * -# free resources using the <i>monDestroyMeasure()</i>
 *
 * The usage of separate instances for separate peer pairs/message types allows maximum flexibility.
 *
 * <b>Requirements:</b>
 * This specification assumes that the data transfer (Messaging) layer, used in the P2P application has the
 * notion of PeerID for identifying nodes (peers). The data transfer layer must support a datagram-like service
 * (modeled after the UDP protocol), and must provide certain <i>callback</i> notifications (detailed below)
 *  upon datagram reception/sending.
 * The Messaging Layer must also support the notion of MessageTypes and subscription services for messages of the
 * specified types.
 *
 * Although these requirements may seem to be restrictive, we must note that such functionality can usually be
 * easily implemented as a lighweight "wrapper" functionality around existing datagram transfer libraries.
 *
 * <b>Planned extensions:</b>
 *
 * A version of mon_activate_measure with arrays of NodeID/MsgType could be envisaged (or add/remove peer functions).
 *
 * @author Robert Birke
 *
 * @date  12/28/2009
 */

#include		<stdint.h>
#include		<stdlib.h>
#include		<sys/time.h>

#include "../monl/ids.h"
#include "msg_types.h"

//TODO: not very clean ....
#ifndef _socket_ID
struct _socket_ID;
#endif

typedef struct _socket_ID *SocketId;

typedef uint8_t MsgType;

#ifndef MeasurementName
/** Default name for a measurement */
typedef char *MeasurementName;
#endif

#ifndef MeasurementId
/** Identifier (numerical) for a measurement */
typedef int32_t MeasurementId;
#endif

#ifndef MonHandler
/** Identifier for a measurement */
typedef int32_t MonHandler;
#endif

/** Type for numerical results */
typedef double result;


/** A MeasurementRecord holds measurement values computed by Monitoring plugins (defined in mon.h).
  MeasurementRecords are published to the Repository. */
typedef struct {
	/** The originator (source) of this record 
	(e.g. "ALTOxyz" or a PeerID/SocketId converted to string) */
	char *originator;
	/** "Primary" target PeerID/SocketId  */
	char *targetA;
	/** "Secondary" target PeerID/SocketId */
	char *targetB;
	/** The corresponding MeasurementName. */
	const char* published_name;
	/** Measurement numerical value. NAN if not valid */
	double value;
	/** Measurement string value. "" if not valid */
	char *string_value;
	/** Timestamp of the measurement result */
	struct timeval timestamp;
	/** Name of channel (if relevant) */
	const char *channel;
} MeasurementRecord;


#ifndef MeasurementCapabilities
/** Capabilities  implemented by plugin */
typedef int32_t MeasurementCapabilities;

/* Codes for plugin capabilities */

/** Used to advertise a In-Band measurement (mutually exclusive with OUT_OF_BAND)*/
#define IN_BAND 		1
/** Used to advertise a Out-Of-Band measurement (mutually exclusive with IN_BAND)*/
#define OUT_OF_BAND 	2
/** Used to advertise a Timer-based measurements (mutually exclusive with above two)*/
#define TIMER_BASED 	4

/** Used to advertise a packet level measurement */
#define PACKET 	8
/** Used to advertise a chunk level measurement */
#define DATA 		16


/** The following defines are for internal use only */
/** Used to advertise a local TX based level measurement (internal use only)*/
#define TXLOC		32
/** Used to advertise a local RX based level measurement (internal use only)*/
#define RXLOC		64
/** Used to advertise a remote TX based level measurement (internal use only)*/
#define TXREM		128
/** Used to advertise a remote RX based level measurement (internal use only)*/
#define RXREM		256
/** Defines if this is the local (0) or remote (1) part of a measurement (internal use only)*/
#define REMOTE		512
/** Defines if the measure should provide also remote measurements locally (does not work with all)*/
#define REMOTE_RESULTS	1024


/** Used to advertise a TX based measurement (mutually exclusive with RXONLY, TXRXUNI and TXRXBI) */
#define TXONLY 	TXLOC
/** Used to advertise a RX based measurement (mutually exclusive with TXONLY, TXRXUNI and TXRXBI) */
#define RXONLY		RXLOC
/** Used to advertise a collaborative measurement unidirectional (mutually exclusive with RXONLY, TXONLY and TXRXBI) */
#define TXRXUNI	(TXLOC | RXREM)
/** Used to advertise a collaborative measurement bidirectional (mutually exclusive with RXONLY, TXONLY and TXRXUNI) */
#define TXRXBI		(TXLOC | RXREM | TXREM | RXLOC)


#endif


#pragma pack(push)  /* push current alignment to stack */
#pragma pack(1)     /* set alignment to 1 byte boundary */
/** The monitoring layer packet header */
struct MonPacketHeader {
	uint32_t seq_num;
	uint32_t ts_sec;
	uint32_t ts_usec;
	uint32_t ans_ts_sec;
	uint32_t ans_ts_usec;
	uint8_t initial_ttl;
/** Fields TBD */
};
#pragma pack(pop)   /* restore original alignment from stack */
/** Constant indicating the size of the monitoring layer header for packets */
#define MON_PACKET_HEADER_SIZE 21


#pragma pack(push)  /* push current alignment to stack */
#pragma pack(1)     /* set alignment to 1 byte boundary */
/** The monitoring layer data header */
struct MonDataHeader {
	int32_t seq_num;
	uint32_t ts_sec;
	uint32_t ts_usec;
	uint32_t ans_ts_sec;
	uint32_t ans_ts_usec;
/** Fields TBD */
};
#pragma pack(pop)   /* restore original alignment from stack */
/** Constant indicating the size of the monitoring layer header for data */
#define MON_DATA_HEADER_SIZE 20



#ifndef MonParameterType
/** Parameter type  */
typedef int32_t MonParameterType;
#endif

#ifndef MonParameterValue
/** Type for measurement Paramters */
typedef double MonParameterValue;
#endif


/** Structure describing a measurement parameter */
struct MeasurementParameter {
	/** Parameter handler for the parameter (valid for a specific measurment instance only) */
	MonParameterType ph;

	/** Textual name of the parameter */
	char *name;

	/** Textual parameter description  */
	char *desc;

	/** Default value */
	MonParameterValue dval;
};

/** Structure describing a measurement */
struct MeasurementPlugin {
	/** MeasurementName the plugin implements */
	MeasurementName name;

	/** The numerical Id of measurement */
   MeasurementId id;

	/** MeasurementDescription of the measurement the plugin implements */
	char *desc;

	/** Array of possible dependencies of this plugin (numerical ids)*/
   MeasurementId* deps_id;

	/** The length of the deps array */
	int deps_length;

	/** Array of parameters of this plugin */
	struct MeasurementParameter* params;

	/** Number of possible dependencies of this plugin (length of array)*/
	int params_length;

	/** Bitmask of available capabilities for this plugin.
	(i.e. the measurement can be performed on packets or on chunks or both.)
	*/
	MeasurementCapabilities caps;
};

/** Short status code for a measurement plugin */

typedef enum {
UNKNOWN,
/* Stopped: created but not running */
STOPPED,
/* all is ok and we are collecting samples */
RUNNING,
/* a local error occured while activating the measure */
FAILED,
/* a remote error occured while activating the measure */
RFAILED,
/* you called monActivateMeasure with wrong paramters */
MS_ERROR,
/* the measure is beeing set up */
INITIALISING,
/* the measure is beeing stopped */
STOPPING
} MeasurementStatus;


#ifdef __cplusplus
extern "C" {
#endif


/**
   List of available measurements (registered plugins)

  This function returns an array of MeasurementPlugin each one being a structure describing an available measurement.
  The parameter is used to pass the length of the array.

   @param[out] length pointer to an int to be filled with the array length
   @return Array of struct MeasurementPlugin
*/
struct MeasurementPlugin *monListMeasures (int *length);

/**
   Creates a new instance of a measurement by MeasurementId

	This creates a measurement instance using the measurement id (faster)
   @param[in] id the numerical identifier of the measurement to be created
   @param[in] mc the capabilities which this specific instance of the measure should implement
	(must be a subset of the capabilities advertised by the plugin in struct MeasurementPlugin)
	Must respect some mutual exclusion rules: IN_BAND and OUT_OF_BAND are mutually exclusive,
	TXONLY, RXONLY and TXRX are mutually exclusive.
   @return the handler of the newly created measurement instance (negative value indicates failure)
   @see monCreateMeasureName
*/
MonHandler monCreateMeasure (MeasurementId id, MeasurementCapabilities mc);

/**
   Creates a new instance of a measurement by MeasurementName

	This creates a measurement instance using the measurement name
   @param[in] name the textual name of the measurement to be created
   @param[in] mc the capabilities which this specific instance of the measure should implement
	(must be a subset of the capabilities advertised by the plugin in struct MeasurementPlugin)
	Must respect some mutual exclusion rules: IN_BAND and OUT_OF_BAND are mutually exclusive,
	TXONLY, RXONLY and TXRX are mutually exclusive.
   @return the handler of the newly created measurement instance (negative value indicates failure)
   @see monCreateMeasure
*/
MonHandler monCreateMeasureName (const MeasurementName name, MeasurementCapabilities mc);

/**
   Frees a measurement instance created by mon_create_measure()

   If the instance is currently in use it will be deactivated before being freed.

   @param[in] mh the handler of the measurement to be freed
	@return 0 on success, negative value on error
*/
int monDestroyMeasure (MonHandler mh);

/**
   Lists all parameters of a measurement (by id)

   @param[in] mid Id of the measurement
   @param[out] length pointer to an int to be filled with the array length
   @return Array of struct MeasurementParameter or NULL on error
*/
struct MeasurementParameter *monListParametersById (MeasurementId mid, int *length);


/**
   Lists all parameters of a measurement (by handler)

   @param[in] mh handler of the measurement instance
   @param[out] length pointer to an int to be filled with the array length
   @return Array of struct MeasurementParameter or NULL on error
*/
struct MeasurementParameter *monListParametersByHandler (MonHandler mh, int *length);

/**
   Set a new value for a parameter of a measurement

   @param[in] mh handler of the measurement instance
   @param[in] ph handler of the parameter
   @param[in] p new value
   @return 0 on success, negative value on failure (ie. the value is not within the admissible range)
*/
int monSetParameter (MonHandler mh, MonParameterType ph, MonParameterValue p);

/**
   Get current value of a parameter of a measurement

   @param[in] mh handler of the measurement instance
   @param[in] ph handler of the parameter
   @return current value (NAN on error)
*/
MonParameterValue monGetParameter (MonHandler mh, MonParameterType ph);

/**
   Activate measure

   Used to start a specific instance of a measurement. It puts the instance on the correct execution list of the monitoring layer.
   If necesasry,  the remote party will also be set up using control messages of the monitoring layer.
   The function is always non-blocking and the information of the measurement status can be gathered calling the monGetStatus function.

   @param[in] mh handler of the measurement instance
   @param[in] dst an identifier for the Id of the remote peer
   @param[in] mt the message type filter if set to MSGTYPE_ANY, accept all (only considered for in_band)
   @return 0 on success, negative value on failure
*/

int monActivateMeasure (MonHandler mh, SocketId dst, MsgType mt);

/**
   Publish results

   Used to tell the monitoring layer to publish the results into the repositry. By defualt (As of now) nothing is pubblished. The function needs a working rep client HANDLER to use.

   @param[in] mh handler of the measurement instance
   @param[in] publishing_name the name used to publish the results if NULL the default name from the measurment plugin is used
   @param[in] mh channel name to use
   @param[in] st an array of statistal types to be pubblished (i.e. average value) for each one of these is published adding a suffix to the publishing_name. both are defined in monl/ids.h
	@param[in] length the length of the st array. If 0 nothing is pubblished.
	@param[in] repo_client a valid repoclient HANLDE or NULL to use the default one specifed in MonInit()
   @return 0 on success, negative value on failure
*/

int monPublishStatisticalType(MonHandler mh, const char *publishing_name, const char *channel, enum stat_types st[], int length, void  *repo_client);

/**
  Retrieve results

   Used to retreive a result shortcuting the repository.

   @param[in] mh handler of the measurement instance
   @param[in] st statistal type to be retrieved (i.e. average value) enum defined in monl/ids.h
   @return the value on success, NAN on failure
*/

result monRetrieveResult(MonHandler mh, enum stat_types st);

/**
  Retrieve results

   As above but used to retreive a result usig measurement description instead of handler

   @param[in] src socketId: it is always the address of the other peer involved
   @param[in] mt the message type
   @param[in] flags measurmente falgs used while creating the measure
   @param[in] mid measurmente type id
   @param[in] st statistal type to be retrieved (i.e. average value) enum defined in monl/ids.h
   @return the value on success, NAN on failure
*/

result monRetrieveResultById(SocketId src, MsgType mt, MeasurementCapabilities flags, MeasurementId mid, enum stat_types st);

/**
  Save arbitrary samples

   Used to supply smaples to the generic module

   @param[in] mh handler of the measurement instance
   @return 0 on success, != 0 on failure
*/
int monNewSample(MonHandler mh, result r);

/**
  Set peer's publish name

   This name will be used as originator in all subsequent
   publish operations.

   @param[in] name the name
   @return 0 on success, != 0 on failure
*/
int monSetPeerName(const char *name);

/**
   Deactivate measure

   Removes an instance from the excution lists and therefore stops any measurements performed by this
   instance. The instance can be reused putting it on a different execution list calling the
   monActivateMeasure() function.

   @param[in] mh handler of the measurement instance
   @return 0 on success, negative value on failure
*/
int monDeactivateMeasure (MonHandler mh);


/**
  Get the status code for a measurement instance

  @param[in] mh measurement instance handler
  @return status code
*/
MeasurementStatus monGetStatus(MonHandler mh);

/** Config file parsing instructions for mon */
// Not yet used
//extern cfg_opt_t MONConfig[];

/**
  Initialize the Monitoring Layer.

  Needs to be called to bootstrap the Monitoring Layer.

  @param[in] event_base pointer to the event base of libevent
  @param[in] repo_client pointer to a default repository client to use to publish or NULL
  @param[in] cfg_mon Structure holding the parsed MonL section of the configuration file (not used as of now)
  @return 0 on success, negative value on failure
*/
/* as of now it not uses the config file yet */
//int monInit(cfg_opt_t *cfg_mon);
int monInit(void *event_base, void *repo_client);
#ifdef __cplusplus
}
#endif

#endif
