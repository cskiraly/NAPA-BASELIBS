/*
 *          Policy Management
 *
 *          NEC Europe Ltd. PROPRIETARY INFORMATION
 *
 * This software is supplied under the terms of a license agreement
 * or nondisclosure agreement with NEC Europe Ltd. and may not be
 * copied or disclosed except in accordance with the terms of that
 * agreement.
 *
 *      Copyright (c) 2009 NEC Europe Ltd. All Rights Reserved.
 *
 * Authors: Kristian Beckers  <beckers@nw.neclab.eu>
 *    Sebastian Kiesel  <kiesel@nw.neclab.eu>
 *          
 *
 * NEC Europe Ltd. DISCLAIMS ALL WARRANTIES, EITHER EXPRESS OR IMPLIED,
 * INCLUDING BUT NOT LIMITED TO IMPLIED WARRANTIES OF MERCHANTABILITY
 * AND FITNESS FOR A PARTICULAR PURPOSE AND THE WARRANTY AGAINST LATENT
 * DEFECTS, WITH RESPECT TO THE PROGRAM AND THE ACCOMPANYING
 * DOCUMENTATION.
 *
 * No Liability For Consequential Damages IN NO EVENT SHALL NEC Europe
 * Ltd., NEC Corporation OR ANY OF ITS SUBSIDIARIES BE LIABLE FOR ANY
 * DAMAGES WHATSOEVER (INCLUDING, WITHOUT LIMITATION, DAMAGES FOR LOSS
 * OF BUSINESS PROFITS, BUSINESS INTERRUPTION, LOSS OF INFORMATION, OR
 * OTHER PECUNIARY LOSS AND INDIRECT, CONSEQUENTIAL, INCIDENTAL,
 * ECONOMIC OR PUNITIVE DAMAGES) ARISING OUT OF THE USE OF OR INABILITY
 * TO USE THIS PROGRAM, EVEN IF NEC Europe Ltd. HAS BEEN ADVISED OF THE
 * POSSIBILITY OF SUCH DAMAGES.
 *
 *     THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 */

#include <assert.h>
#include <stdlib.h>   
#include <unistd.h>
#include <errno.h>
#include <string.h>

#include <stdio.h> 
#include <unistd.h>

#include <sys/time.h>
#include <sys/types.h> 

#include <fcntl.h>

#ifndef WIN32
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <netdb.h>
#include <netinet/in.h>
//#include <arpa/nameser.h>
//#include <resolv.h>
#include <net/if.h>
#include <arpa/inet.h>

#else

#include <winsock2.h>
#include <ws2tcpip.h>

#endif

#include "ml_log.h"
#include "stun.h"
#include "udpSocket.h"

#define STUN_VERBOSE false

/// define a structure to hold a stun address             
const UInt8  IPv4Family = 0x01;
const UInt8  IPv6Family = 0x02;
// define  flags                                             
const UInt32 ChangeIpFlag   = 0x04;
const UInt32 ChangePortFlag = 0x02;
// define  stun attributes        
const UInt16 MappedAddress    = 0x0001;
const UInt16 ResponseAddress  = 0x0002;
const UInt16 ChangeRequest    = 0x0003;
const UInt16 SourceAddress    = 0x0004;
const UInt16 ChangedAddress   = 0x0005;
const UInt16 Username         = 0x0006;
const UInt16 Password         = 0x0007;
const UInt16 MessageIntegrity = 0x0008;
const UInt16 ErrorCode        = 0x0009;
const UInt16 UnknownAttribute = 0x000A;
const UInt16 ReflectedFrom    = 0x000B;
const UInt16 XorMappedAddress = 0x8020;
const UInt16 XorOnly          = 0x0021;
const UInt16 ServerName       = 0x8022;
const UInt16 SecondaryAddress = 0x8050; // Non standard extention       
// define types for a stun message                                 
const UInt16 BindRequestMsg               = 0x0001;
const UInt16 BindResponseMsg              = 0x0101;
const UInt16 BindErrorResponseMsg         = 0x0111;
const UInt16 SharedSecretRequestMsg       = 0x0002;
const UInt16 SharedSecretResponseMsg      = 0x0102;
const UInt16 SharedSecretErrorResponseMsg = 0x0112;

