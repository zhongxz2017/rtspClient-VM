
/******************************************************************************
 *
 * \file RTSPReceiver.cpp
 * \brief Class methods for RTSPReceiver
 * \author Yunfang Che
 *
 * Copyright Aqueti 2017.
 * Distributed under the Boost Software License, Version 1.0.
 * (See accompanying file LICENSE_1_0.txt or copy at
 * http://www.boost.org/LICENSE_1_0.txt)
 *
 *****************************************************************************/

#include "RTSPReceiver.h"


namespace atl
{
#define RTSP_URI_HEAD "rtsp://"

#define RTCP_EXT_SEGMENT_START_SEQ 1

#define RTSP_REQUEST_MAX_LEN 4096
#define RTP_PACKET_MAX_LEN 1500
#define RTCP_PACKET_MAX_LEN 1500

unsigned long GetTickCount()
{
	struct timespec ts;
	clock_gettime(CLOCK_MONOTONIC, &ts);
	return (ts.tv_sec * 1000+ ts.tv_nsec/1000000);
}



/**
* \brief Copy constructor; needed to deal with atomic metadata
* \param [in] im reference to BaseContainer object
**/
RTSPReceiver::RTSPReceiver(std::string uri)//, aqt_Status *status)
{
	m_uri = uri;
	//m_status = status;

	if(!parseRTSPUri(uri))
		return;
	
	//RTSP
	m_rtsp_recv_buf = (unsigned char *)malloc(RTSP_REQUEST_MAX_LEN);
	m_rtsp_recv_buf_size = RTSP_REQUEST_MAX_LEN;
	m_rtsp_recv_offset = 0;

	m_rtsp_send_buf = (unsigned char *)malloc(RTSP_REQUEST_MAX_LEN);
	m_rtsp_send_buf_size = RTSP_REQUEST_MAX_LEN;

	//ES
	m_es_frame_buf = (unsigned char *)malloc(1*1024*1024);
	m_es_frame_buf_size = 1*1024*1024;
	m_es_frame_buf_offset = 0;

	//RTP + RTCP
	m_buf = (char *)malloc(RTP_PACKET_MAX_LEN + RTCP_PACKET_MAX_LEN*2);
	m_rtp_buf = m_buf;
	m_rtcp_send_buf = m_buf + RTP_PACKET_MAX_LEN;
	m_rtcp_recv_buf = m_buf + RTP_PACKET_MAX_LEN + RTCP_PACKET_MAX_LEN;

	if(!Connect(m_hostname, m_rtsp_port))
		return;
	postOptionsRequest();
	//Start();
	m_is_init_rtsp_receiver = true;
	
}

RTSPReceiver::~RTSPReceiver()
{
	if(!m_is_init_rtsp_receiver)
		return;
	
	/*if(isRunning())
	{
		Thread::Stop();
		Thread::Join();
	}*/

	//RTP + RTCP
	if(m_buf != NULL)
		free(m_buf);

	//ES
	if(m_es_frame_buf != NULL)
		free(m_es_frame_buf);

	//RTSP
	if(m_rtsp_recv_buf != NULL)
		free(m_rtsp_recv_buf);

	if(m_rtsp_send_buf != NULL)
		free(m_rtsp_send_buf);
}

/*aqt_Status RTSPReceiver::getStatus()
{
	return *m_status;
}*/

bool RTSPReceiver::parseRTSPUri(std::string uri)
{
	int index;
	int cIndex; // colon index
	int sIndex; // Slash index
	int rtspUriHeadLength = strlen(RTSP_URI_HEAD);
	char buffer[20];
	std::size_t length;


	index = uri.find(RTSP_URI_HEAD,0);
	if(index != 0)
		return false;
	m_uri = uri;
	cIndex = uri.find(':',rtspUriHeadLength);
	if(cIndex != uri.npos)
	{
		length = uri.copy(buffer,cIndex - rtspUriHeadLength, rtspUriHeadLength);
		buffer[length] = '\0';
		m_hostname = buffer;
		sIndex = uri.find('/',rtspUriHeadLength);
		length = uri.copy(buffer, sIndex - cIndex - 1, cIndex+1);
		buffer[length] = '\0';
		m_rtsp_port = atoi(buffer);
	}
	else
	{
		sIndex = uri.find('/',rtspUriHeadLength);
		length = uri.copy(buffer, sIndex - rtspUriHeadLength, rtspUriHeadLength);
		buffer[length]='\0';
		m_hostname = buffer;
		m_rtsp_port = 554;
		m_uri.insert(sIndex,":554");
	}
	std::cout<<"uri: "<<m_uri<<std::endl;
	return true;
}

void RTSPReceiver::mainLoop()
{
	struct timeval tv;
	fd_set readfds;
	int max_fd = -1;
	int sr;

	while(m_running)//isRunning())
	{
		tv.tv_sec = 1;
		tv.tv_usec = 0;
		FD_ZERO(&readfds);
		if(m_rtsp_fd > 0)
		{
			FD_SET(m_rtsp_fd, &readfds);
			max_fd = m_rtsp_fd;
		}
		if(m_rtp_fd > 0)
		{
			FD_SET(m_rtp_fd, &readfds);
			if(max_fd < m_rtp_fd)
				max_fd = m_rtp_fd;
		}
		if(m_rtcp_fd > 0)
		{
			FD_SET(m_rtcp_fd, &readfds);
			if(max_fd < m_rtcp_fd)
				max_fd = m_rtcp_fd;
		}
		sr = select(max_fd+1, &readfds, NULL, NULL, &tv);
		if( sr == 0)
		{
		}
		else if(sr < 0)
		{
		}
		else
		{
			if(m_rtsp_fd > 0 && FD_ISSET(m_rtsp_fd, &readfds))
			{
				rtspStreamLoop();
			}
			else if(m_rtp_fd > 0 && FD_ISSET(m_rtp_fd, &readfds))
			{
				rtpStreamLoop();
			}
			else if(m_rtcp_fd > 0 && FD_ISSET(m_rtcp_fd, &readfds))
			{
				rtcpStreamLoop();
			}
			else
			{
			}
		}
	}
}

bool RTSPReceiver::rtspStreamLoop()
{
	bool ret;
	int recvBytes;
	int recvCount;
	char *rspsPacketHead;
	bool isRspsIntegrity;
	unsigned long curTickCount;

	do{
		errno = 0;
		recvBytes = recv(m_rtsp_fd, m_rtsp_recv_buf+m_rtsp_recv_offset, m_rtsp_recv_buf_size- m_rtsp_recv_offset -1, 0);
		if(recvBytes == 0)
		{
		 	ret = false;
			break;
		}
		else if(recvBytes < 0)
		{
			if(errno == ETIMEDOUT || errno == EWOULDBLOCK || errno == EAGAIN)  // timeout
				ret = true;
			else
				ret = false;
			break;
		}
		else
		{
			setLastRspsRecvdTickCount(GetTickCount());
			m_rtsp_recv_offset += recvBytes;
			m_rtsp_recv_buf[m_rtsp_recv_offset] = '\0';
			rspsPacketHead = (char *)m_rtsp_recv_buf;
			if(RTSPParse::verifiedDataIntegrity(rspsPacketHead, isRspsIntegrity) != 0)
			{
				ret = false;
				break;
			}
			if(!isRspsIntegrity)
				continue;
			m_rtsp_recv_offset = 0;
			switch(m_rtsp_method)
			{
				case RTSP_METHOD_OPTIONS:
					postDescribeRequest();
					break;
				case RTSP_METHOD_DESCRIBE:
					if(parseDescribeResponse(rspsPacketHead))
					{
						postSetupRequest();
					}
					break;
				case RTSP_METHOD_SETUP:
					if(parseSetupResponse(rspsPacketHead))
					{
						postPlayRequest();
					}
					else
						std::cout<<"parse setup response error"<<std::endl;
						
					break;
				case RTSP_METHOD_PLAY:
					if(parsePlayResponse(rspsPacketHead))
					{
						curTickCount = GetTickCount();
						setLastPacketRecvdTickCount(curTickCount);
					}
					break;
			}


			ret = true;
			break;
		}
	}while(m_running);
	return ret;
}

bool RTSPReceiver::rtpStreamLoop()
{
	bool ret;
	int recvBytes;
	RTP_packet_head packetHead;
	int headLen;

	unsigned char nalType;
	int payloadType;
	unsigned char* esDataPos;
	unsigned int esDataLen;
	unsigned int timestamp;
	
	do{
		errno = 0;
		recvBytes = recv(m_rtp_fd, m_rtp_buf, RTP_PACKET_MAX_LEN, 0);
		if(recvBytes == 0){
			ret = false;
			break;
		}
		else if(recvBytes < 0){
			if(errno == ETIMEDOUT || errno == EWOULDBLOCK || errno == EAGAIN)  // timeout
			{
				ret = true;
				break;
			}
			else{
				ret = false;
				break;
			}
		}
		else{
			if(RTPParse::parseRtpPacketHead((unsigned char *)m_rtp_buf, recvBytes, packetHead, headLen) != 0)
			{
				std::cout<< "parseRtpPackHead error"<<std::endl;
				continue;
			}
			setLastPacketRecvdTickCount(GetTickCount());
			if(countMediaPacket(&packetHead) != 0)
			{
				std::cout<<"countMediaPacket error"<<std::endl;
				continue;
			}
			if(RTPParse::unpackRtpH264Payload((unsigned char *)m_rtp_buf, recvBytes, headLen, &esDataPos, esDataLen, nalType, payloadType,m_pri_data) != 0)
				continue;
			if(payloadType == RTP_PAYLOAD_FU_START_H264_NAL || payloadType == RTP_PAYLOAD_FULL_H264_NAL)
				m_nal_type = nalType;
			if(!m_stream_start_flag)
			{
				if((payloadType == RTP_PAYLOAD_FU_START_H264_NAL || payloadType == RTP_PAYLOAD_FULL_H264_NAL) && (nalType == H264_NAL_TYPE_IDR || nalType == 7))  // IDR
				{	
					m_stream_start_flag = true;
				}
				else
				{
					//std::cout<<"Mark "<<(int)packetHead.mark<<", head len "<<headLen<<", nal_type "<<(int)nalType<<", payload_type "<<payloadType<<std::endl;
					continue;
				}
			}
			if(pushESFrameData(esDataPos, esDataLen) != 0) // error
			{
				m_es_frame_buf_offset = 0;
				m_stream_start_flag = false;
				continue;
			}
			if(payloadType == RTP_PAYLOAD_FULL_H264_NAL && packetHead.mark == 1)
			{
				timestamp = (packetHead.timestamp[0]<<24 | packetHead.timestamp[1] << 16 | packetHead.timestamp[2] << 8 | packetHead.timestamp[3]);
				std::cout <<"h264 frame nalType "<<(int)m_nal_type<<", frame len "<<m_es_frame_buf_offset<<", timestamp "<<timestamp<<std::endl;
				printf("private data gain: 0x%08x, ExpTime: 0x%08x, RGain: 0x%08x, GRGain: 0x%08x, GBGain: 0x%08x, BGain: 0x%08x\n", \
					m_pri_data.gain, m_pri_data.exp_time, m_pri_data.r_gain, m_pri_data.gr_gain,m_pri_data.gb_gain, m_pri_data.b_gain);
				m_es_frame_buf_offset = 0;
			}
		}
	}while(m_running); //isRunning());
	return ret;
}

bool RTSPReceiver::rtcpStreamLoop()
{
	bool ret;
	RTCP_SR_Info srInfo;
	int recvBytes;
	std::cout<<"in rtcpStreamLoop"<<std::endl;
	errno = 0;
	recvBytes = recv(m_rtcp_fd, m_rtcp_recv_buf, RTCP_PACKET_MAX_LEN, 0);
	if(recvBytes == 0)
	{
		ret = false;
	}
	else if(recvBytes < 0)
	{
		if(errno == ETIMEDOUT || errno == EWOULDBLOCK || errno == EAGAIN)  // timeout
			ret = true;
		else
			ret = false;
	}
	else
	{
		if(RTPParse::parseRtcpSenderReport((unsigned char *)m_rtcp_recv_buf, recvBytes, srInfo) == 0)
		{
			setLastSRInfo(&srInfo);
			sendRTCPRRPacket();
		}
		ret = true;
	}
	std::cout<<"out rtcpStreamLoop"<<std::endl;
	return ret;
	
}	

bool RTSPReceiver::Connect(const std::string hostname, unsigned short port)
{
	//check whether IP is a true IP, or a hostname and resolve it if needed
	std::string ipString;
	struct in_addr ipStruct;
	if(hostname != "localhost" && !inet_pton(AF_INET,hostname.c_str(), &ipStruct))
	{
		std::cout<<"INFO: Attempting to resolve hostname" <<hostname <<std::endl;
		struct hostent *phe = gethostbyname(hostname.c_str());
		if(phe == NULL)
		{
			std::cout<<"ERROR: Failed to resolve hostname"<<hostname<<std::endl;
			return false;
		}
		in_addr *address = (in_addr *)phe->h_addr;
		ipString = inet_ntoa(*address);
		std::cout<<"INFO: Resolved hostname"<<hostname<<"to address"<<ipString<<std::endl;
	}
	else
	{
		ipString = hostname;
	}
	if(!createTcpClient(ipString,port))
	{
		return false;
	}
	return true;
}


int RTSPReceiver::countMediaPacket(RTP_packet_head *packetHead)
{
	int ret = 0;
	m_media_packet_statistic_lock.lock();
	if(m_is_init_media_stream_info)
	{
		if(packetHead->seq != m_media_packet_statistic_info.last_seq)
		{
			if(packetHead->seq > m_media_packet_statistic_info.last_seq)
			{
				m_media_packet_statistic_info.last_seq = packetHead->seq;
				m_media_packet_statistic_info.from_last_rr_recv_packet_count++;
				m_media_packet_statistic_info.cumulative_recv_packet_count++;
			}
			else{
				if(m_media_packet_statistic_info.last_seq - packetHead->seq > 1000) // seq reset
				{
					m_media_packet_statistic_info.ext_segment_last_seq++;
					if(m_media_packet_statistic_info.ext_segment_last_seq == 0) //ext seq reset
					{
						m_media_packet_statistic_info.ext_segment_last_seq = m_media_packet_statistic_info.ext_segment_start_seq;
						m_media_packet_statistic_info.start_seq = packetHead->seq;
						m_media_packet_statistic_info.last_seq = packetHead->seq;
						m_media_packet_statistic_info.from_last_rr_recv_packet_count = 1;
						m_media_packet_statistic_info.cumulative_recv_packet_count = 1;
						m_is_init_last_rr_info = false;
					}
					else{
						m_media_packet_statistic_info.last_seq = packetHead->seq;
						m_media_packet_statistic_info.from_last_rr_recv_packet_count++;
						m_media_packet_statistic_info.cumulative_recv_packet_count++;
					}
				}
				else // timeout packet
					ret = -1;
			}
		}
		else // repeat packet
			ret = -1;
	}
	else
	{
		//statistic info
		memcpy(m_media_packet_statistic_info.ssrc, packetHead->ssrc, 4);
		m_media_packet_statistic_info.ext_segment_start_seq = RTCP_EXT_SEGMENT_START_SEQ;

		memcpy(m_last_media_rr_info.sender_ssrc, m_receiver_sdes_info.ssrc, 4);
		memcpy(m_last_media_rr_info.ssrc_1, m_media_packet_statistic_info.ssrc, 4);

		//change info
		m_media_packet_statistic_info.start_seq = packetHead->seq;
		m_media_packet_statistic_info.last_seq = packetHead->seq;
		m_media_packet_statistic_info.ext_segment_last_seq = RTCP_EXT_SEGMENT_START_SEQ;
		m_media_packet_statistic_info.from_last_rr_recv_packet_count = 1;
		m_media_packet_statistic_info.cumulative_recv_packet_count = 1;

		m_media_packet_statistic_info.interarrival_jitter = 0;

		m_is_init_media_stream_info = true;
	}
	m_media_packet_statistic_lock.unlock();
	
	return ret;
}

int RTSPReceiver::pushESFrameData(unsigned char *esData, unsigned int dataLen)
{
	printf("dataLen %d: [", dataLen);
	for (int i = 0; i < dataLen && i < 18; i++)
	{
		printf(" %02x", *(esData+i));
	}
	printf(" ]\n");
	if(m_es_frame_buf_offset + dataLen > m_es_frame_buf_size)
		return -1;
	memcpy(m_es_frame_buf+m_es_frame_buf_offset, esData, dataLen);
	m_es_frame_buf_offset += dataLen;

	return 0;
}

void RTSPReceiver::setLastSRInfo(RTCP_SR_Info *srInfo)
{
	memcpy(&m_last_media_sr_info, srInfo, sizeof(RTCP_SR_Info));
	if(!m_is_get_first_sender_report)
	{
		m_first_sr_recv_tick_count = GetTickCount();
		m_is_get_first_sender_report = true;
	}
}

int RTSPReceiver::sendRTCPRRPacket()
{
	m_media_packet_statistic_lock.lock();
	if(m_is_init_media_stream_info)
	{
		unsigned long lowest_seq = (((unsigned long)m_media_packet_statistic_info.ext_segment_start_seq)<<16) + m_media_packet_statistic_info.start_seq;
		unsigned long highest_seq = (((unsigned long)m_media_packet_statistic_info.ext_segment_last_seq)<<16) + m_media_packet_statistic_info.last_seq;
		unsigned long expected_recv_packet_count = highest_seq - lowest_seq + 1;

		unsigned long from_last_rr_expected_recv_pack_count;
		if(!m_is_init_last_rr_info)
		{
			from_last_rr_expected_recv_pack_count = expected_recv_packet_count;
			m_is_init_last_rr_info = true;
		}
		else
			from_last_rr_expected_recv_pack_count = highest_seq - m_last_media_rr_info.highest_seq;

		if(from_last_rr_expected_recv_pack_count == 0) // error
			m_last_media_rr_info.fraction_lost = 0;
		else
			m_last_media_rr_info.fraction_lost = 256 * (from_last_rr_expected_recv_pack_count - m_media_packet_statistic_info.from_last_rr_recv_packet_count)/from_last_rr_expected_recv_pack_count;

		m_last_media_rr_info.cumulation_lost = m_media_packet_statistic_info.cumulative_recv_packet_count - expected_recv_packet_count;

		m_last_media_rr_info.highest_seq = highest_seq;

		if(m_is_get_first_sender_report)
		{
			memcpy(m_last_media_rr_info.last_sr_timestamp, m_last_media_sr_info.ntp+2, 4);
			m_last_media_rr_info.delay_last_sr_timestamp = (GetTickCount() - m_first_sr_recv_tick_count)*1000/65536;
			m_last_media_rr_info.interarrival_jitter = m_media_packet_statistic_info.interarrival_jitter;
		}
		else
		{
			memset(m_last_media_rr_info.last_sr_timestamp, 0, 4);
			m_last_media_rr_info.delay_last_sr_timestamp = 0;
			m_last_media_rr_info.interarrival_jitter = 0;
		}

		m_media_packet_statistic_info.from_last_rr_recv_packet_count = 0;
	}
	m_media_packet_statistic_lock.unlock();

	int rrLen = RTPParse::packRtcpReceiverReport(&m_last_media_rr_info, (unsigned char *)m_rtcp_send_buf, RTCP_PACKET_MAX_LEN);

	send(m_rtcp_fd, m_rtcp_send_buf, rrLen, 0);
	return 0;
}

/**
* \brief function that creates a TcpClient
* 
* \param [in] hostname name of the system to connect
* \param [in] port to connect to on the remote machine
**/
bool RTSPReceiver::createTcpClient(std::string hostname, int port)
{
	int tcpSocket;
	struct sockaddr_in remote;
	struct hostent* phe;
	
	m_hostname.assign(hostname);

	phe = gethostbyname(hostname.c_str()); 

	//set up the socket. 
	tcpSocket = socket(AF_INET,SOCK_STREAM,0);
	if(tcpSocket < 0){
		std::cerr<<"RTSPReceiver::createTcpClient: can't open socket" <<std::endl;
		return false;
	}

	remote.sin_addr.s_addr = inet_addr(phe->h_addr);
	remote.sin_family = AF_INET;
	remote.sin_port = htons(port);

	if(phe != NULL)
	{
		memcpy((void*)&remote.sin_addr, (const void *)phe->h_addr, phe->h_length);
	}
	else
	{
		std::cerr<<"RTSPReciver::createTcpClient:: port("<<port<<") can't get "<< hostname<<" host entry"<<std::endl;
		close(tcpSocket);
		return false;
	}

	int size = 1024 *256;
	if(setsockopt(tcpSocket, SOL_SOCKET, SO_RCVBUF, (char *)&size, sizeof(int)) < 0)
	{
		close(tcpSocket);
		return false;
	}

	//Connect to remote server
	if(connect(tcpSocket, (struct sockaddr *)&remote, sizeof(remote))< 0)
	{
		in_addr *address = (in_addr *)phe->h_addr;
		perror("connect fiailed. Error");
		fprintf(stderr,"RTSPReciver::createTcpClient: Could not connect to machine %s port %d \n", inet_ntoa(*address), port);
		close(tcpSocket);
		return false;
	}
	m_rtsp_fd = tcpSocket;
	setWaitTime(m_rtsp_fd, m_waitTime);
	return true;
}

/**
* \brief sets how long to wait for incoming data
*
* \param [in] wTime wait time as a double
**/
void RTSPReceiver::setWaitTime(int fd, double wTime)
{
	m_waitTime = wTime;
	if(fd >0)
	{
		struct timeval tv;
		tv.tv_sec = (int )m_waitTime;
		tv.tv_usec = (int)(m_waitTime * 1000000);
		setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO,(char*)&tv, sizeof(struct timeval));
	}
}

