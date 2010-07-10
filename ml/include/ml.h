#ifndef ML_H
#define ML_H

/**
 * @file ml.h
 * @brief NAPA-WINE messaging layer API.
 * 
 * The messaging layer is a network layer abstraction for p2p systems, that has the primitives:
 *       - connect to a peer
 *       - send arbitrary size data to a peer
 *
 *  Furthermore, it provides the functionalites:
 *       - PMTU discovery
 *       - NAT traversal
 *       - data fragmentation and reassembly
 *
 * The messaging layer (ML) abstracts the access to transport 
 * (UDP/TCP) protocols and the corresponding service access points 
 * offered by the operating system by extending their capabilities 
 * and providing an overlay level addressing, i.e., assigning a 
 * unique identifier to each peer. 
 * 
 * For example, it provides the ability to send a chunk of
 * data to another peer, which has to be segmented and then
 * reassembled to fit into UDP segments. The messaging layer
 * also provides an interface to the monitoring module, which is
 * used for passive measurements: whenever a message is sent or
 * received an indication will be given to the monitoring module,
 * which then can update its statistics.
 * Another important feature of the messaging layer is mechanisms
 * for the traversal of NAT routers. Network Address Translation 
 * allows to attach several hosts to the Internet
 * using only one globally unique IP address. Therefore it is
 * very popular with private customers, who are also the main
 * target audience for P2P TV. However, the presence of a NAT
 * device may impede the ability to establish connections to other
 * peers, therefore, special NAT traversal functions are needed
 * in the messaging layer.
 *
 * @author Kristian Beckers  <beckers@nw.neclab.eu>
 * @author Sebastian Kiesel  <kiesel@nw.neclab.eu>  
 *
 * @date 7/28/2009
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/time.h>


/**
 * @brief The size of a socketID
 */
#define SOCKETID_SIZE 60

/**
 * @brief The size of a socketID in string representation
 */
#define SOCKETID_STRING_SIZE 44

/**
 * @brief A struct with direction information of a packet or data that is about to be send. This structure is used to providethe monitoring module with information about where a data/packet will be sent and what the messagetype is.
*/
struct _socket_ID;

/**
 * @brief Typedef that defines a socketID handle a pointer to a socket_ID struct
 */
typedef struct _socket_ID *socketID_handle;

/**
 * @brief Typedef for a callback when a new connection is established
 */
typedef void (*receive_connection_cb)(int connectionID, void *arg);

/**
 * @brief A struct with a couple of buffers and length pairs for the send_all function
 */
typedef struct {

  char *buffer_1; ///< A buffer pointer
  int length_1; ///< The length of the buffer
  char *buffer_2; ///< A buffer pointer
  int length_2; ///< The length of the buffer
  char *buffer_3; ///< A buffer pointer
  int length_3; ///< The length of the buffer
  char *buffer_4; ///< A buffer pointer
  int length_4; ///< The length of the buffer
  char *buffer_5; ///< A buffer pointer
  int length_5; ///< The length of the buffer

} send_all_data_container;


/**
 * @brief A struct with direction information of a packet or data that is about to be send.
 */
typedef struct {

  socketID_handle remote_socketID; ///< The remote socketID
  char msgtype; ///< The message type

} send_pkt_type_and_dst_inf;

/**
  * @brief A structure for parameters that is used in the mlSendData function. This structure contains options of how the sending should occur.
  */
typedef struct
{
  bool priority; ///<  A boolean variable that states if the data shall be send with priority.
  bool padding; ///<  A boolean that states if missing fragments shall be replaced with '0'
  bool confirmation; ///<   A boolean that causes a receive confirmation
  bool reliable; ///< A boolean that sends the data over a reliable protocol. Warning: If the messaging layer fails to establish a connection with a reliable protocol it falls back to UDP.
  int  keepalive; ///< Send KEEPALIVE messages over this connection at every n. seconds. Set to <= 0 to disable keepalive
} send_params;

/**
 * @brief A struct that is returned from the messaging layer with metadata about the received data.
 */
