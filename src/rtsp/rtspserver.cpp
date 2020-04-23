#include "rtspserver.h"

int RTSPSERVER::createTcpSocket()
{
    int sockfd;
    int on = 1;

    sockfd = socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd < 0)
        return -1;

    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));

    return sockfd;
}

int RTSPSERVER::createUdpSocket()
{
    int sockfd;
    int on = 1;

    sockfd = socket(AF_INET, SOCK_DGRAM, 0);
    if(sockfd < 0)
        return -1;

    setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));

    return sockfd;
}

int RTSPSERVER::bindSocketAddr(int sockfd, const char* ip, int port)
{
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    if(bind(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr)) < 0)
        return -1;

    return 0;
}

int RTSPSERVER::acceptClient(int sockfd, char* ip, int* port)
{
    int clientfd;
    socklen_t len = 0;
    struct sockaddr_in addr;

    memset(&addr, 0, sizeof(addr));
    len = sizeof(addr);

    clientfd = accept(sockfd, (struct sockaddr *)&addr, &len);
    if(clientfd < 0)
        return -1;
    
    strcpy(ip, inet_ntoa(addr.sin_addr));
    *port = ntohs(addr.sin_port);

    return clientfd;
}

inline int RTSPSERVER::startCode3(char* buf)
{
    if(buf[0] == 0 && buf[1] == 0 && buf[2] == 1)
        return 1;
    else
        return 0;
}

inline int RTSPSERVER::startCode4(char* buf)
{
    if(buf[0] == 0 && buf[1] == 0 && buf[2] == 0 && buf[3] == 1)
        return 1;
    else
        return 0;
}

char* RTSPSERVER::findNextStartCode(char* buf, int len)
{
    int i;

    if(len < 3)
        return NULL;

    for(i = 0; i < len-3; ++i)
    {
        if(startCode3(buf) || startCode4(buf))
            return buf;
        
        ++buf;
    }

    if(startCode3(buf))
        return buf;

    return NULL;
}

int RTSPSERVER::getFrameFromBuffer(char* frame,int* frametype,ParaEncUserInfo *uinf)
{
	int frameSize;
	int videoheadlen = sizeof(VideoFrameHeader);
	int audioheadlen = sizeof(AudioFrameHeader);
	int getRet;

	FILE* fp;

	while(1)
	{
		getRet = ModInterface::GetInstance()->CMD_handle(MOD_ENC,CMD_ENC_GETFRAME,(void*)uinf);
		if( getRet!= -1 && *(uinf->buffer) && uinf->frameinfo->FrmLength > 0 )
		{
			*frametype = uinf->frameinfo->Flag;
			printf("info.Flag=%d,info.FrmLength=%ld\n",uinf->frameinfo->Flag,uinf->frameinfo->FrmLength);
			if( uinf->frameinfo->Flag == 3 )
			{
				memcpy(frame, *(uinf->buffer) + audioheadlen + 4, uinf->frameinfo->FrmLength - audioheadlen - 4);
				frameSize = uinf->frameinfo->FrmLength - audioheadlen - 4;
			}
			else
			{
				memcpy(frame, *(uinf->buffer) + videoheadlen, uinf->frameinfo->FrmLength - videoheadlen);
				frameSize = uinf->frameinfo->FrmLength - videoheadlen;

				//fp = fopen("/tmp/111.h264","a+b");
				//fwrite(uinf->buffer + videoheadlen,1,frameSize,fp);
				//fclose(fp);
			}
			break;
		}
	}
	return frameSize;
}

int RTSPSERVER::rtpSendg711uFrame(struct RtpPacket* rtpPacket, uint8_t* frame, uint32_t frameSize)
{
    int sendBytes = 0;
    int ret;

    if (frameSize <= RTP_MAX_PKT_SIZE) // nalu长度小于最大包场：单一NALU单元模式
    {
        memcpy(rtpPacket->payload, frame, frameSize);
        ret = rtpSendPacket(m_ServerRtpSockfd, m_ClientIp, m_ClientARtpPort, rtpPacket, frameSize);
        if(ret < 0)
            return -1;

        rtpPacket->rtpHeader.seq++;
        sendBytes += ret;
    }

    return sendBytes;
}