void pmtu_error_cb_stun(char *buf,int bufsize){

  error("error: MTU size of stun message too big !");

}

/* sends a request to a stun server */
int send_stun_request(const int udpSocket,struct sockaddr_in *stunServ)
{

  assert( sizeof(UInt8 ) == 1 );
  assert( sizeof(UInt16) == 2 );
  assert( sizeof(UInt32) == 4 );

  bool verbose = STUN_VERBOSE;
  
  //set the verbose mode 
  verbose = true;

  StunMessage req;
  memset(&req, 0, sizeof(StunMessage));

  StunAtrString username;
  StunAtrString password;
  username.sizeValue = 0;
  password.sizeValue = 0;

  /* build a stun request message  */
  /* creates a stun message struct with the given atributes  */
  stunBuildReqSimple( &req, username,
                      false , false ,
                      0x0c );

  /* buffer for the stun request  */
  char buf[STUN_MAX_MESSAGE_SIZE];
  int len = STUN_MAX_MESSAGE_SIZE;

  /* encode the stun message  */
  len = stunEncodeMessage( req, buf, len, password, verbose);

  struct iovec iov;
  iov.iov_len = len;
  iov.iov_base = buf;
debug("Sending STUN %d!!!!!\n",len); 
  sendPacket(udpSocket,&iov,1,stunServ);

  return 0;

}

/* sends a keep alive to a server  */
int send_stun_keep_alive(const int udpSocket,struct sockaddr_in *stunServ)
{

  send_stun_request(udpSocket,stunServ);

  return 0;
}

/* parses a given stun message and sets the refelxive address  */
int recv_stun_message(char *msgbuf,int msgLen,StunMessage *response){

  //StunMessage resp;
  //memset(&resp, 0, sizeof(StunMessage));
  int returnValue = 0;
  //bool verbose = true;
  bool verbose = STUN_VERBOSE;

  //debug("still alive 4 \n ");
  //if ( verbose ) info("Got a response \n");

  bool ok = stunParseMessage(msgbuf, msgLen, response, verbose );

  if (!ok){
    
    returnValue = 1;
    error("parse STUN message failed \n");

  }

      UInt32 ip_addr = response->mappedAddress.ipv4.addr;
      char mapped_addr[16];
      int length;
      length = sprintf(mapped_addr,"%d.%d.%d.%d",(ip_addr >> 24) & 0xff,(ip_addr >> 16) & 0xff,(ip_addr >> 8) & 0xff,ip_addr & 0xff);

      ip_addr = response->changedAddress.ipv4.addr;
      char changed_addr[16];
      length = sprintf(changed_addr,"%d.%d.%d.%d",(ip_addr >> 24) & 0xff,(ip_addr >> 16)\
	      & 0xff,(ip_addr >> 8) & 0xff,ip_addr & 0xff);

  if ( verbose )
  {

      info( "\t mappedAddr= ipv4 %s port %d \n \t changedAddr= ipv4 %s port %d \n",
              mapped_addr ,
              response->mappedAddress.ipv4.port,
              changed_addr,
              response->changedAddress.ipv4.port);

      }

  //reflexiveAddr.sin_addr.s_addr = inet_addr(changed_addr);
  //reflexiveAddr->sin_addr.s_addr = inet_addr("1.1.1.1");
  //reflexiveAddr->sin_port = htons(resp.mappedAddress.ipv4.port);
  
  return returnValue;
}

