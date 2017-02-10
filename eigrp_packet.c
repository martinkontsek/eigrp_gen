/*
 * eigrp_packet.c
 *
 *  Created on: Nov 11, 2015
 *      Author: Martin Kontsek
 */

#include "eigrp_packet.h"

int Seq = EIGRP_START_SEQNUM;

/*
 * Compute checksum of EIGRP packet from block of data specified by (paStruct, paStructLen)
 * and adds also checksum of previous block of data (paStartChecksum)
 */
unsigned short calcChecksum(unsigned short paStartChecksum, void *paStruct, int paStructLen)
{
	int i;
	unsigned int checksum = paStartChecksum;
	unsigned short *smernik;

	/*
	 * Get 2 bytes of data, add the value to previous result,
	 * and check, if the variable has overflown
	 */
	smernik = (unsigned short *)paStruct;
	for(i=0; i<paStructLen/2; i++)
	{
		checksum += *(smernik+i);
		if(checksum > 0xffff)
			checksum -= 0xffff;
	}
	return checksum;
}

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
		int paSendRoute, int paRouteType, int paMaxDelay, int paGoodbye)
{
	struct sockaddr_in SendAddr;
	unsigned short checksum = 0;
	int StructCount = 0;
	struct iovec bufs[10];

	struct EIGRP_Header_t Header;
	struct EIGRP_TLV_Param_t TLV_Param;
	struct EIGRP_TLV_SW_Version_t TLV_Version;
	struct EIGRP_TLV_Route_t TLV_Route;
	struct EIGRP_TLV_Peer_Termination_t TLV_Termination;
	struct msghdr MsgHead;
	struct in_addr Neighbor_addr;

	if(inet_aton(EIGRP_NEIGHBOR_ADDR, &Neighbor_addr) == 0)
	{
		fprintf(stderr, "inet_aton: Error converting address\n");
		exit(EXIT_ERROR);
	}

	/* Prepare destination address */
	memset(&SendAddr, 0, sizeof(SendAddr));
	SendAddr.sin_family = AF_INET;
	SendAddr.sin_addr = paAddress;

	/* Header of EIGRP packet */
	memset(&Header, 0, sizeof(Header));
	Header.Version = EIGRP_VERSION;
	Header.Opcode = paPacketType;
	Header.Flags = htonl(paFlags);
	/* if paSeq -1, increment previously used sequence number value  */
	if(paSeq == -1)
	{
		Header.SeqNum = htonl(Seq);
		Seq++;
	}
	else
		Header.SeqNum = htonl(paSeq);
	Header.AckNum = htonl(paAck);
	Header.ASN = htons(EIGRP_ASN);
	bufs[StructCount].iov_base = &Header;
	bufs[StructCount].iov_len = sizeof(Header);
	checksum = calcChecksum(checksum, &Header, sizeof(Header));
	StructCount++;

	/* TLV inside HELLO packet */
	if(paPacketType == EIGRP_OPC_HELLO)
	{
		memset(&TLV_Param, 0, sizeof(TLV_Param));
		TLV_Param.Type = htons(EIGRP_TLV_PARAM_TYPE);
		TLV_Param.Length = htons(EIGRP_TLV_PARAM_LEN);
		if(paGoodbye == 1)
		{
			TLV_Param.K1 = 0xff;
			TLV_Param.K2 = 0xff;
			TLV_Param.K3 = 0xff;
			TLV_Param.K4 = 0xff;
			TLV_Param.K5 = 0xff;
			TLV_Param.K6 = 0xff;
		} else {
			TLV_Param.K1 = 1;
			TLV_Param.K3 = 1;
		}
		TLV_Param.HoldTime = htons(EIGRP_TLV_PARAM_HOLD);
		bufs[StructCount].iov_base = &TLV_Param;
		bufs[StructCount].iov_len = sizeof(TLV_Param);
		checksum = calcChecksum(checksum, &TLV_Param, sizeof(TLV_Param));
		StructCount++;

		memset(&TLV_Version, 0, sizeof(TLV_Version));
		TLV_Version.Type = htons(EIGRP_TLV_VER_TYPE);
		TLV_Version.Length = htons(EIGRP_TLV_VER_LEN);
		TLV_Version.Release = htons(EIGRP_TLV_VER_REL);
		TLV_Version.TLV_Ver = htons(EIGRP_TLV_VER_TLVVER);
		bufs[StructCount].iov_base = &TLV_Version;
		bufs[StructCount].iov_len = sizeof(TLV_Version);
		checksum = calcChecksum(checksum, &TLV_Version, sizeof(TLV_Version));
		StructCount++;

		if(paGoodbye == 2)
		{
			memset(&TLV_Termination, 0, sizeof(TLV_Termination));
			TLV_Termination.Type = htons(EIGRP_TLV_TERM_TYPE);
			TLV_Termination.Length = htons(EIGRP_TLV_TERM_LEN);
			TLV_Termination.Unknown = EIGRP_TLV_TERM_UNK;
			TLV_Termination.NeighborIP = Neighbor_addr.s_addr;
			bufs[StructCount].iov_base = &TLV_Termination;
			bufs[StructCount].iov_len = EIGRP_TLV_TERM_LEN;
			checksum = calcChecksum(checksum, &TLV_Termination, sizeof(TLV_Termination));
			StructCount++;
		}
	}

	if(paSendRoute == 1)
	{
		memset(&TLV_Route, 0, sizeof(TLV_Route));
		TLV_Route.Type = htons(EIGRP_TLV_ROUTE_TYPE);
		TLV_Route.Length = htons(EIGRP_TLV_ROUTE_LEN);
		TLV_Route.NextHop = htonl(EIGRP_TLV_ROUTE_NHOP);
		/* QUERY packet will have MAX delay and active route flag */
		if(paPacketType == EIGRP_OPC_QUERY)
		{
			TLV_Route.Delay = htonl(EIGRP_MAX_DELAY);
			TLV_Route.Flags = EIGRP_ROUTE_FLAG_ACTIVE;
		} else {
			TLV_Route.Delay = htonl(EIGRP_TLV_ROUTE_DELAY);
			TLV_Route.Bandwidth = htonl(EIGRP_TLV_ROUTE_BW);
			TLV_Route.MTUaHC = htonl(EIGRP_TLV_ROUTE_MTUHC);
			TLV_Route.Reliability = EIGRP_TLV_ROUTE_RELIAB;
			TLV_Route.Load = EIGRP_TLV_ROUTE_LOAD;
		}
		/* if paMaxDelay is 1, set MAX delay */
		if(paMaxDelay == 1)
			TLV_Route.Delay = htonl(EIGRP_MAX_DELAY);
		TLV_Route.PrefixLen = 24;
		/* put route inside packet */
		if(paRouteType == 0)
		{
			TLV_Route.Dest1 = 10;
			TLV_Route.Dest2 = 10;
			TLV_Route.Dest3 = 5;
		} else if(paRouteType == 1)
		{
			TLV_Route.Dest1 = 192;
			TLV_Route.Dest2 = 9;
			TLV_Route.Dest3 = 9;
		}
		bufs[StructCount].iov_base = &TLV_Route;
		bufs[StructCount].iov_len = sizeof(TLV_Route);
		checksum = calcChecksum(checksum, &TLV_Route, sizeof(TLV_Route));
		StructCount++;
	}

	/* Save bitwise complement of computed checksum into header */
	Header.Checksum = ~checksum;

	/* Prepare Scatter-Gatter struct */
	memset(&MsgHead, 0, sizeof(MsgHead));
	MsgHead.msg_name = &SendAddr;
	MsgHead.msg_namelen = sizeof(SendAddr);
	MsgHead.msg_iov = (void *)bufs;
	MsgHead.msg_iovlen = StructCount;

	/* Send packet */
	if(sendmsg(paSocket, &MsgHead, 0) == -1)
	{
		perror("sendmsg");
		close(paSocket);
		exit(EXIT_ERROR);
	}
}

