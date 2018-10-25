/******************************************************************************
 *
 * \file RTSPParse.cpp
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

#include "StringParse.h"
#include "Base64.h"
#include "RTSPParse.h"

namespace atl
{

//**********************************************************//
//RTSP Defined
#define RTSP_STATUS_CODE_LEN	3

#define RTSP_VERSION_1D0_STRING   "RTSP/1.0"


#define RTSP_FIELD_NAME_BUF_MAX_LEN 64
#define RTSP_FIELD_VAL_BUF_MAX_LEN 1024   //sps, pps max length
#define RTSP_MAX_METHOD_NUM  32
#define RTSP_METHOD_NAME_MAX_LEN 32


#define RTSP_IS_SEQ_FIELD_NAME(str) (strcasecmp((str), "CSeq") == 0)
#define RTSP_IS_CONTENTTYPE_FIELD_NAME(str) (strcasecmp((str), "Content-Type") == 0)
#define RTSP_IS_CONTENTLENGTH_FIELD_NAME(str) (strcasecmp((str), "Content-Length") == 0)
#define RTSP_IS_METHODLIST_FIELD_NAME(str) (strcasecmp((str), "Public") == 0)
#define RTSP_IS_SESSION_ID_FIELD_NAME(str) (strcasecmp((str), "Session") == 0)
#define RTSP_IS_TRANSPORT_FIELD_NAME(str) (strcasecmp((str), "Transport") == 0)

#define RTSP_CONTENTTYPE_IS_SDP(str) (strcasecmp((str), "application/sdp") == 0)


//**********************************************************//
//SDP Defined
#define SDP_IS_MEDIA_FIELD_NAME(str) (strcasecmp((str), "m") == 0)
#define SDP_IS_ATTRIBUTE_FIELD_NAME(str) (strcasecmp((str), "a") == 0)
#define SDP_ATTRIBUTE_IS_CONTROL(str) (strcasecmp((str), "control") == 0)


//**********************************************************//
RTSPParse::RTSPParse(void)
{
}

RTSPParse::~RTSPParse(void)
{
}

int RTSPParse::getOptionsRequest(const char* rtspURL, int cseq, char* buf, unsigned int buflen)
{
	::sprintf(buf, "OPTIONS %s RTSP/1.0\r\n" \
		"CSeq: %d\r\n\r\n", rtspURL, cseq);

	return 0;
}

int RTSPParse::getDescribeRequest(const char* rtspURL, int cseq, char* buf, unsigned int buflen)
{
	::sprintf(buf, "DESCRIBE %s RTSP/1.0\r\n" \
		"CSeq: %d\r\n" \
		"Accept: application/sdp\r\n\r\n", rtspURL, cseq);
	return 0;
}

int RTSPParse::getSetupRequest(const char* rtspMediaTrackURL, int cseq, unsigned short rtpPort, unsigned short rtcpPort, char* buf, unsigned int buflen)
{
	::sprintf(buf, "SETUP %s RTSP/1.0\r\n" \
		"CSeq: %d\r\n" \
		"Transport: RTP/AVP;unicast;client_port=%d-%d\r\n\r\n"
		, rtspMediaTrackURL, cseq, rtpPort, rtcpPort);

	return 0;
}

int RTSPParse::getPlayRequest(const char* rtspURL, int cseq, const char*  session_id, char* buf, unsigned int buflen)
{
	::sprintf(buf, "PLAY %s RTSP/1.0\r\n" \
		"CSeq:%d\r\n" \
		"Session: %s\r\n" \
		"Range: npt=0.000-\r\n\r\n", rtspURL, cseq, session_id);

	return 0;
}

int RTSPParse::getTeardownRequest(const char* rtspURL, int cseq, const char*  session_id, char* buf, unsigned int buflen)
{
	::sprintf(buf, "TEARDOWN %s RTSP/1.0\r\n" \
		"CSeq:%d\r\n" \
		"Session: %s\r\n\r\n", rtspURL, cseq, session_id);

	return 0;
}