bool 
stunParseAtrAddress( char* body, unsigned int hdrLen, StunAtrAddress4 *result, bool verbose )
{
   if ( hdrLen != 8 )
   {
      error("hdrlen is not 8 \n ");
      return false;
   }
   result->pad = *body++;
   result->family = *body++;

   if (result->family == IPv4Family)
   {

      UInt16 nport;
      memcpy(&nport, body, 2); body+=2;
      
      if (verbose) info("port %u \n",ntohs(nport));

      result->ipv4.port = ntohs(nport);

		
      UInt32 naddr;
      memcpy(&naddr, body, 4); body+=4;
      //debug("port %d \n",ntohl(naddr));
      
      if (verbose)
	{
      UInt32 ip_addr = ntohl(naddr);
      char str_p[20];

      sprintf(str_p,"%d.%d.%d.%d",(ip_addr >> 24) & 0xff,(ip_addr >> 16) & 0xff,(ip_addr >> 8) & 0xff,ip_addr & 0xff);          

      info(" ipv4 as str %s \n  ",str_p);
	}

      result->ipv4.addr = ntohl(naddr);

      return true;
   }

   else if (result->family == IPv6Family)

   {
      warn("ipv6 not supported  \n");
   }
   else
   {
      error("bad address family: %i \n ",result->family );
   }
	
   return false;
}



