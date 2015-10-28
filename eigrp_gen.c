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



int main(void)
{
	int Socket;
	struct sockaddr_in SockAddr;

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


	struct EIGRP_Header_t Header;
	memset(&Header, 0, sizeof(Header));
	Header.Version = EIGRP_VERSION;
	Header.Opcode = EIGRP_OPC_HELLO;  //Hello packet
	Header.Flags = htonl(0x0000);
	Header.ASN = htons(100);

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
	unsigned short *smernik;
	int i;

	smernik = (unsigned short *)&Header;
	for(i=0; i<sizeof(Header)/2; i++)
	{
		checksum += *(smernik+i);
	}
	smernik = (unsigned short *)&TLV_Param;
	for(i=0; i<sizeof(TLV_Param)/2; i++)
	{
		checksum += *(smernik+i);
	}
	smernik = (unsigned short *)&TLV_Version;
	for(i=0; i<sizeof(TLV_Version)/2; i++)
	{
		checksum += *(smernik+i);
	}

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


	if(sendmsg(Socket, &MsgHead, 0) == -1)
	{
		perror("sendmsg");
		close(Socket);
		exit(EXIT_ERROR);
	}

	return 0;
}
