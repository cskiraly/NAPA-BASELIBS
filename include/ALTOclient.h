#ifndef ALTOCLIENT_H
#define ALTOCLIENT_H

/**
* @file ALTOclient.h
*
* @brief Public functions to interface with ALTO client.

These functions allow the user to set up and handle the communications between ALTO client and ALTO server. Moreover, all needed functionalities to inteface ALTO client with other application modules are provided.
*
*
*/

#include <arpa/inet.h>

/*
 * Uncomment the following line to use the local reply.xml file 
 * instead of a real ALTO server query.
 */
/* #define USE_LOCAL_REPLY_XML */


/**
Operators relative preference.

These defines are the definitions of the RATING criteria.
In the PRIMARY rating criteria only ONE option can be choosen
For the SECOND rating criteria a mixture of these options is possible 
(e.g. 6 means TOP_DIST and MIN_BOUN).
*/
#define REL_PREF	1
/**
Topological distance (Number of AS hops).
*/
#define TOP_DIST	2
/**
Minimum boundary for latency.
*/
#define MIN_BOUN	4

/**
 * @brief A single element for the internal interface.
 *
 * This is the struct of one element for the internal interface. Make lists out of it to interact with the client.
 */
typedef struct alto_guidance_t{
	/**
	 * @brief The address of the host.
	 */
	struct in_addr alto_host;
	/**
	 * @brief The prefix associated to the host.
	 */
	int prefix;

/**
 * @brief The rating associated to the host.
 */
	int rating;
}ALTO_GUIDANCE_T;


// external API functions

/**
 * @brief Start the ALTO client.
 *
 * 	It starts the ALTO client, initialize the internal DB and provide the API
 * 	for upcoming requests.
 */
void start_ALTO_client();


/**
 * @brief Stop the ALTO client.
 *
 * 	It stops the ALTO client and clean-up the internal DB.
 */
void stop_ALTO_client();

/**
 * @brief Update the internal state.
 *
 * 	This function starts the synchronization of the internal peer list
 *  with the ALTO server.
 *  This function can be called after every dedicated request to the
 *  client or periodically triggered from extern to keep the peer-list
 *  up2date.
 *
 * 	@param	rc_host		The host IP from the client to which the Server
 * 						should do the rating
 * 	@param	pri_rat		Set the primary rating criteria (only 1,2 & 4
 * 						allowed)
 * 	@param	sec_rat		Set the secondary rating criteria (0-7 possible to
 * 						allow combined secondray rating criteria)
 */
void do_ALTO_update(struct in_addr rc_host, int pri_rat, int sec_rat);

/**
 * 	@brief Set the URI for the ALTO server.
 *
 * 	After calling the function, all upcoming requests are sent to the given
 * 	URI from this call.
 * 	Be careful, it is not tested if the ALTO server can be really reached
 * 	here, so double check before.
 *
 * 	@param	string	The URI of the ALTO server as string
 */
int set_ALTO_server(char * string);


/**
 * 	@brief Get the actual URI of the ALTO server.
 *
 * 	To assure that the settings for the upcoming ALTOrequests are right, use
 * 	this function to assure the the actual given ALTO server is the right one.
 * 	@return	Pointer to a string where the URI of the ALTO server is stored.
 */
char *get_ALTO_server(void);

/**
 *	@brief Rate a list of hosts given in a text file.
 *	@brief
 *	With this function a given list of hosts in a txt file will be ALTOrated
 *	This function will read from a text file all given hosts, transform them into
 *	the internal ALTO structure and get the alto rating for them. Afterwards it
 *	will write teh ALTO results in a new text file which will be the same name,
 *	but with the ending .out.
 *
 *	This function is used for functionality tests, it is not recommended
 *	to use it in a real ALTOclient implementation.
 *
 * 	@param	rc_host		The host IP from the client to which the Server
 * 						should do the rating
 * 	@param	pri_rat		Set the primary rating criteria (only 1,2 & 4
 * 						allowed)
 * 	@param	sec_rat		Set the secondary rating criteria (0-7 possible to
 * 						allow combined secondray rating criteria)
 * 	@return				1 = SUCCESS / 0 = ERROR
 */
int get_ALTO_guidance_for_txt(char * txt, struct in_addr rc_host, int pri_rat, int sec_rat);

/**
 *	@brief Rate a list of hosts.
 *	@brief
 *	With this function a given list of hosts in the ALTOstruct (alto_guidance_t) gets
 *	ALTO rated.
 *	This function will use the given struct, transform it into the internal ALTO structure
 *	and get the alto rating for them. Afterwards it will return the struct where the ALTO
 *	rating is inserted in the int.rating of this struct.
 *
 * 	@param	list		a pointer to the alto_guidance_t list
 * 	@param	num			the number of elements in the list which should be rated
 * 	@param	rc_host		The host IP from the client to which the Server
 * 						should do the rating
 * 	@param	pri_rat		Set the primary rating criteria (only 1,2 & 4
 * 						allowed)
 * 	@param	sec_rat		Set the secondary rating criteria (0-7 possible to
 * 						allow combined secondray rating criteria)
 * 	@return				1 = SUCCESS / 0 = ERROR
 */
int get_ALTO_guidance_for_list(ALTO_GUIDANCE_T * list, int num, struct in_addr rc_host, int pri_rat, int sec_rat);



#endif /* ALTOCLIENT_H */
