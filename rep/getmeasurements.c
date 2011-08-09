#define LOG_MODULE "[rep] "
#include        "repoclient_impl.h"

void _getMeasurements_callback(struct evhttp_request *req,void *arg);

HANDLE repGetMeasurements(HANDLE rep, cb_repGetMeasurements cb, void *cbarg, int maxResults, const char *originator, const char *targetA, const char *targetB, const char *name, const char *channel) {
	static int id = 1;
	if (!check_handle(rep, __FUNCTION__)) return NULL;

	debug("About to call repGetMeasurements, maxresults: %d, originator: %s, targetA: %s, targetB: %s, name: %s", maxResults, originator, targetA, targetB, name);

	char uri[1024];
	request_data *rd = (request_data *)calloc(sizeof(request_data),1);
	if (!rd) return NULL;
	rd->id = (void *)rd;
	rd->server = rep;
	rd->cb = cb;
	rd->data = maxResults;

        sprintf(uri, "/GetMeasurements?maxresults=%d", maxResults);
	char buf[256];
	if (originator) { sprintf(buf, "&originator=%s", originator); strcat(uri, buf); }
	if (targetA) { sprintf(buf, "&targetA=%s", targetA); strcat(uri, buf); }
	if (targetB) { sprintf(buf, "&targetB=%s", targetB); strcat(uri, buf); }
	if (name) { sprintf(buf, "&name=%s", name); strcat(uri, buf); }
	if (channel) { sprintf(buf, "&channel=%s", channel); strcat(uri, buf); }
	debug("URI: %s", uri);

	make_request(uri, _getMeasurements_callback, (void *)rd);
	return (HANDLE)(rd);
}

void _getMeasurements_callback(struct evhttp_request *req,void *arg) {
	if (req == NULL || arg == NULL) return;
	request_data *cbdata = (request_data *)arg;

	void (*user_cb)(HANDLE rep, HANDLE id, void *cbarg, MeasurementRecord *result, int nResults) = cbdata->cb;	
	HANDLE id = cbdata->id;
	HANDLE rep = cbdata->server;
	int max = cbdata->data;
	if (max <= 0) max = 10000;
	free(cbdata);

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
		if (user_cb) user_cb(rep, id, cbdata->cbarg, NULL, 0);
		return;
	}

	MeasurementRecord *result = calloc(max, sizeof(MeasurementRecord));
	int i = 0;
	char ts[1024];

	while (1) {
		char *line = evbuffer_readln(req->input_buffer, NULL, EVBUFFER_EOL_ANY);
		if (!line || i == max) break;

		debug("Line received (i: %d, max: %d): %s", i, max, line);
		result[i++] = parse_measurementrecord(line);

		if (i % 10000 == 0) {
			max += 10000;
			result = realloc(result, max * sizeof(MeasurementRecord));
		}
	}

	if (user_cb) user_cb(rep, id, cbdata->cbarg, result, i);
}

MeasurementRecord parse_measurementrecord(const char *line) {
#ifdef __MINGW32__
        extern char *strtok_r(char *str, const char *delim, char **savep); // mingw fails to define this
#endif
	MeasurementRecord r;
	char *work = strdup(line);
	char *fieldptr;

	r.originator = "(Unknown)";
	r.targetA = "(Unknown)";
	r.targetB = NULL;
	r.published_name = "(Unknown)";
	r.value = FP_NAN;
	r.string_value = NULL;
	r.channel = NULL;
	r.timestamp.tv_sec = 0;
	r.timestamp.tv_usec = 0;

	char *token = strtok_r(work, "&", &fieldptr);
	if (!token) {
		warn("Invalid measurement record received: %s\n", line);
		return r;
	}
	while (token) {
		char *eqptr;

		char *field = strdup(token);
		char *name = strtok_r(field, "=", &eqptr);
		char *value = strtok_r(NULL, "=", &eqptr);

		if (!strcmp("originator", name)) r.originator = strdup(value);
		else if (!strcmp("published_name", name)) r.published_name = strdup(value);
		else if (!strcmp("string_value", name)) r.string_value = strdup(value);
		else if (!strcmp("value", name)) {
			sscanf(value, "%lf", &r.value);
		}
		else if (!strcmp("targetA", name)) r.targetA = strdup(value);
		else if (!strcmp("targetB", name)) r.targetB = strdup(value);
		else if (!strcmp("channel", name)) r.channel = strdup(value);

		free(field);
		token = strtok_r(NULL, "&", &fieldptr);
	}
 	free(work);	
	return r;
}

