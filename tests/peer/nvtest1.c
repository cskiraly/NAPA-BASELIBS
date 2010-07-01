/*
 */
#include "grapes.h"
#include "grapes_log.h"
#include "chunk.h"
#include "ml.h"

#define URLPARTLEN 50
#define HTTP_PREFIX "http://"
#define DEFAULT_CHUNK_PATH "/NapaTest/chunk"


void  http_close_cb(struct evhttp_connection *conn, void *arg) {
      ev_uint16_t port;
      char* namebuf;
      evhttp_connection_get_peer(conn, &namebuf, &port);
fprintf(stderr, "HTTP Request complete: %s : %d\n", namebuf, port);

}

void http_request_cb(struct evhttp_request *req, void *arg) {
      struct evbuffer *inb = evhttp_request_get_input_buffer(req);
      fprintf(stderr, "HTTP Request (%s) callback %d, %s\n",
                      req->kind == EVHTTP_REQUEST ? "REQ" : "RESP",
                      req->response_code, req->response_code_line);
      char *cc;
      while((cc = evbuffer_readln(inb, NULL, EVBUFFER_EOL_ANY)) != NULL) {
        printf("Line X: %s!\n",cc);
      }

//	  evbuffer_free(req->output_buffer);
//      evhttp_request_free(req);
}

void chunk_delay_callback(struct chunk *chunk, void *arg) {
	struct evhttp_connection *htc = (struct evhttp_connection *)arg;
    struct evhttp_request *reqobj = evhttp_request_new (http_request_cb, "VAVAVAV");
	char uri[50], nbuf[10];

	evbuffer_add(reqobj->output_buffer,chunk->buf, chunk->len);

    reqobj->kind = EVHTTP_REQUEST;
	  {
			char nbuf[50];
			char *p;
			uint16_t port;
			evhttp_connection_get_peer(htc,&p,&port);
			sprintf(nbuf,"%s:%d",p, port);

			evhttp_add_header(reqobj->output_headers,"Host",nbuf);
			printf("Chunk transmission: %d %d %s\n", chunk->len, chunk->chunk_num, nbuf);
	  }
	sprintf(nbuf,"%d", chunk->len);
	evhttp_add_header(reqobj->output_headers,"Content-Length",nbuf);
	sprintf(uri,"%s/%d", DEFAULT_CHUNK_PATH, chunk->chunk_num);

    evhttp_make_request(htc, reqobj, EVHTTP_REQ_POST, DEFAULT_CHUNK_PATH);
}

/**
	* break up uri string to hostname, port and path-prefix 
	* valid formats is [[<address>]:]<port>[/[<path-prefix>]]
	* optional address and path-prefix fields, if not detected, are not assigned a value
	* output parameters may be NULL, meaning no assignment requested
	*
	* @param url the original string
	* @param host a buffer to receive the host part (URIPARTLEN bytes minimum). NULL means ignore.
	* @param port this will receive the port part. NULL means ignore.
	* @param path a buffer to receive the path part (URIPARTLEN bytes minimum). NULL means ignore.
	* @return the number of fields detected: 1: port only 2: hostname + port, 3 all fields.
*/


int     breakup_url(const char *url, char *host, int *port, char *path) {
			char hostpart[URLPARTLEN + 1], pathpart[URLPARTLEN + 1];
			int portpart = -1;

//			int fields = sscanf(url,"%"  #URLPARTLEN "[^:]:%d/%" #URLPARTLEN "s", hostpart, &portpart, pathpart);
			int fields = sscanf(url,"%50[^:]:%d/%50s", hostpart, &portpart, pathpart);
			if(fields < 2) {
				fields = sscanf(url + (*url == ':'),"%d",&portpart);
			}
			if( portpart <= 0 || portpart > 65535) return 0; 
			switch(fields) {
					break;
				case 3:
					if(path != NULL) { *path = '/'; strcpy(path+1, pathpart); }
				case 2:
					if(host != NULL) strcpy(host, hostpart);
				case 1:
					if(port != NULL) *port = portpart;
					break;
				default: return 0;
			}
			return fields;
}


