#ifndef __RTSP_SERVER_H_
#define __RTSP_SERVER_H_
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/prctl.h>
#include <fcntl.h>
#include <assert.h>
#include "buffermanage.h"
#include "modinterface.h"

#include "rtp.h"

#define SERVER_PORT      8554
#define SERVER_RTP_PORT  55532
#define SERVER_RTCP_PORT 55533
#define BUF_MAX_SIZE     (1024*1024)

class RTSPSERVER
{
	private:
		int createTcpSocket();
		int createUdpSocket();
		int bindSocketAddr(int sockfd, const char* ip, int port);
		int acceptClient(int sockfd, char* ip, int* port);
		char* getLineFromBuf(char* buf, char* line);
		int handleCmd_OPTIONS(char* result, int cseq);
		int handleCmd_DESCRIBE(char* result, int cseq, char* url);
		int handleCmd_SETUP(char* result, int cseq, char* url);
		int handleCmd_PLAY(char* result, int cseq);
		int handleCmd_TEARDOWN(char* result, int cseq);
		void doClient();


	public:
		int m_ServerTcpSockfd;
		int m_ServerRtpSockfd;
		int m_ServerRtcpSockfd;
		int m_ClientSockfd;
		char m_ClientIp[40];
		int m_ClientPort;
		int m_ClientVRtpPort;
		int m_ClientVRtcpPort;
		int m_ClientARtpPort;
		int m_ClientARtcpPort;
		int g_OnPlay;

		int StartRtspServer();
		int getFrameFromBuffer(char* frame,int* frametype,ParaEncUserInfo* uinf);
		inline int startCode3(char* buf);
		inline int startCode4(char* buf);
		char* findNextStartCode(char* buf, int len);
		int rtpSendH264Frame(struct RtpPacket* rtpPacket, uint8_t* frame, uint32_t frameSize);
		int rtpSendH265Frame(struct RtpPacket* rtpPacket, uint8_t* frame, uint32_t frameSize);
		int rtpSendg711uFrame(struct RtpPacket* rtpPacket, uint8_t* frame, uint32_t frameSize);
};

int InitRtspServer();
#endif