/**
* \brief Closes the existing connection
**/
void RTSPReceiver::closeSocket(int &fd)
{
	if(fd > 0)
	{
		/*setWaitTime(fd, 1);
		if(!shutdown(fd, SHUT_DRWR))
		{
			uint8_t *buffer = new uint8_t[1024];

			//read out all incoming bytes before closing socket
			//Any error other than timeout, close socket
			//A successful shutdown will report errno 107, ENOTCONN
			int rc;
			bool timeout;
			do{
				rc = read(fd, buffer,sizeof(buffer));
				timeout = (errno == ETIMEDOUT || errno == EWOULDBLOCK || errno == EAGAIN);
			}while((rc < 0 && timeout) || rc > 0);
			
			delete[] buffer;
		}*/
		close(fd);
		fd = -1;
	}
}

ssize_t RTSPReceiver::sendRTSPRequest(unsigned char* buffer, unsigned int length)
{
	ssize_t sofar = 0; // How many charaters sent so far
	int ret; // Return value from send() 
	
	// Make sure we're writing to a valid file descriptor
	if(m_rtsp_fd < 0)
	{
		std::cerr<<"RTSPSocket asked to send to an undefined socket" <<std::endl;
		return -2;
	}
	
	do{
		errno = 0;
		ret = send(m_rtsp_fd, buffer+sofar, length - sofar,0);
		sofar += ret;
		if(ret == -1)
		{
			if(errno == EPIPE)
				ret = -2;
			else
			{
				sofar += 1;
				ret = -3;
			}
		}
		
	}while((ret > 0) && (static_cast<size_t>(sofar) < length));

	if(ret <=0)
		return ret;
	else
		return (sofar);
	
}

