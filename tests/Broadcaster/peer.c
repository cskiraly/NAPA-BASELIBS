/*
 * % vim: syntax=c ts=4 tw=76 cindent shiftwidth=4
 * 
 * Peer implementation for the Broadcaster demo application
 *
 * This simple peer registers itself in the Repository, and then passively wait for chunks to arrive
 *
 * 
 * 
 */

#include "peer.h"

extern char *channel;

cfg_opt_t cfg_log[] = {
	CFG_INT("level", 3, CFGF_NONE),
	CFG_STR("logfile", "stdout", CFGF_NONE),
	CFG_STR("filemode", "w", CFGF_NONE),
	CFG_END()
};

/* Main configuration sections */
cfg_opt_t cfg_main[] = {
	CFG_SEC("network", cfg_ml, CFGF_NONE),
	CFG_SEC("repository", cfg_rep, CFGF_NONE),
	CFG_SEC("measurements", cfg_mon, CFGF_NONE),
	CFG_SEC("som", cfg_som, CFGF_NONE),
	CFG_SEC("source", cfg_source, CFGF_NONE),
	CFG_SEC("playout", cfg_playout, CFGF_NONE),
	CFG_SEC("log", cfg_log, CFGF_NONE),
	CFG_END()
};

static void usage(char *argv[]) {
	fprintf(stderr, "Peer version %s.\nUsage: %s [-d debuglevel(0-*3*-5)] [-p UDP port]"
			" configfile.cfg\n", version,argv[0]);
	exit(-1);
}


int main(int argc, char *argv[]) {
	int log_level = -1;
	int port = 0;
	int i;

	if (argc == 2 && (strcmp(argv[1], "-h") == 0 || strcmp(argv[1], "--help")
				== 0)) usage(argv);
	if (!(argc == 2 || argc == 4 || argc == 6)) usage(argv);

	for (i = 1; i < (argc - 1); i += 2) {
		if (strcmp(argv[i], "-d") == 0) {
			log_level = atoi(argv[i + 1]);
			if (log_level < 0 || log_level > 5) usage(argv);
			}
		else if (strcmp(argv[i], "-p") == 0) {
			port = atoi(argv[i + 1]);
			if (port < 1023 || port > 65535) usage(argv);
		}
		else usage(argv);
	}

	// Read the config file using libconfuse
	cfg_t *main_config = cfg_init(cfg_main, CFGF_NONE);
	mon_validate_cfg(main_config);
	if(cfg_parse(main_config, argv[argc - 1]) == CFG_PARSE_ERROR)
		fatal("Unable to parse config file %s", argv[1]);

	cfg_t *log_config = cfg_getsec(main_config, "log");

	// Initialize logging
	if (log_level == -1) log_level = cfg_getint(log_config, "level");
	grapesInitLog(log_level, cfg_getstr(log_config, "logfile"),
			cfg_getstr(log_config, "filemode"));
	grapesInit(event_base_new());

	// Check config file
	int fd = open(argv[argc - 1], 0);
	if (fd < 0) usage(argv);
	close(fd);

	info("This is peer version %s initializing...", version);

	cfg_t *network_cfg = cfg_getsec(main_config, "network");
	ml_init(network_cfg, port);

	cfg_t *rep_cfg = cfg_getsec(main_config, "repository");
	rep_init(rep_cfg);

	cfg_t *mon_cfg = cfg_getsec(main_config, "measurements");
	mon_init(mon_cfg, channel);

	cfg_t *som_cfg =  cfg_getsec(main_config, "som");
	som_init(som_cfg);

	cfg_t *src_cfg = cfg_getsec(main_config, "source");
	src_init(src_cfg);

	cfg_t *playout_cfg = cfg_getsec(main_config, "playout");
	playout_init(playout_cfg);

	event_base_dispatch(eventbase);
	return 0;
}

/**
    Callback for the "Publish MeasurementRecord" utility function.
	*/
static void publishMeasurementRecord_cb(HANDLE h, HANDLE id, void *arg, int result) {
	    if (result) 
			warn("Repository publish for \"%s\" returned with result code %d",
					(char *)arg, result);
}

/**
    Utility function to publish a MeasurementRecord 
*/
void publishMeasurementRecord(MeasurementRecord *mr, char *msg) {
	repPublish(repository, publishMeasurementRecord_cb, msg, mr);
}

/** Utility function to print a string array */
const char *print_list(char **list, int n, bool should_free) {
	static char buffer[4096];
	strcpy(buffer, "[ ");
	int i;
	for (i = 0; i != n; i++) {
		if (i) strcat(buffer, ", ");
		strcat(buffer, list[i]);
		if (should_free)
			free(list[i]);
	}
	strcat(buffer, " ]");
	if (should_free) free(list);
	return buffer;
}

  
