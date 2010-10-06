#define	LOG_MODULE "[rep] "
#include	"repoclient_impl.h"

/** Parse a server specification for repOpen */
int parse_serverspec(const char *server, char **addr, unsigned short *port);
/** Batch publish callback */
void deferred_publish_cb(evutil_socket_t fd, short what, void *arg);

void repInit(const char *config) {
#if !WIN32 && !MAC_OS
	if ((publish_streambuffer.stream = open_memstream(&publish_streambuffer.buffer, &publish_streambuffer.len)) 
		== NULL) fatal("Unable to initilize repoclient, file %s, line %d", __FILE__, __LINE__);
#else
	publish_streambuffer.buffsize = PUBLISH_BUFFER_SIZE;
	publish_streambuffer.buffer = malloc(publish_streambuffer.buffsize);
        if(publish_streambuffer.buffer == NULL) {
		publish_streambuffer.buffsize = 0;
		fatal("Unable to initilize repoclient, file %s, line %d", __FILE__, __LINE__);
	}
#endif
	else debug("The Repository client library is ready");
}

/** Initialize the repoclient instance by parsing the server spec string and setting up things */
HANDLE repOpen(const char *server, int publish_delay) { 
	struct reposerver *rep;

        if(server == NULL || strlen(server) == 0 || strcmp(server, "-") == 0) {
                warn("Repository publishing is disabled");
		return NULL;
	}

	rep = malloc(sizeof(struct reposerver));
	if (!rep) fatal("Out of memory while initializing repository client for %s", server);
	rep->magic=REPOSERVER_MAGIC;
	rep->publish_buffer = NULL;
	rep->publish_buffer_entries = 0;
	rep->publish_delay = publish_delay;
	rep->in_transit = NULL;
	rep->in_transit_entries = 0;
	parse_serverspec(server, &(rep->address), &(rep->port));

	info("Opening repository client %p to http://%s:%d", rep, rep->address,  rep->port);

	if ((rep->evhttp_conn = evhttp_connection_base_new(eventbase,  rep->address,  rep->port)) == NULL) 
		fatal("Unable to establish connection to %s:%d", rep->address, rep->port);

	if (publish_delay) {
		rep->publish_buffer = calloc(sizeof(struct deferred_publish), PUBLISH_BUFFER_SIZE);
		rep->publish_buffer_entries = 0;
		/* Schedule the batch publisher */
		struct timeval t = { publish_delay, 0 };
		event_base_once(eventbase, -1, EV_TIMEOUT, deferred_publish_cb, rep, &t);
	}

        debug("X.repOpen cclosing\n");
        return rep;
}

/** Close the repoclient instance and free resources */
void repClose(HANDLE h) {
	if (!check_handle(h, __FUNCTION__)) return;
	struct reposerver *rep = (struct reposerver *)h;

	debug("Closing repository client %p to %s:%hu", h, rep->address, rep->port);
	evhttp_connection_free(rep->evhttp_conn);
	rep->magic=0;
	if (rep->publish_buffer_entries && rep->publish_buffer) {
		while (rep->publish_buffer_entries) {
			free_measurementrecord(&(rep->publish_buffer[--rep->publish_buffer_entries].r));
		}
	}
	if (rep->publish_buffer) free(rep->publish_buffer);
	if (rep->in_transit_entries && rep->in_transit) {
		while (rep->in_transit_entries) {
			free_measurementrecord(&(rep->in_transit[--rep->in_transit_entries].r));
		}
	}
	if (rep->in_transit) free(rep->in_transit);
	free(rep);
}

HANDLE repListMeasurementNames(HANDLE rep, cb_repListMeasurementNames cb, void *cbarg, int maxResults, const char *ch) {
	static int id = 1;
	if (!check_handle(rep, __FUNCTION__)) return NULL;
	debug("About to call listMeasurementIds with maxResults %d", maxResults);

	char uri[1024];
	request_data *rd = (request_data *)malloc(sizeof(request_data));
	if (!rd) return NULL;
	rd->id = (void *)rd;
	rd->server = (struct reposerver *)rep;
	rd->cb = cb;
	rd->data = maxResults;

	sprintf(uri, "/ListMeasurementNames?maxResults=%d",  maxResults);
	make_request(uri, _stringlist_callback, (void *)rd);
	return (HANDLE)(rd);
}