typedef struct {
  char msgtype; ///< The message type.
  int connectionID; ///< The connectionID.
  int nrMissingBytes; ///< The number of bytes that did not arrive.
  int recvFragments; ///< The overall number of fragments.
  char firstPacketArrived; ///< A boolean that states if the first fragment arrived.
  socketID_handle remote_socketID; ///< The remote socketID
} recv_params;

/**
 * @brief A struct that contains metadata about messaging layer data. It used to transfer metadata about arrived messaging layer data to the monitoring module.
 *
 */
typedef struct {

  socketID_handle remote_socketID; ///< The remote socketID
  char *buffer; ///< A pointer to the data that was received
  int bufSize; ///< The size of the data that was received
  char msgtype; ///< The message type
  char monitoringHeaderType; ///<  This value indicates if a monitoring header was added to the data. The header is added when the value is either 1 or 3.
  char *monitoringDataHeader; ///<  A pointer to the monitoring header.
  int monitoringDataHeaderLen;
  struct timeval arrival_time; ///< The arrival time of the data
  char firstPacketArrived; ///< A boolean that states if the first fragment arrived.
  int nrMissingBytes; ///<  The number of bytes that did not arrive.
  int recvFragments; ///< The overall number of fragments arrived
  bool priority; ///< A boolean variable that states if the data was send with priority.
  bool padding; ///< A boolean that states if missing fragments were replaced with '0'
  bool confirmation; ///<  A boolean that states if a receive confirmation was send
  bool reliable; ///< A boolean that the data was send  over a reliable protocol.

} mon_data_inf;

/**
 * @brief A struct of metadata about messaging layer packets. It is used to inform the monitoring module about arrived packets
 */
typedef struct {

  socketID_handle remote_socketID; ///< The remote socketID
	char *buffer; ///< A pointer to the data that was received
  int bufSize; ///< The size of the data that was received
  char msgtype; ///<  The message type
  int dataID; ///< The data ID field from the messaging layer header.
  int offset; ///< The data offset field from the messaging layer header.
  int datasize; ///< The datasize field from the messaging layer header.
  char *monitoringHeader; ///< A pointer to the monitoring header.
  int monitoringHeaderLen;
  int ttl; ///< The TTL Field from the IP Header of the packet.
  struct timeval arrival_time; ///< The arrival time of the packet.

} mon_pkt_inf;

/**
 * Typedef for a callback when a UDP packet is received.
 * The callback receives a pointer to a mon_pkt_inf struct as param
 * This shall be used by the monitoring module
*/
typedef void (*get_recv_pkt_inf_cb)(void *arg);

/**
 * Typedef for a callback when a packet is send
 * The param is a pointer to a mon_pkt_inf struct
 */
typedef void (*get_send_pkt_inf_cb)(void *arg);

/**
 * Typedef for a callback that asks if a monitoring module packet header shall be set
 * the first param is a pointer to the monitoring module packet header
 * the second param is a pointer to a send_pkt_type_and_dst_inf struct
 * return value returns 0 for no header and 1 for header
 */
typedef int (*set_monitoring_header_pkt_cb)(socketID_handle, uint8_t);

/**
 * Typedef for a callback that is envoked when data has been received
 * The param is a pointer to a mon_data_inf struct
 */
typedef void (*get_recv_data_inf_cb)(void *arg);

/**
 * Typedef for a callback when data has been send
 * The param is a pointer to a mon_data_inf struct
 */
typedef void (*get_send_data_inf_cb)(void *arg);

/**
 * Typedef for a callback that asks if a monitoring module header for data shall be set
 * the first param is a pointer to the monitoring module data header
 * the second param is a pointer to a send_pkt_type_and_dst_inf struct
 * return value returns 0 for no header and 1 for header
 */
typedef int (*set_monitoring_header_data_cb)(socketID_handle, uint8_t);

/**
 * Typedef for a callback when a socketID is ready to be used
 * SocketIDhandle is a handle for the local socketID
 * Errorstatus is set to -1 if an socket error occurred and the socket is not usable and 1 if the NAT traversal failed and 0 if no error occurred
 */
typedef void (*receive_localsocketID_cb)(socketID_handle local_socketID,int errorstatus);

