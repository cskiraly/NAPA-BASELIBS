/*
 * % vim: syntax=c ts=4 tw=76 cindent shiftwidth=4
 *
 * User layer services the Peer implementation for the Broadcaster demo application
 *
 */

#include "peer.h"

#include <chunk_external_interface.h>
#include <http_default_urls.h>


/** Configuration options for the Source */
cfg_opt_t cfg_source[] = {
	CFG_STR("stream", NULL, CFGF_NONE),
	CFG_FLOAT("chunk_duration", 1.0, CFGF_NONE),
	CFG_END()
};

/** This function gets called periodically and arranges the publishing of a
 * MeasurementRecord with SrcLag = 0.0 
 */
void periodicPublishSrcLag(HANDLE h, void *arg) {
	MeasurementRecord mr;
	memset(&mr, 0, sizeof(mr));
	mr.published_name = "SrcLag";
	mr.value = 0.0;
	mr.originator = mr.targetA = mr.targetB = LocalPeerID;
	mr.channel = channel;
	publishMeasurementRecord(&mr, "SrcLag");
}

void src_init(cfg_t *source_cfg) {
	char *stream = cfg_getstr(source_cfg, "stream");
	if (!stream) return;
	double chunk_duration = cfg_getfloat(source_cfg,"chunk_duration");
	crcInit();
	info("Initializing SOURCE mode: tuning to stream %s, sampling rate is "
			"%.2lf Hz",
		stream, (double)1.0/chunk_duration);

	init_source_ul(stream, chunk_duration);
	grapesSchedulePeriodic(NULL, 1/60.0, periodicPublishSrcLag, NULL);
}

int init_source_ul(const char *stream, double chunk_duration) {
	char addr[128];
	int port,i;

	if (sscanf(stream, "http://%127[^:]:%i", addr, &port) != 2) {
		fatal("Unable to parse source specification %s", stream);
	}

	if ( ulChunkReceiverSetup(addr, UL_DEFAULT_CHUNKBUFFER_PORT) != UL_RETURN_OK ) {
		fatal("Unable to start the HTTP receiver to http://%s:%d", addr, UL_DEFAULT_CHUNKBUFFER_PORT);
	}
}



