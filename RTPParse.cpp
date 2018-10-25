/******************************************************************************
 *
 * \file RTPParse.cpp
 * \brief 
 *        
 * \author Yunfang Che
 *
 * Copyright Aqueti 2017
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 *****************************************************************************/

#include "RTPParse.h"


namespace atl
{

#define RTP_FIXED_PACKETHEAD_LEN 12
#define RTP_VERSION_NUM 2

#define RTCP_SR_TYPEID 200
#define RTCP_RR_TYPEID 201
#define RTCP_SDES_TYPEID 202

unsigned int ASCII2UINT32(unsigned char *str)
{
	unsigned int re = 0;
	int i;
	for(i = 0; i < 8;i++)
	{
		if(str[i] >= '0' && str[i] <= '9')
		{
			re = re <<4 | (str[i]-'0');
		}
		else if((str[i] >= 'A' && str[i] <= 'F') || (str[i] >= 'a' && str[i] <= 'f'))
		{
			re = re <<4 | (str[i]-'A'+10);
		}
		else
		{
			re = -1;
			break;
		}
	}
	return re;
}



RTPParse::RTPParse(void)
{
}

RTPParse::~RTPParse(void)
{
}

int RTPParse::parseRtpPacketHead(unsigned char* data, unsigned long dataLen, RTP_packet_head& packet_head, int& headLen)
{
	if(dataLen < RTP_FIXED_PACKETHEAD_LEN)
		return -1;

	unsigned char version = (unsigned char)(data[0] >> 6);
	if(version != RTP_VERSION_NUM)
		return -1;

	packet_head.mark = (unsigned char)(data[1] >> 7);
	packet_head.playload_type = data[1] & 0x7F;
	packet_head.seq = ntohs(*(unsigned short*)(data+2));
	
	memcpy(packet_head.timestamp, data+4, 4);
	memcpy(packet_head.ssrc, data+8, 4);

	int index = 0;
	unsigned char csrc_count = data[0] & 0x0F;
	if(csrc_count == 0)
	{
		index = RTP_FIXED_PACKETHEAD_LEN;
	}
	else
	{
		index = RTP_FIXED_PACKETHEAD_LEN + csrc_count*4;
		if(dataLen < index)
			return -1;
	}


	unsigned char extend_flag = (data[0] >> 4) & 0x01;
	if(extend_flag == 1)
	{
		if(dataLen < index+4)
			return -1;

		index+=2;
		int extend_head_count =  ntohs(*(unsigned short*)(data+index));
		
		index += 2+extend_head_count*4;
		if(dataLen < index)
			return -1;
	}

	headLen = index;
	
	return 0;
}


int  RTPParse::unpackRtpH264Payload(unsigned char*  data, unsigned int len, unsigned int headLen, unsigned char** h264DataPos
								 ,  unsigned int& h264Len, unsigned char& nal_type, int& payloadType, LIB2A_2ACTRL_RESULT_S &priData)
 {
	 h264Len = 0;

	 if (len < headLen)
	 {
		 return -1;
	 }  
 
	 unsigned char* rtpPlayloadPos = data+headLen;
	 unsigned char fu_indicator = rtpPlayloadPos[0];
	 unsigned char fu_head = rtpPlayloadPos[1];
	 unsigned char fu_indicator_type = fu_indicator & 0x1F;
	 nal_type = fu_head & 0x1F;

	 if(fu_indicator_type == 28)  //FU-A slice
	 {
		 unsigned char fu_head_ser = (unsigned char)(fu_head & 0xE0);

		 switch(fu_head_ser)
		 {
		 case 0x80:			//nal start
			 {
			 	if(nal_type == 0x01 || nal_type == 0x07)
			 	{
			 		priData.gain = ASCII2UINT32(rtpPlayloadPos +2);
					priData.exp_time = ASCII2UINT32(rtpPlayloadPos +2+8);
					priData.r_gain = ASCII2UINT32(rtpPlayloadPos +2+8*2);
					priData.gr_gain = ASCII2UINT32(rtpPlayloadPos +2+8*3);
					priData.gb_gain = ASCII2UINT32(rtpPlayloadPos +2+8*4);
					priData.b_gain = ASCII2UINT32(rtpPlayloadPos +2+8*5);
					
			 		*h264DataPos = rtpPlayloadPos + 48 - 3;
					*(*h264DataPos) = 0x00;
					*((*h264DataPos)+1) = 0x00;
					*((*h264DataPos)+2) = 0x00;
					*((*h264DataPos)+3) = 0x01;
					*((*h264DataPos)+4) = (fu_indicator & 0xE0) | (fu_head & 0x1F);
					h264Len = len - headLen - 48 + 3;
			 	}
				else
				{
					*h264DataPos = rtpPlayloadPos-3;
					*(*h264DataPos) = 0x00;
					*((*h264DataPos)+1) = 0x00;
					*((*h264DataPos)+2) = 0x00;
					*((*h264DataPos)+3) = 0x01;
					*((*h264DataPos)+4) = (fu_indicator & 0xE0) | (fu_head & 0x1F);
					h264Len = len - headLen + 3;
				}
				 
				 
				 payloadType = RTP_PAYLOAD_FU_START_H264_NAL;
			 }

			 break;
		 case 0x00:		//nal middle
			 {
				 *h264DataPos = rtpPlayloadPos+2;
				 h264Len = len - headLen - 2;
				 payloadType = RTP_PAYLOAD_FU_MIDDLE_H264_NAL;
			 }
			 break;
		 case 0x40:		//nal end
			 {
				 *h264DataPos = rtpPlayloadPos+2;
				 h264Len = len - headLen - 2;
				 payloadType = RTP_PAYLOAD_FU_END_H264_NAL;
			 }
			 break;
		 default:
			 return -1;

		 }
	 }
	 else if(fu_indicator_type >= 1 && fu_indicator_type <= 23)		// single packet
	 {
	 	nal_type = fu_indicator_type;
	 	if(nal_type == 0x01 || nal_type == 0x07)
	 	{
	 		priData.gain = ASCII2UINT32(rtpPlayloadPos +1);
			priData.exp_time = ASCII2UINT32(rtpPlayloadPos +1+8);
			priData.r_gain = ASCII2UINT32(rtpPlayloadPos +1+8*2);
			priData.gr_gain = ASCII2UINT32(rtpPlayloadPos +1+8*3);
			priData.gb_gain = ASCII2UINT32(rtpPlayloadPos +1+8*4);
			priData.b_gain = ASCII2UINT32(rtpPlayloadPos +1+8*5);

			*h264DataPos = rtpPlayloadPos + 48 - 4;
			*(*h264DataPos) = 0x00;
		 	*((*h264DataPos)+1) = 0x00;
		 	*((*h264DataPos)+2) = 0x00;
		 	*((*h264DataPos)+3) = 0x01;
			*((*h264DataPos)+4) = fu_indicator;
			h264Len = len - headLen - 48 + 4;
			
	 	}
		else
		{
			 *h264DataPos = rtpPlayloadPos-4;
		 	*(*h264DataPos) = 0x00;
		 	*((*h264DataPos)+1) = 0x00;
		 	*((*h264DataPos)+2) = 0x00;
		 	*((*h264DataPos)+3) = 0x01;
		 	h264Len = len - headLen + 4;
		}

		  payloadType = RTP_PAYLOAD_FULL_H264_NAL;
	 }
	 else
	 {
		 return -1;
	 }

	 return 0;
}  


int RTPParse::parseRtcpSenderReport(unsigned char* sr_data, unsigned int data_len, RTCP_SR_Info& sr_info)
{
	if(data_len < 28)
		return -1;

	if(sr_data[1] != RTCP_SR_TYPEID)
		return -1;

	memcpy(sr_info.sender_ssrc, sr_data+4, 4);
	memcpy(sr_info.ntp, sr_data+8, 8);
	memcpy(sr_info.rtp_timestamp, sr_data+16, 4);
	sr_info.send_pack_count = ::ntohl(*(unsigned long*)(sr_data+20));
	sr_info.send_bytes_count = ::ntohl(*(unsigned long*)(sr_data+24));

	
	return 0;
}

int RTPParse::packRtcpHead(RTCP_packet_head* head_info, unsigned char* head_buf, unsigned int head_buf_len)
{
	head_buf[0] = (head_info->vision << 6) + (head_info->padding << 5) + head_info->count;
	head_buf[1] = head_info->type;
	*((unsigned short*)(head_buf+2)) = htons(head_info->len);

	return 0;
}

int RTPParse::packRtcpReceiverReport(RTCP_RR_Info* rr_info, unsigned char* rr_buf, unsigned int rr_buf_len)
{
	memcpy(rr_buf+4, rr_info->sender_ssrc, 4);
	memcpy(rr_buf+8, rr_info->ssrc_1, 4);

	rr_buf[12] = rr_info->fraction_lost;

	unsigned long net_seq_cumulation_lost = htonl(rr_info->cumulation_lost);
	memcpy(rr_buf+13, ((unsigned char*)&net_seq_cumulation_lost)+1, 3);
	*((unsigned long*)(rr_buf+16)) = htonl(rr_info->highest_seq);
	*((unsigned long*)(rr_buf+20)) = htonl(rr_info->interarrival_jitter);
	memcpy(rr_buf+24, rr_info->last_sr_timestamp, 4);
	*((unsigned long*)(rr_buf+28)) = htonl(rr_info->delay_last_sr_timestamp);

	int totalLen = 32;

	RTCP_packet_head head;
	head.vision = RTP_VERSION_NUM;
	head.padding = 0;
	head.count = 1; 
	head.type = RTCP_RR_TYPEID;
	head.len = totalLen/4-1;   //(7+1)*4=32bytes

	packRtcpHead(&head, rr_buf, 4);
	
	return totalLen;
}

int RTPParse::packRtcpSdes(RTCP_SDES_Info* sdes_info, unsigned char* sdes_buf, unsigned int sdes_buf_len)
{
	memcpy(sdes_buf+4, sdes_info->ssrc, 4);
	sdes_buf[8] = sdes_info->cname;

	unsigned int textlen = strlen(sdes_info->text);
	if(textlen > 0xFF)
		textlen = 0xFF;

	sdes_buf[9] = textlen;
	memcpy(sdes_buf+10, sdes_info->text, textlen);

	int validDataLen = 10+textlen;

	int fillLen = 0;
	int remainder = validDataLen % 4;
	if(remainder != 0)
	{
		fillLen = 4-remainder;
		memset(sdes_buf+validDataLen, 0, fillLen);
	}

	int totalLen = validDataLen+fillLen;

	RTCP_packet_head head;
	head.vision = RTP_VERSION_NUM;
	head.padding = 0;
	head.count = 1; 
	head.type = RTCP_SDES_TYPEID;
	head.len = totalLen/4-1;   

	packRtcpHead(&head, sdes_buf, 4);

	return totalLen;
}

}