int RTSPSERVER::rtpSendH264Frame(struct RtpPacket* rtpPacket, uint8_t* frame, uint32_t frameSize)
{
    uint8_t naluType; // nalu第一个字节
    int sendBytes = 0;
    int ret;

    naluType = frame[0];

    if (frameSize <= RTP_MAX_PKT_SIZE) // nalu长度小于最大包场：单一NALU单元模式
    {
        /*
         *   0 1 2 3 4 5 6 7 8 9
         *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         *  |F|NRI|  Type   | a single NAL unit ... |
         *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         */
        memcpy(rtpPacket->payload, frame, frameSize);
        ret = rtpSendPacket(m_ServerRtpSockfd, m_ClientIp, m_ClientVRtpPort, rtpPacket, frameSize);
        if(ret < 0)
            return -1;

        rtpPacket->rtpHeader.seq++;
        sendBytes += ret;
    }
    else // nalu长度小于最大包场：分片模式
    {
        /*
         *  0                   1                   2
         *  0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3
         * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         * | FU indicator  |   FU header   |   FU payload   ...  |
         * +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         */

        /*
         *     FU Indicator
         *    0 1 2 3 4 5 6 7
         *   +-+-+-+-+-+-+-+-+
         *   |F|NRI|  Type   |
         *   +---------------+
         */

        /*
         *      FU Header
         *    0 1 2 3 4 5 6 7
         *   +-+-+-+-+-+-+-+-+
         *   |S|E|R|  Type   |
         *   +---------------+
         */

        int pktNum = frameSize / RTP_MAX_PKT_SIZE;       // 有几个完整的包
        int remainPktSize = frameSize % RTP_MAX_PKT_SIZE; // 剩余不完整包的大小
        int i, pos = 1;

        /* 发送完整的包 */
        for (i = 0; i < pktNum; i++)
        {
            rtpPacket->payload[0] = (naluType & 0x60) | 28;
            rtpPacket->payload[1] = naluType & 0x1F;
            
            if (i == 0) //第一包数据
                rtpPacket->payload[1] |= 0x80; // start
            else if (remainPktSize == 0 && i == pktNum - 1) //最后一包数据
                rtpPacket->payload[1] |= 0x40; // end

            memcpy(rtpPacket->payload+2, frame+pos, RTP_MAX_PKT_SIZE);
            ret = rtpSendPacket(m_ServerRtpSockfd, m_ClientIp, m_ClientVRtpPort, rtpPacket, RTP_MAX_PKT_SIZE+2);
            if(ret < 0)
                return -1;

            rtpPacket->rtpHeader.seq++;
            sendBytes += ret;
            pos += RTP_MAX_PKT_SIZE;
        }

        /* 发送剩余的数据 */
        if (remainPktSize > 0)
        {
            rtpPacket->payload[0] = (naluType & 0x60) | 28;
            rtpPacket->payload[1] = naluType & 0x1F;
            rtpPacket->payload[1] |= 0x40; //end

            memcpy(rtpPacket->payload+2, frame+pos, remainPktSize);
            ret = rtpSendPacket(m_ServerRtpSockfd, m_ClientIp, m_ClientVRtpPort, rtpPacket, remainPktSize+2);
            if(ret < 0)
                return -1;

            rtpPacket->rtpHeader.seq++;
            sendBytes += ret;
        }
    }
    return sendBytes;
}

