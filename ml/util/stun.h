/*
 * Copyright (c) 2009 NEC Europe Ltd. All Rights Reserved.
 * Authors: Kristian Beckers  <beckers@nw.neclab.eu>
 *          Sebastian Kiesel  <kiesel@nw.neclab.eu>
 *
 * This file is part of the Messaging Library.
 *
 * The Messaging Library is free software: you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation, either version 3 of the
 * License, or (at your option) any later version.
 *
 * The Messaging Library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser
 * General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with the Messaging Library.  If not, see <http://www.gnu.org/licenses/>.
 *
 */

/**
 * @file stun.h
 * @brief This file holds functions for the NAT traversal with STUN. 
 *
 * The functionality in this is used by the messaging layer to implement a STUN client. This client can send STUN request and parse STUN responses. 
 * @author Kristian Beckers  <beckers@nw.neclab.eu>                             
 * @author Sebastian Kiesel  <kiesel@nw.neclab.eu>                              
 *                                                                              
 * @date 7/28/2009 
 *
 * @note Policy Management
 */



#ifndef STUN_H
#define STUN_H

#include <stdio.h> 
#include <time.h>
#ifndef _WIN32
#include <netinet/in.h>
#endif
#include <string.h>
#include <stdbool.h>

#define LOG_MODULE "[ml] "
#include "ml_log.h"

/**
 * The maximum size of a STUN string 
 */
#define STUN_MAX_STRING 256

/**
 * The maximum number of unkown attributs
 */
#define STUN_MAX_UNKNOWN_ATTRIBUTES 8

/**
 * The maximum length of a STUN message
 */
#define STUN_MAX_MESSAGE_SIZE 576

/**
 * The standard STUN port
 */
#define STUN_PORT 3478

#ifndef false
/**
  * Boolean define 0 as false
  */
#define false 0
#endif
#ifndef true
/**
  * Boolean define 1 as true
  */
#define true 1
#endif
//@}

/**
 * define basic type UInt8 
 */
typedef unsigned char  UInt8;

/**                                                                         
 * define basic type UInt16
 */
typedef unsigned short UInt16;

/**
 * define basic type UInt32
 */
typedef unsigned int UInt32;

/**
 * define basic type UInt64
 */
typedef unsigned long long UInt64;

/**
 * define basic type UInt128
 */
typedef struct { unsigned char octet[16]; ///< char array  
}  UInt128;

/**
 * STUN message header
 */
typedef struct 
{
      UInt16 msgType; ///< stun message type
      UInt16 msgLength; ///< stun message length
      UInt128 id; ///< the stun ID of the message
} StunMsgHdr;

/** 
 * STUN attribute header           
 */
typedef struct
{
      UInt16 type; ///< the attribute type
      UInt16 length; ///< the attribute length
} StunAtrHdr;

/**
 * The stun ipv4 address
 */
typedef struct
{
      UInt16 port; ///< the port 
      UInt32 addr; ///< the ip address
} StunAddress4;

/**
 * the stun attribute address
 */
typedef struct
{
      UInt8 pad; ///< padding
      UInt8 family; ///< address family
      StunAddress4 ipv4; ///< stun ipv4 address
} StunAtrAddress4; 

/**
 * the stun change request
 */
typedef struct
{
      UInt32 value; ///< the value of the change request
} StunAtrChangeRequest; 

/**
 * stun attribute error 
 */
typedef struct
{
      UInt16 pad; ///< padding with 0
      UInt8 errorClass; ///< stun error class
      UInt8 number; ///< stun error number
      char reason[STUN_MAX_STRING]; ///< the error reason
      UInt16 sizeReason; ///< the reason size
} StunAtrError;

/**
 * stun attribute unknown
 */
typedef struct
{
      UInt16 attrType[STUN_MAX_UNKNOWN_ATTRIBUTES]; ///< attribute type 
      UInt16 numAttributes; ///< number of attributes
} StunAtrUnknown;

/**
 * stun attribute string
 */
typedef struct
{
      char value[STUN_MAX_STRING]; ///< the value of the string      
      UInt16 sizeValue; ///< the size of the string
} StunAtrString;

/**
 * stun attribute integrity 
 */
typedef struct
{
      char hash[20]; ///< a hash value 
} StunAtrIntegrity; 

