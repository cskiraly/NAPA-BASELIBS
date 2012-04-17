/*
 * Compile with:
 * gcc  -o test test.c chunk.c -levent
 */
#include "chunk.h"

#ifndef DISABLE_ML
#include "ml.h"
void receive_outconn_cb (int connectionID, void *arg);
#endif

#define URLPARTLEN 50
#define HTTP_PREFIX "http://"
#define DEFAULT_CHUNK_PATH "/NapaTest/chunk"


struct event_base *evb;

/** invoked when HTTP client session is closed
 *  this is superfluous right now, may be removed 
 */ 
void  http_close_cb(struct evhttp_connection *conn, void *arg) {
      ev_uint16_t port;
      char* namebuf;
      evhttp_connection_get_peer(conn, &namebuf, &port);
fprintf(stderr, "HTTP Request complete: %s : %d\n", namebuf, port);

}

/** invoked when HTTP client session is answered
 *  apart from debugging, this is superfluous right now and may be removed 
 */ 
void http_request_cb(struct evhttp_request *req, void *arg) {
      struct evbuffer *inb = evhttp_request_get_input_buffer(req);
      fprintf(stderr, "HTTP Request (%s) callback %d, %s\n",
                      req->kind == EVHTTP_REQUEST ? "REQ" : "RESP",
                      req->response_code, req->response_code_line);
      char *cc;
      while((cc = evbuffer_readln(inb, NULL, EVBUFFER_EOL_ANY)) != NULL) {
        printf("Line X: %s!\n",cc);
      }

}