int RTSPSERVER::rtpSendH265Frame(struct RtpPacket* rtpPacket, uint8_t* frame, uint32_t frameSize)
{
    uint8_t naluType[2]; // nalu前两个字节
    int sendBytes = 0;
    int ret;

    naluType[0] = frame[0];
    naluType[1] = frame[1];

    if (frameSize <= RTP_MAX_PKT_SIZE) // nalu长度小于最大包场：单一NALU单元模式
    {
        /*
		 *   0               1               2
         *   0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3
         *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         *  |F|   Type    |  LayerId  | TID |a single NAL unit ...  |
         *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         */
        memcpy(rtpPacket->payload, frame, frameSize);
        ret = rtpSendPacket(m_ServerRtpSockfd, m_ClientIp, m_ClientVRtpPort, rtpPacket, frameSize);
        if(ret < 0)
            return -1;

        rtpPacket->rtpHeader.seq++;
        sendBytes += ret;
    }
    else // nalu长度小于最大包场：分片模式
    {
        /*
         *   0               1               2               3
         *   0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7 0 1 2
         *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         *  |           FU indicator        |   FU header   |   FU payload   ...  |
         *  +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         */

        /*
         *     FU Indicator
         *    0 1 2 3 4 5 6 7 0 1 2 3 4 5 6 7
         *   +-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+-+
         *   |F|   Type    |  LayerId  | TID |
         *   +---------------+-+-+-+-+-+-+-+-+
         */

        /*
         *      FU Header
         *    0 1 2 3 4 5 6 7
         *   +-+-+-+-+-+-+-+-+
         *   |S|E|   Type    |
         *   +---------------+
         */

        int pktNum = frameSize / RTP_MAX_PKT_SIZE;       // 有几个完整的包
        int remainPktSize = frameSize % RTP_MAX_PKT_SIZE; // 剩余不完整包的大小
        int i, pos = 2;

        /* 发送完整的包 */
        for (i = 0; i < pktNum; i++)
        {
            rtpPacket->payload[0] = (naluType[0] & 0x81) | (49 << 1);
            rtpPacket->payload[1] = naluType[1];
            rtpPacket->payload[2] = (naluType[0] & 0x7e) >> 1;
            
            if (i == 0) //第一包数据
                rtpPacket->payload[2] |= 0x80; // start
            else if (remainPktSize == 0 && i == pktNum - 1) //最后一包数据
                rtpPacket->payload[2] |= 0x40; // end

            memcpy(rtpPacket->payload+3, frame+pos, RTP_MAX_PKT_SIZE);
            ret = rtpSendPacket(m_ServerRtpSockfd, m_ClientIp, m_ClientVRtpPort, rtpPacket, RTP_MAX_PKT_SIZE+3);
            if(ret < 0)
                return -1;

            rtpPacket->rtpHeader.seq++;
            sendBytes += ret;
            pos += RTP_MAX_PKT_SIZE;
        }

        /* 发送剩余的数据 */
        if (remainPktSize > 0)
        {
            rtpPacket->payload[0] = (naluType[0] & 0x81) | (49 << 1);
            rtpPacket->payload[1] = naluType[1];
            rtpPacket->payload[2] = (naluType[0] & 0x7e) >> 1;
            rtpPacket->payload[2] |= 0x40; //end

            memcpy(rtpPacket->payload+3, frame+pos, remainPktSize);
            ret = rtpSendPacket(m_ServerRtpSockfd, m_ClientIp, m_ClientVRtpPort, rtpPacket, remainPktSize+3);
            if(ret < 0)
                return -1;

            rtpPacket->rtpHeader.seq++;
            sendBytes += ret;
        }
    }
    return sendBytes;
}

