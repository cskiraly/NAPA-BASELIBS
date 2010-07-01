#ifndef _SOM_H

/** @file som.h
 *
 * Scheduler & Overlay Manager API.
 *
 * The Scheduler and Overlay Manager module controls chunk exchange with peers. In order to provide flexibility
 * for various scheduling and overlay management algorithms, the SOM module is further subdivided into the 
 * internal functional entities shown below:
 *
 * APIs for the internal functional entities are defined in the following .h files:
	- <b>Chunk buffer</b> - chb.h
	- <b>NeighborList</b> - neighborlist.h
	- <b>Trading Logic</b> - trading.h
	- <b>Topology Controller</b> - topo.h

 * @image html SOM_Internals.png "Internals of the SOM module" width=50
 */

#include	"pulse.h"
#include	"trading.h"

/**
  Initialise and start up this SOM instance.

  Needs to be called when joining a channel.

  @param[in] cfg_som Structure holding the parsed MonL section of the configuration file. @todo make it more flexible
  @return handle to the SOM instance.
*/
HANDLE start_som(cfg_opt_t *cfg_som);

/**
  Stop the SOM instance and de-allocate resources.
  @see start_som

  @param[in] h handle to the instance.
*/
void stop_som(HANDLE h);

/**
  Retrieves the ChunkBuffer handle associated with this SOM instance.

  @param[in] h handle to the SOM instance.
  @return handle to the ChunkBuffer or NULL on error
*/
HANDLE get_chunkbuffer(HANDLE h);

/** 
  Retrieves the NeighborList handle associated with this SOM instance.
  @see neighborlist.h

  @param[in] h handle to the SOM instance.
  @return handle to the InfoBase or NULL on error
*/
HANDLE get_neighborlist(HANDLE h);

/**
  Retrieves the Topology Controller handle associated with this SOM instance.
  @see topo.h

  @param[in] h handle to the SOM instance.
  @return handle to the TopologyController or NULL on error
*/
HANDLE get_topologycontroller(HANDLE h);

/**
  Retrieves the BufferMapExchangePlugin associated with this SOM instance.

  @param[in] h handle to the SOM instance.
  @return reference to the BufferMapExchangePlugin or NULL on error.
*/
BufferMapExchangePlugin *get_bufferexchangeprotocol(HANDLE h);

/**
  Retrieves the ChunkPeerSelectionPlugin associated with this SOM instance.

  @param[in] h handle to the SOM instance.
  @param[in] d direction: should be either SEND or RECEIVE (i.e. requesting)
  @return reference to the ChunkPeerSelectionPlugin or NULL on error.
*/
ChunkPeerSelectionPlugin *get_chunkpeerselectionprotocol(HANDLE h, Direction d);

#define _SOM_H