void chunk_to_http_post_callback(struct chunk *chunk, void *arg) {
	struct evhttp_connection *htc = (struct evhttp_connection *)arg;
    struct evhttp_request *reqobj = evhttp_request_new (http_request_cb, "VAVAVAV");
	char uri[50], nbuf[10];

	printf("HTTP EJECT %d\n", chunk->len);
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
			if(!strncmp(url, "http://", 7)) url += 7;

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
 * @param delay requested delay in sec relative to the original timestamps.
 * @return -1 if error (i.e. params failed), 0 otherwise
*/

int	setup_http_ejector(const char const *url, double delay) {
		char host[URLPARTLEN+1]; 
		int port; 
		if(breakup_url(url, host, &port, NULL) < 2) return -1;



		printf("Registering HTTP ejector toward %s:%d  (delay: %f)\n", host, port, delay);
		struct evhttp_connection *htc = evhttp_connection_base_new(evb, NULL, host, port); // NULL -> dns
        evhttp_connection_set_closecb(htc, http_close_cb, "DUMMY");
// TODO: THIS SEEMS LOGICAL, BUT CAUSES COREDUMP
//		evhttp_connection_set_timeout(htc,1);
		chbuf_add_listener((int)(delay*1000000), chunk_to_http_post_callback, htc);
		return 0;
}


/**
* send a test event of 50 bytes to the specified destination
*
* @param addr: address or hostname
* @param port: port number
* @param addr: url prefix (/1111) is appended in request
* 
*/
void send_test_http_request(const char *addr, int port, char *pprefix) {
		fprintf(stdout, "Sending HTTP request to %s:%d\n", addr, port);
                  int res;
                  struct evhttp_connection *htc = evhttp_connection_base_new(evb, NULL, addr, port); // NULL -> dns
                  evhttp_connection_set_closecb(htc, http_close_cb, "ABCABCT");
                  struct evhttp_request *reqobj = evhttp_request_new (http_request_cb, "VAVAVAV");
                  reqobj->kind = EVHTTP_REQUEST;

				  evbuffer_add(reqobj->output_buffer,"aaaaaaaaa aaaaaaaaa aaaaaaaaa aaaaaaaaa aaaaaaaaa ",50);

				  {
						char nbuf[50];
						char *p;
						uint16_t port;
						evhttp_connection_get_peer(htc,&p,&port);
						sprintf(nbuf,"%s:%d",p, port);

						evhttp_add_header(reqobj->output_headers,"Host",nbuf);
				  }
                  evhttp_add_header(reqobj->output_headers,"Content-Length","50");
              //    evbuffer_add(reqobj->output_buffer, "\n\n",2);

				  char path[100];
				  sprintf(path, "%s/1111", pprefix);
                  evhttp_make_request(htc, reqobj, EVHTTP_REQ_POST, path);
}

	
static double parse_double_param(const char *param, double def_val) {
					double ret = def_val;
					if(param != NULL) {
						ret = atof(param);
						if(ret == 0 && strncmp("0.00000000", param, strlen(param))) {
							ret = def_val;
						}
					}
					return ret;
}

#include <sys/queue.h>
#include <event2/event_struct.h>

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
		// TODO is it good?
		struct evkeyvalq params;
		evhttp_parse_query(req->uri, &params);
#if(1)
		{
		struct evkeyval *header;
        TAILQ_FOREACH(header,  &params, next) {
			printf("Param %s : %s\n", header->key, header->value);

        }
		}
#endif
        if(req->type == EVHTTP_REQ_POST || req->type == EVHTTP_REQ_PUT) {

            if(!strncmp(path, path_pat , strlen(path_pat))) {
				struct timeval tv = {0,0};
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

                                 goto finally;
                         }
						 if(p < strlen(path)) {
							 if(path[p] == '?') {
								 const char *time = evhttp_find_header(&params,"timestamp");
								 if(time != NULL) {
									 if(isdigit(time[0])) {
									 double t = parse_double_param(time, 0);
									 if(t > 0.0) {
										 tv.tv_sec = (unsigned int)t;
										 tv.tv_usec = (unsigned int)(1000000.0 * (t - tv.tv_sec));
									 }
									 }
									 else {
										 struct tm tmm;
										 strptime(time, "%a, %y %b %d %T GMT", &tmm);
										 time_t ltime = mktime(&tmm);
										 if(ltime > 0) tv.tv_sec = ltime;
									 }
								 }
							 }
							 else goto bad_url;
						 }
                     }
					 fprintf(stdout, "Contains data: %d bytes\n", evbuffer_get_length(req->input_buffer));

					 if(tv.tv_sec > 0) {
						 char buf[100];
   			 		     strftime(buf, 100, "%a, %y %b %d %T GMT", gmtime(&tv.tv_sec));
						 printf("Request contains time: %ld.%ld -> %s\n", tv.tv_sec, tv.tv_usec, buf);
					 }
					 chbuf_enqueue_from_evbuf(req->input_buffer, chunk_num, tv.tv_sec > 0 ? &tv : NULL);
                     evhttp_send_reply(req,200,"OK",NULL);
					 goto finally;
                }
        }
        if(req->type == EVHTTP_REQ_POST || req->type == EVHTTP_REQ_GET) {
#define REGFORM_FILE_NAME "register-receptor.html"
			char *register_path = "/register-receptor";
            if(!strncmp(path, register_path , strlen(register_path))) {
				const char *url = evhttp_find_header(&params,"url");
				if(url != NULL) {
					double delay = parse_double_param(evhttp_find_header(&params,"delay"), 5.0);
					double validity = parse_double_param(evhttp_find_header(&params,"validity"), 300);
					if(setup_http_ejector(url,delay)) {
						printf("Could not start up ejector");
						evhttp_send_reply(req,305,"REQUEST PROCESSING FAILED",NULL);
						goto finally;
					}
					evbuffer_add_printf(req->output_buffer,"<html><head><title>NAPA - Registration</title></head>"
						"<body><h3>Successfully registered to %s (delay: %f)</h3></body></html>", url, delay);
				}
				else {
					int fd = open(REGFORM_FILE_NAME,O_RDONLY);
					if(fd > 0) evbuffer_add_file(req->output_buffer, fd, 0, 10000);
					else evbuffer_add_printf(req->output_buffer,"<html><head><title>NAPA - Error</title></head>"
						"<body><h3>Error!! Cannot load file %s</h3></body></html>", REGFORM_FILE_NAME);
				}
				evhttp_send_reply(req,200,"OK",NULL);
				goto finally;
			}
		}
		fprintf(stdout,"Unknown HTTP request");
        evhttp_send_reply(req,404,"NOT FOUND",NULL);