void *RtspPlayProcess(void *arg)
{
	prctl(PR_SET_NAME, __FUNCTION__);
	pthread_detach(pthread_self());
	/* 开始播放，发送RTP包 */
	int frameSize, nalusize, startCode,frametype;
	char* frame = (char*)malloc(500000);
	struct RtpPacket* rtpVPacket = (struct RtpPacket*)malloc(500000);
	struct RtpPacket* rtpAPacket = (struct RtpPacket*)malloc(500);
	RTSPSERVER *rs = (RTSPSERVER *)arg;
	int offset = 0;
	int preoffset = 0;
	char *nextStartCode;
	FrameInfo frame_info;
	unsigned char *video_data = NULL;
	ParaEncUserInfo uinf;

	uinf.ch = 0;
	uinf.userid = 0;
	uinf.buffer = &video_data;
	uinf.frameinfo = &frame_info;

	rtpHeaderInit(rtpVPacket, 0, 0, 0, RTP_VESION, RTP_PAYLOAD_TYPE_H264, 0, 0, 0, 0x88923423);
	rtpHeaderInit(rtpAPacket, 0, 0, 0, RTP_VESION, RTP_PAYLOAD_TYPE_PCMA, 0, 0, 0, 0x88923423);

	printf("start play\n");
	printf("client ip:%s\n", rs->m_ClientIp);
	printf("client audio port:%d\n", rs->m_ClientARtpPort);
	printf("client video port:%d\n", rs->m_ClientVRtpPort);

	ModInterface::GetInstance()->CMD_handle(MOD_ENC,CMD_ENC_RESETUSER,(void*)&uinf);
	while (rs->g_OnPlay)
	{
		frameSize = rs->getFrameFromBuffer(frame,&frametype,&uinf);
		if(frameSize < 0)
		{
			break;
		}

		if(frametype == 3)
		{
			rs->rtpSendg711uFrame(rtpAPacket, (uint8_t*)frame, frameSize);
			rtpAPacket->rtpHeader.timestamp += 160;
		}
		else
		{
			do
			{
				if(rs->startCode3(frame + preoffset))
				{
					startCode = 3;
				}
				else
				{
					startCode = 4;
				}
				nextStartCode = rs->findNextStartCode(frame + preoffset + 3,frameSize - preoffset - 3);
				if(nextStartCode)
				{
					offset = nextStartCode - frame;
					nalusize = offset - startCode - preoffset;
				}
				else
				{
					nalusize = frameSize - startCode - preoffset;
				}

				//rs->rtpSendH264Frame(rtpVPacket, (uint8_t*)frame + startCode + preoffset, nalusize);
				rs->rtpSendH265Frame(rtpVPacket, (uint8_t*)frame + startCode + preoffset, nalusize);
				preoffset = offset;
			}while(nextStartCode);
			offset = 0;
			preoffset = 0;
			rtpVPacket->rtpHeader.timestamp += 90000/30;
		}
		usleep(20 * 1000);
	}
	free(frame);
	free(rtpAPacket);
	free(rtpVPacket);
	pthread_exit(0);
}

char* RTSPSERVER::getLineFromBuf(char* buf, char* line)
{
    while(*buf != '\n')
    {
        *line = *buf;
        line++;
        buf++;
    }

    *line = '\n';
    ++line;
    *line = '\0';

    ++buf;
    return buf; 
}

int RTSPSERVER::handleCmd_OPTIONS(char* result, int cseq)
{
    sprintf(result, "RTSP/1.0 200 OK\r\n"
                    "CSeq: %d\r\n"
                    "Public: OPTIONS, DESCRIBE, SETUP, PLAY, TEARDOWN\r\n"
                    "\r\n",
                    cseq);
                
    return 0;
}

int RTSPSERVER::handleCmd_DESCRIBE(char* result, int cseq, char* url)
{
    char sdp[500];
    char localIp[100];

    sscanf(url, "rtsp://%[^:]:", localIp);

    sprintf(sdp, "v=0\r\n"
                 "o=- 9%ld 1 IN IP4 %s\r\n"
                 "t=0 0\r\n"
                 "a=control:*\r\n"
                 "m=video 0 RTP/AVP 96\r\n"
                 "a=rtpmap:96 H265/90000\r\n"
                 "a=control:track0\r\n",
#if 0
                 "m=audio 0 RTP/AVP 8\r\n"
                 "a=rtpmap:8 PCMA/8000/1\r\n"
                 "a=control:track1\r\n",
#endif
                 time(NULL), localIp);
    
    sprintf(result, "RTSP/1.0 200 OK\r\nCSeq: %d\r\n"
                    "Content-Base: %s\r\n"
                    "Content-type: application/sdp\r\n"
                    "Content-length: %d\r\n\r\n"
                    "%s",
                    cseq,
                    url,
                    strlen(sdp),
                    sdp);
    
    return 0;
}

