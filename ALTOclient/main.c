/*
 * main.c
 *
 *  Created on: Jan 28, 2010
 *      Author: ewald
 */
#include "ALTOclient.h"
#include <stdio.h>


// This is the address of the resource consumer
struct in_addr rc_addr;

/*
 * 	This is a dummy/test programm which should show you
 * 	the functionaliyt of the ALTO client
 */
int main(int argc, char *argv[]){

	// Some first tests
	if(argc!=5){
		fprintf(stdout, "This is for internal & Stresstest only \n");
		fprintf(stdout, "For usage, please use this format: \n");
		fprintf(stdout, "arg[1] - the file with IPs you want to have parsed \n");
		fprintf(stdout, "arg[2] - the Primary Rating Criteria [1:2:4] \n");
		fprintf(stdout, "arg[3] - the Secondary Rating Criteria [0<8] \n");
		fprintf(stdout, "arg[4] - number of RUNs for testing \n");

		// Try it again, this time right PLZ
		return -1;
	}

	// For testing, set the local RC manually
	rc_addr.s_addr = inet_addr("95.37.70.39");

	// Configure set the ALTO server URL
	set_ALTO_server("http://10.10.251.107/cgi-bin/alto-server.cgi");

	int count;
	for(count=1; count<=atoi(argv[4]); count++){
		fprintf(stdout, "Run %d \n", count);
		// start ALTO client
		start_ALTO_client();
		// And now get the result for this list
		get_ALTO_guidance_for_txt(argv[1], rc_addr, atoi(argv[2]), atoi(argv[3]));
		// and stop the client
		stop_ALTO_client();
	}

	return 1;
}