/**
 * Typedef for a callback when a connection attempt failed
 */
typedef void (*connection_failed_cb)(int connectionID,void *arg);

/**
 * Typedef for a callback when data is received.
 * The first param is a pointer to a buffer that has the data
 * The second param is an int that holds the length of the data
 * The third param is the message type
 * The fourth param is a pointer to a recv_params struct
 */
typedef void (*receive_data_cb)(char *buffer,int buflen,unsigned char msgtype,recv_params *rparams);

#ifdef __cplusplus
extern "C" {
#endif

/**
 * This function is to register a callback that informs the upper layer that the local socketID is ready to use.        
 * @param local_socketID_cb  A function pointer to a callback function from the type receive_localsocketID_cb
 */
void register_recv_localsocketID_cb(receive_localsocketID_cb local_socketID_cb);


/**
 * @brief Initialize the Messaging Layer.
 * The init method needs to be called to initialize the messaging layer
 * @param recv_data_cb This is a boolean that decides how an upper layer receives data. When the boolean is set to true data is received via callback. When it is set to false the upper layer has to poll with the recv_data function.   
* @param timeout_value A timeout value for the data receival   
* @param port A port that will be used to create a new UDP socket    
* @param ipaddr An ipaddress that will be used to create a new UDP socket. If left NULL the messaging layer binds it to localhost.
* @param stun_port The port of the stun server.
* @param stun_ipaddr The ip address of the stun server.
* @param local_socketID_cb A callback from the type receive_localsocketID_cb that is envoke once the socketID is ready to use.
* @param arg A void pointer that is used in this implementation to handover the libevent pointer.
* @return The file descriptor used by the ML, or <0 on error
 */
int mlInit(bool recv_data_cb,struct timeval timeout_value,const int port,const char *ipaddr,const int stun_port,const char *stun_ipaddr,receive_localsocketID_cb local_socketID_cb,void *arg);

/**
  * Configure the parameters for output rate control.
  * These values may also be set while packets are being transmitted.
  * @param bucketsize The size of the bucket in kbytes
  * @param drainrate The amount of kbytes draining in a second. If drainrate is <=0, then rateControl is completely disabled (all packets are passed).
*/
void mlSetThrottle(int bucketsize, int drainrate);

/**
 * @brief Register a received packet callback.
 * This function is to register a callback that is invoked when a messaging layer packet is received.
 * @param recv_pkt_inf_cb A function pointer to a callback function from the type get_recv_pkt_inf_cb
 */
void mlRegisterGetRecvPktInf(get_recv_pkt_inf_cb  recv_pkt_inf_cb);

/**
 * @brief Register a send packet callback.
 * This function is to register a callback that is invoked when a messaging layer packet is send.
 * @param send_pkt_inf_cb A function pointer to a callback function from the type get_send_pkt_inf_cb
 */
void mlRegisterGetSendPktInf(get_send_pkt_inf_cb  send_pkt_inf_cb);

/**
 * @brief Register a monitoring packet callback.
 * This function is to register a callback that allows to add a monitoring module header to a packet.
 * @param monitoring_header_pkt_cb A function pointer to a callback function from the type set_monitoring_header_pkt_cb
 */
void mlRegisterSetMonitoringHeaderPktCb(set_monitoring_header_pkt_cb monitoring_header_pkt_cb);

/**
 * @brief Register a receive data callback.
 * Register a callback that is called when data is received  
 * @param recv_data_inf_cb  A function pointer to a callback function from the type get_recv_data_inf_cb
 */
void mlRegisterGetRecvDataInf(get_recv_data_inf_cb recv_data_inf_cb);

/**
 * @brief Register a send data callback.
 * Register a callback that is invoked when data is sent  
 * @param send_data_inf_cb  A function pointer to a callback function from the type get_send_data_inf_cb
 */
void mlRegisterGetSendDataInf(get_send_data_inf_cb  send_data_inf_cb);

/**
 * @brief Register a monitoring data callback.
 * Register a callback that allows the monitoring module to set a header on data  
 * @param monitoring_header_data_cb A function pointer to a callback function from the type set_monitoring_header_data_cb
 */
void mlRegisterSetMonitoringHeaderDataCb(set_monitoring_header_data_cb monitoring_header_data_cb);

/**
 * @brief Register a receive connection callback.
 * This function is to register a callback that informs the upper layer that a new connection is established.
 * @param recv_conn_cb  A function pointer to a callback function from the type receive_connection_cb
 */
void mlRegisterRecvConnectionCb(receive_connection_cb recv_conn_cb);

/**
 * @brief Register an error connection callback.
 * This function is to register a calllback that informs the upper layer that a connection attempt failed. 
 * @param conn_failed  A function pointer to a callback function from the type connection_failed_cb
 */
void mlRegisterErrorConnectionCb(connection_failed_cb conn_failed);

/**
 * @brief Register a receive data callback.
 * This function is to register callbacks for received messages.
 * At maximum 127 callbacks can be registered, one per message type.
 * When a message type is set twice with a different function pointer it will simply override the first.
 * @param data_cb A function pointer to a callback function from the type receive_data_cb
 * @param msgtype The message type: an integer in [0..126]
 */

void mlRegisterRecvDataCb(receive_data_cb data_cb,unsigned char msgtype);

/**
 * @brief Close a socket.
 * This function destroys a messaging layer socket.
 * @param socketID The socketID handle to the socket ID that shall be destroyed.
 */
void mlCloseSocket(socketID_handle socketID);


/**
 * @brief Open a connection.
 * This fuction opens a connection between two socketIDs.
 * @param external_socketID The remote socketID
 * @param *connectionID A pointer to an int. The connectionID will be stored in that int. 
 * @param connection_cb  A function pointer to a callback function from the type receive_connect\
ion_cb                                                                                           
 * @param arg A void pointer that is returned via the callback. 
 * @param default_send_params send_params to use if not specified at the send functions
 * @return returnValue the connection id and -1 for a failure
 */
int mlOpenConnection(socketID_handle external_socketID,receive_connection_cb connection_cb,void *arg, const send_params default_send_params);

/**
 * @brief Open a connection.
 * This function destroys a messaging layer connection.
 * @param connectionID 
 */
void mlCloseConnection(const int connectionID);

/**
 * @brief Send data from multiple buffers.
 * This function sends data. The data is provided as a list of buffer and length pairs.
 * @param connectionID The connection the data should be send to.
 * @param container A container for several buffer pointer and length pairs from type send_all_data_container/
 * @param nr_entries The number of buffer pointer and length pairs in the container. The maximum nr is 5.
 * @param msgtype The message type.
 * @param sParams A pointer to a send_params struct. If NULL, the default is used given at mlOpenConnection
 * @return 0 if a problem occured, 1 if everything was alright.
 */
int mlSendAllData(const int connectionID,send_all_data_container *container,int nr_entries,unsigned char msgtype,send_params *sParams);

/**
 * @brief Send buffered data.
 * This function sends data to a remote messaging layer instance.
 * @param connectionID The connection the data should be send to. 
 * @param sendbuf A pointer to the send buffer. 
 * @param bufsize The buffersize of the send buffer. 
 * @param msgtype The message type. 
 * @param sParams A pointer to a send_params struct. If NULL, the default is used given at mlOpenConnection
 */
void mlSendData(const int connectionID,char *sendbuf,int bufsize,unsigned char msgtype,send_params *sParams);

/**
 * @brief Receive newly arrived data.
 * This function receives data from a remote messaging layer instance.  
 * @param connectionID The connection the data came from. 
 * @param recvbuf A pointer to a buffer the data will be stored in. 
 * @param bufsize A pointer to an int that tells the size of the received data. 
 * @param rParams A pointer to a recv_params struct. 
 */
int mlRecvData(const int connectionID,char *recvbuf,int *bufsize,recv_params *rParams);

/**
 * @brief Set the STUN server.
 * This function sets the stun server address  
 * @param port The port of the stun server. 
 * @param ipaddr The ip address of the stun server. 
 */
void mlSetStunServer(const int port,const char *ipaddr);

/**
 * @brief Set a receive timeout.
 * This function sets a receive timeout for the data arrival. Should not all fragments of a data buffer arrive within this timeframe the arrival buffer is closed and the fragments that have arrived are returned to the upper layer. 
 * @param timeout_value The timeout value for receiving data. 
 */
void mlSetRecvTimeout(struct timeval timeout_value);

/**
 * @brief Get the TTL.
 * A function that returns the standard TTL value the operating system places in every IP packet
 * @param socketID A local socketID
 * @param ttl A pointer to an integer
 * @return 0 if everything went well and -1 if an error occured 
 */
int mlGetStandardTTL(socketID_handle socketID,uint8_t *ttl);

/**
 * @brief Get the local socket.
 * A function that returns a handle for the local socketID
 * @param errorstatus Returns 0 if the socketID is operational and 2 if the NAT traversal failed
* @return A socketID handle
*/
socketID_handle mlGetLocalSocketID(int *errorstatus);

/**
 * @brief Get the external IP address.
 * A function that returns the external IP from the local socketID
 * @param external_addr A pointer to a char buffer that holds the external address
 * @return -1 if an error occurred and 0 if it succeeded
 */
int mlGetExternalIP(char* external_addr);

/**
 * @brief Give a string representation of a SocketID.
 * A function that returns a string from a socketID handler
 * @param socketID
 * @param socketID_string
 * @param len length of socketID_string buffer
 * @return -1 if an error occurred and 0 if it succeded
 */
int mlSocketIDToString(socketID_handle socketID,char* socketID_string, size_t len);

/**
 * @brief Make a SOcketID out of a proper string.
 * A function that returns a socketID handler from a string.
 * @param socketID A socketID in string format
 * @return a pointer to a socketID
 */
int mlStringToSocketID(const char* socketID_string, socketID_handle socketID);

/**
 * @brief Get the connection status.
 * A function that returns the whenever a connection is ready to use or not.
 * @param connectionID The connectionID of the connection that is queried.                       
 * @return 1 if READY 0 if not or -1 if the connectionID does not exist                      
 */
int mlGetConnectionStatus(int connectionID);

/**
 * @brief Check if a connection already exists.
 * A function that returns the whenever a connection is ready to use or not.
 * @param socketID A remote socketID to which the connection has been established.               
 * @param ready if true, returns the connID on READY connections only 
 * @return -1 if the connection does not exist and the connection ID if it exists.               
 */
int mlConnectionExist(socketID_handle socketID, bool ready);

/**
 * @brief Try and guess the own IP.
 * Convenience function to guess the IP address if not specified in the configuration.
 * Not foolproof.
 * @return a static structure holding the address or NULL
 */
const char *mlAutodetectIPAddress();

//Comodity functions added By Robert Birke
//int mlPrintSocketID(socketID_handle socketID);

/**
 * @brief Give a hash for a SocketID.
 * Provide the hash code of the given SocketID argument.
 * @param sock A pointer to a socket_ID.
 * @return The hash of the argument.
 */
int mlHashSocketID(socketID_handle sock);

/**
 * @brief Compare two SocketIDs.
 * Test for equality two given SocketIDs.
 * @param sock1 A pointer to a socket_ID.
 * @param sock2 A pointer to a socket_ID.
 * @return 0 if the arguments are equual ; 1 otherwise.
 */
int mlCompareSocketIDs(socketID_handle sock1, socketID_handle sock2);

/**
 * @brief Compare the port number of two SocketIDs.
 * Test for equality the port number associated to two given SocketIDs.
 * @param sock1 A pointer to a socket_ID.
 * @param sock2 A pointer to a socket_ID.
 * @return 0 if the arguments have the same port number ; 1 otherwise.
 */
int mlCompareSocketIDsByPort(socketID_handle sock1, socketID_handle sock2);

/**
 * @brief Get the MTU of a given connection
 * Get the MTU associated to a given connection ID.
 * @param ConnectionID The ID of an open connection.
 * @return The MTU associated to the given connection.
 */
int mlGetPathMTU(int ConnectionId);

#ifdef __cplusplus
}
#endif

#endif