void make_request(const char *uri, void (*callback)(struct evhttp_request *, void *), void *cb_arg) {
	struct evhttp_request *req = evhttp_request_new(callback, cb_arg);
	request_data *rd = (request_data *)cb_arg;

        if (!req) {
                error("Failed to create request object");
		return;
	}
        evhttp_add_header(req->output_headers, "Connection", "close");

        if (evhttp_make_request(rd->server->evhttp_conn, req, EVHTTP_REQ_GET, uri)) {
                warn("evhttp_make_request failed");
        }
}

int make_post_request(const char *uri, const char *data, void (*callback)(struct evhttp_request *, void *), void *cb_arg) {
	struct evhttp_request *req = evhttp_request_new(callback, cb_arg);
	struct reposerver *rep = (struct reposerver *)cb_arg;
	
        if (!req) {
                error("Failed to create request object");
		return -1;
	}

        if (evhttp_add_header(req->output_headers, "Connection", "close") < 0) {
                error("Failed to add header");
		return -2;
        }

	if (evbuffer_add(req->output_buffer, data, strlen(data) + 1) < 0) {
                error("Failed to add data to request");
		return -3;
	}

        if (evhttp_make_request(rep->evhttp_conn, req, EVHTTP_REQ_POST, uri) < 0) {
                warn("evhttp_make_request failed");
		return -4;
        }
        return 0;
}

/** Parse a server specification of the form addr:port */
int parse_serverspec(const char *rep, char **addr, unsigned short *port) {
	int p = 0;
	if (!rep) fatal("NULL repository is provided to repoclient, unable to continue");

	/* Find the address part first */
	while (rep[p] != ':' && rep[p] != 0) p++;
	*addr = strdup(rep);
	(*addr)[p] = 0;
	/* Parse the port spec, 80 if not specified */
	*port  = 80;
	if (rep[p] != 0 && sscanf(rep + p + 1, "%hu", port) != 1) {
		fatal("Unable to parse repository server specification \"%s\"", rep);
	}
	return 0;
}

#if 0

void _countPeers_callback(struct evhttp_request *req,void *arg) {
	if (req == NULL || arg == NULL) return;
	request_data *cbdata = (request_data *)arg;

	void (*user_cb)(HANDLE rep, HANDLE id, int result) = cbdata->cb;	
	HANDLE id = cbdata->id;
	free(cbdata);

	int result = -1;
	if (req->response_code != HTTP_OK) {
		warn("Failed repository operation (id %p, error is %d %s): %s", 
			id, req->response_code, req->response_code_line, req->uri);
		size_t response_len = evbuffer_get_length(req->input_buffer);
		if (response_len) {
			char *response = malloc(response_len + 1);
			evbuffer_remove(req->input_buffer, response, response_len);
			response[response_len] = 0;
			debug("Response string (len %d): %s", response_len, response);
			free(response);
		}
		if (user_cb) user_cb(client, id, result);
		return;
	}

	char *line = evbuffer_readln(req->input_buffer, NULL, EVBUFFER_EOL_ANY);
	if (line) sscanf(line, "%d", &result);
	if (user_cb && result >= 0) user_cb(client, id, result);
}

HANDLE countPeers(HANDLE rep, cb_countPeers cb, Constraint *cons, int clen) {

	static int id = 1;
	if (!check_handle(rep, __FUNCTION__)) return NULL;
	debug("About to call countPeers with  constaints %s", constraints2str(cons, clen, 0));

	char uri[10240];
	request_data *rd = (request_data *)malloc(sizeof(request_data));
	if (!rd) return NULL;
	rd->id = (void *)id;
	rd->cb = cb;

        sprintf(uri, "/CountPeers?%s",  constraints2str(cons, clen, 0));
	debug("Making countPeers request with URI %s", uri);
	make_request(uri, _stringlist_callback, (void *)rd);
	return (HANDLE)(id++);
}

#endif

