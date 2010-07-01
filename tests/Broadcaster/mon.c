/*
 * % vim: syntax=c ts=4 tw=76 cindent shiftwidth=4
 *
 * Monitoring services fror the Peer implementation for the Broadcaster demo application
 *
 */

#include "peer.h"

cfg_opt_t cfg_parameters[] = {
	CFG_INT("publishing_rate", 60, CFGF_NONE),
	CFG_INT("window_size", 60, CFGF_NONE),
	CFG_INT("oob_frequency", 60, CFGF_NONE),
	CFG_END()
};

cfg_opt_t cfg_measurement[] = {
	CFG_STR("published_name", NULL, CFGF_NONE),
	CFG_STR("direction", NULL, CFGF_NONE),
	CFG_STR("granularity", NULL, CFGF_NONE),
	CFG_STR("signaling", NULL, CFGF_NONE),
	CFG_STR("plugin", NULL, CFGF_NONE),
	CFG_SEC("parameters", cfg_parameters, CFGF_NONE),
	CFG_STR_LIST("calculate", "{LAST}", CFGF_NONE),
	CFG_END()
};

cfg_opt_t cfg_mon[] = {
	CFG_SEC("measurement", cfg_measurement, CFGF_TITLE | CFGF_MULTI),
	CFG_END()
};

static cfg_t *config = NULL;

static MonHandler *monHandles = NULL;
int num_measurements = 0;

const char *direction_allowed_values[] = { "rxonly", "txonly", "txrxuni", "txrxbi" };
const char *signaling_allowed_values[] = { "in-band", "out-of-band" };
const char *granularity_allowed_values[] = { "packet", "message" };
const char *known_plugins[] = { "Byte", "Hopcount" };

static int validate(cfg_t *cfg, cfg_opt_t *opt, const char *value, const char *allowed_values[]) {
	int i;
	for (i = 0; i != sizeof(allowed_values); i++) {
		if (!strcasecmp(value, allowed_values[i])) return 0;
	}
	cfg_error(cfg, "option '%s' has incorrect value in section '%s'", opt->name, cfg->name);
	return -1;
}

/** Validator for the "direction" configration option */
int cfg_validate(cfg_t *cfg, cfg_opt_t *opt) {
	const char *value = cfg_getstr(cfg, opt->name);

	if (!strcmp(opt->name, "direction")) 
		return validate(cfg, opt, value, direction_allowed_values);
	else if (!strcmp(opt->name, "granularity")) 
		return validate(cfg, opt, value, granularity_allowed_values);
	else if (!strcmp(opt->name, "signaling")) 
		return validate(cfg, opt, value, signaling_allowed_values);
	else if (!strcmp(opt->name, "plugin"))
		return validate(cfg, opt, value, known_plugins);
	return 0;
}

void mon_validate_cfg(cfg_t *cfg) {
	cfg_set_validate_func(cfg, "measurements|measurement|direction", cfg_validate);
	cfg_set_validate_func(cfg, "measurements|measurement|granularity", cfg_validate);
	cfg_set_validate_func(cfg, "measurements|measurement|signaling", cfg_validate);
	cfg_set_validate_func(cfg, "measurements|measurement|plugin", cfg_validate);
}

void mon_init(cfg_t *mon_config, char *cnl) {
	if (!mon_config) return;

	if (monInit(eventbase, repository)) fatal("Unable to init monitoring layer!");
	config = mon_config;

	num_measurements = cfg_size(mon_config, "measurement");
	monHandles = calloc(sizeof(MonHandler),  num_measurements);

	int j;
	for(j = 0; j < num_measurements; j++) {
		cfg_t *cfg_meas = cfg_getnsec(mon_config, "measurement", j);

		const char *measurementName = cfg_title(cfg_meas);
		const char *direction = cfg_getstr(cfg_meas, "direction");
		const char *granularity = cfg_getstr(cfg_meas, "granularity");
		const char *signaling= cfg_getstr(cfg_meas, "signaling");
		const char *plugin = cfg_getstr(cfg_meas, "plugin");
		cfg_t *cfg_params = cfg_getsec(cfg_meas, "parameters");
		int publishing_rate = cfg_getint(cfg_params, "publishing_rate");
		int window_size = cfg_getint(cfg_params, "window_size");
		int oob_frequency = cfg_getint(cfg_params, "oob_frequency");

		MeasurementCapabilities flags = 0;

		if (!strcasecmp(signaling, "in-band")) flags += IN_BAND;
		else if (!strcasecmp(signaling, "out-of-band")) flags += OUT_OF_BAND;

		if (!strcasecmp(granularity, "packet")) flags += PACKET;
		else if (!strcasecmp(granularity, "message")) flags += DATA;

		if (!strcasecmp(direction, "txonly")) flags += TXONLY;
		else if (!strcasecmp(direction, "rxonly")) flags += RXONLY;
		else if (!strcasecmp(direction, "txrxuni")) flags += TXRXUNI;
		else if (!strcasecmp(direction, "txrxbi")) flags += TXRXBI;

		debug("Measurement flags for %s: %d", measurementName, (int)flags);

		/* Create the Measurement */
		monHandles[j] =  monCreateMeasureName((MeasurementName)plugin, flags);
		if (monHandles[j] < 0) fatal("Error %d creating measurement %s",
				monHandles[j], plugin);

		int nstats = cfg_size(cfg_meas, "calculate");
		enum stat_types *stats = calloc(sizeof(enum stat_types), nstats);
		int i;
		for(i = 0; i < nstats; i++) {
			const char *value = cfg_getnstr(cfg_meas, "calculate", i);

			if (!strcasecmp("AVG", value)) stats[i] = AVG;
			else if (!strcasecmp("LAST", value)) stats[i] = LAST;
			else if (!strcasecmp("MIN", value)) stats[i] = MIN;
			else if (!strcasecmp("MAX", value)) stats[i] = MAX;
			else fatal("Unknown statistical type %s in configuration of "
					"measurement %s", value, measurementName);
		}

		int result = monPublishStatisticalType(monHandles[j],
				measurementName, cnl, stats, i, repository);
#if 0
		if (monSetParameter(monHandles[j], P_WINDOW_SIZE, window_size))
			fatal("Unable to set monitoring parameter WINDOW_SIZE for %s",
					measurementName);
#endif
		if (monSetParameter(monHandles[j], P_PUBLISHING_RATE,
					publishing_rate))
			fatal("Unable to set monitoring parameter PUBLISHING_RATE for %s",
					measurementName);
		debug("Creating measurement %d - result: %d, MonHandler: %d, length: %d", j, result, monHandles[j], i);
		info("Measurement %s has been created", measurementName);
	}

	info("Monitoring is initialized");
}

void activateMeasurements(socketID_handle remsocketID) {

	char peer[64] = "";
	if (mlSocketIDToString(remsocketID, peer, sizeof(peer)))  fatal("Internal error");
	socketID_handle cp = (socketID_handle)malloc(SOCKETID_SIZE);
	memcpy(cp, remsocketID, SOCKETID_SIZE);

	int j;
	for(j = 0; j < num_measurements; j++) {
		info("Activating measurement %d towards %s", j, peer);
		int ret = monActivateMeasure(monHandles[j], cp, MSG_TYPE_CHUNK);
		if (ret) error("Error activating measurement %d", j);
	}
}

