/******************************************************************************
 *
 * \file RTPParse.h
 * \brief rtp parser
 *        
 * \author Yunfang Che
 *
 * Copyright Aqueti 2017
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 *****************************************************************************/
//TODO new functionality fully implemented/tested, but still need to go back
//     and remove deprecated functions/variables after updating ACI
#ifndef RTP_H
#define RTP_H

#pragma once

//System header files
#include <arpa/inet.h>
#include <netdb.h>
#include <sys/utsname.h>

#include <string.h>

namespace atl
{

#define H264_NAL_TYPE_IDR  5

typedef enum RTP_H264_PAYLOAD_UNIT_TYPE
{
	RTP_PAYLOAD_FULL_H264_NAL = 0,
	RTP_PAYLOAD_FU_START_H264_NAL,
	RTP_PAYLOAD_FU_MIDDLE_H264_NAL,
	RTP_PAYLOAD_FU_END_H264_NAL,
}RTP_H264_PAYLOAD_UNIT_TYPE;

typedef struct RTP_packet_head
{
  unsigned char mark;	
  unsigned char playload_type;
  unsigned short seq;
  unsigned char timestamp[4]; 
  unsigned char ssrc[4];
}RTP_packet_head;

typedef struct RTCP_packet_head
{
	unsigned char vision;
	unsigned char padding;
	unsigned char count;
	unsigned char type;
	unsigned char len;
}RTCP_packet_head;

typedef struct RTCP_SR_Info
{
	unsigned char sender_ssrc[4];
	unsigned char ntp[8];
	unsigned char rtp_timestamp[4];
	unsigned long send_pack_count;
	unsigned long send_bytes_count;
}RTCP_SR_Info;

typedef struct RTCP_RR_Info
{
	unsigned char sender_ssrc[4];
	unsigned char ssrc_1[4];
	unsigned char fraction_lost;
	unsigned long cumulation_lost;    //Only using 3 bytes
	unsigned long highest_seq;
	unsigned long interarrival_jitter;
	unsigned char last_sr_timestamp[4];
	unsigned long delay_last_sr_timestamp;
}RTCP_RR_Info;

typedef struct RTCP_SDES_Info
{
	unsigned char ssrc[4];
	unsigned char cname;
	char text[32];
}RTCP_SDES_Info;

typedef struct ctLIB2A_2ACTRL_RESULT_S
{
	unsigned int gain;  	// 1024 means 1times
	unsigned int exp_time; // Unit:us
	unsigned int r_gain; 	// 256 means 1 times
	unsigned int gr_gain; 	// 256 means 1 times
	unsigned int gb_gain; 	// 256 means 1 times
	unsigned int b_gain; 	// 256 meands 1 times
}LIB2A_2ACTRL_RESULT_S;

class RTPParse
{
public:
	RTPParse(void);
	~RTPParse(void);

	static int parseRtpPacketHead(unsigned char* data, unsigned long dataLen, RTP_packet_head& packet_head, int& headLen);
	static int unpackRtpH264Payload(unsigned char*  data, unsigned int len, unsigned int headLen, unsigned char** h264DataPos,  \
		unsigned int& h264Len, unsigned char& nal_type, int& payloadType, LIB2A_2ACTRL_RESULT_S &priData);

	static int parseRtcpSenderReport(unsigned char* sr_data, unsigned int data_len, RTCP_SR_Info& sr_info);
	static int packRtcpReceiverReport(RTCP_RR_Info* rr_info, unsigned char* rr_buf, unsigned int rr_buf_len);

	static int packRtcpSdes(RTCP_SDES_Info* sdes_info, unsigned char* sdes_buf, unsigned int sdes_buf_len);

private:
	static int packRtcpHead(RTCP_packet_head* head_info, unsigned char* head_buf, unsigned int head_buf_len);

};


};
#endif
