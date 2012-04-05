#ifndef UPNP_H
#define UPNP_H

#define upnp_add_TCP_redir(p) upnp_add_redir(p, "TCP")
#define upnp_add_UDP_redir(p) upnp_add_redir(p, "UDP")

int upnp_init(void);
int upnp_add_redir(int port, const char *protocol);
void upnp_rem_redir(int port);

#endif /* UPNP_H */
