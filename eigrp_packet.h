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


unsigned short calcChecksum(unsigned short paStartChecksum, void *paStruct, int paStructLen);
void sendPacket(int paSocket, struct in_addr paAddress, unsigned char paPacketType,
		unsigned int paFlags, unsigned int paSeq, unsigned int paAck,
		int paSendRoute, int paRouteType, int paMaxDelay);
void receivePacket(int paSocket);

#endif /* EIGRP_PACKET_H_ */