bool
stunParseMessage( char* buf, unsigned int bufLen, StunMessage *msg, bool verbose)
{
  if (verbose) info("Received stun message: %d bytes \n",bufLen);
   memset(msg, 0, sizeof(msg));
	
   if (sizeof(StunMsgHdr) > bufLen)
   {
     error("Bad message \n");
      return false;
   }
	
   memcpy(&msg->msgHdr, buf, sizeof(StunMsgHdr));
   msg->msgHdr.msgType = ntohs(msg->msgHdr.msgType);
   msg->msgHdr.msgLength = ntohs(msg->msgHdr.msgLength);
	
   if (msg->msgHdr.msgLength + sizeof(StunMsgHdr) != bufLen)
   {
     error("Message header length doesn't match message size: %d - %d \n"
	    ,msg->msgHdr.msgLength,bufLen);
      return false;
   }
	
   char* body = buf + sizeof(StunMsgHdr);
   unsigned int size = msg->msgHdr.msgLength;
	
//   debug("bytes after header = %d\n",size);
	
   while ( size > 0 )
   {
      // !jf! should check that there are enough bytes left in the buffer
		
     StunAtrHdr* attr = (StunAtrHdr*)(body);
		
      unsigned int attrLen = ntohs(attr->length);
      int atrType = ntohs(attr->type);
		
      //if (verbose) clog << "Found attribute type=" << AttrNames[atrType] << " length=" << attrLen << endl;
      if ( attrLen+4 > size ) 
      {
	debug( "claims attribute is larger than size of message " \
		"(attribute type= %d \n" ,atrType);
         return false;
      }
		
      body += 4; // skip the length and type in attribute header
      size -= 4;
		
      if( atrType == MappedAddress)
      { 
        msg->hasMappedAddress = true;
	//debug("has mapped address \n");
        if ( stunParseAtrAddress(body, attrLen, &msg->mappedAddress, verbose) == false)
            {
        	debug("problem parsing MappedAddress \n");
               return false;
            }
            else
            {
	      if (verbose) {info("MappedAddress -------------------\n");}
            }
					
      }else 
      if (atrType == ResponseAddress)
	{
            msg->hasResponseAddress = true;
            if ( stunParseAtrAddress(  body,  attrLen,  &msg->responseAddress, verbose )== false )
            {
            	debug("problem parsing ResponseAddress \n");
              return false;
            }
            else
            {
	      if (verbose) {info("ResponseAddress ---------------------\n");}
            }
             
	}else if(atrType == ChangeRequest)
	{
            msg->hasChangeRequest = true;
            if (stunParseAtrChangeRequest( body, attrLen, &msg->changeRequest) == false)
            {
            	debug("problem parsing ChangeRequest \n");
               return false;
            }
            else
            {
	      if (verbose) info("ChangeRequest = %ud \n", msg->changeRequest.value);
            }
            
	} else if (atrType == SourceAddress)
	{
            msg->hasSourceAddress = true;
            if ( stunParseAtrAddress(  body,  attrLen,  &msg->sourceAddress, verbose )== false )
            {
            	debug("problem parsing SourceAddress \n");
               return false;
            }
            else
            {
	      if (verbose) info("SourceAddress ------------------\n");
            }
	} else if (atrType ==  ChangedAddress)
	{
            msg->hasChangedAddress = true;
            if ( stunParseAtrAddress(  body,  attrLen, &msg->changedAddress, verbose )== false )
            {
            	debug("problem parsing ChangedAddress \n");
               return false;
            }
            else
            {
	      if (verbose) info("ChangedAddress = ---------------------\n");
            }
	} else if (atrType ==  Username) 
	{
            msg->hasUsername = true;
            if (stunParseAtrString( body, attrLen, &msg->username) == false)
            {
            	debug("problem parsing Username \n");
               return false;
            }
            else
            {
	      if (verbose) info("Username = %s \n",msg->username.value);
            }
	}else if (atrType ==  Password) 
	 {
            msg->hasPassword = true;
            if (stunParseAtrString( body, attrLen, &msg->password) == false)
            {
	      debug("problem parsing Password \n");
               return false;
            }
            else
            {
	      if (verbose) info("Password = %s \n",(char *)&msg->password.value);
            }
	 }else if (atrType == MessageIntegrity)
	{
            msg->hasMessageIntegrity = true;
            if (stunParseAtrIntegrity( body, attrLen,&msg->messageIntegrity) == false)
            {
	      debug("problem parsing MessageIntegrity \n");
               return false;
            }
            else
            {
               //if (verbose) clog << "MessageIntegrity = " << msg.messageIntegrity.hash << endl;
            }
					
            // read the current HMAC
            // look up the password given the user of given the transaction id 
            // compute the HMAC on the buffer
            // decide if they match or not
          
	} else if (atrType == ErrorCode)
	{
            msg->hasErrorCode = true;
            if (stunParseAtrError(body, attrLen, &msg->errorCode) == false)
            {
	      debug("problem parsing ErrorCode \n");
               return false;
            }
            else
            {
	      if (verbose) info("ErrorCode = %d %d %s \n",(int)(msg->errorCode.errorClass)                                 ,(int)(msg->errorCode.number)
				 ,msg->errorCode.reason);
            }
					
	} else if (atrType == UnknownAttribute)
	{			
            msg->hasUnknownAttributes = true;
            if (stunParseAtrUnknown(body, attrLen,&msg->unknownAttributes) == false)
            {
	      debug("problem parsing UnknownAttribute \n");
               return false;
            }
	} else if (atrType == ReflectedFrom)
	{
            msg->hasReflectedFrom = true;
            if ( stunParseAtrAddress(  body,  attrLen,  &msg->reflectedFrom, verbose ) == false )
            {
	      debug("problem parsing ReflectedFrom \n");
               return false;
            }
	} else if (atrType == XorMappedAddress)
	{		
            msg->hasXorMappedAddress = true;
            if ( stunParseAtrAddress(  body,  attrLen,  &msg->xorMappedAddress, verbose ) == false )
            {
	      debug("problem parsing XorMappedAddress \n");
               return false;
            }
            else
            {
	      if (verbose) info("XorMappedAddress ------------------- \n");
            }
	} else if (atrType == XorOnly)
	{
            msg->xorOnly = true;
            if (verbose) 
            {
	      debug("xorOnly = true \n");
            }
	} else if (atrType == ServerName) 
	{			
            msg->hasServerName = true;
            if (stunParseAtrString( body, attrLen, &msg->serverName) == false)
            {
	      debug("problem parsing ServerName \n");
               return false;
            }
            else
            {
	      if (verbose) info("ServerName = %s \n",(char *)&msg->serverName.value);
            }
	} else if (atrType == SecondaryAddress)
	{
            msg->hasSecondaryAddress = true;
            if ( stunParseAtrAddress(  body,  attrLen, &msg->secondaryAddress, verbose ) == false )
            {
	      debug("problem parsing secondaryAddress \n");
               return false;
            }
            else
            {
	      if (verbose) info("SecondaryAddress ---------------- \n");
            }
	}  else 
	{				
         
	   if (verbose) info("Unknown attribute: %i \n",atrType);
            if ( atrType <= 0x7FFF ) 
            {
               return false;
            }
	}
   
		
      body += attrLen;
      size -= attrLen;
   }
    
   return true;
}


