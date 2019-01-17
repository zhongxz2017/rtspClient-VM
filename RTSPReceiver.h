/******************************************************************************
 *
 * \file RTSPReceiver.h
 * \brief Class header for RTSPReceiver
 * \author Yunfang Che
 *
 * Copyright Aqueti 2017.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 *****************************************************************************/

//TODO new functionality fully implemented/tested, but still need to go back
//     and remove deprecated functions/variables after updating ACI
#ifndef RTSP_RECEIVER_HPP
#define RTSP_RECEIVER_HPP

#pragma once

//#include "atl/BaseContain.h"
//#include "atl/CamImage.h"
//#include "atl/Pusher.tcc"
#include <thread>
#include <mutex>
#include <iostream>
#include <string>
#include "RTSPParse.h"
#include "RTPParse.h"



namespace atl
{
typedef struct _MediaStreamSetupParam
{
	int rtp_fd;
	int rtcp_fd;
	unsigned short rtp_port;
	unsigned short rtcp_port;
	RTSP_media_description media_describe;
	RTSP_setup_param media_setup;
}MediaStreamSetupParam;

typedef struct _MediaStreamClientInfo
{
	unsigned long ip;
	unsigned short udp_port;
}MediaStreamClientInfo;

typedef struct _RTP_packet_statistic_info
{
	unsigned char ssrc[4];
	unsigned short start_seq;
	unsigned short last_seq;
	unsigned short ext_segment_start_seq;
	unsigned short ext_segment_last_seq;

	unsigned long from_last_rr_recv_packet_count;
	unsigned long cumulative_recv_packet_count;

	unsigned long interarrival_jitter;
}RTP_packet_statistic_info;

class RTSPReceiver
	//: public atl::Pusher<atl::BaseContainer>, public atl::Thread
{
public:
	RTSPReceiver(std::string uri);//, aqt_Status *status = NULL);
	~RTSPReceiver();
	//aqt_Status getStatus();
	void mainLoop();
	bool rtspStreamLoop();
	bool rtpStreamLoop();
	bool rtcpStreamLoop();

	bool m_running = false;
	bool m_rtp_running =false;
	bool m_rtcp_running =  false;

private:
	bool parseRTSPUri(std::string uri);
	
	
	
	bool Connect(const std::string hostname, unsigned short port);
	bool createTcpClient(std::string hostname, int port);
	void setWaitTime(int fd, double wTime);
	void closeSocket(int &fd);

	ssize_t sendRTSPRequest(unsigned char* buffer, unsigned int length);
	ssize_t recvRTSPResponse(unsigned char *buffer, unsigned int length);

	bool postOptionsRequest();
	bool postDescribeRequest();
	bool postSetupRequest();
	bool postPlayRequest();

	bool parseDescribeResponse(char* data);
	bool parseSetupResponse(char* data);
	bool parsePlayResponse(char* data);

	void setLastRspsRecvdTickCount(unsigned long tickCount){m_last_rtsp_recvd_tick_count = tickCount;}
	void setLastPacketRecvdTickCount(unsigned long tickCount){m_last_packet_recv_tick_count = tickCount; }
	int countMediaPacket(RTP_packet_head *packetHead);
	int pushESFrameData(unsigned char *esData, unsigned int dataLen);
	void setLastSRInfo(RTCP_SR_Info *srInfo);

	int sendRTCPRRPacket();
	
private:
	std::string m_uri;
	//aqt_Status *m_status;
	bool m_is_init_rtsp_receiver = false;
	

	//atomic_int m_rtsp_fd = -1;
	int m_rtsp_fd = -1;
	int m_rtsp_method;
	std::string m_hostname;
	int m_rtsp_port = -1;

	double m_waitTime = 0.1;

	unsigned long m_cseq = 1;
	
	unsigned char *m_rtsp_recv_buf;
	unsigned int m_rtsp_recv_buf_size;
	unsigned int m_rtsp_recv_offset;
	unsigned char *m_rtsp_send_buf;
	unsigned int m_rtsp_send_buf_size;


	MediaStreamSetupParam m_stream_setup_param;
	RTP_packet_statistic_info m_media_packet_statistic_info;
	RTCP_SR_Info m_last_media_sr_info;
	RTCP_RR_Info m_last_media_rr_info;
	RTCP_SDES_Info m_receiver_sdes_info;

	unsigned long m_last_rtsp_recvd_tick_count;
	unsigned long m_first_sr_recv_tick_count;
	unsigned long m_last_packet_recv_tick_count;
	
	bool m_is_init_media_stream_info = false;
	std::mutex m_media_packet_statistic_lock;
	bool m_stream_start_flag = false;
	bool m_is_get_first_sender_report = false;
	bool m_is_init_last_rr_info = false;

	//atomic_int m_rtp_fd = -1;
	//atomic_int m_rtcp_fd = -1;
	int m_rtp_fd = -1;
	int m_rtcp_fd = -1;

	unsigned char m_nal_type = 0;
	unsigned char* m_es_frame_buf;
	unsigned int m_es_frame_buf_size;
	unsigned int m_es_frame_buf_offset;
	unsigned char m_timestamp[4];

	char *m_buf;
	char *m_rtp_buf;
	char *m_rtcp_send_buf;
	char *m_rtcp_recv_buf;

	
	LIB2A_2ACTRL_RESULT_S m_pri_data;
	
};

};
#endif
