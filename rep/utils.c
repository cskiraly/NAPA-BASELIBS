#define LOG_MODULE "[rep] "
#include	"repoclient_impl.h"

/** Check the validity (i.e. it is open) of the handle from function fn */
int check_handle(HANDLE h, const char *fn) {
        struct reposerver *r = (struct reposerver *)h;

	if (!r) {
		error("Trying to use a NULL reposerver handle in fn %s", fn);
		return 0;
	}
	if (r->magic != REPOSERVER_MAGIC) {
		error("Trying to use a corrupt reposerver handle in fn %s", fn);
		return 0;
	}
	return 1;
}

const char *measurementrecord2str(const MeasurementRecord r) {
	return encode_measurementrecord(&r);
}


/** Helper callback for operations returning a string-list (i.e. char **). 
    Such ops are ListMeasurementNames and GetPeers
*/
void _stringlist_callback(struct evhttp_request *req,void *arg) {
	if (req == NULL || arg == NULL) return;
	request_data *cbdata = (request_data *)arg;

	void (*user_cb)(HANDLE rep, HANDLE id, void *cbarg, char **result, int nResults) = cbdata->cb;	
	struct reposerver *server = cbdata->server;
	HANDLE id = cbdata->id;
	/* maxresults is stored in data */
	int max = cbdata->data;
	if (max <= 0) max = 10000;

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
		if (user_cb) user_cb((HANDLE)server, id, cbdata->cbarg, NULL, 0);
		free(cbdata);
		return;
	}

	char **result = calloc(max, sizeof(char *));
	int i = 0;

	while (1) {
		char *line = evbuffer_readln(req->input_buffer, NULL, EVBUFFER_EOL_ANY);
		if (!line || i == max) break;
		result[i++] = strdup(line);
		if (i % 10000 == 0) {
			max += 10000;
			result = realloc(result, max * sizeof(char *));
		}
	}
	//info("cbarg in getpeers.c: %p", cbdata->cbarg);
	if (user_cb) user_cb((HANDLE)server, id, cbdata->cbarg, result, i);
	free(cbdata);
}

/** Free the contents of a MR */
void free_measurementrecord(MeasurementRecord *r) {
	if (r->originator) free(r->originator);
	if (r->targetA) free(r->targetA);
	if (r->targetB) free(r->targetB);
	if (r->published_name) free((void *)r->published_name);
	if (r->string_value) free(r->string_value);
        if (r->channel) free((void *)r->channel);
}

/** Copy a MeasurementRecord by strdup-ing members */
void copy_measurementrecord(MeasurementRecord *dst, const MeasurementRecord *src) {
	if (src->originator) dst->originator = strdup(src->originator);
	else dst->originator = NULL;

	if (src->targetA) dst->targetA = strdup(src->targetA);
	else dst->targetA = NULL;
	
	if (src->targetB) dst->targetB = strdup(src->targetB);
	else dst->targetB = NULL;

	if (src->published_name) dst->published_name = strdup(src->published_name);
	else dst->published_name = NULL;

        dst->value = src->value;

	if (src->string_value) dst->string_value = strdup(src->string_value);
	else dst->string_value = NULL;

        dst->timestamp = src->timestamp;;

	if (src->channel) dst->channel = strdup(src->channel);
	else dst->channel =  NULL;;
}



