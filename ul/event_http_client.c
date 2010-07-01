/**
 * @file event_http_client.c
 *
 * libevent-based simple http client.
 *
 * No threads, thus it is based on callbacks and timer events
 *
 * Napa-Wine project 2009-2010
 * @author: Giuseppe Tropea <giuseppe.tropea@lightcomm.it>
 * @author: Bakay Árpád <arpad.bakay@netvisor.hu>
 */

#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <netinet/in.h>

#include "chunk_external_interface.h"
#include "ul_commons.h"

int ulEventHttpClientSetup(const char *address, unsigned short port, struct evhttp_connection **htc) {
  struct evhttp_connection *local_htc = NULL;
  debug("Setting up event-based http client towards %s:%d", address, port);
  local_htc = evhttp_connection_base_new(eventbase, address, port);
  //WARN i am not sure that if anything goes wrong the pointer stays at NULL
  if(local_htc != NULL) {
    //fill return value
    *htc = local_htc;
    info("Event-based http client towards %s:%d has been setup", address, port);
    return UL_RETURN_OK;
  }
  else {
    error("Setup of event-based http client towards %s:%d FAILED", address, port);
    return UL_RETURN_FAIL;
  }
}


int ulEventHttpClientPostData(uint8_t *data, unsigned int data_len, struct evhttp_connection *htc, const char *addr, const int port, const char *path) {
  struct sockaddr_in *remote=NULL;
  int sock;
  int tmpres=0;
  char *post=NULL;
  int post_size=0;

  if((sock = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0){
    error("Can't create TCP socket");
    perror(NULL);
    return UL_RETURN_FAIL;
  }
  remote = (struct sockaddr_in *)malloc(sizeof(struct sockaddr_in *));
  if(remote == NULL) {
    error("Can't allocate memory for remote struct");
    close(sock);
    return UL_RETURN_FAIL;
  }
  remote->sin_family = AF_INET;
  tmpres = inet_pton(AF_INET, addr, (void *)(&(remote->sin_addr.s_addr)));
  if(tmpres < 0) {
    error("Can't set remote->sin_addr.s_addr");
    free(remote);
    close(sock);
    return UL_RETURN_FAIL;
  }
  else if(tmpres == 0) {
    fprintf(stderr, "%s is not a valid IP address\n", addr);
    free(remote);
    close(sock);
    return UL_RETURN_FAIL;
  }
  remote->sin_port = htons(port);

  if(connect(sock, (struct sockaddr *)remote, sizeof(struct sockaddr)) < 0){
    error("Could not connect");
    free(remote);
    close(sock);
    return UL_RETURN_FAIL;
  }

  if( (post_size = build_post_query(&post, addr, port, path, data, data_len)) == -1) {
    error("Could not allocate post buffer");
    free(remote);
    close(sock);
    return UL_RETURN_FAIL;
  }

  //Send the query to the server
  int sent = 0;
  while(sent < post_size) {
    tmpres = send(sock, post+sent, post_size-sent, 0);
    if(tmpres == -1){
      error("Can't http send post query");
      free(post);
      free(remote);
      close(sock);
      return UL_RETURN_FAIL;
    }
    sent += tmpres;
  }

  free(post);
  free(remote);
  close(sock);
  return UL_RETURN_OK;
}

int build_post_query(char **post, char *addr, int port, char *path, uint8_t *data, unsigned int data_len)
{
  char host[UL_URL_SIZE];
  char len[UL_URL_SIZE];
  char *header;
  char *postpath = path;
  char *tpl = "POST /%s HTTP/1.0\r\nHost: %s\r\nUser-Agent: Napa-WinePEER/0.1\r\nContent-Length: %s\r\n\r\n";
  int header_size=0;

  if(postpath[0] == '/') {
    //Removing leading slash
    postpath = postpath + 1;
  }
//  sprintf(host, "localhost:%d\0", port);
  sprintf(host, "%s:%d\0", addr, port);
  sprintf(len,"%d\0", data_len);
  // -6 is to consider the %s %s %s in tpl (6 extra chars)
  // the ending \0 is not to be accounted for since it gets overwritten by the data block
  header_size = strlen(host)+strlen(postpath)+strlen(len)+strlen(tpl)-6;
  if( ((*post) = (char *)malloc(header_size + data_len)) == NULL ) {
    return -1;
  }
  sprintf(*post, tpl, postpath, host, len);
  debug("post header is: %s", *post);
  memcpy(*post+header_size, data, data_len);
  return header_size + data_len;
}

/*
int ulEventHttpClientPostData(uint8_t *data, unsigned int data_len, struct evhttp_connection *htc, const char *addr, const int port, const char *path) {
  char nbuf[UL_URL_SIZE];
  char peer_addr[UL_URL_SIZE];
  uint16_t peer_port;
  struct evhttp_connection *local_htc = NULL;
  struct evhttp_request *reqobj = NULL;
  struct evbuffer *output_buffer;
	int ret = UL_RETURN_FAIL;

  local_htc = evhttp_connection_base_new(eventbase, addr, port);
  if(local_htc == NULL) {
    error("Setup of event-based http client towards %s:%d FAILED", addr, port);
    return UL_RETURN_FAIL;
  }

  //i hope passing two NULLs is ok for initializing
  reqobj = evhttp_request_new(NULL, NULL);
  if(reqobj != NULL) {
    info("http request container initialized");
  }
  else {
    error("http request container initialization FAILED");
    return UL_RETURN_FAIL;
  }
  reqobj->kind = EVHTTP_REQUEST;

  output_buffer = evhttp_request_get_output_buffer(reqobj);
  // put data into the request
  evbuffer_add(output_buffer, data, data_len);

  debug("prepared an http request for %d bytes", data_len);

  //fill up the request's headers
  //evhttp_connection_get_peer(htc, &peer_addr, &peer_port);
  sprintf(nbuf, "%s:%d\0", addr, port);
  evhttp_add_header(reqobj->output_headers, "Host", nbuf);
  debug("filled Host header: %s", nbuf);

  sprintf(nbuf,"%d\0", data_len);
  evhttp_add_header(reqobj->output_headers, "Content-Length", nbuf);
  debug("filled Content-Length header: %s", nbuf);

  //evhttp_make_request returns -1 in case of fail and 0 for success
  if( ret = evhttp_make_request(local_htc, reqobj, EVHTTP_REQ_POST, path) ) {
	  debug("make request failed");
  	evhttp_request_free(reqobj);
		evhttp_connection_free(local_htc);
  	return UL_RETURN_FAIL;
	}
  event_base_dispatch(eventbase);
	//evhttp_connection_free(local_htc);
	return ret;
}
*/