finally:
		evhttp_clear_headers(&params);
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

		struct evhttp* evh = evhttp_new(evb);
		evhttp_bind_socket(evh,host,port);
		evhttp_set_gencb(evh, http_receive, path);
		return 0;
}


/**
 * Keyboard command listening loop
 *
 * Commands available:
 *	i [<address>:]<port>[/<prefix>] : start a HTTP injector
 *	e <address>:<port>[/<prefix>] : start a HTTP ejector
 * 	s <address>:<port>/<prefix> : send a test message
 *	q : quit
 *	h : help
 *
 * @param fd this is normally the stdin
 * @param unused_event is unused
 * @param contains the event
 */

void
stdin_read(int fd, short unused_event, void *arg)
{
	char buf[255];
	int len;
	struct event *ev = arg;


	len = read(fd, buf, sizeof(buf) - 1);

	if (len == -1) {
		perror("read");
		return;
	} else if (len == 0) {
		fprintf(stderr, "Connection closed\n");
		event_del(ev);

		return;
	}

	buf[len] = '\0';
	if(buf[0] == 'i') {
		char url[100];
		sprintf(url,"localhost:9955");
		sscanf(buf+1," %s",url);
		if(setup_http_injector(url)) printf("Could not start up injector");
	}
	else if(buf[0] == 'e') {
		double delay = 5.0;
		char url[100];
		sprintf(url,"localhost:9955");
		sscanf(buf+1," %s %lf",url, &delay);
		if(setup_http_ejector(url,delay)) printf("Could not start up ejector");
	}
	else if(buf[0] == 's') {
		char host[URLPARTLEN+1] = "0.0.0.0";
		char path[URLPARTLEN+1] = DEFAULT_CHUNK_PATH;
		int port;
		if(breakup_url(buf+2, host, &port, path) < 2) printf("s command requires a url: <host>:<port>[/<pathprefix>] !\n");
		else send_test_http_request(host, port, path);
	}
#ifndef DISABLE_ML
	else if(buf[0] == 'c') {
		  socketID_handle remote_socketID = malloc(SOCKETID_SIZE);
		  mlStringToSocketID(buf+2, remote_socketID);
      		  send_params sParams;
		  if(mlOpenConnection(remote_socketID,&receive_outconn_cb,"DUMMY ARG",sParams) != 0) {
		    printf("ML open_connection failed\n");
		  }
	}
#endif
	else if(buf[0] == 'q') {
		fprintf(stdout, "Bye!\n", buf);
		exit(-1);
//		event_del(ev);
	}
	else if(buf[0] == 'h') {
		printf("Commands:\n");
		printf(" q: quit\n");
		printf(" i [<address>:]<port>[/<prefix>] : start a HTTP injector \n");
		printf(" e <address>:<port>[/<prefix>] [<delay>]: start a HTTP ejector. Delay is in seconds, default=5.0 .\n");
		printf(" s <address>:<port>/<prefix> : send a test message \n");
		printf(" c <remote_socket_id>        : connect to peer and forward all chunks to it\n");
		printf(" h : help\n");
	}
	else fprintf(stdout, "Unknown command: %s, use 'h' for help\n", buf);
}




/** ********************** MESSAGING LAYER OPERATIONS ************************ */

#ifndef DISABLE_ML

void receive_local_socketID_cb(socketID_handle local_socketID,int errorstatus){

  char buf[100];
  mlSocketIDToString(local_socketID,buf, 100);

    printf("--Monitoring module: received a local ID <%s>\n", buf);

}

static connID;
#define CHUNK_MSG 15

void chunk_transmit_callback(struct chunk *chunk, void *arg) {
	printf("Sending chunk with data of length: %d\n", chunk->len);
	mlSendData((int)arg,(char *)chunk,sizeof(struct chunk) + chunk->len,CHUNK_MSG,NULL);
}


/**
  * A callback function that tells a connection has been established.
  * @param connectionID The connection ID
  * @param *arg An argument for data about the connection
  */
