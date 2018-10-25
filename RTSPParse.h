/******************************************************************************
 *
 * \file RTSPParse.h
 * \brief rtsp parser
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
#ifndef RTSP_PARSE_H
#define RTSP_PARSE_H

#pragma once
#include <sys/ioctl.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/tcp.h>
#include <arpa/inet.h>
#include <netdb.h>
#include <unistd.h>
#include <sys/signal.h>
#include <ifaddrs.h>
#include <errno.h>
#include <stdio.h>
#include <cstring>
#include <assert.h>

#include <string>
#include <iostream>

namespace atl
{

#define RTSP_RSPS_STATUSCODE_OK 200

#define H264_SPS_PPS_MAX_LEN 512
 
typedef enum _RTSP_METHOD
{
	RTSP_METHOD_UNKNOWN = 0,
	RTSP_METHOD_OPTIONS,
	RTSP_METHOD_DESCRIBE,
	RTSP_METHOD_SETUP,
	RTSP_METHOD_TEARDOWN,
	RTSP_METHOD_PLAY,
	RTSP_METHOD_PAUSE,
	RTSP_METHOD_SET_PARAMETER,
	RTSP_METHOD_GET_PARAMETER
}RTSP_METHOD;

typedef struct _RTSP_Transport_field_param
{
	struct sockaddr_in server_rtp_addr;
	struct sockaddr_in server_rtcp_addr;
}RTSP_Transport_field_param;

typedef struct RTSP_setup_param
{
	std::string session_id;
	RTSP_Transport_field_param transport_file_param;
}RTSP_setup_param;

typedef struct _RTSP_media_description
{
	std::string videoURL;
	std::string audioURL;

	unsigned char h264_sps_pps_nal_len;
	unsigned char h264_sps_pps_nal_data[H264_SPS_PPS_MAX_LEN];
	_RTSP_media_description() : videoURL(""), audioURL(""), h264_sps_pps_nal_len(0){}
}RTSP_media_description;

class RTSPParse
{
public:
	RTSPParse(void);
	~RTSPParse(void);

	static int getOptionsRequest(const char* rtspURL, int cseq, char* buf, unsigned int buflen);
	static int getDescribeRequest(const char* rtspURL, int cseq, char* buf, unsigned int buflen);
	static int getSetupRequest(const char* rtspMediaTrackURL, int cseq, unsigned short rtpPort, unsigned short rtcpPort, char* buf, unsigned int buflen);
	static int getPlayRequest(const char* rtspURL, int cseq, const char* session_id, char* buf, unsigned int buflen);
	static int getTeardownRequest(const char* rtspURL, int cseq, const char* session_id, char* buf, unsigned int buflen);
	static int getParamGetRequest(const char* rtspURL, int cseq, const char* session_id, char* buf, unsigned int buflen);
	static int parseOptionsResponse(const char* data, int& rspsRet, int& rspsCSeq, int* methodCodeArray, unsigned int arraySize, unsigned int& methodNum);
	static int parseDescribeResponse(const char* data, int& rspsRet, int& rspsCSeq, RTSP_media_description& describeInfo);
	static int parseSetupResponse(const char* data, int& rspsRet, int& rspsCSeq, RTSP_setup_param& setupParam);
	static int parsePlayResponse(const char* data, int& rspsRet, int& rspsCSeq){ 
		return parseSimpleResponse(data, rspsRet, rspsCSeq);
	}
	static int parseTeardownResponse(const char* data, int& rspsRet, int& rspsCSeq){ 
		return parseSimpleResponse(data, rspsRet, rspsCSeq);
	}

	static int verifiedDataIntegrity(const char* data, bool& isIntegrity);
	

private:
	

	static int parseSimpleResponse(const char* data, int& rspsRet, int& rspsCSeq);
	static int parseResponseFirstLine(const char* text, int& result, unsigned int& nextlineIndex);
	static int parseMethodListField(const char* text, int* methodCodeArray, unsigned int arraySize, unsigned int& methodNum);
	static int parseTransportField(const char* text, RTSP_Transport_field_param& transport_file_param);

	static int parseMediaSDP(const char* text, RTSP_media_description& describeInfo);

	static int getMethodCode(const char* methodName);

	static int getField(const char* text, char* nameBuf, unsigned int nameBufLen \
		, char* valBuf, unsigned int valBufLen, unsigned int* nextFieldIndex = NULL \
		, char fieldSplitChar = ':', char endChar = '\0');


	static int getLine(const char* text, unsigned int& lineLen, unsigned int& nextLineIndex);
	static int getElem(const char* text, unsigned int& elemStartIndex, unsigned int& elemLen, unsigned int& nextElemIndex);  //空格分隔的元素	

	
};

};
#endif