void
stunBuildReqSimple(StunMessage* msg,
                    const StunAtrString username,
                    bool changePort, bool changeIp, unsigned int id )
{
   assert( msg );
   memset( msg , 0 , sizeof(*msg) );
	
   msg->msgHdr.msgType = BindRequestMsg;

   int i;	
   for (i=0; i<16; i=i+4 )
   {
      assert(i+3<16);
      int r = stunRand();
      msg->msgHdr.id.octet[i+0]= r>>0;
      msg->msgHdr.id.octet[i+1]= r>>8;
      msg->msgHdr.id.octet[i+2]= r>>16;
      msg->msgHdr.id.octet[i+3]= r>>24;
   }
	
   if ( id != 0 )
   {
      msg->msgHdr.id.octet[0] = id; 
   }
	
   msg->hasChangeRequest = true;
   msg->changeRequest.value =(changeIp?ChangeIpFlag:0) | 
      (changePort?ChangePortFlag:0);
	
   if ( username.sizeValue > 0 )
   {
      msg->hasUsername = true;
      msg->username = username;
   }
}


unsigned int
stunEncodeMessage( const StunMessage msg, 
                   char* buf, 
                   unsigned int bufLen, 
                   const StunAtrString password, 
                   bool verbose)
{
   assert(bufLen >= sizeof(StunMsgHdr));
   char* ptr = buf;
	
   ptr = encode16(ptr, msg.msgHdr.msgType);
   char* lengthp = ptr;
   ptr = encode16(ptr, 0);
   ptr = encode(ptr, (const char*)(msg.msgHdr.id.octet), sizeof(msg.msgHdr.id));
	
   if (verbose) debug("Encoding stun message: ");

   if (msg.hasMappedAddress)
   {
     //if (verbose) info("Encoding MappedAddress: %s \n",msg.mappedAddress.ipv4);
      ptr = encodeAtrAddress4 (ptr, MappedAddress, msg.mappedAddress);
   }
   if (msg.hasResponseAddress)
   {
     //if (verbose) info("Encoding ResponseAddress: %s \n",msg.responseAddress.ipv4);
      ptr = encodeAtrAddress4(ptr, ResponseAddress, msg.responseAddress);
   }
   if (msg.hasChangeRequest)
   {
     //if (verbose) info("Encoding ChangeRequest: %s \n",msg.changeRequest.value);
      ptr = encodeAtrChangeRequest(ptr, msg.changeRequest);
   }
   if (msg.hasSourceAddress)
   {
     //if (verbose) info("Encoding SourceAddress: %s \n ",msg.sourceAddress.ipv4);
      ptr = encodeAtrAddress4(ptr, SourceAddress, msg.sourceAddress);
   }
   if (msg.hasChangedAddress)
   {
     //if (verbose) info("Encoding ChangedAddress: %s \n ",msg.changedAddress.ipv4);
      ptr = encodeAtrAddress4(ptr, ChangedAddress, msg.changedAddress);
   }
   if (msg.hasUsername)
   {
     if (verbose) info("Encoding Username: %s \n", msg.username.value);
      ptr = encodeAtrString(ptr, Username, msg.username);
   }
   if (msg.hasPassword)
   {
     if (verbose) info("Encoding Password: %s \n ",msg.password.value);
      ptr = encodeAtrString(ptr, Password, msg.password);
   }
   if (msg.hasErrorCode)
   {
     if (verbose) info("Encoding ErrorCode: class= %d number= %d reason= %s \n"
			 ,(int)msg.errorCode.errorClass  
			 ,(int)msg.errorCode.number 
		         ,msg.errorCode.reason 
			 );
		
      ptr = encodeAtrError(ptr, msg.errorCode);
   }
   if (msg.hasUnknownAttributes)
   {
     if (verbose) info("Encoding UnknownAttribute: ??? \n");
      ptr = encodeAtrUnknown(ptr, msg.unknownAttributes);
   }
   if (msg.hasReflectedFrom)
   {
     //if (verbose) info("Encoding ReflectedFrom: %s \n ",msg.reflectedFrom.ipv4);
      ptr = encodeAtrAddress4(ptr, ReflectedFrom, msg.reflectedFrom);
   }
   if (msg.hasXorMappedAddress)
   {
     //if (verbose) info("Encoding XorMappedAddress: %s \n ",msg.xorMappedAddress.ipv4 );
      ptr = encodeAtrAddress4 (ptr, XorMappedAddress, msg.xorMappedAddress);
   }
   if (msg.xorOnly)
   {
     if (verbose) info("Encoding xorOnly: \n");
      ptr = encodeXorOnly( ptr );
   }
   if (msg.hasServerName)
   {
     if (verbose) info("Encoding ServerName: %s \n ",msg.serverName.value);
      ptr = encodeAtrString(ptr, ServerName, msg.serverName);
   }
   if (msg.hasSecondaryAddress)
   {
     //if (verbose) info("Encoding SecondaryAddress: %s \n", msg.secondaryAddress.ipv4);
      ptr = encodeAtrAddress4 (ptr, SecondaryAddress, msg.secondaryAddress);
   }

   if (password.sizeValue > 0)
   {
     if (verbose) info("HMAC with password: %s \n ",password.value);
		
      StunAtrIntegrity integrity;
      //computeHmac(integrity.hash, buf, (int)(ptr-buf) , password.value, password.sizeValue);
      ptr = encodeAtrIntegrity(ptr, integrity);
   }
	
   encode16(lengthp, (UInt16)(ptr - buf - sizeof(StunMsgHdr)));
   return (int)(ptr - buf);
}


