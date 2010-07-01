/* packet-grapes.c
 * Routines for GRAPES NapaWine dissection
 * Copyright 2010, Robert Birke <robert[dot]birke[at]polito[dot]it>
 *
 * $Id$
 *
 * Wireshark - Network traffic analyzer
 * By Gerald Combs <gerald@wireshark.org>
 * Copyright 1998 Gerald Combs
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif


#include <glib.h>

#include <epan/packet.h>
#include <epan/prefs.h>

#define GRAPES_ML_HDR_SIZE 23
#define GRAPES_MONL_PKT_HDR_SIZE 21
#define GRAPES_MONL_DATA_HDR_SIZE 20

/* Forward declaration we need below (if using proto_reg_handoff...
   as a prefs callback)       */
void proto_reg_handoff_grapes(void);

/* Initialize the protocol and registered fields */
static int proto_grapes = -1;

/* ML header */
static int hf_grapes_ml_msg_type = -1;
static int hf_grapes_ml_len_mon_data_hdr = -1;
static int hf_grapes_ml_len_mon_packet_hdr = -1;
static int hf_grapes_ml_offset = -1;
static int hf_grapes_ml_msg_length = -1;
static int hf_grapes_ml_local_con_id = -1;
static int hf_grapes_ml_remote_con_id = -1;
static int hf_grapes_ml_msg_seq_num = -1;

/* MONL packet header */
static int hf_grapes_monl_p_seq_num = -1;
static int hf_grapes_monl_p_ts_s = -1;
static int hf_grapes_monl_p_ts_us = -1;
static int hf_grapes_monl_p_ans_ts_s = -1;
static int hf_grapes_monl_p_ans_ts_us = -1;
static int hf_grapes_monl_p_initial_ttl = -1;

/* MONL data header */
static int hf_grapes_monl_d_seq_num = -1;
static int hf_grapes_monl_d_ts_s = -1;
static int hf_grapes_monl_d_ts_us = -1;
static int hf_grapes_monl_d_ans_ts_s = -1;
static int hf_grapes_monl_d_ans_ts_us = -1;


static dissector_handle_t data_handle;

/* Global sample port pref */
static guint udp_port = 6100;

/* Initialize the subtree pointers */
static gint ett_grapes = -1;
static gint ett_grapes_monl_p = -1;
static gint ett_grapes_monl_d = -1;

/* Code to actually dissect the packets */
/* Diseect MONL packet header */
static int
dissect_grapes_monl_packet(tvbuff_t *tvb, int offset, int size, proto_tree *tree)
{
	proto_item *ti;
	proto_tree *monl_p_tree;
	int reported_length;

	if (!tree)
		return offset;

	if (size < GRAPES_MONL_PKT_HDR_SIZE)
		return 0;

	reported_length = tvb_reported_length_remaining(tvb, offset);
	if(tvb_reported_length_remaining(tvb, offset) < GRAPES_MONL_PKT_HDR_SIZE) {
		proto_tree_add_text(tree, tvb, offset, reported_length, "MONL packet header (truncated)");
		return offset;
	}

	ti = proto_tree_add_text(tree, tvb, offset, GRAPES_MONL_PKT_HDR_SIZE, "MONL packet header");
	monl_p_tree = proto_item_add_subtree(ti, ett_grapes_monl_p);

	proto_tree_add_item(monl_p_tree, hf_grapes_monl_p_seq_num, tvb, offset, 4, FALSE);
	proto_tree_add_item(monl_p_tree, hf_grapes_monl_p_ts_s, tvb, offset + 4, 4, FALSE);
	proto_tree_add_item(monl_p_tree, hf_grapes_monl_p_ts_us, tvb, offset + 8, 4, FALSE);
	proto_tree_add_item(monl_p_tree, hf_grapes_monl_p_ans_ts_s, tvb, offset + 12, 4, FALSE);
	proto_tree_add_item(monl_p_tree, hf_grapes_monl_p_ans_ts_us, tvb, offset + 16, 4, FALSE);
	proto_tree_add_item(monl_p_tree, hf_grapes_monl_p_initial_ttl, tvb, offset + 20, 1, FALSE);

	return size;
}

/* Code to actually dissect the packets */
/* Diseect MONL data header */
static int
dissect_grapes_monl_data(tvbuff_t *tvb, int offset, int size, proto_tree *tree)
{
	proto_item *ti;
	proto_tree *monl_d_tree;
	int reported_length;

	if (!tree)
		return 0;

	if (size < GRAPES_MONL_DATA_HDR_SIZE)
		return 0;

	reported_length = tvb_reported_length_remaining(tvb, offset);
	if(tvb_reported_length_remaining(tvb, offset) < GRAPES_MONL_DATA_HDR_SIZE) {
		proto_tree_add_text(tree, tvb, offset, reported_length, "MONL data header (truncated)");
		return 0;
	}

	ti = proto_tree_add_text(tree, tvb, offset, GRAPES_MONL_DATA_HDR_SIZE, "MONL data header");
	monl_d_tree = proto_item_add_subtree(ti, ett_grapes_monl_d);

	proto_tree_add_item(monl_d_tree, hf_grapes_monl_d_seq_num, tvb, offset, 4, FALSE);
	proto_tree_add_item(monl_d_tree, hf_grapes_monl_d_ts_s, tvb, offset + 4, 4, FALSE);
	proto_tree_add_item(monl_d_tree, hf_grapes_monl_d_ts_us, tvb, offset + 8, 4, FALSE);
	proto_tree_add_item(monl_d_tree, hf_grapes_monl_d_ans_ts_s, tvb, offset + 12, 4, FALSE);
	proto_tree_add_item(monl_d_tree, hf_grapes_monl_d_ans_ts_s, tvb, offset + 16, 4, FALSE);

	return size;
}

