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
		sendPacket(Socket, SendAddr, EIGRP_OPC_HELLO, 0, 0, 0, 0, 0);
		sleep(HELLO_INTERVAL);
	}
}

void sendUserThread(void *arg)
{
	int Socket = arg;
	struct in_addr SendAddr, MultiAddr;
	unsigned char packetType;
	unsigned int flags;
	unsigned int seqNum;
	unsigned int ackNum;
	int sendRoute = 0;
	int routeType = 0;

	if(inet_aton(EIGRP_MCAST, &MultiAddr) == 0)
	{
		fprintf(stderr, "inet_aton: Invalid destination Address\n");
		close(Socket);
		exit(EXIT_ERROR);
	}

	char buffer[10];
	int hello = 0;

	for(;;)
	{
		seqNum = 0;
		ackNum = 0;

		printf("Input packet address and packet type [m,u][h,q,r,u]:");
		fgets(buffer, 10, stdin);

		if(buffer[0] == 'm')
			SendAddr = MultiAddr;
		else
			SendAddr = NeiAddr;

		if(buffer[1] == 'h')
		{
			packetType = EIGRP_OPC_HELLO;
			hello = 1;
		}
		else if(buffer[1] == 'q')
		{
			packetType = EIGRP_OPC_QUERY;
			hello = 0;
		}
		else if(buffer[1] == 'r')
		{
			packetType = EIGRP_OPC_REPLY;
			hello = 0;
		}
		else if(buffer[1] == 'u')
		{
			packetType = EIGRP_OPC_UPDATE;
			hello = 0;
		}

		memset(buffer, '\0', 10);
		printf("Input packet flags:");
		fgets(buffer, 10, stdin);
		flags = atoi(buffer);

		if(!hello)
		{
			memset(buffer, '\0', 10);
			printf("Input packet sequence number:");
			fgets(buffer, 10, stdin);
			seqNum = atoi(buffer);

			memset(buffer, '\0', 10);
			printf("Input packet ack number:");
			fgets(buffer, 10, stdin);
			ackNum = atoi(buffer);

			memset(buffer, '\0', 10);
			printf("Do you want to send route (y/n):");
			fgets(buffer, 10, stdin);
			if(buffer[0] == 'y')
			{
				sendRoute = 1;

				memset(buffer, '\0', 10);
				printf("Input route index:");
				fgets(buffer, 10, stdin);
				routeType = atoi(buffer);
			}
		}

		printf("\n\n");

		sendPacket(Socket, SendAddr, packetType, flags, seqNum, ackNum, sendRoute, routeType);
		//Seq++;
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

//	pthread_t SendHelloThread;
//	pthread_create(&SendHelloThread, NULL, helloThread, (void *) Socket);


	pthread_t SendFromUserThread;
	pthread_create(&SendFromUserThread, NULL, sendUserThread, (void *) Socket);


	for(;;)
	{
		receivePacket(Socket);
	}

	close(Socket);
	return 0;
}
