/*
 * main.c
 *
 *  Created on: Jan 28, 2010
 *      Author: ewald
 */
#include "ALTOclient.h"


// This is the address of the resource consumer
struct in_addr rc_addr;

/*
 * 	This is a dummy/test programm which should show you
 * 	the functionaliyt of the ALTO client
 */
int main(){

	// For testing, set the local RC manually
	//rc_addr.s_addr = inet_addr("95.37.70.39");
	rc_addr.s_addr = inet_addr("195.37.70.39");
	//rc_addr.s_addr = inet_addr("10.1.6.27");

	// start ALTO client
	start_ALTO_client();

	// Configure set the ALTO server URL
	set_ALTO_server("http://10.10.251.107/cgi-bin/alto-server.cgi");

	// And now get the result for this list
	get_ALTO_guidance_for_txt("ip.list", rc_addr, REL_PREF, 7);

	// and stop the client
	stop_ALTO_client();

	return (1);
}