int RTSPParse::getParamGetRequest(const char* rtspURL, int cseq, const char* session_id, char* buf, unsigned int buflen)
{
	::sprintf(buf, "GET_PARAMETER %s RTSP/1.0\r\n" \
		"CSeq:%d\r\n" \
		"Session:%s\r\n\r\n", rtspURL, cseq, session_id);

	return 0;
}

int RTSPParse::verifiedDataIntegrity(const char* data, bool& isIntegrity)
{
	isIntegrity = false;
	int index = StringParse::Find(data, "\r\n\r\n");
	if(index == -1)
		return -1;

	int contentLenIndex = StringParse::Find(data, "Content-Length", true);
	if(contentLenIndex != -1)
	{
		int splitIndex = StringParse::Find(data+contentLenIndex, ":", '\r');
		if(splitIndex == -1)
			return -1;

		char strContentLen[16];
		if(0 != StringParse::GetSubString(data, contentLenIndex+splitIndex+1, '\r', strContentLen, 16, true))
			return -1;

		if(!StringParse::IsNumericString(strContentLen))
			return -1;

		int contentLen = ::atoi(strContentLen);
		if(index+4+contentLen == strlen(data))
			isIntegrity = true;
	}
	else
	{
		isIntegrity = true;
	}
	
	return 0;
}

int RTSPParse::parseSimpleResponse(const char* data, int& rspsRet, int& rspsCSeq)
{
	rspsRet = -1;
	rspsCSeq = -1;

	unsigned int nextlineIndex;
	if(parseResponseFirstLine(data, rspsRet, nextlineIndex) != 0)
		return -1;

	unsigned int lineLen = 0;
	char* pos = (char*)data+nextlineIndex;
	
	char namebuf[RTSP_FIELD_NAME_BUF_MAX_LEN];
	char valbuf[RTSP_FIELD_VAL_BUF_MAX_LEN];
	bool bGetCSeq = false;

	while(0 == getLine(pos, lineLen, nextlineIndex) && lineLen > 0)
	{
		if(0 == getField(pos, namebuf, RTSP_FIELD_NAME_BUF_MAX_LEN, valbuf, RTSP_FIELD_VAL_BUF_MAX_LEN))
		{
			//CSeq Field
			if(RTSP_IS_SEQ_FIELD_NAME(namebuf))
			{
				if(!StringParse::IsNumericString(valbuf))
					return -1;

				rspsCSeq = ::atoi(valbuf);

				bGetCSeq = true;
				break;
			}
		}

		pos += nextlineIndex;
	}

	if(!bGetCSeq)
		return -1;

	return 0;
}

int RTSPParse::parseOptionsResponse(const char* data, int& rspsRet, int& rspsCSeq,  int* methodCodeArray, unsigned int arraySize, unsigned int& methodNum)
{
	rspsRet = -1;
	rspsCSeq = -1;
	methodNum = 0;

	unsigned int nextlineIndex;
	if(parseResponseFirstLine(data, rspsRet, nextlineIndex) != 0)
		return -1;


	unsigned int lineLen = 0;
	char* pos = (char*)data+nextlineIndex;
	
	char namebuf[RTSP_FIELD_NAME_BUF_MAX_LEN];
	char valbuf[RTSP_FIELD_VAL_BUF_MAX_LEN];
	bool bGetCSeq = false;
	bool bGetMethodList = false;

	while(0 == getLine(pos, lineLen, nextlineIndex) && lineLen > 0)
	{
		if(0 == getField(pos, namebuf, RTSP_FIELD_NAME_BUF_MAX_LEN, valbuf, RTSP_FIELD_VAL_BUF_MAX_LEN))
		{
			//CSeq Field
			if(!bGetCSeq && RTSP_IS_SEQ_FIELD_NAME(namebuf))
			{
				if(!StringParse::IsNumericString(valbuf))
					return -1;

				rspsCSeq = ::atoi(valbuf);

				bGetCSeq = true;
				if(bGetMethodList || rspsRet != RTSP_RSPS_STATUSCODE_OK)
					break;
			}
			else if(rspsRet == RTSP_RSPS_STATUSCODE_OK && RTSP_IS_METHODLIST_FIELD_NAME(namebuf))
			{
				//Public Field
				if(0 != parseMethodListField(valbuf, methodCodeArray, arraySize, methodNum))
					return -1;

				bGetMethodList = true;
				if(bGetCSeq)
					break;
			}
		}

		pos += nextlineIndex;
	}

	if(!bGetCSeq)
		return -1;

	if(rspsRet == RTSP_RSPS_STATUSCODE_OK)
	{
		if(!bGetMethodList)
			return -1;
	}

	return 0;
}

