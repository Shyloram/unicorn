#include <sys/types.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <net/if.h>
#include "rtspserver.h"
#include "aes.h"
#include "zmdconfig.h"
#include "modinterface.h"
#include "threadpool.h"

#ifdef ZMD_APP_ENCODE_BUFFERMANAGE_IFRAME_ENCRYPTION
int AesDecrypt(unsigned char *input,unsigned char *output)
{
	int ret = 0;
	mbedtls_aes_context ctx;
	unsigned char iv_str[100]={0};
	unsigned char key_str[100]={0};
	ModInterface::GetInstance()->CMD_handle(MOD_ENC,CMD_ENC_GETAESKEY,(void*)key_str);

	mbedtls_aes_init(&ctx);
	mbedtls_aes_setkey_dec(&ctx, key_str, 256);
	ret = mbedtls_aes_crypt_cbc(&ctx, MBEDTLS_AES_DECRYPT, 256, iv_str, input, output);
	if(ret != 0)
	{
		RTSPERR("dec error ret %d\n",ret);
	}
	mbedtls_aes_free(&ctx);
	return ret;
}
#endif

int GetLocalIP(char *ip)
{
	int ret = 0;
	int sockfd = 0;
	struct ifreq ifr;

	strcpy(ifr.ifr_name, "eth0");

	sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	if(sockfd)
	{
		ret = ioctl(sockfd, SIOCGIFADDR, &ifr);
		if(!ret)
		{
			strcpy(ip, inet_ntoa(((struct sockaddr_in *)&ifr.ifr_addr)->sin_addr));
		}
		else
		{
			strcpy(ip,"127.0.0.1");
		}
	}
	return 0;
}

/*******************************************
 *     CLASS     RTES CLIENT
 ******************************************/
RtspClient::RtspClient()
{
	m_free = 1;
}

RtspClient::~RtspClient()
{
}

inline int RtspClient::startCode3(char* buf)
{
    if(buf[0] == 0 && buf[1] == 0 && buf[2] == 1)
        return 1;
    else
        return 0;
}

inline int RtspClient::startCode4(char* buf)
{
    if(buf[0] == 0 && buf[1] == 0 && buf[2] == 0 && buf[3] == 1)
        return 1;
    else
        return 0;
}

char* RtspClient::findNextStartCode(char* buf, int len)
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

int RtspClient::getFrameFromBuffer(char* frame,int* frametype,ParaEncUserInfo *uinf)
{
	int frameSize;
	int videoheadlen = sizeof(VideoFrameHeader);
	int audioheadlen = sizeof(AudioFrameHeader);
	int getRet;

	while(1)
	{
		getRet = ModInterface::GetInstance()->CMD_handle(MOD_ENC,CMD_ENC_GETFRAME,(void*)uinf);
		if( getRet!= -1 && *(uinf->buffer) && uinf->frameinfo->FrmLength > 0 )
		{
			*frametype = uinf->frameinfo->Flag;
			//RTSPLOG("info.Flag=%d,info.FrmLength=%ld\n",uinf->frameinfo->Flag,uinf->frameinfo->FrmLength);
			if( *frametype == 3 )
			{
				memcpy(frame, *(uinf->buffer) + audioheadlen + 4, uinf->frameinfo->FrmLength - audioheadlen - 4);
				frameSize = uinf->frameinfo->FrmLength - audioheadlen - 4;
			}
			else
			{
				memcpy(frame, *(uinf->buffer) + videoheadlen, uinf->frameinfo->FrmLength - videoheadlen);
				frameSize = uinf->frameinfo->FrmLength - videoheadlen;
#ifdef ZMD_APP_ENCODE_BUFFERMANAGE_IFRAME_ENCRYPTION
				if(*frametype == 1)
				{
					unsigned char decinput[256] = {0};
					memcpy(decinput,frame,256);
					AesDecrypt(decinput,(unsigned char*)frame);
				}
#endif
			}
			break;
		}
	}
	return frameSize;
}

int RtspClient::rtpSendg711uFrame(struct RtpPacket* rtpPacket, uint8_t* frame, uint32_t frameSize)
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

int RtspClient::rtpSendH264Frame(struct RtpPacket* rtpPacket, uint8_t* frame, uint32_t frameSize)
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

int RtspClient::rtpSendH265Frame(struct RtpPacket* rtpPacket, uint8_t* frame, uint32_t frameSize)
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

