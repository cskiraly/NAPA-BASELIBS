#define LOG_MODULE "[rep] "
#include        "repoclient_impl.h"

HANDLE repGetPeers(HANDLE rep, cb_repGetPeers cb, void *cbarg, int maxResults, Constraint *cons, int clen, 
	Ranking *ranks, int rlen, const char *ch) {
	if (!check_handle(rep, __FUNCTION__)) return NULL;
	debug("About to call getPeers with maxResults %d, constaints %s and rankings %s", 
		maxResults, constraints2str(cons, clen), rankings2str(ranks, rlen));

	char uri[10240];
	request_data *rd = (request_data *)malloc(sizeof(request_data));
	if (!rd) return NULL;
	rd->id = (void *)rd;
	rd->cb = cb;
	rd->cbarg = cbarg;
	rd->server = rep;
	rd->data = maxResults;

        sprintf(uri, "/GetPeers?maxresults=%d&%s%s%s",  
		maxResults, constraints2str(cons, clen), 
		(clen ? "&" : ""), rankings2str(ranks, rlen));
	if (ch) {
		if (clen + rlen > 0) strcat(uri, "&");
		strcat(uri, "channel=");
		strcat(uri, ch);
	}
	debug("Making getPeers request with URI %s", uri);
	
	make_request(uri, _stringlist_callback, (void *)rd);
	return (HANDLE)(rd);
}

const char *constraints2str(Constraint *cons, int len) {
	static char buf[1024];
	buf[0] = 0;
	if (len) strcat(buf, "constraints=");
	int i;

	for (i = 0; i != len; i++) {
		char tmp[1024] = "";

		if (i) strcat(buf, ";");
		if (cons[i].strValue) sprintf(tmp, "%s,%s", cons[i].published_name, cons[i].strValue);
		else sprintf(tmp, "%s,%f,%f", cons[i].published_name, (float)cons[i].minValue, (float)cons[i].maxValue);

		strcat(buf, tmp);
	}
	return buf;
}

const char *rankings2str(Ranking *ranks, int len) {
	static char buf[1024];
	buf[0] = 0;
	if (len) strcat(buf, "rankings=");
	int i;

	for (i = 0; i!= len; i++) {
	char tmp[128];

	if (i) strcat(tmp, ";");
	sprintf(tmp, "%s,%f", ranks[i].published_name, ranks[i].weight);
	strcat(buf, tmp);
	}
return buf;
}

