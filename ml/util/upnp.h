#ifndef UPNP_H
#define UPNP_H

#define upnp_add_TCP_redir(i,e) upnp_add_redir(i,e, "TCP")
#define upnp_add_UDP_redir(i,e) upnp_add_redir(i,e, "UDP")

int upnp_init();
int upnp_add_redir(int iport, int eport, const char *protocol);
void upnp_rem_redir(int port);

#endif /* UPNP_H */