void RtspClient::DoPlay()
{
	/* 开始播放，发送RTP包 */
	int frameSize, nalusize, startCode,frametype;
	char* frame = (char*)malloc(500000);
	struct RtpPacket* rtpVPacket = (struct RtpPacket*)malloc(500000);
	struct RtpPacket* rtpAPacket = (struct RtpPacket*)malloc(500);
	int offset = 0;
	int preoffset = 0;
	char *nextStartCode;
	FrameInfo frame_info;
	unsigned char *video_data = NULL;

	m_uinf.buffer = &video_data;
	m_uinf.frameinfo = &frame_info;

	rtpHeaderInit(rtpVPacket, 0, 0, 0, RTP_VESION, RTP_PAYLOAD_TYPE_H264, 0, 0, 0, 0x88923423);
	rtpHeaderInit(rtpAPacket, 0, 0, 0, RTP_VESION, RTP_PAYLOAD_TYPE_PCMA, 0, 0, 0, 0x88923423);

	RTSPLOG("start play\n");
	RTSPLOG("ch:%d,userid:%d,venctype:%d\n", m_uinf.ch, m_uinf.userid, m_uinf.venctype);
	RTSPLOG("client ip:%s\n", m_ClientIp);
	RTSPLOG("client audio port:%d\n", m_ClientARtpPort);
	RTSPLOG("client video port:%d\n", m_ClientVRtpPort);

	ModInterface::GetInstance()->CMD_handle(MOD_ENC,CMD_ENC_RESETUSER,(void*)&(m_uinf));
	while (g_OnPlay)
	{
		frameSize = getFrameFromBuffer(frame,&frametype,&(m_uinf));
		if(frameSize < 0)
		{
			break;
		}

		if(frametype == 3)
		{
			rtpSendg711uFrame(rtpAPacket, (uint8_t*)frame, frameSize);
			rtpAPacket->rtpHeader.timestamp += 160;
		}
		else
		{
			do
			{
				if(startCode3(frame + preoffset))
				{
					startCode = 3;
				}
				else
				{
					startCode = 4;
				}
				nextStartCode = findNextStartCode(frame + preoffset + 3,frameSize - preoffset - 3);
				if(nextStartCode)
				{
					offset = nextStartCode - frame;
					nalusize = offset - startCode - preoffset;
				}
				else
				{
					nalusize = frameSize - startCode - preoffset;
				}

				if(m_venctype == PT_H264)
				{
					rtpSendH264Frame(rtpVPacket, (uint8_t*)frame + startCode + preoffset, nalusize);
				}
				else if(m_venctype == PT_H265)
				{
					rtpSendH265Frame(rtpVPacket, (uint8_t*)frame + startCode + preoffset, nalusize);
				}
				preoffset = offset;
			}while(nextStartCode);
			offset = 0;
			preoffset = 0;
			rtpVPacket->rtpHeader.timestamp += 90000/30;
		}
		usleep(20 * 1000);
	}
	ModInterface::GetInstance()->CMD_handle(MOD_ENC,CMD_ENC_RELEASEUSER,(void*)&(m_uinf));
	free(frame);
	free(rtpAPacket);
	free(rtpVPacket);
}

void *RtspPlayProcess(void *arg)
{
	prctl(PR_SET_NAME, __FUNCTION__);
	RtspClient *rc = (RtspClient *)arg;
	rc->DoPlay();
	return 0;
}

char* RtspClient::getLineFromBuf(char* buf, char* line)
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

int RtspClient::handleCmd_OPTIONS(char* result, int cseq)
{
    sprintf(result, "RTSP/1.0 200 OK\r\n"
                    "CSeq: %d\r\n"
                    "Public: OPTIONS, DESCRIBE, SETUP, PLAY, TEARDOWN\r\n"
                    "\r\n",
                    cseq);
                
    return 0;
}

