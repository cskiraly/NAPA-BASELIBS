/*
 * GRAPES test peer implementation
 */

#include	"peer.h"

Peer *peer = NULL;

#if 0
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

void getPeers_callback(HANDLE client, HANDLE id, void *cbarg, char **result, int n) {
        info("GetPeers id %p done, result: %s", id, print_list(result, n, 1));
}

void showPeers() {
	int ncons = 0;
	int nranks = 0;
	Constraint *cons = NULL;
	Ranking *ranks = NULL;
	if (ncons) cons = calloc(ncons, sizeof(Constraint));
	if (nranks) ranks = calloc(nranks, sizeof(Ranking));
	HANDLE h = repGetPeers(peer->repository, getPeers_callback, NULL,0, cons, ncons, ranks, nranks);
}
#endif


int main(int argc, char *argv[]) {
	if (argc != 2) {
		fprintf(stderr, "Usage: peer config.cfg\n");
		exit(-1);
	}

        grapesInitLog(LOG_INFO, NULL, NULL);		/* Initialize logging */
	peer = peer_init(argv[1]);	/* Initialize the GRAPES infrastructure */

	info("Initialization done");

	event_base_dispatch(eventbase);

	return 0;
}

#if 0
void newPeers_cb(HANDLE rep, void *cbarg) {
	peer->num_neighbors = peer->neighborlist_size;
	if (!peer->neighborlist) return;
	neighborlist_query(peer->neighborlist, peer->neighbors, &(peer->num_neighbors));

	int i;
	for (i = 0; i != peer->num_neighbors; i++) {
		socketID_handle remote_socketID = malloc(SOCKETID_SIZE);
		mlStringToSocketID(peer->neighbors[i].peer, remote_socketID);
		info("Opening connection to %s", peer->neighbors[i].peer);
		if(open_connection(remote_socketID,&receive_outconn_cb,"DUMMY ARG") != 0) {
			error("ML open_connection failed to %s", peer->neighbors[i].peer);
		}
	}
}
#endif

