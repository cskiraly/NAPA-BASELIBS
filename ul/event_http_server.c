/**
 * @file event_http_server.c
 *
 * Simple http server for internal use.
 *
 * No threads, thus it is based on callbacks and timer events.
 *
 * Napa-Wine project 2009-2010
 * @author: Giuseppe Tropea <giuseppe.tropea@lightcomm.it>
 * @author: Bakay Árpád <arpad.bakay@netvisor.hu>
 */

#include <string.h>

#include "chunk_external_interface.h"
#include "http_default_urls.h"

int ulEventHttpServerSetup(const char *address, unsigned short port, data_processor_function data_processor) {
  struct evhttp* evh = NULL;
  debug("Setting up event-based http server listening at %s:%d", address, port);

  evh = evhttp_new(eventbase);

  if(evh != NULL) {
    info("Event-based http server at %s:%d has been setup", address, port);
  }
  else {
    error("Setup of event-based http server at %s:%d FAILED", address, port);
    return UL_RETURN_FAIL;
  }

  if(evhttp_bind_socket(evh, address, port) != -1) {
    info("Event-based http server socket bind with %s:%d OK", address, port);
  }
  else {
    error("Bind of event-based http server with %s:%d FAILED", address, port);
    return UL_RETURN_FAIL;
  }

  //when a request for a generic path comes to the server, trigger the ulEventHttpServerProcessRequest
  //function and also pass to it a pointer to an external data_processor function, which is
  //able to handle the received data specifically
  evhttp_set_gencb(evh, ulEventHttpServerProcessRequest, data_processor);
  return UL_RETURN_OK;
}

int ulEventHttpServerProcessRequest(struct evhttp_request *req, void *context) {
  struct evbuffer* input_buffer;
  const uint8_t *data;
  int data_len;
  char *path = req->uri;
  int ret;

  //cast the context as it is a data_processor_function
  data_processor_function data_processor = (data_processor_function)context;

  debug("HTTP request received for %s of type %d", req->uri, req->type);
  input_buffer = evhttp_request_get_input_buffer(req);
  data_len = evbuffer_get_length(input_buffer);
  if(req->type == EVHTTP_REQ_POST) {
    //extract the path of request
    if(!strncmp(path, UL_HTTP_PREFIX , strlen(UL_HTTP_PREFIX))) {  //if it begins by "http://"
      path = strchr(path + strlen(UL_HTTP_PREFIX),'/'); //skip "http://host:port" part
    }
    debug("HTTP POST request is for path %s", path);
    if(!strcmp(path, UL_DEFAULT_CHUNKBUFFER_PATH)) {
      //give back the data
      //should i copy it in order to "free" the req pointer??
      //no, because the data processr makes a copy
      //data = (uint8_t*)req->input_buffer;
      data = (const uint8_t *)evbuffer_pullup(input_buffer, data_len);
 
      //sending a reply to the client
      evhttp_send_reply(req, 200, "OK", NULL);
      //debug("HTTP REPLY OK");
      if(context != NULL) {
        //invoke the data_processor function, which returns -1 and 0 as well depending on outcome
        debug("HTTP server invoking the data processor");
        ret = (*data_processor)(data, data_len);
      }
      else {
        debug("HTTP server NOT invoking the data processor: NULL");
        ret = UL_RETURN_OK;
      }
    }
    else {
      evhttp_send_reply(req, 400, "BAD URI", NULL);
      error("HTTP REPLY BAD URI ");
      ret = UL_RETURN_FAIL;
    }
  }
  else if(req->type == EVHTTP_REQ_GET || req->type == EVHTTP_REQ_PUT) {
    error("Received GET or PUT HTTP request");
    evhttp_send_reply(req, 404, "NOT FOUND", NULL);
    ret = UL_RETURN_FAIL;
  }
  else {
    error("HTTP Unknown request");
    evhttp_send_reply(req, 404, "NOT FOUND", NULL);
    ret = UL_RETURN_FAIL;
  }
  //evbuffer_free(input_buffer);
  //evhttp_request_free(req);
  return ret;
}
