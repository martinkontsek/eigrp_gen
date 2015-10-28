/*
 * rigrp_gen.c
 *
 *  Created on: Oct 21, 2015
 *      Author: Martin
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "eigrp_packet.h"
#include "eigrp_const.h"

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

int main(void)
{
	int Socket;
	struct sockaddr_in SockAddr;
	struct ip_mreqn MultiJoin;

	if((Socket = socket(AF_INET, SOCK_RAW, PROTO_EIGRP)) == -1)
	{
		perror("socket");
		exit(EXIT_ERROR);
	}

	memset(&SockAddr, 0, sizeof(SockAddr));
	SockAddr.sin_family = AF_INET;

	if(inet_aton(EIGRP_MCAST, &SockAddr.sin_addr) == 0)
	{
		fprintf(stderr, "inet_aton: Invalid destination Address\n");
		close(Socket);
		exit(EXIT_ERROR);
	}

	if(bind(Socket, (struct sockaddr *) &SockAddr, sizeof(SockAddr)) == -1)
	{
		perror("bind");
		close(Socket);
		exit(EXIT_ERROR);
	}

	//clenstvo v multicast skupine
	if(inet_aton(EIGRP_MCAST, &MultiJoin.imr_multiaddr) == 0)
	{
		fprintf(stderr, "inet_aton: Invalid multicast address\n");
		close(Socket);
		exit(EXIT_ERROR);
	}
	MultiJoin.imr_address.s_addr = INADDR_ANY;
	MultiJoin.imr_ifindex = 0;

	if(setsockopt(Socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &MultiJoin, sizeof(MultiJoin)) == -1)
	{
		perror("setsockopt");
		close(Socket);
		exit(EXIT_ERROR);
	}


	/* Inicializacia EIGRP struktur */

	struct EIGRP_Header_t Header;
	memset(&Header, 0, sizeof(Header));
	Header.Version = EIGRP_VERSION;
	Header.Opcode = EIGRP_OPC_HELLO;
	Header.Flags = htonl(0x0000);
	Header.ASN = htons(1);

	struct EIGRP_TLV_Param_t TLV_Param;
	memset(&TLV_Param, 0, sizeof(TLV_Param));
	TLV_Param.Type = htons(EIGRP_TLV_PARAM_TYPE);
	TLV_Param.Length = htons(EIGRP_TLV_PARAM_LEN);
	TLV_Param.K1 = 1;
	TLV_Param.K3 = 1;
	TLV_Param.HoldTime = htons(EIGRP_TLV_PARAM_HOLD);

	struct EIGRP_TLV_SW_Version_t TLV_Version;
	memset(&TLV_Version, 0, sizeof(TLV_Version));
	TLV_Version.Type = htons(EIGRP_TLV_VER_TYPE);
	TLV_Version.Length = htons(EIGRP_TLV_VER_LEN);
	TLV_Version.Release = htons(EIGRP_TLV_VER_REL);
	TLV_Version.TLV_Ver = htons(EIGRP_TLV_VER_TLVVER);

	//vypocet Checksumu
	unsigned short checksum = 0;
	checksum += calcChecksum(&Header, sizeof(Header));
	checksum += calcChecksum(&TLV_Param, sizeof(TLV_Param));
	checksum += calcChecksum(&TLV_Version, sizeof(TLV_Version));
	Header.Checksum = ~checksum;


	struct iovec bufs[3];
	bufs[0].iov_base = &Header;
	bufs[0].iov_len = sizeof(Header);
	bufs[1].iov_base = &TLV_Param;
	bufs[1].iov_len = sizeof(TLV_Param);
	bufs[2].iov_base = &TLV_Version;
	bufs[2].iov_len = sizeof(TLV_Version);

	struct msghdr MsgHead;
	memset(&MsgHead, 0, sizeof(MsgHead));
	MsgHead.msg_name = &SockAddr;
	MsgHead.msg_namelen = sizeof(SockAddr);
	MsgHead.msg_iov = (void *)bufs;
	MsgHead.msg_iovlen = 3;


	char RecvPacket[1000];
	ssize_t Bytes;
	socklen_t AddrLen = sizeof(SockAddr);
	for(;;)
	{
		if(sendmsg(Socket, &MsgHead, 0) == -1)
		{
			perror("sendmsg");
			close(Socket);
			exit(EXIT_ERROR);
		}

		memset(&RecvPacket, 0, 1000);
		if((Bytes = recvfrom(Socket, &RecvPacket, 1000, 0, (struct sockaddr *)&SockAddr, &AddrLen)) == -1)
		{
			perror("recvfrom");
			break;
		}

		struct EIGRP_Header_t *RecvHdr = (struct EIGRP_Header_t *)&RecvPacket;
		printf("Packet from %s (%ld):\n\tOpcode: %d\n\tFlags: %x\n\tSeq: %d\n\tAck: %d\n\n",
				inet_ntoa(SockAddr.sin_addr),
				Bytes,
				RecvHdr->Opcode,
				ntohl(RecvHdr->Flags),
				ntohl(RecvHdr->SeqNum),
				ntohl(RecvHdr->AckNum));

	}

	close(Socket);
	return 0;
}
