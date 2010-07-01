/*
 * % vim: syntax=c ts=4 tw=76 cindent shiftwidth=4
 *
 */

#define LOG_MODULE "[rep] "
#include	"repoclient_impl.h"

struct streambuffer publish_streambuffer = { NULL, NULL, 0};

const char *encode_measurementrecord(const MeasurementRecord *r) {
	rewind(publish_streambuffer.stream);

	fprintf(publish_streambuffer.stream, "originator=%s&", r->originator);
	if (r->targetA)
		fprintf(publish_streambuffer.stream, "targetA=%s&", r->targetA);
	if (r->targetB)
		fprintf(publish_streambuffer.stream, "targetB=%s&", r->targetB);
	fprintf(publish_streambuffer.stream, "published_name=%s", r->published_name);

	if (r->string_value)
		fprintf(publish_streambuffer.stream, "&string_value=%s", r->string_value);
	else if (r->value != (0.0/0.0)) 
		fprintf(publish_streambuffer.stream, "&value=%lf", r->value);

	if (r->channel) 
		fprintf(publish_streambuffer.stream, "&channel=%s", r->channel);

	if (r->timestamp.tv_sec + r->timestamp.tv_usec != 0) {
		fprintf(publish_streambuffer.stream, "&timestamp=%s",
				timeval2str(&(r->timestamp)));
	}

	fflush(publish_streambuffer.stream);
	/*debug("result: %s", publish_streambuffer.buffer);*/
	return publish_streambuffer.buffer;
}

void _publish_callback(struct evhttp_request *req,void *arg) {
	if (req == NULL || arg == NULL) return;
	request_data *cbdata = (request_data *)arg;
	struct reposerver *server = cbdata->server;
	cb_repPublish user_cb = cbdata->cb;
	HANDLE id = cbdata->id;
	free(cbdata);
	
	if (req->response_code != HTTP_OK) {
		warn("Failed PUBLISH operation (server %s:%hu,id %p, error is %d %s): %s", 
			server->address, server->port, id, req->response_code, req->response_code_line, req->uri);
		size_t response_len = evbuffer_get_length(req->input_buffer);
		if (response_len) {
			char *response = malloc(response_len + 1);
			evbuffer_remove(req->input_buffer, response, response_len);
			response[response_len] = 0;
			debug("Response string (len %d): %s", response_len, response);
			free(response);
		}
		if (user_cb) user_cb((HANDLE)server, id, cbdata->cbarg, req->response_code ? req->response_code : -1);
		return;
	}
	if (user_cb) user_cb((HANDLE)server, id, cbdata->cbarg,0);
}

HANDLE repPublish(HANDLE h, cb_repPublish cb, void *cbarg, MeasurementRecord *r) {
	static int id = 1;
	if (!check_handle(h, __FUNCTION__)) return NULL;
	struct reposerver *rep = (struct reposerver *)h;
	if (!r) {
		warn("Attempting to publish a NULL MeasurementRecord!");
		return 0;
	}
	if (isnan(r->value) && r->string_value == NULL) {
		warn("Attempting to publish an EMPTY record");
		return 0;
	}

	fprintf(stderr,"abouttopublish,%s,%s,%s,%s,%lf,%s,%s,%s\n", 
		r->originator, r->targetA, r->targetB, r->published_name, r->value, 
		r->string_value, r->channel, timeval2str(&(r->timestamp)));
	char uri[1024];
	char buf[1024];
	const char *encoded = encode_measurementrecord(r);

	request_data *rd = (request_data *)malloc(sizeof(request_data));
	if (!rd) return NULL;
	rd->id = (void *)rd;
	rd->cb = cb;
	rd->cbarg = cbarg;
	rd->server = (struct reposerver *)rep;

	if (rep->publish_delay) {
		if (rep->publish_buffer_entries >= PUBLISH_BUFFER_SIZE) {
			warn("Publish buffer overflow!");
			return;
		}
		int p = rep->publish_buffer_entries++;
		copy_measurementrecord(&(rep->publish_buffer[p].r), r);
		rep->publish_buffer[p].requestdata = rd;
	}
	else {
		sprintf(uri, "/Publish?%s", encode_measurementrecord(r));
		make_request(uri, _publish_callback, (void *)rd);
	}
	return (HANDLE)(rd);
}

/** libevent callback for deferred publishing */
void _batch_publish_callback(struct evhttp_request *req,void *arg) {
	if (req == NULL || arg == NULL) return;
	struct reposerver *rep = (struct reposerver *)arg;
	int response = req->response_code;

	if (response != HTTP_OK) {
		warn("Failed BATCH PUBLISH operation (server %s:%hu, error is %d %s)", 
			rep->address, rep->port, req->response_code, req->response_code_line);
		size_t response_len = evbuffer_get_length(req->input_buffer);
		if (response_len) {
			char *response = malloc(response_len + 1);
			evbuffer_remove(req->input_buffer, response, response_len);
			response[response_len] = 0;
			debug("Response string (len %d): %s", response_len, response);
			free(response);
		}
	}

	int i;
	for (i = 0; i != rep->in_transit_entries; i++) {
		request_data *cbdata = rep->in_transit[i].requestdata;
		cb_repPublish user_cb = cbdata->cb;
		HANDLE id = cbdata->id;
		free(cbdata);
		if (user_cb) user_cb((HANDLE)rep, id, cbdata->cbarg, response == 200 ? 0 : response);
		free_measurementrecord(&(rep->in_transit[i].r));
	}
	debug("Freeing up %d in-transit entries", rep->in_transit_entries);
	free(rep->in_transit);
	rep->in_transit = NULL;
	rep->in_transit_entries = 0;
}

/** Batch publish callback */
void deferred_publish_cb(evutil_socket_t fd, short what, void *arg) {
	struct reposerver *rep = (struct reposerver *)arg;

	if (rep->publish_buffer_entries) {
		debug("Deferred publish callback: %d entries to publish",
				rep->publish_buffer_entries);
		char *post_data = malloc(rep->publish_buffer_entries * 256);
		if (!post_data) fatal("Out of memory!");
		post_data[0] = 0;

		int i;
		for (i = 0; i != rep->publish_buffer_entries; i++) {
			strcat(post_data,
					encode_measurementrecord(&(rep->publish_buffer[i].r)));
			strcat(post_data, "\n");
		}

		make_post_request("/BatchPublish", post_data,
				_batch_publish_callback, rep);

		rep->in_transit = rep->publish_buffer;
		rep->in_transit_entries = rep->publish_buffer_entries;
		rep->publish_buffer_entries = 0;
		rep->publish_buffer = calloc(sizeof(struct deferred_publish),
				PUBLISH_BUFFER_SIZE);
		if (!rep->publish_buffer) fatal("Out of memory");

		free(post_data);

	}

	if (rep->publish_delay) {
		struct timeval t = { rep->publish_delay, 0 };
		event_base_once(eventbase, -1, EV_TIMEOUT, deferred_publish_cb, rep, &t); 
	}
}