/**
 * stun HMAC status
 */
typedef enum 
{
   HmacUnkown=0, ///< HMAC is unknown
   HmacOK, ///< HMAC is ok
   HmacBadUserName, ///< HAMC has a bad username
   HmacUnkownUserName, ///< HMAC has an unkown username
   HmacFailed, ///< HMAC failed
} StunHmacStatus;

/**
 * a stun message
 */
typedef struct
{
      StunMsgHdr msgHdr; ///< stun message header
	
      bool hasMappedAddress; ///< boolean if a mapped address is present
      StunAtrAddress4  mappedAddress; ///< stun mapped address 
	
      bool hasResponseAddress; ///< boolean if a response address is present
      StunAtrAddress4  responseAddress; ///< stun response address
	
      bool hasChangeRequest; ///< boolean if a change request is present
      StunAtrChangeRequest changeRequest; ///< change request
	
      bool hasSourceAddress; ///< boolean if a source address is present
      StunAtrAddress4 sourceAddress; ///< source address
	
      bool hasChangedAddress;///< boolean if a changed address is present
      StunAtrAddress4 changedAddress; ///< changed address
	
      bool hasUsername;///< boolean if a username is present
      StunAtrString username; ///< username
	
      bool hasPassword;///< boolean if a password is present
      StunAtrString password; ///< password
	
      bool hasMessageIntegrity;///< boolean if a message integrity check is present
      StunAtrIntegrity messageIntegrity; ///< message integrity check
	
      bool hasErrorCode;///< boolean if a error code is present
      StunAtrError errorCode; ///< stun error code
	
      bool hasUnknownAttributes;///< boolean if an unkown attribute is present
      StunAtrUnknown unknownAttributes; ///< unkown stun attributes
	
      bool hasReflectedFrom;///< boolean if a reflected from is present
      StunAtrAddress4 reflectedFrom; ///< reflected from

      bool hasXorMappedAddress;///< boolean if a xor mapped address is present
      StunAtrAddress4  xorMappedAddress; ///< xor mapped address
	
      bool xorOnly;///< boolean if the mapping is XOR only

      bool hasServerName;///< boolean if a server name is present
      StunAtrString serverName; ///< stun server name
      
      bool hasSecondaryAddress;///< boolean if a secondary address is present
      StunAtrAddress4 secondaryAddress;///< secondary address
} StunMessage; 

/**
 * Define enum with different types of NAT 
 */
typedef enum 
{
   StunTypeUnknown=0,
   StunTypeFailure,
   StunTypeOpen,
   StunTypeBlocked,

   StunTypeIndependentFilter,
   StunTypeDependentFilter,
   StunTypePortDependedFilter,
   StunTypeDependentMapping,

   //StunTypeConeNat,
   //StunTypeRestrictedNat,
   //StunTypePortRestrictedNat,
   //StunTypeSymNat,
   
   StunTypeFirewall,
} NatType;

/**
 * Define socket file descriptor
 */
typedef int Socket;

/**
 * Define the maximum ammount of media relays
 */
#define MAX_MEDIA_RELAYS 500

/**
 * Define the maximum rtp message size
 */
#define MAX_RTP_MSG_SIZE 1500

/**
 * Define the media replay timeout
 */
#define MEDIA_RELAY_TIMEOUT 3*60

/**
 * stun media relay
 */
typedef struct 
{
      int relayPort;       ///< media relay port
      int fd;              ///< media relay file descriptor
      StunAddress4 destination; ///< NAT IP:port
      time_t expireTime;      ///< if no activity after time, close the socket 
} StunMediaRelay;

/**
 * stun server information
 */
typedef struct
{
      StunAddress4 myAddr; ///< server address
      StunAddress4 altAddr; ///< alternative server address
      Socket myFd; ///< socket file descriptor
      Socket altPortFd; ///< alternative port 
      Socket altIpFd; ///< altertnative ip address
      Socket altIpPortFd; ///< port and ip address
      bool relay; ///< true if media relaying is to be done
      StunMediaRelay relays[MAX_MEDIA_RELAYS]; ///< stun media relays
} StunServerInfo;

/**
 * A callback for pmtu error messages. These are used by the udpSocket to tell the stun client that the send stun request had a too big pmtu size.  
 * @param *buf for retruned message
 * @param bufsize returned message size
 */