int RTSPSERVER::handleCmd_SETUP(char* result, int cseq, char* url)
{
	if(strstr(url,"track0") != NULL)
	{
		sprintf(result, "RTSP/1.0 200 OK\r\n"
				"CSeq: %d\r\n"
				"Transport: RTP/AVP;unicast;client_port=%d-%d;server_port=%d-%d\r\n"
				"Session: 66334873\r\n"
				"\r\n",
				cseq,
				m_ClientVRtpPort,
				m_ClientVRtpPort+1,
				SERVER_RTP_PORT,
				SERVER_RTCP_PORT);
	}
	else if(strstr(url,"track1") != NULL)
	{
		sprintf(result, "RTSP/1.0 200 OK\r\n"
				"CSeq: %d\r\n"
				"Transport: RTP/AVP;unicast;client_port=%d-%d;server_port=%d-%d\r\n"
				"Session: 66334873\r\n"
				"\r\n",
				cseq,
				m_ClientARtpPort,
				m_ClientARtpPort+1,
				SERVER_RTP_PORT,
				SERVER_RTCP_PORT);
	}

	return 0;
}

int RTSPSERVER::handleCmd_PLAY(char* result, int cseq)
{
	pthread_t pid;
    sprintf(result, "RTSP/1.0 200 OK\r\n"
                    "CSeq: %d\r\n"
                    "Range: npt=0.000-\r\n"
                    "Session: 66334873; timeout=60\r\n\r\n",
                    cseq);
    
	g_OnPlay = 1;
	if(pthread_create(&pid,NULL,RtspPlayProcess,(void *)this) < 0)
	{
		printf("pthread create failed!\n");
		return -1;
	}
    return 0;
}

int RTSPSERVER::handleCmd_TEARDOWN(char* result, int cseq)
{
    sprintf(result, "RTSP/1.0 200 OK\r\n"
                    "CSeq: %d\r\n\r\n",
                    cseq);
	g_OnPlay = 0;
    
    return 0;
}