int RTSPParse::parseDescribeResponse(const char* data, int& rspsRet, int& rspsCSeq, RTSP_media_description& describeInfo)
{
	rspsRet = -1;
	rspsCSeq = -1;

	unsigned int nextlineIndex;
	int result;

	if(parseResponseFirstLine(data, result, nextlineIndex) != 0)
	{
		return -1;
	}

	rspsRet = result;

	unsigned int lineLen = 0;
	char* pos = (char*)data+nextlineIndex;
	
	char namebuf[RTSP_FIELD_NAME_BUF_MAX_LEN];
	char valbuf[RTSP_FIELD_VAL_BUF_MAX_LEN];
	bool bGetCSeq = false;
	bool bGetContentType = false;
	bool bGetContentLen = false;
	unsigned int contentLen = 0;

	while((0 == getLine(pos, lineLen, nextlineIndex)) && (lineLen > 0))
	{
		if(0 == getField(pos, namebuf, RTSP_FIELD_NAME_BUF_MAX_LEN, valbuf, RTSP_FIELD_VAL_BUF_MAX_LEN))
		{
			//CSet Field
			if(!bGetCSeq && RTSP_IS_SEQ_FIELD_NAME(namebuf))
			{
				if(!StringParse::IsNumericString(valbuf))
					return -1;

				rspsCSeq = ::atoi(valbuf);
				std::cout<<"CSeq is "<<rspsCSeq<<std::endl;
				bGetCSeq = true;

				if(result != RTSP_RSPS_STATUSCODE_OK)
					break;
			}
			else if(result == RTSP_RSPS_STATUSCODE_OK)
			{
				//Content-Type Field
				if(!bGetContentType && RTSP_IS_CONTENTTYPE_FIELD_NAME(namebuf))
				{
					if(!RTSP_CONTENTTYPE_IS_SDP(valbuf))
						return -1;
					std::cout<<"content type is application/sdp"<<std::endl;
					bGetContentType = true;
				}
				//Content-Length Field
				else if(!bGetContentLen && RTSP_IS_CONTENTLENGTH_FIELD_NAME(namebuf))
				{
					if(!StringParse::IsNumericString(valbuf))
						return -1;

					contentLen = ::atoi(valbuf);
					std::cout<<"content length is "<<contentLen<<std::endl;
					bGetContentLen = true;
				}

			}
		}

		pos += nextlineIndex;

	}

	if(!bGetCSeq)
		return -1;

	if(result == RTSP_RSPS_STATUSCODE_OK)
	{
		if(!bGetContentType || !bGetContentLen)
			return -1;
		return parseMediaSDP(pos, describeInfo);
	}

	return 0;
}