/*
 * Receive EIGRP packet cez socket (paSoket)
 * and respond to received packet
 */
void receivePacket(int paSocket)
{
	char RecvPacket[10000];
		ssize_t Bytes;
		struct sockaddr_in RecvAddr;
		socklen_t AddrLen = sizeof(RecvAddr);
		struct iphdr *IP_Header;
		struct EIGRP_Header_t *RecvHdr;

		struct in_addr SendAddr;
		if(inet_aton(EIGRP_MCAST, &SendAddr) == 0)
		{
			fprintf(stderr, "inet_aton: Invalid destination Address\n");
			close(paSocket);
			exit(EXIT_ERROR);
		}

		memset(&RecvPacket, 0, 10000);
		memset(&RecvAddr, 0, AddrLen);
		if((Bytes = recvfrom(paSocket, RecvPacket, 10000, 0, (struct sockaddr *)&RecvAddr, &AddrLen)) == -1)
		{
			perror("recvfrom");
			return;
		}

		IP_Header = (struct iphdr *)RecvPacket;
		RecvHdr = (struct EIGRP_Header_t *)(RecvPacket+20);
		int opc = RecvHdr->Opcode;
		struct in_addr src, dst;
		src.s_addr = IP_Header->saddr;
		dst.s_addr = IP_Header->daddr;

		/* Print info about reveived packets */
//		char src_s[15];
//		strcpy(&src_s, inet_ntoa(src));
//		char dst_s[15];
//		strcpy(&dst_s, inet_ntoa(dst));
//
//		printf("Packet from %s to %s (%ld) Proto %d :\n\tOpcode: %s (%d)\n\tFlags: %x\n\tSeq: %d\n\tAck: %d\n\tAs: %d\n\n",
//				src_s,
//				dst_s,
//				Bytes,
//				IP_Header->protocol,
//				(opc==EIGRP_OPC_HELLO)?"Hello":
//						((opc==EIGRP_OPC_UPDATE)?"Update":
//								((opc==EIGRP_OPC_QUERY)?"Query":
//										((opc==EIGRP_OPC_REPLY)?"Reply":opc))),
//				opc,
//				ntohl(RecvHdr->Flags),
//				ntohl(RecvHdr->SeqNum),
//				ntohl(RecvHdr->AckNum),
//				ntohs(RecvHdr->ASN));


		if(ntohl(RecvHdr->SeqNum) != 0  && opc != EIGRP_OPC_HELLO)
		{
			//send ACK packet
			sendPacket(paSocket, src, EIGRP_OPC_HELLO, 0, 0, ntohl(RecvHdr->SeqNum), 0, 0, 0, 0);
		}

		/* Respond to UPDATE packet with INIT flag */
		if((opc==EIGRP_OPC_UPDATE) && (ntohl(RecvHdr->Flags) == 1))
		{
			sendPacket(paSocket, src, EIGRP_OPC_UPDATE, 1, Seq, 0, 0, 0, 0, 0);
			NeiAddr = src;
			Seq++;
		}

}