void RTSPSERVER::doClient()
{
    char method[40];
    char url[100];
    char version[40];
    int cseq;
    char *bufPtr;
    char* rBuf = (char*)malloc(BUF_MAX_SIZE);
    char* sBuf = (char*)malloc(BUF_MAX_SIZE);
    char line[400];
	int recvLen;

    while(1)
    {

        recvLen = recv(m_ClientSockfd, rBuf, BUF_MAX_SIZE, 0);
        if(recvLen <= 0)
		{
			break;
		}

        rBuf[recvLen] = '\0';
        printf("---------------C->S--------------\n");
        printf("%s", rBuf);

        /* 解析方法 */
        bufPtr = getLineFromBuf(rBuf, line);
        if(sscanf(line, "%s %s %s\r\n", method, url, version) != 3)
        {
            printf("parse err\n");
            break;
        }

        /* 解析序列号 */
        bufPtr = getLineFromBuf(bufPtr, line);
        if(sscanf(line, "CSeq: %d\r\n", &cseq) != 1)
        {
            printf("parse err\n");
            break;
        }

        /* 如果是SETUP，那么就再解析client_port */
        if(!strcmp(method, "SETUP"))
		{
			if(strstr(url,"track0") != NULL)
			{
				while(1)
				{
					bufPtr = getLineFromBuf(bufPtr, line);
					if(!strncmp(line, "Transport:", strlen("Transport:")))
					{
						sscanf(line, "Transport: RTP/AVP;unicast;client_port=%d-%d\r\n",
								&m_ClientVRtpPort, &m_ClientVRtcpPort);
						break;
					}
				}
			}
			else if(strstr(url,"track1") != NULL)
			{
				while(1)
				{
					bufPtr = getLineFromBuf(bufPtr, line);
					if(!strncmp(line, "Transport:", strlen("Transport:")))
					{
						sscanf(line, "Transport: RTP/AVP;unicast;client_port=%d-%d\r\n",
								&m_ClientARtpPort, &m_ClientARtcpPort);
						break;
					}
				}
			}
		}

        if(!strcmp(method, "OPTIONS"))
        {
            if(handleCmd_OPTIONS(sBuf, cseq))
            {
                printf("failed to handle options\n");
                break;
            }
        }
        else if(!strcmp(method, "DESCRIBE"))
        {
            if(handleCmd_DESCRIBE(sBuf, cseq, url))
            {
                printf("failed to handle describe\n");
                break;
            }
        }
        else if(!strcmp(method, "SETUP"))
        {
            if(handleCmd_SETUP(sBuf, cseq, url))
            {
                printf("failed to handle setup\n");
                break;
            }
        }
        else if(!strcmp(method, "PLAY"))
        {
            if(handleCmd_PLAY(sBuf, cseq))
            {
                printf("failed to handle play\n");
                break;
            }
        }
		else if(!strcmp(method, "TEARDOWN"))
		{
            if(handleCmd_TEARDOWN(sBuf, cseq))
            {
                printf("failed to handle TEARDOWN\n");
                break;
            }
		}
        else
        {
            break;
        }

        printf("---------------S->C--------------\n");
        printf("%s", sBuf);
        send(m_ClientSockfd, sBuf, strlen(sBuf), 0);
    }
    printf("finish\n");
    close(m_ClientSockfd);
    free(rBuf);
    free(sBuf);
}

int RTSPSERVER::StartRtspServer()
{
    m_ServerTcpSockfd = createTcpSocket();
    if(m_ServerTcpSockfd < 0)
    {
        printf("failed to create tcp socket\n");
        return -1;
    }

    if(bindSocketAddr(m_ServerTcpSockfd, "0.0.0.0", SERVER_PORT) < 0)
    {
        printf("failed to bind addr\n");
        return -1;
    }

    if(listen(m_ServerTcpSockfd, 10) < 0)
    {
        printf("failed to listen\n");
        return -1;
    }

    m_ServerRtpSockfd = createUdpSocket();
    m_ServerRtcpSockfd = createUdpSocket();
    if(m_ServerRtpSockfd < 0 || m_ServerRtcpSockfd < 0)
    {
        printf("failed to create udp socket\n");
        return -1;
    }

    if(bindSocketAddr(m_ServerRtpSockfd, "0.0.0.0", SERVER_RTP_PORT) < 0 ||
        bindSocketAddr(m_ServerRtcpSockfd, "0.0.0.0", SERVER_RTCP_PORT) < 0)
    {
        printf("failed to bind addr\n");
        return -1;
    }
    
    printf("rtsp://127.0.0.1:%d\n", SERVER_PORT);

    while(1)
    {
        m_ClientSockfd = acceptClient(m_ServerTcpSockfd, m_ClientIp, &m_ClientPort);
        if(m_ClientSockfd < 0)
        {
            printf("failed to accept client\n");
            return -1;
        }

        printf("accept client;client ip:%s,client port:%d\n", m_ClientIp, m_ClientPort);

        doClient();
    }
	return 0;
}

void *RtspServerProcess(void *arg)
{
	prctl(PR_SET_NAME, __FUNCTION__);
	pthread_detach(pthread_self());
	RTSPSERVER rs;
	rs.StartRtspServer();
    return 0;
}

int InitRtspServer()
{
	pthread_t pid;
	if(pthread_create(&pid,NULL,RtspServerProcess,NULL) < 0)
	{
		printf("pthread create failed!\n");
		return -1;
	}
	return 0;
}
