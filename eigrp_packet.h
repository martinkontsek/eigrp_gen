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
 * Vypocita checksum EIGRP paketu z bloku dat (paStruct, paStructLen)
 * a pripocita aj checksum predchadzajuceho bloku dat (paStartChecksum)
 */
unsigned short calcChecksum(unsigned short paStartChecksum, void *paStruct, int paStructLen);

/*
 * Posle EIGRP paket
 *
 * paSocket - socket, cez ktory sa paket posle
 * paAddress - adresa, na ktoru sa paket odosle
 * paPacketType - typ EIGRP paketu (Hello, Update...)
 * paFlags - ake flagy ma paket obsahovat
 * paSeq - sekvencne cislo paketu, ak -1, tak si program cisluje sam
 * paAck - sekvencne cislo, ktore chceme potvrdit
 * paSendRoute - 1 ak chceme poslat v pakete cestu, 0 ak nie
 * paRouteType - index predkonfigurovanej cesty, ktora sa ma poslat
 * paMaxDelay - 1 ak chceme v pakete MAX hodnotu delay, 0 standardna hodnota
 */
void sendPacket(int paSocket, struct in_addr paAddress, unsigned char paPacketType,
		unsigned int paFlags, unsigned int paSeq, unsigned int paAck,
		int paSendRoute, int paRouteType, int paMaxDelay);

/*
 * Funkcia prijime EIGRP paket cez soket (paSoket)
 * a reaguje nan
 */
void receivePacket(int paSocket);

#endif /* EIGRP_PACKET_H_ */