int RtspClient::handleCmd_DESCRIBE(char* result, int cseq, char* url)
{
    char sdp[500];
    char localIp[100];
	int ch = -1;

    sscanf(url, "rtsp://%[^:]:%*[^/]/live%d", localIp, &ch);
	m_uinf.ch = ch;
	ModInterface::GetInstance()->CMD_handle(MOD_ENC,CMD_ENC_REGISTERUSER,(void*)&m_uinf);
	m_venctype = m_uinf.venctype;

    sprintf(sdp, "v=0\r\n"
                 "o=- 9%ld 1 IN IP4 %s\r\n"
                 "t=0 0\r\n"
                 "a=control:*\r\n"
#ifdef ZMD_APP_ENCODE_AUDIO
                 "m=audio 0 RTP/AVP 8\r\n"
                 "a=rtpmap:8 PCMA/8000/1\r\n"
                 "a=control:track1\r\n"
#endif
                 "m=video 0 RTP/AVP 96\r\n"
                 "a=rtpmap:96 %s/90000\r\n"
                 "a=control:track0\r\n\r\n",
                 time(NULL), localIp, (m_venctype==PT_H264)?"H264":"H265");
    
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

int RtspClient::handleCmd_SETUP(char* result, int cseq, char* url)
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

int RtspClient::handleCmd_PLAY(char* result, int cseq)
{
	ptask_t task = {};
    sprintf(result, "RTSP/1.0 200 OK\r\n"
                    "CSeq: %d\r\n"
                    "Range: npt=0.000-\r\n"
                    "Session: 66334873; timeout=60\r\n\r\n",
                    cseq);
    
	g_OnPlay = 1;
	task.run = RtspPlayProcess;
	task.arg = (void*)this;
	ModInterface::GetInstance()->CMD_handle(MOD_TPL,CMD_TPL_ADDTASK,(void*)&task);
    return 0;
}

int RtspClient::handleCmd_TEARDOWN(char* result, int cseq)
{
    sprintf(result, "RTSP/1.0 200 OK\r\n"
                    "CSeq: %d\r\n\r\n",
                    cseq);
	g_OnPlay = 0;
    
    return 0;
}

int RtspClient::GetFreeFlag()
{
	return m_free;
}

void RtspClient::SetFreeFlag()
{
	m_free = 1;
}

void RtspClient::ClearFreeFlag()
{
	m_free = 0;
}

int RtspClient::InitClientPara(int srtpsockfd,int srtcpsockfd, int csockfd,char *ip,int port)
{
	m_ServerRtpSockfd = srtpsockfd;
	m_ServerRtcpSockfd = srtcpsockfd;
	m_ClientSockfd = csockfd;
	m_ClientPort = port;
	strncpy(m_ClientIp,ip,40);
	return 0;
}

void RtspClient::CloseClient()
{
	if(m_free == 0)
	{
		g_OnPlay = 0;
		g_OnClient = 0;
	}
}

void RtspClient::DoClient()
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
	g_OnClient = 1;

    while(g_OnClient)
    {

        recvLen = recv(m_ClientSockfd, rBuf, BUF_MAX_SIZE, 0);
        if(recvLen <= 0)
		{
			break;
		}

        rBuf[recvLen] = '\0';
        RTSPLOG("recv from client\n");
        printf("---------------C->S--------------\n");
        printf("%s", rBuf);

        /* 解析方法 */
        bufPtr = getLineFromBuf(rBuf, line);
        if(sscanf(line, "%s %s %s\r\n", method, url, version) != 3)
        {
            RTSPERR("parse err\n");
            break;
        }

        /* 解析序列号 */
        bufPtr = getLineFromBuf(bufPtr, line);
        if(sscanf(line, "CSeq: %d\r\n", &cseq) != 1)
        {
            RTSPERR("parse err\n");
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
                RTSPERR("failed to handle options\n");
                break;
            }
        }
        else if(!strcmp(method, "DESCRIBE"))
        {
            if(handleCmd_DESCRIBE(sBuf, cseq, url))
            {
                RTSPERR("failed to handle describe\n");
                break;
            }
        }
        else if(!strcmp(method, "SETUP"))
        {
            if(handleCmd_SETUP(sBuf, cseq, url))
            {
                RTSPERR("failed to handle setup\n");
                break;
            }
        }
        else if(!strcmp(method, "PLAY"))
        {
            if(handleCmd_PLAY(sBuf, cseq))
            {
                RTSPERR("failed to handle play\n");
                break;
            }
        }
		else if(!strcmp(method, "TEARDOWN"))
		{
            if(handleCmd_TEARDOWN(sBuf, cseq))
            {
                RTSPERR("failed to handle TEARDOWN\n");
                break;
            }
		}
        else
        {
            break;
        }

        RTSPLOG("send to client\n");
        printf("---------------S->C--------------\n");
        printf("%s", sBuf);
        send(m_ClientSockfd, sBuf, strlen(sBuf), 0);
    }
    RTSPLOG("finish\n");
	RTSPLOG("client exit;client ip:%s,client port:%d\n", m_ClientIp, m_ClientPort);
    close(m_ClientSockfd);
    free(rBuf);
    free(sBuf);
	SetFreeFlag();
}

/*******************************************
 *     CLASS     RTES SERVER
 ******************************************/
RtspServer* RtspServer::m_instance = new RtspServer;
RtspServer* RtspServer::GetInstance(void)
{
	return m_instance;
}

RtspServer::RtspServer()
{
}

RtspServer::~RtspServer()
{
}

int RtspServer::CreateSocket(enum  SocketType stype)
{
	int sockfd = 0;
	int on = 1;

	if(stype == SOCKET_TCP) 
	{
		sockfd = socket(AF_INET, SOCK_STREAM, 0);
	}
	else if(stype == SOCKET_UDP)
	{
		sockfd = socket(AF_INET, SOCK_DGRAM, 0);
	}
	if(sockfd < 0)
		return -1;

	setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, (const char*)&on, sizeof(on));

	return sockfd;
}

