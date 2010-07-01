#include	<stdio.h>
#include	<stdlib.h>
#include	<string.h>

#include	<confuse.h>
#include        <event2/event.h>
#include        <event2/buffer.h>
#include        <event2/http.h>
#include        <event2/http_struct.h>

#include	<grapes.h>
#include	<grapes_log.h>
#include	<repoclient.h>
#include	<neighborlist.h>

struct event_base *eventbase;

/* Configuration file parsing instructions. see neighborlist.cfg for an example */

cfg_opt_t opts_neighspec[] = {
	CFG_INT("desired_size", 5, CFGF_NONE),
	CFG_INT("update_period", 30, CFGF_NONE),
	CFG_END()
};
cfg_opt_t opts_repospec[] = {
	CFG_STR("server", "192.168.35.203:9832", CFGF_NONE),
	CFG_END()
};
cfg_opt_t neighborlist_opts[] = {
	CFG_SEC("repository", opts_repospec, CFGF_NONE),
	CFG_SEC("neighborlist", opts_neighspec, CFGF_NONE),
	CFG_END()
};

cfg_t *neighborlist_cfg;
const char *print_list(NeighborListEntry *list, int n);

void periodic_query(HANDLE h, void *arg) {
	HANDLE neighborlist = (HANDLE)arg;

	int size = cfg_getint(neighborlist_cfg, "desired_size");
	NeighborListEntry *list = calloc(sizeof(NeighborListEntry), size);
	
	int success = neighborlist_query(neighborlist, list, &size);
	if (!success) info("%d peers: %s", size, print_list(list, size));
}


int main(int argc, char *argv[]) {
	/* Check for usage (arguments) */
	if (argc < 2) {
		fprintf(stderr, "Usage: %s neighborlist.cfg\n", argv[0]);
		exit(1);
	}
	/* Initialize libevent and logging */
	eventbase = event_base_new();
	grapesInitLog(-1, NULL, NULL);
	repInit("");

	/* Parse config file and init repoclient with the "repository" cfg section */
	cfg_t *cfg = cfg_init(neighborlist_opts, CFGF_NONE);
	if(cfg_parse(cfg, argv[1]))
		fatal("Unable to parse config file %s", argv[1]);

	neighborlist_cfg = cfg_getsec(cfg, "neighborlist");

	HANDLE repoclient = repOpen(cfg_getstr(cfg_getsec(cfg, "repository"), "server"),0);
	if (repoclient == NULL) fatal("Unable to initialize repoclient");

	HANDLE neighborlist = neighborlist_init(repoclient, 
		cfg_getint(neighborlist_cfg, "desired_size"),
		cfg_getint(neighborlist_cfg, "update_period"), NULL, NULL, NULL);

	struct timeval begin = { 5, 0 };
	grapesSchedulePeriodic(&begin, 1.0/10.0, periodic_query, neighborlist);


	event_base_loop(eventbase, 0);

	neighborlist_close(neighborlist);
	repClose(repoclient);

	cfg_free(cfg);
	event_base_free(eventbase);
}

/** Helper to print a string list */
const char *print_list(NeighborListEntry *list, int n) {
        static char buffer[4096];

	struct timeval now;
	gettimeofday(&now, NULL);

        strcpy(buffer, "[ ");
        int i;
        for (i = 0; i != n; i++) {
                if (i) strcat(buffer, ", ");
                strcat(buffer, list[i].peer);
		char buf[32];
		sprintf(buf, "(%d)", (int)(now.tv_sec - list[i].update_timestamp.tv_sec));
		strcat(buffer, buf);
        }
        strcat(buffer, " ]");
        return buffer;
}