int RTSPParse::parseSetupResponse(const char* data, int& rspsRet, int& rspsCSeq, RTSP_setup_param& setupParam)
{
	rspsRet = -1;
	rspsCSeq = -1;

	unsigned int nextlineIndex;
	int result;

	if(parseResponseFirstLine(data, result, nextlineIndex) != 0)
		return -1;

	rspsRet = result;

	unsigned int lineLen = 0;
	char* pos = (char*)data+nextlineIndex;
	
	char namebuf[RTSP_FIELD_NAME_BUF_MAX_LEN];
	char valbuf[RTSP_FIELD_VAL_BUF_MAX_LEN];

	bool bGetCSeq = false;
	bool bGetTransParam = false;
	bool bGetSessionID = false;

	while(0 == getLine(pos, lineLen, nextlineIndex) && lineLen > 0)
	{
		if(0 == getField(pos, namebuf, RTSP_FIELD_NAME_BUF_MAX_LEN, valbuf, RTSP_FIELD_VAL_BUF_MAX_LEN))
		{
			//CSet Field
			if(!bGetCSeq && RTSP_IS_SEQ_FIELD_NAME(namebuf))
			{
				if(!StringParse::IsNumericString(valbuf))
					return -1;

				rspsCSeq = ::atoi(valbuf);
				std::cout<<"setup CSeq is "<<rspsCSeq<<std::endl;
				bGetCSeq = true;
				if((bGetTransParam && bGetSessionID) || result != RTSP_RSPS_STATUSCODE_OK)
					break;
			}
			else if(result == RTSP_RSPS_STATUSCODE_OK)
			{
				//Transport Field
				if(!bGetTransParam && RTSP_IS_TRANSPORT_FIELD_NAME(namebuf))
				{
					if(0 != parseTransportField(valbuf, setupParam.transport_file_param))
						return -1;

					bGetTransParam = true;
					if(bGetCSeq && bGetSessionID)
						break;
				}
				//Session ID Field
				else if(!bGetSessionID && RTSP_IS_SESSION_ID_FIELD_NAME(namebuf))
				{
					setupParam.session_id = valbuf;
					
					int index = setupParam.session_id.find(';');
					if(index != std::string::npos)
					{
						setupParam.session_id = setupParam.session_id.substr(0, index);
					}
					std::cout<<"session id is "<<setupParam.session_id<<std::endl;

					bGetSessionID = true;
					if(bGetCSeq && bGetTransParam)
						break;
				}

			}
		}

		pos += nextlineIndex;

	}

	if(!bGetCSeq)
		return -1;

	if(result == RTSP_RSPS_STATUSCODE_OK)
	{
		if(!bGetTransParam || !bGetSessionID)
			return -1;
	}

	return 0;
}


int RTSPParse::parseResponseFirstLine(const char* text, int& result, unsigned int& nextlineIndex)
{
	unsigned int lineLen, tempNextLineIndex;
	if(getLine(text, lineLen, tempNextLineIndex) != 0)
		return -1;

	nextlineIndex = tempNextLineIndex;
	if(lineLen == 0)
		return -1;

	char* pos = (char*)text;

	unsigned int elemStartIndex, nextelemIndex, elemLen;

	if(0 != getElem(pos, elemStartIndex, elemLen, nextelemIndex))
		return -1;

	if(elemLen < strlen(RTSP_VERSION_1D0_STRING))
		return -1;

	if(strncasecmp(pos+elemStartIndex, RTSP_VERSION_1D0_STRING, elemLen) != 0)
		return -1;

	pos += nextelemIndex;
	if(0 != getElem(pos, elemStartIndex, elemLen, nextelemIndex))
		return -1;

	if(elemLen != RTSP_STATUS_CODE_LEN)
		return -1;
	
	char strTemp[16]={0};
	strncpy(strTemp, pos+elemStartIndex, elemLen);
	std::cout<<"parse response first line strTemp "<<strTemp<<std::endl;
	if(!StringParse::IsNumericString(strTemp))
		return -1;

	result = ::atoi(strTemp);
	
	return 0;
}