int 
stunRand()
{
   // return 32 bits of random stuff
   assert( sizeof(int) == 4 );
   static bool init=false;
   if ( !init )
   { 
      init = true;
		
      UInt64 tick;

#if defined(__GNUC__) && ( defined(__i686__) || defined(__i386__) || defined(__amd64__) )
      asm("rdtsc" : "=A" (tick));
#elif defined (__SUNPRO_CC) || defined( __sparc__ )	
      tick = gethrtime();
#elif defined(__MACH__) 
      int fd=open("/dev/random",O_RDONLY);
      read(fd,&tick,sizeof(tick));
      closesocket(fd);
#else
#     error Need some way to seed the random number generator 
#endif 
      int seed = (int)(tick);
#ifdef WIN32
      srand(seed);
#else
      srandom(seed);
#endif
   }
	

   return random(); 

}


UInt64
stunGetSystemTimeSecs()
{
   UInt64 time=0;
#if defined(WIN32)  
   SYSTEMTIME t;
   // CJ TODO - this probably has bug on wrap around every 24 hours
   GetSystemTime( &t );
   time = (t.wHour*60+t.wMinute)*60+t.wSecond; 
#else
   struct timeval now;
   gettimeofday( &now , NULL );
   //assert( now );
   time = now.tv_sec;
#endif
   return time;
}


bool
stunParseServerName( char* name, StunAddress4 addr)
{
   assert(name);
	
   // TODO - put in DNS SRV stuff.
	
   bool ret = stunParseHostName( name, addr.addr, addr.port, 3478); 
   if ( ret != true ) 
   {
       addr.port=0xFFFF;
   }	
   return ret;
}


