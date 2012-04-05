/* upnp.c */

#include "upnp.h"

#include <stdio.h>
#include <string.h>
#include <malloc.h>
#include <miniupnpc/miniupnpc.h>
#include <miniupnpc/miniwget.h>
#include <miniupnpc/upnpcommands.h>

//#include "xchat.h"
//#include "network.h"
//#include "upnp.h"

//ls → declspec.h  igd_desc_parse.h  miniupnpc.h  miniwget.h  upnpcommands.h  upnperrors.h  upnpreplyparse.h

static struct UPNPUrls urls;
static struct IGDdatas data;
char lanaddr[64] = "";


int upnp_init(void)
{
    struct UPNPDev * devlist;
    struct UPNPDev * dev;
    char * descXML;
    int descXMLsize = 0;
    int upnperror = 0;
    int ret;

    if(lanaddr[0]!='\0'){
      printf("TB : init yet done\n");
      return 0;
    }

    printf("TB : init_upnp()\n");
    memset(&urls, 0, sizeof(struct UPNPUrls));
    memset(&data, 0, sizeof(struct IGDdatas));
    devlist = upnpDiscover(2000, NULL/*multicast interface*/, NULL/*minissdpd socket path*/, 0/*sameport*/);
    /*LIBSPEC struct UPNPDev * upnpDiscover(int delay, const char * multicastif,
      const char * minissdpdsock, int sameport);*/

    if (devlist) {
      ret = UPNP_GetValidIGD(devlist, &urls, &data, lanaddr, sizeof(lanaddr));
      if(ret!=1){
	printf("ERRORE, da gestire magari\n");
      }
      dev = devlist;
      while (dev) {
	if (strstr (dev->st, "InternetGatewayDevice"))
	  break;
	dev = dev->pNext;
      }
      if (!dev){
	dev = devlist; /* defaulting to first device */
	printf("NOT dev\n");
      }
      
      printf("UPnP device :\n"
	     " desc: %s\n st: %s\n",
	     dev->descURL, dev->st);
      
      descXML = miniwget(dev->descURL, &descXMLsize);
      if (descXML) {
	parserootdesc (descXML, descXMLsize, &data);
	free ((void *)descXML);
	descXML = 0;
	GetUPNPUrls (&urls, &data, dev->descURL);
      }
      freeUPNPDevlist(devlist);
    }
    else {
      /* error ! */
      printf("TB: no Gateway devices found\n");
      return 1;
    }
    return 0;
}

int upnp_add_redir(int port, const char *protocol)
{
    char port_str[16];
    char desc[50];
    int r;
    
    if(lanaddr[0]=='\0'){
      printf("TB: the init is failed\n");
      return 1;
    }

    printf("TB : upnp_add_redir (%s, %d)\n", lanaddr, port);
    if(urls.controlURL[0] == '\0')
    {
        printf("TB : the init was not done !\n");
        return 2;
    }
    sprintf(port_str, "%d", port);
    sprintf(desc, "PeerStreamer %c %d",protocol[0],port);
    r = UPNP_AddPortMapping(urls.controlURL, data.first.servicetype,
                            port_str, port_str, lanaddr, desc, protocol, NULL);
;
    /*LIBSPEC int
UPNP_AddPortMapping(const char * controlURL, const char * servicetype,
                    const char * extPort,
				    const char * inPort,
					const char * inClient,
					const char * desc,

                    const char * proto,
                    const char * remoteHost);*/
    if(r)
        printf("AddPortMapping(%s, %s, %s) failed\n", port_str, port_str, lanaddr);
    return 0;
}

void upnp_rem_redir (int port)
{
    char port_str[16];
    int t;
    printf("TB : upnp_rem_redir (%d)\n", port);

    if(lanaddr[0]=='\0'){
      printf("TB: the init is failed\n");
      return;
    }
    if(urls.controlURL[0] == '\0')
    {
        printf("TB : the init was not done !\n");
        return;
    }
    sprintf(port_str, "%d", port);
    UPNP_DeletePortMapping(urls.controlURL, data.first.servicetype, port_str, "TCP", NULL);
}
