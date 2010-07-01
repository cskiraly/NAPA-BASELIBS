#ifndef _NEIGHBORLIST_H
#define _NEIGHBORLIST_H

/** @file neighborlist.h
 *
 * @brief Neighbor List API.
 *
 * The NeighborList maintains a repository-based list of PeerIDs. These PeerIDs are periodically updated via
 * repository queries. Tuning of these queries is done via the NeighborList manager's API.
 * The list of PeerIDs provided by the NeighborList might be used to initialize one Peer's trading neighborhood,
 * or to extend it with PeerIDs obtained from Repositories (i.e. not via gossiping or other non-centralized manner).
 * Several intances of NeighborList might be running at the same time in order to facilitate different
 * Active Peer data for different channels. 
 * Typically one NeighborList is created for each channel.
 *
 * A background NeighborList process takes care of communicating the Repositories, and updating the list.
 * 
 */

#include	"grapes.h"

/** A NeighborListEntry holds information for Peers in the NeighborList */
typedef struct {
	/** ID of the Peer */
	char peer[64];
	/** last time this entry was updated */
	struct timeval update_timestamp;
} NeighborListEntry;

/** Notifier callback for informing that the list has changed 

  @param h Handle to the NeighborList instance generating the notification.
  @param cbarg arbitrary user-provided parameter for the callback
*/
typedef void (*cb_NeighborListChange)(HANDLE rep, void *cbarg);

/**
  Initialize the NeighborList instance.

  @param[in] som a handle to repoclient instance to be used.
  @param[in] desired_size desired size of the NeighborList
  @param[in] update_freq how often (in seconds) should the list updated
  @param[in] channel channel filter (NULL for any channel)
  @param[in] notifier function to be called if the list changes (may be NULL)
  @param[in] notifier_arg cbarg to be passed on notifier call

  @returns a Handle to this instance.
*/
HANDLE neighborlist_init(HANDLE rep, int desired_size, int update_freq, const char *channel, cb_NeighborListChange notifier, void *notifier_arg);

/**
  Close this NeighborList instance and deallocate all associated resources.

  @param h Handle to the NeighborList instance to be closed.
*/
void neighborlist_close(HANDLE h);

/**
  Query a list of peers from the NeighborList.

  The function returns a COPY of the current NeighborList.
  @param[in] h Handle to the NeighborList instance.
  @param[in] result a sufficiently sized array to hold the results.
  @param n [in] return at most n records [out] the number of records returned.
  @return 0 on success, <0 on error
*/
int neighborlist_query(HANDLE h, NeighborListEntry *result, int *n);

/**
  Count the number of entries in the NeighborList.

  @param[in] h Handle to the NeighborList instance.
  @return number of NeighborList Entries the instance has.
*/
int neighborlist_count(HANDLE h);

/**
  Get the record for a given peer.

  @param[in] h Handle to the NeighborList instance.
  @param[in] peer PeerID of the peer to be queried.
  @param[out] result the corresponding NeighborListEntry is copied into the memory buffer provided.
  @return 1 if the record was found, 0 if not.
*/
int neighborlist_get(HANDLE h, char *peer, NeighborListEntry *result);

#endif