// returns true if it scucceeded
bool 
stunParseHostName( char* peerName,
               UInt32 ip,
               UInt16 portVal,
               UInt16 defaultPort )
{
   struct in_addr sin_addr;
    
   char host[512];
   strncpy(host,peerName,512);
   host[512-1]='\0';
   char* port = NULL;
	
   int portNum = defaultPort;
	
   // pull out the port part if present.
   char* sep = strchr(host,':');
	
   if ( sep == NULL )
   {
      portNum = defaultPort;
   }
   else
   {
      *sep = '\0';
      port = sep + 1;
      // set port part
		
      char* endPtr=NULL;
		
      portNum = strtol(port,&endPtr,10);
		
      if ( endPtr != NULL )
      {
         if ( *endPtr != '\0' )
         {
            portNum = defaultPort;
         }
      }
   }
    
   if ( portNum < 1024 ) return false;
   if ( portNum >= 0xFFFF ) return false;
	
   // figure out the host part 
   struct hostent* h;

   h = gethostbyname( host );
   if ( h == NULL )
   {
     //int err = getErrno();
      error("error was %d \n",errno);
      ip = ntohl( 0x7F000001L );
      return false;
   }
   else
   {
      sin_addr = *(struct in_addr*)h->h_addr;
      ip = ntohl( sin_addr.s_addr );
   }

	
   portVal = portNum;
	
   return true;
}



bool 
stunParseAtrChangeRequest( char* body, unsigned int hdrLen, StunAtrChangeRequest *result )
{
   if ( hdrLen != 4 )
   {
      error("hdr length = %u expecting %u \n ",hdrLen,sizeof(result));

      error("Incorrect size for ChangeRequest \n ");

      return false;
   }
   else
   {
      memcpy(&result->value, body, 4);
      result->value = ntohl(result->value);
      return true;
   }
}




bool 
stunParseAtrUnknown( char* body, unsigned int hdrLen,  StunAtrUnknown *result)
{
   if ( hdrLen >= sizeof(StunAtrUnknown) )
   {
      return false;
   }
   else
   {
      if (hdrLen % 4 != 0) return false;
      result->numAttributes = hdrLen / 4;
      int i;
      for (i=0; i<result->numAttributes; i++)
      {
         memcpy(&result->attrType[i], body, 2); body+=2;
         result->attrType[i] = ntohs(result->attrType[i]);
      }
      return true;
   }
}



bool 
stunParseAtrString( char* body, unsigned int hdrLen,  StunAtrString *result )
{
   if ( hdrLen >= STUN_MAX_STRING )
   {
     debug( "String is too large \n");
      return false;
   }
   else
   {
      if (hdrLen % 4 != 0)
      {
	debug("Bad length string %d \n ",hdrLen);
         return false;
      }
		
      result->sizeValue = hdrLen;
      memcpy(&result->value, body, hdrLen);
      result->value[hdrLen] = 0;
      return true;
   }
}


bool 
stunParseAtrIntegrity( char* body, unsigned int hdrLen,StunAtrIntegrity *result )
{
   if ( hdrLen != 20)
   {
     debug("MessageIntegrity must be 20 bytes \n");
      return false;
   }
   else
   {
      memcpy(&result->hash, body, hdrLen);
      return true;
   }
}


void
stunCreateErrorResponse(StunMessage response, int cl, int number, const char* msg)
{
   response.msgHdr.msgType = BindErrorResponseMsg;
   response.hasErrorCode = true;
   response.errorCode.errorClass = cl;
   response.errorCode.number = number;
   strcpy(response.errorCode.reason, msg);
}

