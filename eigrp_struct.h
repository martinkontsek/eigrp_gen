/*
 * eigrp_struct.h
 *
 *  Created on: Nov 11, 2015
 *      Author: Martin Kontsek
 */

#ifndef EIGRP_STRUCT_H_
#define EIGRP_STRUCT_H_


struct EIGRP_Header_t
{
	unsigned char Version;
	unsigned char Opcode;
	unsigned short Checksum;
	unsigned int Flags;
	unsigned int SeqNum;
	unsigned int AckNum;
	unsigned short VrID;
	unsigned short ASN;
} __attribute__((packed));

struct EIGRP_TLV_Param_t
{
	unsigned short Type;
	unsigned short Length;
	unsigned char K1;
	unsigned char K2;
	unsigned char K3;
	unsigned char K4;
	unsigned char K5;
	unsigned char K6;
	unsigned short HoldTime;
} __attribute__((packed));

struct EIGRP_TLV_SW_Version_t
{
	unsigned short Type;
	unsigned short Length;
	unsigned short Release;
	unsigned short TLV_Ver;
} __attribute__((packed));

struct EIGRP_TLV_Route_t
{
	unsigned short Type;
	unsigned short Length;
	unsigned int NextHop;
	unsigned int Delay;
	unsigned int Bandwidth;
	unsigned int MTUaHC;
	unsigned char Reliability;
	unsigned char Load;
	unsigned char RouteTag;
	unsigned char Flags;
	unsigned char PrefixLen;
	unsigned char Dest1;
	unsigned char Dest2;
	unsigned char Dest3;
} __attribute__((packed));


#endif /* EIGRP_STRUCT_H_ */
