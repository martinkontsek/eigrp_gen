/*
 * eigrp_packet.c
 *
 *  Created on: Nov 11, 2015
 *      Author: root
 */

#include "eigrp_packet.h"


int jeSusedstvo = 0;
int Seq = 100;
int cakamNaAck = 0;


unsigned short calcChecksum(void *paStruct, int paStructLen)
{
	int i;
	unsigned short checksum = 0;
	unsigned short *smernik;

	smernik = (unsigned short *)paStruct;
	for(i=0; i<paStructLen/2; i++)
	{
		checksum += *(smernik+i);
	}

	return checksum;
}

void sendPacket(int Socket, struct in_addr paAddress, unsigned char paPacketType,
		unsigned int paFlags, unsigned int paSeq, unsigned int paAck)
{
	struct sockaddr_in SendAddr;
	unsigned short checksum = 0;
	int StructCount = 0;
	struct iovec bufs[10];

	struct EIGRP_Header_t Header;
	struct EIGRP_TLV_Param_t TLV_Param;
	struct EIGRP_TLV_SW_Version_t TLV_Version;
	struct EIGRP_TLV_Route_t TLV_Route;
	struct msghdr MsgHead;


	memset(&SendAddr, 0, sizeof(SendAddr));
	SendAddr.sin_family = AF_INET;
	SendAddr.sin_addr = paAddress;


	memset(&Header, 0, sizeof(Header));
	Header.Version = EIGRP_VERSION;
	Header.Opcode = paPacketType;
	Header.Flags = htonl(paFlags);
	Header.SeqNum = htonl(paSeq);
	Header.AckNum = htonl(paAck);
	Header.ASN = htons(EIGRP_ASN);
	bufs[StructCount].iov_base = &Header;
	bufs[StructCount].iov_len = sizeof(Header);
	checksum += calcChecksum(&Header, sizeof(Header));
	StructCount++;

	if(paPacketType == EIGRP_OPC_HELLO)
	{

		memset(&TLV_Param, 0, sizeof(TLV_Param));
		TLV_Param.Type = htons(EIGRP_TLV_PARAM_TYPE);
		TLV_Param.Length = htons(EIGRP_TLV_PARAM_LEN);
		TLV_Param.K1 = 1;
		TLV_Param.K3 = 1;
		TLV_Param.HoldTime = htons(EIGRP_TLV_PARAM_HOLD);
		bufs[StructCount].iov_base = &TLV_Param;
		bufs[StructCount].iov_len = sizeof(TLV_Param);
		checksum += calcChecksum(&TLV_Param, sizeof(TLV_Param));
		StructCount++;


		memset(&TLV_Version, 0, sizeof(TLV_Version));
		TLV_Version.Type = htons(EIGRP_TLV_VER_TYPE);
		TLV_Version.Length = htons(EIGRP_TLV_VER_LEN);
		TLV_Version.Release = htons(EIGRP_TLV_VER_REL);
		TLV_Version.TLV_Ver = htons(EIGRP_TLV_VER_TLVVER);
		bufs[StructCount].iov_base = &TLV_Version;
		bufs[StructCount].iov_len = sizeof(TLV_Version);
		checksum += calcChecksum(&TLV_Version, sizeof(TLV_Version));
		StructCount++;
	}

	if( (paPacketType == EIGRP_OPC_UPDATE) && ((paFlags == 0) || (paFlags == 10)) )
	{
		memset(&TLV_Route, 0, sizeof(TLV_Route));
		TLV_Route.Type = htons(EIGRP_TLV_ROUTE_TYPE);
		TLV_Route.Length = htons(EIGRP_TLV_ROUTE_LEN);
		TLV_Route.NextHop = htonl(EIGRP_TLV_ROUTE_NHOP);
		TLV_Route.Delay = htonl(EIGRP_TLV_ROUTE_DELAY);
		TLV_Route.Bandwidth = htonl(EIGRP_TLV_ROUTE_BW);
		TLV_Route.MTUaHC = htonl(EIGRP_TLV_ROUTE_MTUHC);
		TLV_Route.Reliability = EIGRP_TLV_ROUTE_RELIAB;
		TLV_Route.Load = EIGRP_TLV_ROUTE_LOAD;
		TLV_Route.PrefixLen = 24;
		TLV_Route.Dest1 = 10;
		TLV_Route.Dest2 = 10;
		TLV_Route.Dest3 = 5;
		bufs[StructCount].iov_base = &TLV_Route;
		bufs[StructCount].iov_len = sizeof(TLV_Route);
		checksum += calcChecksum(&TLV_Route, sizeof(TLV_Route));
		StructCount++;
	}


	Header.Checksum = ~checksum;

	memset(&MsgHead, 0, sizeof(MsgHead));
	MsgHead.msg_name = &SendAddr;
	MsgHead.msg_namelen = sizeof(SendAddr);
	MsgHead.msg_iov = (void *)bufs;
	MsgHead.msg_iovlen = StructCount;


	if(sendmsg(Socket, &MsgHead, 0) == -1)
	{
		perror("sendmsg");
		close(Socket);
		exit(EXIT_ERROR);
	}
}

