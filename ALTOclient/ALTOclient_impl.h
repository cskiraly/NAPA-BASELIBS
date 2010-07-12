/*
 ============================================================================
 Name        : ALTOclient.h
 Author      : T. Ewald <ewald@nw.neclab.eu>
 Version     : 243
 Proprietary : NEC Europe Ltd. PROPRIETARY INFORMATION
			   This software is supplied under the terms of a license
			   agreement or nondisclosure agreement with NEC Europe Ltd. and
			   may not be copied or disclosed except in accordance with the
			   terms of that agreement. The software and its source code
			   contain valuable trade secrets and confidential information
			   which have to be maintained in confidence.
			   Any unauthorized publication, transfer to third parties or
			   duplication of the object or source code - either totally or in
			   part - is prohibited.
 Copyright	 : Copyright (c) 2004 NEC Europe Ltd. All Rights Reserved.
			   NEC Europe Ltd. DISCLAIMS ALL WARRANTIES, EITHER EXPRESS OR
			   IMPLIED, INCLUDING BUT NOT LIMITED TO IMPLIED WARRANTIES OF
			   MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE AND THE
			   WARRANTY AGAINST LATENT DEFECTS, WITH RESPECT TO THE PROGRAM
			   AND THE ACCOMPANYING DOCUMENTATION.
			   No Liability For Consequential Damages IN NO EVENT SHALL NEC
			   Europe Ltd., NEC Corporation OR ANY OF ITS SUBSIDIARIES BE
			   LIABLE FOR ANY DAMAGES WHATSOEVER (INCLUDING, WITHOUT LIMITATION,
			   DAMAGES FOR LOSS OF BUSINESS PROFITS, BUSINESS INTERRUPTION, LOSS
			   OF INFORMATION, OR OTHER PECUNIARY LOSS AND INDIRECT,
			   CONSEQUENTIAL, INCIDENTAL, ECONOMIC OR PUNITIVE DAMAGES) ARISING
			   OUT OF THE USE OF OR INABILITY TO USE THIS PROGRAM, EVEN IF NEC
			   Europe Ltd. HAS BEEN ADVISED OF THE POSSIBILITY OF SUCH DAMAGES.
 Modification: THIS HEADER MAY NOT BE EXTRACTED OR MODIFIED IN ANY WAY.
 Description : First try of the ALTO client header
 ============================================================================
 */

#ifndef ALTOCLIENT_IMPL_H_
#define ALTOCLIENT_IMPL_H_

#include <stdio.h>
#include <string.h>
#include <stdlib.h>

// for having all the fancy host structs
#include <arpa/inet.h>


// and now the XML stuff
// don't forget to link it against xml2 and the path
#include <libxml/xmlversion.h>
#include <libxml/xmlIO.h>
#include <libxml/parser.h>
#include <libxml/tree.h>

// don't forget to install CURL http://curl.haxx.se/download.html
// and install as described!!!!!
#ifdef USE_CURL
#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>
#endif



// This is needed for the CURL reply
#define ALTO_REP_BUF_SIZE 524288


/*
 *  This is the internal ALTO DB
 *  [TODO can be replaced by central host DB in future]
 */
typedef struct alto_db_t{
	int num_of_elements;
	struct alto_db_element_t *first;
	struct alto_db_element_t *last;
}ALTO_DB_T;
typedef ALTO_DB_T *altoDbPtr;


/*
 *  These are the elements of the ALTO DB
 *
 *  Within this structure you can store the hosts to query
 *  but also the result delivered from the ALTO server
 */
typedef struct alto_db_element_t{
	// Internal structure
	struct alto_db_t *alto_db;			// pointer to the DB [internal]
	struct alto_db_element_t *next;		// pointer to the prev element [internal]
	struct alto_db_element_t *prev;		// pointer to the prev element [internal]
	time_t stamp;						// store when this record was updated

	// User requested Data
	struct in_addr host;				// the "IP" of the ALTO host
	int host_mask;						// The prefix for this host

	// Matched Data
	int rating;							// the rating of the ALTO host

	// Additional Data
	struct in_addr subnet;				// The subnet where to search in
	int subnet_mask;					// The mask of the subnet

}ALTO_DB_ELEMENT_T;

/*
 * 	This is needed for the CURL intearction with the ALTO server
 */
#ifdef USE_CURL
struct curl_reply_buffer_t {
    size_t          size;
    size_t          fill;
    char            buffer[ALTO_REP_BUF_SIZE];
};
#endif





/*
 *  And here the internal functions:
 */
struct in_addr get_ALTO_host_IP(char * host_string);
int16_t get_ALTO_host_mask(char * host_string);

xmlDocPtr alto_create_request_XML(struct alto_db_t * db, struct in_addr rc_host, int pri_rat, int sec_rat);
xmlDocPtr ALTO_request_to_server(xmlDocPtr doc, char* endPoint);

#ifdef USE_CURL
xmlDocPtr query_ALTO_server_curl(xmlDocPtr doc, char* ALTO_server_URL);
size_t curl_copy_reply_to_buf(void *ptr,size_t size,size_t nmemb,void *stream);
#endif

void print_Alto_XML_info(xmlDocPtr doc);
void dump_internal_ALTO_db(struct alto_db_t * db);
void alto_dump_db(struct alto_db_t *db);
void alto_dump_element(struct alto_db_element_t * cur);


// Functions for DB management
struct alto_db_t * alto_db_init(void);
int alto_free_db(struct alto_db_t * db);
int alto_add_element(struct alto_db_t *db, struct alto_db_element_t * element);
int alto_parse_from_list(struct alto_db_t *db, struct alto_guidance_t *list, int num_of_elements);
int alto_rem_element(struct alto_db_element_t * element);


// Helper Functions
struct in_addr compute_subnet(struct in_addr host, int prefix);
int ask_helper_func(struct in_addr subnet, ALTO_DB_T * db);
int alto_do_the_magic(ALTO_DB_T * ALTO_db_req, ALTO_DB_T * ALTO_db_res);


int get_alto_rating(ALTO_DB_ELEMENT_T * element, ALTO_DB_T * db);
int get_alto_subnet_mask(ALTO_DB_ELEMENT_T * element, ALTO_DB_T * db);
int get_ALTO_rating_for_host(struct in_addr add, ALTO_DB_T * db);

int alto_parse_from_file(altoDbPtr db, char *file_name);
int alto_parse_from_XML(altoDbPtr db, xmlDocPtr doc);
int alto_write_to_file(altoDbPtr db, char *file_name);





#endif /* ALTOCLIENT_IMPL_H_ */