/* Code to actually dissect the packets */
/* Diseect ML header */
static int
dissect_grapes(tvbuff_t *tvb, packet_info *pinfo, proto_tree *tree)
{
	/* Set up structures needed to add the protocol subtree and manage it */
	proto_item *ti;
	proto_tree *grapes_tree;
	int offset, monl_p_hdr_size, monl_d_hdr_size;

	/* Check that there's enough data */
	if (tvb_length(tvb) < GRAPES_ML_HDR_SIZE)
		return 0;

	/* Get some values from the packet header, probably using tvb_get_*() */
	//TODO add further checks if possible

	/* Make entries in Protocol column and Info column on summary display */
	col_set_str(pinfo->cinfo, COL_PROTOCOL, "GRAPES");

	col_clear(pinfo->cinfo, COL_INFO);
	col_add_fstr(pinfo->cinfo, COL_INFO, "MsgType: %d Seq: %d Offset: %d", tvb_get_guint8(tvb, 20), tvb_get_ntohl(tvb, 16), tvb_get_ntohl(tvb, 0));

	if (!tree)
		return tvb_length(tvb);

	/* create display subtree for the protocol */
	ti = proto_tree_add_item(tree, proto_grapes, tvb, 0, GRAPES_ML_HDR_SIZE, FALSE);
	grapes_tree = proto_item_add_subtree(ti, ett_grapes);

	proto_tree_add_item(grapes_tree, hf_grapes_ml_offset, tvb, 0, 4, FALSE);
	proto_tree_add_item(grapes_tree, hf_grapes_ml_msg_length, tvb, 4, 4, FALSE);
	proto_tree_add_item(grapes_tree, hf_grapes_ml_local_con_id, tvb, 8, 4, FALSE);
	proto_tree_add_item(grapes_tree, hf_grapes_ml_remote_con_id, tvb, 12, 4, FALSE);
	proto_tree_add_item(grapes_tree, hf_grapes_ml_msg_seq_num, tvb, 16, 4, FALSE);
	proto_tree_add_item(grapes_tree, hf_grapes_ml_msg_type, tvb, 20, 1, FALSE);
	proto_tree_add_item(grapes_tree, hf_grapes_ml_len_mon_data_hdr, tvb, 21, 1, FALSE);
	proto_tree_add_item(grapes_tree, hf_grapes_ml_len_mon_packet_hdr, tvb, 22, 1, FALSE);

	offset = GRAPES_ML_HDR_SIZE;

	monl_p_hdr_size = tvb_get_guint8(tvb, 22);
	if(monl_p_hdr_size != 0)
		offset += dissect_grapes_monl_packet(tvb, offset, monl_p_hdr_size, grapes_tree);

	monl_d_hdr_size = tvb_get_guint8(tvb, 21);
	if(monl_d_hdr_size != 0 && tvb_get_ntohl(tvb, 0) == 0)
		offset += dissect_grapes_monl_data(tvb, offset, monl_d_hdr_size, grapes_tree);

	// dissect payload as data
	call_dissector(data_handle, tvb_new_subset(tvb, offset, -1, -1), pinfo, grapes_tree);

/* Return the amount of data this dissector was able to dissect */
	return tvb_length(tvb);
}


/* Register the protocol with Wireshark */

