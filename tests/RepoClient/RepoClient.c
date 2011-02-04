#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>

#include	<confuse.h>
#include        <event2/event.h>
#include        <event2/buffer.h>
#include        <event2/http.h>
#include        <event2/http_struct.h>

#include	<napa.h>
#include	<napa_log.h>
#include	<repoclient.h>
#include	<ml.h>

#define	PUBLISH		"Publish"
#define LISTNAMES	"ListMeasurementNames"
#define GETPEERS	"GetPeers"
#define COUNTPEERS	"CountPeers"
#define GETMEASUREMENTS "GetMeasurementRecords"

struct event_base *eventbase;

/* Configuration file parsing instructions. see repoclient_test.cfg for an example */
cfg_opt_t opts_listMeasurementNames[] = {
	CFG_INT("maxresults", 0, CFGF_NONE),
	CFG_STR("channel", NULL, CFGF_NONE),
	CFG_END()
};
cfg_opt_t opts_publish[] = {
	CFG_STR("originator", "", CFGF_NONE),
	CFG_STR("targetA", "?", CFGF_NONE),
	CFG_STR("targetB", NULL, CFGF_NONE),
	CFG_STR("name", "", CFGF_NONE),
	CFG_FLOAT("value", 0.0, CFGF_NONE),
	CFG_STR("string_value", NULL, CFGF_NONE),
	CFG_STR("timestamp", "now", CFGF_NONE),
	CFG_STR("channel", NULL, CFGF_NONE),
	CFG_END()
};
cfg_opt_t opts_constraint[] = {
	CFG_STR("name", "", CFGF_NONE),
	CFG_STR("string_value", "", CFGF_NONE),
	CFG_FLOAT("min", 0.0, CFGF_NONE),
	CFG_FLOAT("max", 0.0, CFGF_NONE),
	CFG_END()
};
cfg_opt_t opts_ranking[] = {
	CFG_STR("name", "", CFGF_NONE),
	CFG_FLOAT("weight", 1.0, CFGF_NONE),
	CFG_END()
};
cfg_opt_t opts_getpeers[] = {
	CFG_INT("maxresults", 0, CFGF_NONE),
	CFG_SEC("constraint", opts_constraint, CFGF_MULTI),
	CFG_SEC("ranking", opts_ranking, CFGF_MULTI),
	CFG_STR("channel", NULL, CFGF_NONE),
	CFG_END()
	
};
cfg_opt_t opts_getmeasurements[] = {
	CFG_INT("maxresults", 0, CFGF_NONE),
	CFG_STR("originator", NULL, CFGF_NONE),
	CFG_STR("targetA", NULL, CFGF_NONE),
	CFG_STR("targetB", NULL, CFGF_NONE),
	CFG_STR("name", NULL, CFGF_NONE),
	CFG_STR("channel", NULL, CFGF_NONE),
	CFG_END()
};
cfg_opt_t opts_test[] = {
	CFG_SEC(LISTNAMES, opts_listMeasurementNames, CFGF_MULTI),
	CFG_SEC(PUBLISH, opts_publish, CFGF_MULTI),
	CFG_SEC(GETPEERS, opts_getpeers, CFGF_MULTI),
	CFG_SEC(GETMEASUREMENTS, opts_getmeasurements, CFGF_MULTI),
	CFG_END()
};
cfg_opt_t opts_repospec[] = {
	CFG_STR("server", "192.168.35.203:9832", CFGF_NONE),
	CFG_END()
};
cfg_opt_t repotest_opts[] = {
	CFG_SEC("repository", opts_repospec, CFGF_NONE),
	CFG_SEC("test", opts_test, CFGF_MULTI),
	CFG_END()
};

int tests_done = 0;
struct timeval parse_timestamp(const char *ts);

/** Helper to print a string list */
const char *print_list(char **list, int n, bool should_free) {
	static char buffer[4096];
	strcpy(buffer, "[ ");
	int i;
	for (i = 0; i != n; i++) {
		if (i) strcat(buffer, ", ");
		strcat(buffer, list[i]);
		if (should_free) free(list[i]);
	}
	strcat(buffer, " ]");
	if (should_free) free(list);
	return buffer;
}

void getMeasurements_callback(HANDLE client, HANDLE id, void *cbarg, MeasurementRecord *result, int n) {
	tests_done++;
	info("GetMeasurements id %p done, size: %d", id, n);
	if (!n) return;
	int i;
	for (i = 0; i != n; i++) {
		debug("i: %d, channel %p", i, result[i].channel);
		info("Record %d: %s", i+1, measurementrecord2str(result[i]));
	}
	free(result);
}

void listMeasurementNames_callback(HANDLE client, HANDLE id, void *cbarg, char **result, int n) {
	tests_done++;
	info("ListMeasurementIds id %p done, result: %s", id, print_list(result, n, 1));
}

void getPeers_callback(HANDLE client, HANDLE id, void *cbarg, char **result, int n) {
	tests_done++;
	info("GetPeers id %p done, result: %s", id, print_list(result, n, 1));
}

void publish_callback(HANDLE client, HANDLE id, void *cbarg, int result) {
	info("Publish done(id:%p), result %d", id, result);
	tests_done++;
}