ssize_t RTSPReceiver::recvRTSPResponse(unsigned char *buffer, unsigned int length)
{
	if(m_rtsp_fd < 0)
	{
		std::cerr<<"RTSPSocket asked to send to an undefined socket" <<std::endl;
		return -2;
	}	
}

bool RTSPReceiver::postOptionsRequest()
{
	RTSPParse::getOptionsRequest(m_uri.c_str(), m_cseq++, (char *)m_rtsp_send_buf, m_rtsp_send_buf_size);
	if(sendRTSPRequest(m_rtsp_send_buf, strlen((char*)m_rtsp_send_buf)) <=0)
		return false;
	else
	{
		m_rtsp_method = RTSP_METHOD_OPTIONS;
		return true;
	}
}

bool RTSPReceiver::postDescribeRequest()
{
	RTSPParse::getDescribeRequest(m_uri.c_str(), m_cseq++, (char *)m_rtsp_send_buf, m_rtsp_send_buf_size);
	if(sendRTSPRequest(m_rtsp_send_buf, strlen((char*)m_rtsp_send_buf)) <=0)
		return false;
	else
	{
		m_rtsp_method = RTSP_METHOD_DESCRIBE;
		return true;
	}
}

bool RTSPReceiver::postSetupRequest()
{
	int udpSocket;
	socklen_t nameLen;
	struct sockaddr_in rtpLocalAddr;
	struct sockaddr_in rtcpLocalAddr;
	udpSocket = socket(AF_INET, SOCK_DGRAM, 0);
	if(udpSocket == -1)
	{
		return false;
	}
	bzero(&rtpLocalAddr, sizeof(rtpLocalAddr));
	rtpLocalAddr.sin_family = AF_INET;
	rtpLocalAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	rtpLocalAddr.sin_port = htons(0);

	if(::bind(udpSocket, (struct sockaddr *)&rtpLocalAddr, sizeof(rtpLocalAddr)) < 0)
	{
		close(udpSocket);
		return false;
	}
		
	int size = 256 *256;
	if(setsockopt(udpSocket, SOL_SOCKET, SO_RCVBUF, (char *)&size, sizeof(int)) < 0)
	{
		close(udpSocket);
		return false;
	}
	nameLen = sizeof(rtpLocalAddr);
	if(getsockname(udpSocket,(struct sockaddr *)&rtpLocalAddr, &nameLen) < 0)
	{
		close(udpSocket);
		return false;
	}
	m_rtp_fd = udpSocket;
	setWaitTime(m_rtp_fd, m_waitTime);
	m_stream_setup_param.rtp_fd = m_rtp_fd;
	m_stream_setup_param.rtp_port = ntohs(rtpLocalAddr.sin_port);

	udpSocket = socket(AF_INET,SOCK_DGRAM, 0);
	if(udpSocket == -1)
	{
		close(m_rtp_fd);
		return false;
	}
	bzero(&rtcpLocalAddr, sizeof(rtcpLocalAddr));
	rtcpLocalAddr.sin_family = AF_INET;
	rtcpLocalAddr.sin_addr.s_addr = htonl(INADDR_ANY);
	rtcpLocalAddr.sin_port = htons(m_stream_setup_param.rtp_port+1);

	if(::bind(udpSocket, (struct sockaddr *)&rtcpLocalAddr, sizeof(rtcpLocalAddr)) < 0)
	{
		close(udpSocket);
		close(m_rtp_fd);
		m_rtp_fd = -1;
		return false;
	}
	nameLen = sizeof(rtcpLocalAddr);
	if(getsockname(udpSocket,(struct sockaddr *)&rtcpLocalAddr, &nameLen) < 0)
	{
		close(udpSocket);
		close(m_rtp_fd);
		m_rtp_fd = -1;
		return false;
	}
	m_rtcp_fd = udpSocket;
	setWaitTime(m_rtcp_fd, m_waitTime);
	m_stream_setup_param.rtcp_fd = m_rtcp_fd;
	m_stream_setup_param.rtcp_port = ntohs(rtcpLocalAddr.sin_port);

	RTSPParse::getSetupRequest(m_stream_setup_param.media_describe.videoURL.c_str(), m_cseq++, m_stream_setup_param.rtp_port, m_stream_setup_param.rtcp_port, \
		(char *)m_rtsp_send_buf,  m_rtsp_send_buf_size);
	if(sendRTSPRequest(m_rtsp_send_buf, strlen((char*)m_rtsp_send_buf)) <=0)
	{
		close(m_rtp_fd);
		m_rtp_fd = -1;
		close(m_rtcp_fd);
		m_rtcp_fd = -1;
		return false;
	}
	else
	{
		m_rtsp_method = RTSP_METHOD_SETUP;
		return true;
	}
}