int RtspServer::BindSocketAddr(int sockfd, const char* ip, int port)
{
    struct sockaddr_in addr;

    addr.sin_family = AF_INET;
    addr.sin_port = htons(port);
    addr.sin_addr.s_addr = inet_addr(ip);

    if(bind(sockfd, (struct sockaddr *)&addr, sizeof(struct sockaddr)) < 0)
        return -1;

    return 0;
}

int RtspServer::AcceptClient(int sockfd, char* ip, int* port)
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

RtspClient * RtspServer::GetFreeClient()
{
	int i = 0;
	for(i = 0;i < 10;i++)
	{
		if(m_rtsp_client[i].GetFreeFlag())
		{
			m_rtsp_client[i].ClearFreeFlag();
			return &m_rtsp_client[i];
		}
	}
	return NULL;
}

void *RtspClientProcess(void *arg)
{
	prctl(PR_SET_NAME, __FUNCTION__);
	RtspClient *rc = (RtspClient *)arg;
	rc->DoClient();
    return 0;
}

int RtspServer::StartRtspServer()
{
	RtspClient *client = NULL;
	int clientsockfd = 0;
	char localip[40] = {0};
	char clientip[40] = {0};
	int clientport;
	g_OnServer = 1;
	ptask_t task = {};

    m_ServerTcpSockfd = CreateSocket(SOCKET_TCP);
    if(m_ServerTcpSockfd < 0)
    {
        RTSPERR("failed to create tcp socket\n");
        return -1;
    }

    if(BindSocketAddr(m_ServerTcpSockfd, "0.0.0.0", SERVER_PORT) < 0)
    {
        RTSPERR("failed to bind addr\n");
        return -1;
    }

    if(listen(m_ServerTcpSockfd, 10) < 0)
    {
        RTSPERR("failed to listen\n");
        return -1;
    }

    m_ServerRtpSockfd = CreateSocket(SOCKET_UDP);
    m_ServerRtcpSockfd = CreateSocket(SOCKET_UDP);
    if(m_ServerRtpSockfd < 0 || m_ServerRtcpSockfd < 0)
    {
        RTSPERR("failed to create udp socket\n");
        return -1;
    }

    if(BindSocketAddr(m_ServerRtpSockfd, "0.0.0.0", SERVER_RTP_PORT) < 0 ||
        BindSocketAddr(m_ServerRtcpSockfd, "0.0.0.0", SERVER_RTCP_PORT) < 0)
    {
        RTSPERR("failed to bind addr\n");
        return -1;
    }
    
	GetLocalIP(localip);
    RTSPLOG("rtsp://%s:%d/live[ch] (ch:0/1/2 ----HD/MD/LD)\n", localip, SERVER_PORT);

    while(g_OnServer)
    {
        clientsockfd = AcceptClient(m_ServerTcpSockfd, clientip, &clientport);
        if(clientsockfd < 0)
        {
            RTSPERR("failed to accept client\n");
            return -1;
        }

        RTSPLOG("accept client;client ip:%s,client port:%d\n", clientip, clientport);

		client = GetFreeClient();
		if(client)
		{
			client->InitClientPara(m_ServerRtpSockfd,m_ServerRtcpSockfd,clientsockfd,clientip,clientport);
			task.run = RtspClientProcess;
			task.arg = (void*)client;
			ModInterface::GetInstance()->CMD_handle(MOD_TPL,CMD_TPL_ADDTASK,(void*)&task);
		}
		else
		{
			RTSPERR("the client is full!\n");
			close(clientsockfd);
		}
    }
	close(m_ServerRtcpSockfd);
	close(m_ServerRtpSockfd);
	close(m_ServerTcpSockfd);
	return 0;
}

int RtspServer::StopRtspServer()
{
	int i;
	
	for(i = 0;i < 10;i++)
	{
		m_rtsp_client[i].CloseClient();
	}
	g_OnServer = 0;
	return 0;
}

void *RtspServerProcess(void *arg)
{
	prctl(PR_SET_NAME, __FUNCTION__);
	RtspServer *rs = RtspServer::GetInstance();
	rs->StartRtspServer();
    return 0;
}

int StartRtspServer()
{
	pthread_t pid;
	if(pthread_create(&pid,NULL,RtspServerProcess,NULL) < 0)
	{
		RTSPERR("pthread create failed!\n");
		return -1;
	}
	return 0;
}

int StopRtspServer()
{
	RtspServer *rs = RtspServer::GetInstance();
	rs->StopRtspServer();
	return 0;
}
