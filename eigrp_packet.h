/*
 * eigrp_packet.h
 *
 *  Created on: Oct 28, 2015
 *      Author: Martin Kontsek
 */

#ifndef EIGRP_PACKET_H_
#define EIGRP_PACKET_H_

#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <net/if.h>

#include "eigrp_struct.h"
#include "eigrp_const.h"

struct in_addr NeiAddr;

/*
 * Compute checksum of EIGRP packet from block of data specified by (paStruct, paStructLen)
 * and adds also checksum of previous block of data (paStartChecksum)
 */
unsigned short calcChecksum(unsigned short paStartChecksum, void *paStruct, int paStructLen);

/*
 * Send EIGRP packet
 *
 * paSocket - socket for sending packet
 * paAddress - destination address
 * paPacketType - type of EIGRP packet (Hello, Update...)
 * paFlags - flags inside of EIGRP packet
 * paSeq - sequence number of packet, if -1 is specified, application will do autoincrement
 * paAck - acknowledge number
 * paSendRoute - 1 if you want add IPv4 route TLV into packet, 0 without route
 * paRouteType - choose which preconfigured IPv4 route use, effective only if paSendRoute is set to 1
 * paMaxDelay - 1 will send packet with MAX delay value, 0 standard delay value
 * paGoodbye - 1 all the K values inside of HELLO packet will be set to 0xff (Goodbye)
 */
void sendPacket(int paSocket, struct in_addr paAddress, unsigned char paPacketType,
		unsigned int paFlags, unsigned int paSeq, unsigned int paAck,
		int paSendRoute, int paRouteType, int paMaxDelay, int paGoodbye);

/*
 * Receive EIGRP packet cez socket (paSoket)
 * and respond to received packet
 */
void receivePacket(int paSocket);

#endif /* EIGRP_PACKET_H_ */