bool RTSPReceiver::postPlayRequest()
{
	struct sockaddr_in remote;


	printf("hostname is %s, rtp port %d, rtcp port %d\n",m_hostname.c_str(),ntohs(m_stream_setup_param.media_setup.transport_file_param.server_rtp_addr.sin_port), \
		ntohs(m_stream_setup_param.media_setup.transport_file_param.server_rtcp_addr.sin_port));
	remote.sin_addr.s_addr = inet_addr(m_hostname.c_str());
	remote.sin_family = AF_INET;
	remote.sin_port = m_stream_setup_param.media_setup.transport_file_param.server_rtp_addr.sin_port;

	//rtp connect
	if(connect(m_rtp_fd, (struct sockaddr *)&remote, sizeof(remote))< 0)
	{
		return false;
	}
	
	//rtcp connect
	remote.sin_addr.s_addr = inet_addr(m_hostname.c_str());
	remote.sin_family = AF_INET;
	remote.sin_port = m_stream_setup_param.media_setup.transport_file_param.server_rtcp_addr.sin_port;

	//Connect to remote server
	if(connect(m_rtcp_fd, (struct sockaddr *)&remote, sizeof(remote))< 0)
	{
		return false;
	}
	
	RTSPParse::getPlayRequest(m_uri.c_str(), m_cseq++, m_stream_setup_param.media_setup.session_id.c_str(), (char *)m_rtsp_send_buf, m_rtsp_send_buf_size);
	if(sendRTSPRequest(m_rtsp_send_buf, strlen((char*)m_rtsp_send_buf)) <=0)
		return false;
	else
	{
		m_rtsp_method = RTSP_METHOD_PLAY;
		return true;
	}
}