int RTSPParse::parseMethodListField(const char* text, int* methodCodeArray, unsigned int arraySize, unsigned int& methodNum)
{
	char tempMethodArray[RTSP_MAX_METHOD_NUM][RTSP_METHOD_NAME_MAX_LEN];
	int mothodElemNum;
	if(0 != StringParse::SplitString(text, ',', (char*)tempMethodArray, RTSP_MAX_METHOD_NUM, RTSP_METHOD_NAME_MAX_LEN,  mothodElemNum))
		return -1;

	unsigned int count = 0;
	for(int i=0; i<mothodElemNum; i++)
	{
		StringParse::Trim((char*)&(tempMethodArray[i]));
		int methodCode = getMethodCode((char*)&(tempMethodArray[i]));
		if(methodCode != RTSP_METHOD_UNKNOWN)
		{
			if(count >= arraySize-1)
				return -1;

			methodCodeArray[count++] = methodCode;
		}
	}

	methodNum = count;

	return 0;
}

int RTSPParse::parseTransportField(const char* text, RTSP_Transport_field_param& transport_file_param)
{
	char tempTransParamList[16][48]={0};
	int mothodElemNum;
	if(0 != StringParse::SplitString(text, ';', (char*)tempTransParamList, 16, 48, mothodElemNum))
		return -1;

	transport_file_param.server_rtp_addr.sin_family = AF_INET;
	transport_file_param.server_rtp_addr.sin_addr.s_addr = 0;
	transport_file_param.server_rtcp_addr.sin_family = AF_INET;
	transport_file_param.server_rtcp_addr.sin_addr.s_addr = 0;

	int index, portnum;
	char* elemPos;

	bool bGetParam = false;
	std::cout<<"Transport element num is "<<mothodElemNum<<std::endl;
	for(int i=0; i<mothodElemNum; i++)
	{
		elemPos = (char*)&(tempTransParamList[i]);
		StringParse::Trim(elemPos);
		index = StringParse::Find(elemPos, '=');
		printf("%s\n",elemPos);
		if(index != -1)
		{
			if(strncasecmp(elemPos, "source", index) == 0)
			{
				char tempServerIP[48];
				strcpy(tempServerIP, elemPos+index+1);

				StringParse::Trim(tempServerIP);
				unsigned long server_ip = inet_addr(tempServerIP);
				if(INADDR_NONE == server_ip)
					return -1;

				transport_file_param.server_rtp_addr.sin_addr.s_addr = server_ip;
				transport_file_param.server_rtcp_addr.sin_addr.s_addr = server_ip;
			}
			else if(strncasecmp(elemPos, "server_port", index) == 0)
			{
				char tempServerPortParam[2][16]={0};
				
				if(0 != StringParse::SplitString(elemPos+index+1, '-', (char*)tempServerPortParam, 2, 16, portnum) || portnum != 2)
					return -1;
				
				
				for(int j=0; j<2; ++j)
				{
					StringParse::Trim((char*)&(tempServerPortParam[j]));
					std::cout<<"server_port["<<j<<"] is "<<tempServerPortParam[j]<<std::endl;
					if(!StringParse::IsNumericString((char*)&(tempServerPortParam[j])))
						return -1;
					
				}
				
				transport_file_param.server_rtp_addr.sin_port = htons(::atoi((char*)&(tempServerPortParam[0])));
				transport_file_param.server_rtcp_addr.sin_port = htons(::atoi((char*)&(tempServerPortParam[1])));
				bGetParam = true;
				break;
			}
		}

	}

	if(!bGetParam)
		return -1;

	return 0;
}