/**
* Setup a HTTP ejector, a component that will transmit a delayed replay of chunks via HTTP post 
*
 * @param url   the <hostname>:<port>[/<pathprefix>] format URL. (<pathprefix> is currently unused)
 * @param delay requested delay in usec relative to the original timestamps.
 * @return -1 if error (i.e. params failed), 0 otherwise
*/

int	setup_http_ejector(const char const *url, int delay) {
		char host[URLPARTLEN+1]; 
		int port; 
		if(breakup_url(url, host, &port, NULL) < 2) return -1;

		info("Registering HTTP ejector toward %s:%d  (delay: %d)", host, port, delay);
        struct evhttp_connection *htc = evhttp_connection_base_new(eventbase, host, port);
        evhttp_connection_set_closecb(htc, http_close_cb, "DUMMY");
		chbuf_add_listener(delay, chunk_delay_callback, htc);
		return 0;
}


/**
 * Callback for http injector
 *
 * @param req contains info about the request
 * @param contains the path prefix
 */

static void http_receive(struct evhttp_request *req, void *dummy_arg) {
		char *path_pat = (char *)dummy_arg;
        char *path = req->uri;
fprintf(stdout, "Http request received: %s %d\n", req->uri, req->type);
        if(!strncmp(path, HTTP_PREFIX , strlen(HTTP_PREFIX))) {  // skip "http://host:port" part if present
                          path = strchr(path + strlen(HTTP_PREFIX),'/');
        }
        if(req->type == EVHTTP_REQ_POST || req->type == EVHTTP_REQ_PUT) {

            if(!strncmp(path, path_pat , strlen(path_pat))) {
                     int chunk_num;
                     int slashed;
                     path += strlen(path_pat);
                     if(slashed = (*path == '/'))  path++;
                     if(*path == '\0') {
                         chunk_num = chbuf_get_current() + 1;
                     }
                     else {
						 int p;
						 if(sscanf(path, "%u%n",&chunk_num, &p) != 1) { 
bad_url:
                                 evhttp_send_reply(req,400,"BAD URI",NULL);
								 fprintf(stdout,"Bad URL extension in [%s]. Required format: %s/<nnn>.\n", req->uri, path_pat);

                                 return;
                         }
						 if(p < strlen(path)) {
							 if(path[p] == '?') {
	// TODO: process parameters
							 }
							 else goto bad_url;
						 }
                     }
					 fprintf(stdout, "Contains data: %d bytes\n", evbuffer_get_length(req->input_buffer));

                     chbuf_enqueue_from_evbuf(req->input_buffer, chunk_num, NULL);
                     evhttp_send_reply(req,200,"OK",NULL);
					 return;
                }
        }
		fprintf(stdout,"Unknown HTTP request");
        evhttp_send_reply(req,404,"NOT FOUND",NULL);
}


/**
 *  Setup a HTTP injector, a component that will transmit a delayed replay of chunks via HTTP post 
 *  @todo TODO: return error if fisten failed
 * @param url   in <hostname>:<port>/<pathprefix> or <hostname>:<port> or <port> format (default for hostname is 0.0.0.0 and default for pathprefix is "/NapaTest/chunk"
 * @return -1 if error (url format failed) 0 otherwise

*/

int	setup_http_injector(const char const *url) {
		char host[URLPARTLEN+1] = "0.0.0.0";
		char *path = malloc(URLPARTLEN+1);
		int port; 

		sprintf(path, DEFAULT_CHUNK_PATH); 
		if(breakup_url(url, host, &port, path) < 1) return -1;

		printf("Starting HTTP injector listening on %s:%d for uri prefix is %s\n", host, port, path);

		struct evhttp* evh = evhttp_new(eventbase);
		evhttp_bind_socket(evh,host,port);
		evhttp_set_gencb(evh, http_receive, path);
		return 0;
}


