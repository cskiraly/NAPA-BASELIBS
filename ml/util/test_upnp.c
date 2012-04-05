#include <stdio.h>
#include "upnp.h"

main()
{
  printf("test ♥\n");

  if(!upnp_init()){

    if(!upnp_add_TCP_redir(23911))
      printf("Ok!\n");

    upnp_init();

    if(!upnp_add_UDP_redir(23912))
      printf("Ok!\n");

    upnp_rem_redir(23911);
    upnp_rem_redir(23912);
  }
}