bool
stunParseAtrError(char* body,unsigned int hdrLen,StunAtrError *result)
{
   if ( hdrLen >= sizeof(StunAtrError) )
   {
      debug("head on Error too large \n  ");
      return false;
   }
   else
   {
      memcpy(&result->pad, body, 2); body+=2;
      result->pad = ntohs(result->pad);
      result->errorClass = *body++;
      result->number = *body++;
		
      result->sizeReason = hdrLen - 4;
      memcpy(&result->reason, body, result->sizeReason);
      result->reason[result->sizeReason] = 0;
      return true;
   }
}


char* 
encodeAtrString(char* ptr, UInt16 type, const StunAtrString atr)
{
   assert(atr.sizeValue % 4 == 0);
	
   ptr = encode16(ptr, type);
   ptr = encode16(ptr, atr.sizeValue);
   ptr = encode(ptr, atr.value, atr.sizeValue);
   return ptr;
}


char* 
encodeAtrIntegrity(char* ptr, const StunAtrIntegrity atr)
{
   ptr = encode16(ptr, MessageIntegrity);
   ptr = encode16(ptr, 20);
   ptr = encode(ptr, atr.hash, sizeof(atr.hash));
   return ptr;
}

char* 
encodeAtrError(char* ptr, const StunAtrError atr)
{
   ptr = encode16(ptr, ErrorCode);
   ptr = encode16(ptr, 6 + atr.sizeReason);
   ptr = encode16(ptr, atr.pad);
   *ptr++ = atr.errorClass;
   *ptr++ = atr.number;
   ptr = encode(ptr, atr.reason, atr.sizeReason);
   return ptr;
}


char* 
encodeAtrUnknown(char* ptr, const StunAtrUnknown atr)
{
   ptr = encode16(ptr, UnknownAttribute);
   ptr = encode16(ptr, 2+2*atr.numAttributes);
   int i;
   for (i=0; i<atr.numAttributes; i++)
   {
      ptr = encode16(ptr, atr.attrType[i]);
   }
   return ptr;
}


char* 
encodeAtrAddress4(char* ptr, UInt16 type, const StunAtrAddress4 atr)
{
   ptr = encode16(ptr, type);
   ptr = encode16(ptr, 8);
   *ptr++ = atr.pad;
   *ptr++ = IPv4Family;
   ptr = encode16(ptr, atr.ipv4.port);
   ptr = encode32(ptr, atr.ipv4.addr);
	
   return ptr;
}


char* 
encodeAtrChangeRequest(char* ptr, const StunAtrChangeRequest atr)
{
   ptr = encode16(ptr, ChangeRequest);
   ptr = encode16(ptr, 4);
   ptr = encode32(ptr, atr.value);
   return ptr;
}

char* 
encodeXorOnly(char* ptr)
{
   ptr = encode16(ptr, XorOnly );
   return ptr;
}

char* 
encode(char* buf, const char* data, unsigned int length)
{
  memcpy(buf, data, length);
  return buf + length;
}

char* 
encode16(char* buf, UInt16 data)
{
   UInt16 ndata = htons(data);
   memcpy(buf, (void*)(&ndata), sizeof(UInt16));
   return buf + sizeof(UInt16);
}

char* 
encode32(char* buf, UInt32 data)
{
   UInt32 ndata = htonl(data);
   memcpy(buf, (void*)(&ndata), sizeof(UInt32));
   return buf + sizeof(UInt32);
}

/// return a random number to use as a port 
int
stunRandomPort()
{
   int min=0x4000;
   int max=0x7FFF;
	
   int ret = stunRand();
   ret = ret|min;
   ret = ret&max;
	
   return ret;
}


void
toHex(const char* buffer, int bufferSize, char* output) 
{
   static char hexmap[] = "0123456789abcdef";
	
   const char* p = buffer;
   char* r = output;
   int i;
   for (i=0; i < bufferSize; i++)
   {
      unsigned char temp = *p++;
		
      int hi = (temp & 0xf0)>>4;
      int low = (temp & 0xf);
		
      *r++ = hexmap[hi];
      *r++ = hexmap[low];
   }
   *r = 0;
}