bool RTSPReceiver::parseDescribeResponse(char* data)
{
	int rspsRet, rspsCSeq;

	RTSP_media_description *mediaDescription = &(m_stream_setup_param.media_describe);
	mediaDescription->h264_sps_pps_nal_len = 0;
	if(RTSPParse::parseDescribeResponse(data, rspsRet, rspsCSeq, *mediaDescription) != 0)
	{
		return false;
	}
	if((rspsRet != RTSP_RSPS_STATUSCODE_OK)) // || (mediaDescription->h264_sps_pps_nal_len == 0))
		return false;
	if(strncasecmp(m_uri.c_str(), mediaDescription->videoURL.c_str(), strlen(m_uri.c_str())) != 0)
	{
		mediaDescription->videoURL = m_uri + "/" + mediaDescription->videoURL;
	}
	return true;
	
}

bool RTSPReceiver::parseSetupResponse(char* data)
{
	int rspsRet, rspsCSeq;

	if(RTSPParse::parseSetupResponse(data,rspsRet, rspsCSeq, m_stream_setup_param.media_setup) != 0)
		return false;
	if(rspsRet != RTSP_RSPS_STATUSCODE_OK)
		return false;

	return true;
	
}

bool RTSPReceiver::parsePlayResponse(char* data)
{
	int rspsRet, rspsCSeq;

	if(RTSPParse::parsePlayResponse(data, rspsRet, rspsCSeq) != 0)
		return false;
	if(rspsRet != RTSP_RSPS_STATUSCODE_OK)
		return false;
	
	return true;
}

}