void pmtu_error_cb_stun(char *buf,int bufsize);

/**
 * Send a stun request to a stun server
 * @param udpSocket a file descriptor for a udp socket
 * @param *stunServ the address of the stun server
 * @return 0 if successful
 */
int send_stun_request(const int udpSocket,struct sockaddr_in *stunServ);

/**
 * Send a stun keep alive message
 * @param udpSocket a file descriptor for a udp socket
 * @param *stunServ the address of the stun server
 * @return 0 if successful
 */
int send_stun_keep_alive(const int udpSocket,struct sockaddr_in *stunServ);

/**
 * Receive a stun message
 * @param *msg pointer to a stun message
 * @param msgLen stun message length
 * @param *response a pointer to a stun response
 * @return 0 if successful
 */
int recv_stun_message(char *msg, int msgLen,StunMessage *response);

/**
 * Parse a stun attribute address
 * @param body a pointer to a buffer that contains a stun message body
 * @param hdrLen the length of the header
 * @param *result the stun ipv4 address
 * @param verbose A boolean that tells if debug output shall be displayed
 * @return true if parsing was sussessful otherwise false
 */
bool
stunParseAtrAddress(char* body, unsigned int hdrLen,StunAtrAddress4 *result,bool verbose);

/**
 * Parse a stun message
 * @param *buf a buffer that contains the stun message
 * @param bufLen the length of the buffer
 * @param *msg A pointer to a StunMessage struct
 * @param verbose A boolean that tells if debug output shall be displayed
 * @return true if parsing was sussessful otherwise false
 */
bool
stunParseMessage( char* buf, 
                  unsigned int bufLen, 
                  StunMessage *msg, 
                  bool verbose );

/**
 * Build a very simple stun request
 * @param *msg A pointer to a StunMessage struct
 * @param username A stun username
 * @param changePort The change port
 * @param changeIp The change IP
 * @param id A stun id
 */
void
stunBuildReqSimple( StunMessage* msg, const StunAtrString username,
                    bool changePort, bool changeIp, unsigned int id );

/**
 * Encode stun message
 * @param message A pointer to a StunMessage struct
 * @param buf a buffer that contains the stun message
 * @param bufLen the length of the buffer
 * @param password A stun password
 * @param verbose A boolean that tells if debug output shall be displayed
 * @return true if parsing was sussessful otherwise false
 */
unsigned int
stunEncodeMessage( const StunMessage message, 
                   char* buf, 
                   unsigned int bufLen, 
                   const StunAtrString password,
                   bool verbose);

/**
 * create a random stun int 
 * @return random int 
 */
int 
stunRand();

/**
 * Get the system time in seconds 
 * @return system time in seconds 
 */
UInt64
stunGetSystemTimeSecs();

/**
 * Find the IP address of a the specified stun server
 * @return false if parse failse
 */
bool  
stunParseServerName( char* serverName, StunAddress4 stunServerAddr);

/**
 * Parse the stun hostname
 * @param peerName A pointer to a buffer that contains the peer name.
 * @param ip The ip address of the hostname  
 * @param portVal The port of the hostname 
 * @param defaultPort The default port of the hostname 
 * @return true if parsing was sussessful otherwise false 
 */
bool 
stunParseHostName(char* peerName,
                   UInt32 ip,
                   UInt16 portVal,
                   UInt16 defaultPort );

/**
 * Parse a stun change request
 * @param body A pointer to a buffer that contains the stun change request
 * @param hdrLen The length of the buffer
 * @param *result A pointer to a StunAtrChangeRequest struct. 
 * @return true if parsing was sussessful otherwise false
 */
bool
stunParseAtrChangeRequest(char* body,unsigned int hdrLen,StunAtrChangeRequest *result);

/**
 * Parse an unkown attribute
 * @param body A pointer to a stun message body that has an unkown attribute
 * @param hdrLen The length of that header
 * @param *result A pointer to a StunAtrUnknown struct.
 * @return true if parsing was sussessful otherwise false
 */
bool
stunParseAtrUnknown( char* body, unsigned int hdrLen,  StunAtrUnknown *result );