int RTSPParse::parseMediaSDP(const char* text, RTSP_media_description& describeInfo)
{
	describeInfo.audioURL = "";
	describeInfo.videoURL = "";
	describeInfo.h264_sps_pps_nal_len = 0;

	unsigned int lineLen = 0;
	unsigned int nextlineIndex = 0;

	char namebuf[RTSP_FIELD_NAME_BUF_MAX_LEN];
	char valbuf[RTSP_FIELD_VAL_BUF_MAX_LEN];
	char subNameBuf[RTSP_FIELD_NAME_BUF_MAX_LEN];
	char subValBuf[RTSP_FIELD_VAL_BUF_MAX_LEN];

	char attributeDeciptionBuf[RTSP_FIELD_NAME_BUF_MAX_LEN];

	int curMediaType = -1;  //0: video, 1: audio, -1:unknown
	int index;
	unsigned int nextFieldIndex = 0;
	char* lineStartPos = (char*)text;
	char* pos = NULL;

	while(0 == getLine(lineStartPos, lineLen, nextlineIndex))
	{
		if(lineLen == 0)
		{
			lineStartPos += nextlineIndex;
			continue;
		}

		if(0 == getField(lineStartPos, namebuf, RTSP_FIELD_NAME_BUF_MAX_LEN, valbuf, RTSP_FIELD_VAL_BUF_MAX_LEN, NULL, '='))
		{
			if(SDP_IS_MEDIA_FIELD_NAME(namebuf))
			{
				if(StringParse::Find(valbuf, "video", true) != -1)
					curMediaType = 0;
				else if(StringParse::Find(valbuf, "audio", true) != -1)
					curMediaType = 1;
			}
			else if(curMediaType != -1 && SDP_IS_ATTRIBUTE_FIELD_NAME(namebuf))
			{
				index = StringParse::Find(valbuf, ':');
				if(index <= 0)
				{
					lineStartPos += nextlineIndex;
					continue;
				}

				strncpy(attributeDeciptionBuf, valbuf, index);
				attributeDeciptionBuf[index] = '\0';

				pos = valbuf+index+1;
				index = StringParse::Find(pos, ' ');

				if(strcasecmp(attributeDeciptionBuf, "fmtp") == 0)
				{
					if(index < 0)
					{
						lineStartPos += nextlineIndex;
						continue;
					}

					pos += index+1;
					nextFieldIndex = 0;
					while(0 == getField(pos, subNameBuf, RTSP_FIELD_NAME_BUF_MAX_LEN, subValBuf, RTSP_FIELD_VAL_BUF_MAX_LEN, &nextFieldIndex,  '=', ';'))
					{
						if(strcasecmp(subNameBuf, "sprop-parameter-sets") == 0)
						{
							const unsigned char nalpix[4]= {0x00, 0x00, 0x00, 0x01};
							int sps_pps_len = strlen(subValBuf);
							if(sps_pps_len > H264_SPS_PPS_MAX_LEN)
							{
								lineStartPos += nextlineIndex;
								continue;
							}

							index = StringParse::Find(subValBuf, ',');
							if(index <= 0)
							{
								lineStartPos += nextlineIndex;
								continue;
							}

							memcpy(describeInfo.h264_sps_pps_nal_data, nalpix, 4);
							unsigned int spslen = Base64Decode((char*)describeInfo.h264_sps_pps_nal_data+4, subValBuf, index);
							memcpy(describeInfo.h264_sps_pps_nal_data+4+spslen, nalpix, 4);
							unsigned int ppslen = Base64Decode((char*)describeInfo.h264_sps_pps_nal_data+8+spslen, subValBuf+index+1);
							describeInfo.h264_sps_pps_nal_len = 8+spslen+ppslen;
							std::cout<<"h264_sps_pps_nal_len is "<<(8+spslen+ppslen)<<std::endl;
						}
				

						pos += nextFieldIndex;
					}


				}
				else if(strcasecmp(attributeDeciptionBuf, "control") == 0)
				{
					if(index == 0)
					{
						lineStartPos += nextlineIndex;
						continue;
					}

					if(index > 0)
					{
						strncpy(subValBuf, pos, index);
						subValBuf[index] = '\0';
					}
					else
					{
						strcpy(subValBuf, pos);
					}
				

					switch(curMediaType)
					{
					case 0:				//video
						describeInfo.videoURL = subValBuf;
						break;
					case 1:				//audio
						describeInfo.audioURL = subValBuf;
						break;
					}
				}
				
			}
			
		}

		lineStartPos += nextlineIndex;
	}

	return 0;
}

