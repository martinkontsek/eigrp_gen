/*
 * eigrp_packet.c
 *
 *  Created on: Nov 11, 2015
 *      Author: root
 */

#include "eigrp_packet.h"

int Seq = 100;
int jeSusedstvo = 0;
struct in_addr NeiAddr;

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

		if((opc==1) && (ntohl(RecvHdr->Flags) == 1))
		{
			sendPacket(Socket, src, EIGRP_OPC_UPDATE, 1, Seq, ntohl(RecvHdr->SeqNum));
			NeiAddr = src;
			Seq++;
			return;
		}

		if((opc==1) && (ntohl(RecvHdr->Flags) == 8))
		{
			sendPacket(Socket, src, EIGRP_OPC_UPDATE, 8, Seq, ntohl(RecvHdr->SeqNum));
			jeSusedstvo = 1;
			Seq++;
			return;
		}


		if(jeSusedstvo == 1)
		{
			printf("Test Paket.");
			//starts graceful restart
			sendPacket(Socket, NeiAddr, EIGRP_OPC_UPDATE, 5, Seq, ntohl(0));
			//sendPacket(Socket, NeiAddr, EIGRP_OPC_UPDATE, 15, Seq, ntohl(0));
			jeSusedstvo = 0;
			Seq++;
			return;
		}

}