void receivePacket(int Socket)
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
			close(Socket);
			exit(EXIT_ERROR);
		}

		memset(&RecvPacket, 0, 10000);
		memset(&RecvAddr, 0, AddrLen);
		if((Bytes = recvfrom(Socket, RecvPacket, 10000, 0, (struct sockaddr *)&RecvAddr, &AddrLen)) == -1)
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
//				(opc==5)?"Hello":((opc==1)?"Update":opc),
//				opc,
//				ntohl(RecvHdr->Flags),
//				ntohl(RecvHdr->SeqNum),
//				ntohl(RecvHdr->AckNum),
//				ntohs(RecvHdr->ASN));

		if(cakamNaAck != 0)
		{
			if(opc== EIGRP_OPC_HELLO && (ntohl(RecvHdr->AckNum) == cakamNaAck))
			{
				cakamNaAck = 0;
				if(jeSusedstvo == 2)
					jeSusedstvo = 1;

				if(jeSusedstvo == 1)
					jeSusedstvo = 0;
				return;
			}
		}

		if((opc==EIGRP_OPC_UPDATE) && (ntohl(RecvHdr->Flags) == 1))
		{
			if(jeSusedstvo == 0)
			{
				sendPacket(Socket, src, EIGRP_OPC_UPDATE, 1, Seq, 0);
				NeiAddr = src;
				jeSusedstvo = 3;
			}

			if(jeSusedstvo == 3)
			{
				sendPacket(Socket, src, EIGRP_OPC_UPDATE, 1, Seq, ntohl(RecvHdr->SeqNum));
				NeiAddr = src;
				jeSusedstvo = 2;
				cakamNaAck = Seq;
				Seq++;
			}
			return;

//			sendPacket(Socket, SendAddr, EIGRP_OPC_UPDATE, 0, Seq, 0);
//			Seq++;
//			return;
		}

//		if((opc==1) && ((ntohl(RecvHdr->Flags) == 8) || (ntohl(RecvHdr->Flags) == 0)))
//		{
//			sendPacket(Socket, SendAddr, EIGRP_OPC_UPDATE, 0, Seq, 0);
//			//sendPacket(Socket, src, EIGRP_OPC_HELLO, 0, 0, ntohl(RecvHdr->SeqNum));
//			jeSusedstvo = 1;
//			Seq++;
//			//return;
//		}

		if(jeSusedstvo == 1)
		{
			sendPacket(Socket, SendAddr, EIGRP_OPC_UPDATE, 0, Seq, 0);
			cakamNaAck = Seq;
			Seq++;
			return;
		}

		if(ntohl(RecvHdr->SeqNum) != 0 )
		{
			//send ack packet
			sendPacket(Socket, src, EIGRP_OPC_HELLO, 0, 0, ntohl(RecvHdr->SeqNum));
		}


}
