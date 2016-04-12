/*
 * rigrp_gen.c
 *
 *  Created on: Oct 21, 2015
 *      Author: Martin Kontsek
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

/*
 * Vo vlakne posiela HELLO pakety
 */
void * helloThread(void *arg)
{
	int Socket = *((int *)arg);
	struct in_addr SendAddr;
	if(inet_aton(EIGRP_MCAST, &SendAddr) == 0)
	{
		fprintf(stderr, "inet_aton: Invalid destination Address\n");
		close(Socket);
		exit(EXIT_ERROR);
	}

	for(;;)
	{
		sendPacket(Socket, SendAddr, EIGRP_OPC_HELLO, 0, 0, 0, 0, 0, 0, 0);
		sleep(HELLO_INTERVAL);
	}

	return 0;
}

/*
 * Posielanie pouzivatelom zadanych paketov
 */
void * sendUserThread(void *arg)
{
	int Socket = *((int *)arg);
	struct in_addr SendAddr, MultiAddr;
	unsigned char packetType;
	unsigned int flags, seqNum, ackNum;
	int hello, sendRoute, routeType, maxDelay, goodbye;

	if(inet_aton(EIGRP_MCAST, &MultiAddr) == 0)
	{
		fprintf(stderr, "inet_aton: Invalid destination Address\n");
		close(Socket);
		exit(EXIT_ERROR);
	}

	char buffer[10];

	for(;;)
	{
		seqNum = 0;
		ackNum = 0;
		sendRoute = 0;
		routeType = 0;
		maxDelay = 0;
		goodbye = 0;

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
			maxDelay = 1;
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

		if(hello)
		{
			memset(buffer, '\0', 10);
			printf("Do you want to send Goodbye? no(0),all(1),one(2):");
			fgets(buffer, 10, stdin);
			goodbye = atoi(buffer);
		} else {
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

				if(packetType != EIGRP_OPC_QUERY)
				{
					memset(buffer, '\0', 10);
					printf("Do you want to set max delay (y/n):");
					fgets(buffer, 10, stdin);
					if(buffer[0] == 'y')
						maxDelay = 1;
				}
			}
		}

		printf("\n\n");

		sendPacket(Socket, SendAddr, packetType, flags, seqNum,
				ackNum, sendRoute, routeType, maxDelay, goodbye);
	}

	return 0;
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

	/* Posielanie paketov cez pozadovane rozhranie */
	if(setsockopt(Socket, SOL_SOCKET, SO_BINDTODEVICE, IF_NAME, 5) == -1)
	{
		perror("setsockopt_sol");
		close(Socket);
		exit(EXIT_ERROR);
	}

	/* clenstvo v multicast skupine */
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

	/* Vlakno na periodicke posielanie HELLO paketov */
//	pthread_t SendHelloThread;
//	pthread_create(&SendHelloThread, NULL, helloThread, (void *) Socket);

	/* Vlakno na posielanie paketov uzivatelom */
	pthread_t SendFromUserThread;
	pthread_create(&SendFromUserThread, NULL, sendUserThread, (void *) &Socket);

	/* prijima pakety a reaguje na ne */
	for(;;)
	{
		receivePacket(Socket);
	}

	close(Socket);
	return 0;
}
