/*
 * eigrp_const.h
 *
 *  Created on: Oct 28, 2015
 *      Author: Martin Kontsek
 */

#ifndef EIGRP_CONST_H_
#define EIGRP_CONST_H_

#define IF_NAME 				"veth0"

/* EIGRP Standard constants */
#define PROTO_EIGRP 			88
#define EIGRP_VERSION			2
#define EIGRP_MCAST				"224.0.0.10"
#define EIGRP_NEIGHBOR_ADDR		"192.0.2.2"
#define EIGRP_ASN				1
#define EIGRP_START_SEQNUM 		100

/* EIGRP Timers*/
#define HELLO_INTERVAL			100

/* EIGRP Packet Opcodes */
#define EIGRP_OPC_UPDATE 		1
#define EIGRP_OPC_QUERY 		3
#define EIGRP_OPC_REPLY 		4
#define EIGRP_OPC_HELLO 		5

/* EIGRP TLV Param */
#define EIGRP_TLV_PARAM_TYPE 	0x0001
#define EIGRP_TLV_PARAM_LEN 	12
#define EIGRP_TLV_PARAM_HOLD 	300

/* EIGRP TLV Version */
#define EIGRP_TLV_VER_TYPE 		0x0004
#define EIGRP_TLV_VER_LEN 		8
#define EIGRP_TLV_VER_REL	 	0x0c04
#define EIGRP_TLV_VER_TLVVER 	0x0102

/* EIGRP TLV Route */
#define EIGRP_TLV_ROUTE_TYPE 	0x0102
#define EIGRP_TLV_ROUTE_LEN 	28
#define EIGRP_TLV_ROUTE_NHOP 	0
#define EIGRP_TLV_ROUTE_PREFLEN 24
#define EIGRP_TLV_ROUTE_DELAY	128000
#define EIGRP_TLV_ROUTE_BW 		256
#define EIGRP_TLV_ROUTE_MTUHC 	0x0005ea00
#define EIGRP_TLV_ROUTE_RELIAB  255
#define EIGRP_TLV_ROUTE_LOAD 	1
#define EIGRP_MAX_DELAY			0xffffffffU

/* EIGRP TLV Route Flags */
#define EIGRP_ROUTE_FLAG_ACTIVE	0x04

/* EIGRP TLV Peer Termination */
#define EIGRP_TLV_TERM_TYPE		0x0007
#define EIGRP_TLV_TERM_LEN		9
#define EIGRP_TLV_TERM_UNK		0x04

#define EXIT_ERROR 				1


#endif /* EIGRP_CONST_H_ */