int ntests;
cfg_t *cfg;
HANDLE repoclient;
char *configfile;

void do_tests(HANDLE h, void *arg);

int main(int argc, char *argv[]) {
	/* Check for usage (arguments) */
	if (argc < 2) {
		fprintf(stderr, "Usage: %s repotest.cfg\n", argv[0]);
		exit(1);
	}
	/* Initialize libevent and logging */
	eventbase = event_base_new();
	napaInitLog(LOG_DEBUG, NULL, NULL);
	repInit("");

	/* Parse config file and init repoclient with the "repository" cfg section */
	configfile = argv[1];
	cfg = cfg_init(repotest_opts, CFGF_NONE);
	if(cfg_parse(cfg, configfile) == CFG_PARSE_ERROR)
		fatal("Unable to parse config file %s", configfile);

	repoclient = repOpen(cfg_getstr(cfg_getsec(cfg, "repository"), "server"),3);
	if (repoclient == NULL) fatal("Unable to initialize repoclient");

	ntests = cfg_size(cfg, "test");
	if (!ntests) fatal("No tests specified, exiting");

	struct timeval t = { 1, 0 };
	napaSchedulePeriodic(&t, 1.0/5.0, do_tests, NULL);
	do_tests(NULL, NULL);

	repClose(repoclient);

	cfg_free(cfg);
	event_base_free(eventbase);
}

void do_tests(HANDLE h, void *arg) {
	int i;
	for (i = 0; i != ntests; i++) {
		cfg_t *test = cfg_getnsec(cfg, "test", i);

		if ((cfg_size(test, PUBLISH) + cfg_size(test,LISTNAMES) + cfg_size(test,GETPEERS) + 
			cfg_size(test,GETMEASUREMENTS)) != 1)
			fatal("A test clause must contain exactly one action in %s", configfile);

		info("Performing test %d/%d", i+1, ntests);
		cfg_t *action;

		if ((action = cfg_getsec(test, PUBLISH)) != NULL) {
			char AsocketID[SOCKETID_SIZE] = "Unknown";
			char BsocketID[SOCKETID_SIZE] = "Unknown";
			MeasurementRecord r;
			r.originator = cfg_getstr(action, "originator");
			r.targetA = cfg_getstr(action, "targetA");
			r.targetB = cfg_getstr(action, "targetB");
			r.published_name = cfg_getstr(action, "name");
			r.value = cfg_getfloat(action, "value");
			r.string_value = cfg_getstr(action, "string_value");
			r.channel = cfg_getstr(action, "channel");
			r.timestamp = parse_timestamp(cfg_getstr(action, "timestamp"));
			repPublish(repoclient, publish_callback, NULL, &r);
		}
		if ((action = cfg_getsec(test, LISTNAMES)) != NULL) {
			repListMeasurementNames(repoclient, listMeasurementNames_callback, NULL, 
				cfg_getint(action, "maxresults"), cfg_getstr(action, "channel"));
			continue;
		}
		if ((action = cfg_getsec(test, GETMEASUREMENTS)) != NULL) {
			repGetMeasurements(repoclient, getMeasurements_callback, NULL, 
				cfg_getint(action, "maxresults"),
				cfg_getstr(action, "originator"), cfg_getstr(action, "targetA"), 
				cfg_getstr(action, "targetB"), cfg_getstr(action, "name"),
				cfg_getstr(action, "channel"));
			continue;
		}
		if ((action = cfg_getsec(test, GETPEERS)) != NULL) {
			int ncons = cfg_size(action, "constraint");
			int nranks = cfg_size(action, "ranking");
			Constraint *cons = NULL;
			Ranking *ranks = NULL;
			if (ncons) cons = calloc(ncons, sizeof(Constraint));
			if (nranks) ranks = calloc(nranks, sizeof(Ranking));
			int j;
			for (j = 0; j != ncons; j++) {
				cfg_t *con = cfg_getnsec(action, "constraint", j);
				cons[j].published_name = cfg_getstr(con, "name");
				cons[j].maxValue = cfg_getfloat(con, "max");
				cons[j].minValue = cfg_getfloat(con, "min");
			}
			for (j = 0; j != nranks; j++) {
				cfg_t *con = cfg_getnsec(action, "ranking", j);
				ranks[j].published_name = cfg_getstr(con, "name");
				ranks[j].weight = cfg_getfloat(con, "weight");
			}
			HANDLE h = repGetPeers(repoclient, getPeers_callback, NULL, 
				cfg_getint(action, "maxresults"),
				cons, ncons, ranks, nranks, cfg_getstr(action, "channel"));
			continue;
		}
	}
	
	int last_done = tests_done - 1;
	while (tests_done != ntests) {
		if (last_done != tests_done) debug("Test %d/%d done", tests_done, ntests);
		last_done = tests_done;
		event_base_loop(eventbase, EVLOOP_ONCE);
	}
}

struct timeval parse_timestamp(const char *ts) {
	struct timeval ret;
	ret.tv_usec = 0;
	
	if (!strcmp(ts, "now")) {
		ret.tv_sec = time(NULL);
		return ret;
	}
	ret.tv_sec = 0;
	return ret;
}