/**
 * Parse a stun attribute string
 * @param body A pointer to a stun message body that has a stun string
 * @param hdrLen The length of that string
 * @param *result A pointer to a StunAtrString struct. 
 * @return true if parsing was sussessful otherwise false
 */
bool
stunParseAtrString( char* body, unsigned int hdrLen,  StunAtrString *result );

/**
 * Parse stun attribute integrity
 * @param body A pointer to a stun body that holds an attribute
 * @param hdrLen The length of that attribute
 * @param *result A pointer to a StunAtrIntegrity struct. 
 * @return true if parsing was sussessful otherwise false
 */
bool
stunParseAtrIntegrity( char* body, unsigned int hdrLen,  StunAtrIntegrity *result );

/**
 * Create a stun error response 
 * @param response A StunMessage struct
 * @param cl The error class
 * @param number The error number
 * @param *msg A pointer to the stun message
 */
void
stunCreateErrorResponse(StunMessage response, int cl, int number, const char* msg);

/**
 * Parse a stun attribute error 
 * @param body A pointer to a stun body that holds an attribute
 * @param hdrLen The length of that attribute
 * @param *result A pointer to a StunAtrError struct.
 * @return true if parsing was sussessful otherwise false
 */
bool
stunParseAtrError( char* body, unsigned int hdrLen,StunAtrError *result);

/**
 * Encode stun attribute string 
 * @param ptr A pointer to the string
 * @param type The attribute type
 * @param atr A struct from type StunAtrString
 * @return encoded attribute string
 */
char*
encodeAtrString(char* ptr, UInt16 type, const StunAtrString atr);

/**
 * Encode attribute integrity
 * @param ptr A pointer to the string
 * @param atr A struct from type StunAtrString
 * @return encoded attribute integrity value 
 */
char*
encodeAtrIntegrity(char* ptr, const StunAtrIntegrity atr);

/**
 * Encode attribute error
 * @param ptr a pointer to a buffer
 * @param atr A StunAtrError struct
 * @result a pointer to an encoded attribute error  
 */
char*
encodeAtrError(char* ptr, const StunAtrError atr);

/**
 * Encode an unkown attribute
 * @param ptr a pointer to a buffer
 * @param atr A StunAtrUnkown struct
 * @result a pointer to an encoded unkown attribute
 */
char*
encodeAtrUnknown(char* ptr, const StunAtrUnknown atr);

/**
 * Encode a stun ipv4 address
 * @param ptr a pointer to a buffer
 * @param type a attribute type
 * @param atr A StunAtrAddress4 struct	
 * @result a pointer to an encoded attribute ipv4 address
 */
char*
encodeAtrAddress4(char* ptr, UInt16 type, const StunAtrAddress4 atr);

/**
 * Encode a change request stun attribute
 * @param ptr a pointer to a buffer
 * @param atr A StunAtrChangeRequest struct
 * @result a pointer to an encoded unkown attribute
 */
char*
encodeAtrChangeRequest(char* ptr, const StunAtrChangeRequest atr);

/**
 * Encode an XOR only
 * @param ptr a pointer to a buffer
 * @result a pointer to an xor only   
 */
char* encodeXorOnly(char* ptr);

/**
 * Encode data into a buffer
 * @param buf a buffer pointer
 * @param data the data to encode into the buffer 
 * @param length the length of the data
 * @return pointer to the buffer position behind the data
 */
char* encode(char* buf, const char* data, unsigned int length);

/**
 * Encode data into network byte order and use a UInt16 slot of a buffer 
 * @param buf a buffer pointer
 * @param data the data to encode into the buffer 
 * @return pointer to the buffer position behind the data
 */
char* encode16(char* buf, UInt16 data);

/**
 * Encode data into network byte order and use a UInt32 slot of a buffer
 * @param buf a buffer pointer
 * @param data the data to encode into the buffer 
 * @return pointer to the buffer position behind the data
 */
char* encode32(char* buf, UInt32 data);

/**
 * Get a random port number
 * @return return a random number to use as a port 
 */
int
stunRandomPort();

/**
 * Convert to Hex
 * @param buffer A pointer to a buffer with the data to convert
 * @param bufferSize The length of the buffer
 * @param output A pointer to the output buffer
 */
void
toHex(const char* buffer, int bufferSize, char* output);

#endif
