/*
 * % vim: syntax=c ts=4 tw=76 cindent shiftwidth=4
 *
 * Repository client services fror the Peer implementation for the Broadcaster demo application
 *
 */

#include "peer.h"

#include <repoclient.h>

cfg_opt_t cfg_rep[] = {
	CFG_STR("server", NULL, CFGF_NONE),
	CFG_END()
};

HANDLE repository = NULL;

void rep_init(cfg_t *rep_config) {
	repInit("");
	repository = repOpen(cfg_getstr(rep_config, "server"),0);
	if (repository == NULL) fatal("Unable to initialize repoclient");
}