void
proto_register_grapes(void)
{
	module_t *grapes_module;

/* Setup list of header fields  See Section 1.6.1 for details*/
	static hf_register_info hf[] = {
	/* ML header */
		{ &hf_grapes_ml_offset,
			{ "Offset", "grapes.offset",
			FT_UINT32, BASE_DEC, NULL, 0x0,
			"The data offset within the message contained in this packet", HFILL }},
		{ &hf_grapes_ml_msg_length,
			{ "Message length", "grapes.msg_length",
			FT_UINT32, BASE_DEC, NULL, 0x0,
			"The total length of the message thi packet belongs to", HFILL }},
		{ &hf_grapes_ml_local_con_id,
			{ "Src conId", "grapes.src_con_id",
			FT_INT32, BASE_DEC, NULL, 0x0,
			"The connection ID at the source", HFILL }},
		{ &hf_grapes_ml_remote_con_id,
			{ "Dst conId", "grapes.dst_con_id",
			FT_INT32, BASE_DEC, NULL, 0x0,
			"The connection ID at the destination", HFILL }},
		{ &hf_grapes_ml_msg_seq_num,
			{ "Message seqn", "grapes.msg_seq_num",
			FT_UINT32, BASE_DEC, NULL, 0x0,
			"The message sequence number", HFILL }},
		{ &hf_grapes_ml_msg_type,
			{ "Message type", "grapes.msg_type",
			FT_UINT8, BASE_DEC, NULL, 0x0,
			"The message type", HFILL }},
		{ &hf_grapes_ml_len_mon_data_hdr,
			{ "MONL data header length", "grapes.monl_data_len",
			FT_UINT8, BASE_DEC, NULL, 0x0,
			"The size of MONL data header", HFILL }},
		{ &hf_grapes_ml_len_mon_packet_hdr,
			{ "MONL packet header length", "grapes.monl_packet_len",
			FT_UINT8, BASE_DEC, NULL, 0x0,
			"The size of MONL packet header", HFILL }},
	/* MONL packet header */
		{ &hf_grapes_monl_p_seq_num,
			{ "Packet sequence number", "grapes.monl_p_seq_num",
			FT_UINT32, BASE_DEC, NULL, 0x0,
			"The packet sequence number", HFILL }},
		{ &hf_grapes_monl_p_initial_ttl,
			{ "Initial TTL", "grapes.monl_p_initial_ttl",
			FT_UINT8, BASE_DEC, NULL, 0x0,
			"The initial TTL value at sender", HFILL }},
		{ &hf_grapes_monl_p_ts_s,
			{ "Packet TX timestamp [s]", "grapes.monl_p_ts_s",
			FT_UINT32, BASE_DEC, NULL, 0x0,
			"The packet TX timestamp at the source [s]", HFILL }},
		{ &hf_grapes_monl_p_ts_us,
			{ "Packet TX timestamp [us]", "grapes.monl_p_ts_us",
			FT_UINT32, BASE_DEC, NULL, 0x0,
			"The packet TX timestamp at the source [us]", HFILL }},
		{ &hf_grapes_monl_p_ans_ts_s,
			{ "Packet ANS timestamp [s]", "grapes.monl_p_ans_ts_s",
			FT_UINT32, BASE_DEC, NULL, 0x0,
			"The packet answer timestamp [s]", HFILL }},
		{ &hf_grapes_monl_p_ans_ts_us,
			{ "Packet ANS timestamp [us]", "grapes.monl_p_ans_ts_us",
			FT_UINT32, BASE_DEC, NULL, 0x0,
			"The packet answer timestamp [us]", HFILL }},

		/* MONL data header */
		{ &hf_grapes_monl_d_seq_num,
			{ "Sequence number", "grapes.monl_d_seq_num",
			FT_UINT32, BASE_DEC, NULL, 0x0,
			"The MONL data sequence number", HFILL }},
		{ &hf_grapes_monl_d_ts_s,
			{ "Data TX timestamp [s]", "grapes.monl_d_ts_s",
			FT_UINT32, BASE_DEC, NULL, 0x0,
			"The data TX timestamp at the source [s]", HFILL }},
		{ &hf_grapes_monl_d_ts_us,
			{ "Data TX timestamp [us]", "grapes.monl_d_ts_us",
			FT_UINT32, BASE_DEC, NULL, 0x0,
			"The data TX timestamp at the source [us]", HFILL }},
		{ &hf_grapes_monl_d_ans_ts_s,
			{ "Data ANS timestamp [s]", "grapes.monl_d_ans_ts_s",
			FT_UINT32, BASE_DEC, NULL, 0x0,
			"The data answer timestamp [s]", HFILL }},
		{ &hf_grapes_monl_d_ans_ts_us,
			{ "Data ANS timestamp [us]", "grapes.monl_d_ans_ts_us",
			FT_UINT32, BASE_DEC, NULL, 0x0,
			"The data answer timestamp [us]", HFILL }}
	};

/* Setup protocol subtree array */
	static gint *ett[] = {
		&ett_grapes,
		&ett_grapes_monl_p,
		&ett_grapes_monl_d
	};

/* Register the protocol name and description */
	proto_grapes = proto_register_protocol("GRAPES NapaWine", "GRAPES", "grapes");

/* Required function calls to register the header fields and subtrees used */
	proto_register_field_array(proto_grapes, hf, array_length(hf));
	proto_register_subtree_array(ett, array_length(ett));

/* Register preferences module (See Section 2.6 for more on preferences) */
	grapes_module = prefs_register_protocol(proto_grapes,
	    proto_reg_handoff_grapes);

/* Register a sample port preference   */
	prefs_register_uint_preference(grapes_module, "udp.port", "GRAPES UDP Port",
	     " GRAPES UDP port if other than the default",
	     10, &udp_port);
}


void
proto_reg_handoff_grapes(void)
{
	static gboolean initialized = FALSE;
	static dissector_handle_t grapes_handle;
	static int currentPort;

	if (!initialized) {
		grapes_handle = new_create_dissector_handle(dissect_grapes, proto_grapes);
		data_handle = find_dissector("data");
		initialized = TRUE;
	} else {
		dissector_delete("udp.port", currentPort, grapes_handle);
	}

	

	currentPort = udp_port;
	dissector_add("udp.port", currentPort, grapes_handle);
}