int RTSPParse::getMethodCode(const char* methodName)
{
	int methodCode = RTSP_METHOD_UNKNOWN;

	if(strcasecmp(methodName, "DESCRIBE") == 0)
	{
		methodCode = RTSP_METHOD_DESCRIBE; 
	}
	else if(strcasecmp(methodName, "SETUP") == 0)
	{
		methodCode = RTSP_METHOD_SETUP;
	}
	else if(strcasecmp(methodName, "TEARDOWN") == 0)
	{
		methodCode = RTSP_METHOD_TEARDOWN;
	}
	else if(strcasecmp(methodName, "PLAY") == 0)
	{
		methodCode = RTSP_METHOD_PLAY;
	}
	else if(strcasecmp(methodName, "PAUSE") == 0)
	{
		methodCode = RTSP_METHOD_PAUSE;
	}
	else if(strcasecmp(methodName, "SET_PARAMETER") == 0)
	{
		methodCode = RTSP_METHOD_SET_PARAMETER;
	}
	else if(strcasecmp(methodName, "GET_PARAMETER") == 0)
	{
		methodCode = RTSP_METHOD_GET_PARAMETER;
	}

	return methodCode;
}

int RTSPParse::getField(const char* text, char* nameBuf, unsigned int nameBufLen, char* valBuf
						, unsigned int valBufLen, unsigned int* nextFieldIndex, char fieldSplitChar, char endChar)
{
	bool isGetName = false;
	int index = 0;
	unsigned int count = 0;
	int lastNotSpaceIndex = 0;

	for(; text[index]!='\0' && text[index] != '\r' && text[index] != '\n' && text[index] != endChar; ++index)
	{
		if(text[index] == fieldSplitChar)
		{
			isGetName = true;
			break;
		}

		if(text[index] != ' ' || count>0)
		{
			if(nameBufLen <= count+1)
				return -1;

			if(text[index] != ' ')
				lastNotSpaceIndex = count;

			nameBuf[count++] = text[index];
		}
	}

	if(lastNotSpaceIndex == count-1)
		nameBuf[count] = '\0';
	else
		nameBuf[lastNotSpaceIndex+1] = '\0';

	if(!isGetName)
		return -1;

	index++;
	count = 0;
	lastNotSpaceIndex = 0;
	for(; text[index]!='\0' && text[index] != '\r' && text[index] != '\n' && text[index] != endChar; ++index)
	{
		if(text[index] != ' ' || count>0)
		{
			if(valBufLen <= count+1)
				return -1;

			if(text[index] != ' ')
				lastNotSpaceIndex = count;

			valBuf[count++] = text[index];
		}
	}

	if(lastNotSpaceIndex == count-1)
		valBuf[count] = '\0';
	else
		valBuf[lastNotSpaceIndex+1] = '\0';

	if(nextFieldIndex != NULL)
		*nextFieldIndex = index+1;
	
	return 0;
}



int RTSPParse::getLine(const char* text, unsigned int& lineLen, unsigned int& nextLineIndex)
{
	int index = -1;
	for(int i=0; text[i]!='\0'; ++i)
	{
		if(text[i] == '\r' || text[i] == '\n')
		{
			index = i;
			break;
		}
	}

	if(index == -1)
		return -1;

	lineLen = index;

	if(text[index] == '\r' && text[index+1] == '\n')
		nextLineIndex = index+2;
	else
		nextLineIndex = index+1;

	return 0;
}

int RTSPParse::getElem(const char* text, unsigned  int& elemStartIndex, unsigned  int& elemLen, unsigned  int& nextElemIndex)
{
	int tempElemStartIndex = -1;
	int tempElemLen = 0;

	for(int i=0; text[i]!='\0' && text[i] != '\r' && text[i] != '\n'; ++i)
	{
		if(text[i] != ' ')
		{
			if(tempElemStartIndex == -1)
				tempElemStartIndex = i;

			tempElemLen++;
		}
		else if(tempElemLen > 0)
		{
			break;
		}
	}
	

	if(tempElemLen == 0)
		return -1;

	elemStartIndex = tempElemStartIndex;
	elemLen = tempElemLen;

	nextElemIndex = tempElemStartIndex+tempElemLen;

	return 0;
}

}