void receive_outconn_cb (int connectionID, void *arg){

      printf("ML: Successfully set up outgoing connection connectionID %d  \n",connectionID );

      /// transmit data to peer 2
      int size = 10000;
      char buffer[size];
      memset(buffer,'a',size);
      unsigned char msgtype = 12;
      //strcpy(buffer, "Message for peer 2 \n");
  
      mlSendData(connID,buffer,size,msgtype,NULL);

   	  chbuf_add_listener(1000000, chunk_transmit_callback, (void *)connID);

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

  printf("--echoServer: Received data from callback: \n\t \t msgtype %d \t buflen %d \n",msgtype,buflen ); 

}

void recv_chunk_from_peer_cb(char *buffer,int buflen,unsigned char msgtype,void *arg){
  printf("--echoServer: Received chunk from callback: \n\t \t msgtype %d \t buflen %d \n",msgtype,buflen ); 
  if(buflen != ((struct chunk *)buffer)->len + sizeof(struct chunk)) {
	  printf("ERROR!!! received inconsistent chunk with bad length: %d <--> %d \n",((struct chunk *)buffer)->len,buflen ); 
  }
  chbuf_enqueue_chunk((struct chunk *)buffer);
}
#endif

/**
 * Main program for simple transmission tests.
 *
 * Sets up a HTTP injector based on the -i option (<address>:<port>, but typically 0.0.0.0:<port>)
 * sets up a HTTP ejector based on the argument s to the -e option. Args: <address>:<port>[/<prefix>] <delay_in_usec>
 * by default, no sender or receiver is set up
 *
 * @param argc cmdline arguments count (includes program name)
 * @param argv cmdline arguments (argv[0] is the program name)
 * @return command execution return value
 */

int main (int argc, char **argv) {
		evb = event_base_new();


		/* Initalize stdin event loop */
		{
			struct event *evfifo;
			evfifo = event_new(evb,0, EV_READ | EV_PERSIST, stdin_read, &evfifo);

		/* Add it to the active events, without a timeout */
			event_add(evfifo, NULL);
			fprintf(stdout, "Console Command Interpreter. Enter 'h' for help!\n");

		}

		int argc_pos = 1;
		while(argc > argc_pos) {
			if(argc >= argc_pos + 2 && !strcmp(argv[argc_pos],"-i")) {
			/* Initialize HTTP listener */
				char host[50];
				int port;

				breakup_url(argv[argc_pos +1], host, &port, NULL);
				setup_http_injector(argv[argc_pos+1]);

				argc_pos += 2;
			}

			else if(argc >= argc_pos + 3  && !strcmp(argv[argc_pos],"-e")) {
				double delay = 5.0;
				sscanf(argv[argc_pos+2],"%lf",&delay);
				setup_http_ejector(argv[argc_pos+1],delay);
				argc_pos += 3;
			}
#ifndef DISABLE_ML
			else if(argc >= argc_pos + 2  && !strcmp(argv[argc_pos],"-m")) {
				struct timeval timeout_value = { 3, 0 };

				mlInit(true,timeout_value,atoi(argv[argc_pos+1]),strdup(mlAutodetectIPAddress()),3478,"stun.ideasip.com",&receive_local_socketID_cb,evb);
			    mlRegisterRecvConnectionCb(&receive_conn_cb);
                mlRegisterErrorConnectionCb(&conn_fail_cb);
			    mlRegisterRecvDataCb(&recv_data_from_peer_cb,12);
			    mlRegisterRecvDataCb(&recv_chunk_from_peer_cb,15);
			    argc_pos += 2;
			}
#endif
			else {
				printf("Usage: %s [-i <listen-url> ] [-m <port>] [ -e <transmit-url> [<transmit-delay>] ]", argv[0]);
				printf("	-m: startup Messaging Layer on given UDP port\n");
				printf("	-i: startup a HTTP injector\n");
				printf("	-e: startup a HTTP ejector with delay as specified. Delay is in seconds, default 5.0 .\n");
				exit(-1);
			}
		}

		char buf[100];
		time_t ppp;
		time(&ppp);
		strftime(buf, 100, "%a, %y %b %d %T GMT", gmtime(&ppp));
		printf("Time now: (%s)\n",buf);

		event_base_dispatch(evb);
		return (0);
}

