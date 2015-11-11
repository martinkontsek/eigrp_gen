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
#include <netinet/ip.h>
#include <arpa/inet.h>
#include <net/if.h>
#include <pthread.h>

#include "eigrp_packet.h"
#include "eigrp_struct.h"
#include "eigrp_const.h"


void helloThread(void *arg)
{
	int Socket = arg;
	struct in_addr SendAddr;
	if(inet_aton(EIGRP_MCAST, &SendAddr) == 0)
	{
		fprintf(stderr, "inet_aton: Invalid destination Address\n");
		close(Socket);
		exit(EXIT_ERROR);
	}

	for(;;)
	{
		sendPacket(Socket, SendAddr, EIGRP_OPC_HELLO, 0, 0, 0);
		sleep(5);
	}
}

int main(void)
{
	int Socket;
	struct ip_mreqn MultiJoin;
	unsigned int IFindex;

	if((Socket = socket(AF_INET, SOCK_RAW, PROTO_EIGRP)) == -1)
	{
		perror("socket");
		exit(EXIT_ERROR);
	}

	if((IFindex = if_nametoindex(IF_NAME)) == 0)
	{
		perror("if_nametoindex");
		close(Socket);
		exit(EXIT_ERROR);
	}

	if(setsockopt(Socket, SOL_SOCKET, SO_BINDTODEVICE, IF_NAME, 5) == -1)
	{
		perror("setsockopt_sol");
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
	MultiJoin.imr_ifindex = IFindex;

	if(setsockopt(Socket, IPPROTO_IP, IP_ADD_MEMBERSHIP, &MultiJoin, sizeof(MultiJoin)) == -1)
	{
		perror("setsockopt_multicast");
		close(Socket);
		exit(EXIT_ERROR);
	}

	pthread_t SendHelloThread;
	pthread_create(&SendHelloThread, NULL, helloThread, (void *) Socket);


	//sendPacket(Socket);


	for(;;)
	{
		receivePacket(Socket);
	}

	close(Socket);
	return 0;
}