/** ********************** MAIN ************************ */


void receive_local_socketID_cb(socketID_handle local_socketID,int errorstatus){

  char buf[100];
  mlSocketIDToString(local_socketID,buf, 100);

    printf("--Monitoring module: received a local ID <%s>\n", buf);

}

static connID;
#define CHUNK_MSG 15

void chunk_transmit_callback(struct chunk *chunk, void *arg) {
	send_params sParams;
	char headerbuf[100];
	send_all_data_container buflist;
	debug("Sending chunk with data of length: %d", chunk->len);
	sprintf(headerbuf, "CHUNK: CHUNK_NUM=%d LEN=%d LEN=%ld.%ld\n\n", chunk->chunk_num, chunk->len, chunk->rectime.tv_sec, chunk->rectime.tv_usec);
	buflist.buffer_1 = headerbuf;
	buflist.length_1 = strlen(headerbuf);
	buflist.buffer_2 = (char *)chunk->buf;
	buflist.length_2 = chunk->len;
	mlSendAllData((int)arg, &buflist, 2,CHUNK_MSG,&sParams);
}

/**
  * A callback function that tells a connection has been established. 
  * @param connectionID The connection ID
  * @param *arg An argument for data about the connection 
  */
void receive_outconn_cb (int connectionID, void *arg){

      info("ML: Successfully set up outgoing connection connectionID %d  \n",connectionID );

#if 0 
      /// transmit data to peer 2  
      int size = 10000;
      char buffer[size];
      memset(buffer,'a',size);
      unsigned char msgtype = 12;
      send_params sParams;
 
      mlSendData(connID,buffer,size,msgtype,&sParams);
#endif

	chbuf_add_listener(0, chunk_transmit_callback, (void *)connID);
}


/**
  * A callback function that tells a connection has been established. 
  * @param connectionID The connection ID
  * @param *arg An argument for data about the connection 
  */
void receive_conn_cb (int connectionID, void *arg){

  printf("ML: Received incoming connection connectionID %d  \n",connectionID );
  connID=connectionID;
} 

/**
  * A funtion that prints a connection establishment has failed
  * @param connectionID The connection ID
  * @param *arg An argument for data about the connection
  */
void conn_fail_cb(int connectionID,void *arg){

  printf("--echoServer: ConnectionID %d  could not be established \n ",connectionID);

}

/**
  * The peer receives data per callback from the messaging layer. 
  * @param *buffer A pointer to the buffer
  * @param buflen The length of the buffer
  * @param msgtype The message type 
  * @param *arg An argument that receives metadata about the received data
  */
void recv_data_from_peer_cb(char *buffer,int buflen,unsigned char msgtype,void *arg){
  info("CLIENT_MODE: Received data from someone:  msgtype %d buflen %d",msgtype,buflen ); 
}

void recv_chunk_from_peer_cb(char *buffer,int buflen,unsigned char msgtype,void *arg){
  debug("Received chunk from callback: msgtype %d buflen %d",msgtype,buflen ); 
  int chunk_num, chunk_length;
  struct timeval timestamp;
  char c1;
  int headlen;
  if(sscanf(buffer, "CHUNK: CHUNK_NUM=%d LEN=%d LEN=%ld.%ld%*1[\n]%c%n", 
	  &chunk_num, &chunk_length, &(timestamp.tv_sec), &(timestamp.tv_usec), &c1, &headlen) != 5 || c1 != '\n') {
	  warn("WARN!!! received inconsistent chunk with bad header format: %30.30s\n",buffer); 
	  return;
  }
  else if(buflen != headlen + chunk_length ) {
	   warn("WARN!!! received inconsistent chunk with bad length: %d + %d <--> %d \n", headlen, chunk_length, buflen ); 
	  return;
  }
  else {
      chbuf_enqueue_chunk(chunk_num, chunk_length, &timestamp, buffer+headlen);
  }
}

