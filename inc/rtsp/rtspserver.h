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

#ifdef ZMD_APP_DEBUG_RTSP
#define RTSPLOG(format, ...)     fprintf(stdout, "[RTSPLOG][Func:%s][Line:%d], " format, __FUNCTION__,  __LINE__, ##__VA_ARGS__)
#else
#define RTSPLOG(format, ...)
#endif
#define RTSPERR(format, ...)     fprintf(stdout, "\033[31m[RTSPERR][Func:%s][Line:%d], " format"\033[0m", __FUNCTION__,  __LINE__, ##__VA_ARGS__)

#define SERVER_PORT      8554
#define SERVER_RTP_PORT  55532
#define SERVER_RTCP_PORT 55533
#define BUF_MAX_SIZE     (1024*1024)

enum  SocketType
{
	SOCKET_TCP = 0,
	SOCKET_UDP
};

class RtspClient
{
	private:
		int m_ServerRtpSockfd;
		int m_ServerRtcpSockfd;
		int m_ClientSockfd;
		char m_ClientIp[40];
		int m_ClientPort;
		int m_ClientVRtpPort;
		int m_ClientVRtcpPort;
		int m_ClientARtpPort;
		int m_ClientARtcpPort;
		int m_free;
		int g_OnPlay;
		int g_OnClient;
		ParaEncUserInfo m_uinf;
		PAYLOAD_TYPE_E m_venctype;

		int handleCmd_OPTIONS(char* result, int cseq);
		int handleCmd_DESCRIBE(char* result, int cseq, char* url);
		int handleCmd_SETUP(char* result, int cseq, char* url);
		int handleCmd_PLAY(char* result, int cseq);
		int handleCmd_TEARDOWN(char* result, int cseq);
		char* getLineFromBuf(char* buf, char* line);
		int getFrameFromBuffer(char* frame,int* frametype,ParaEncUserInfo* uinf);
		inline int startCode3(char* buf);
		inline int startCode4(char* buf);
		char* findNextStartCode(char* buf, int len);
		int rtpSendH264Frame(struct RtpPacket* rtpPacket, uint8_t* frame, uint32_t frameSize);
		int rtpSendH265Frame(struct RtpPacket* rtpPacket, uint8_t* frame, uint32_t frameSize);
		int rtpSendg711uFrame(struct RtpPacket* rtpPacket, uint8_t* frame, uint32_t frameSize);

	public:

		RtspClient();
		~RtspClient();
		int GetFreeFlag();
		void SetFreeFlag();
		void ClearFreeFlag();
		int InitClientPara(int srtpsockfd,int srtcpsockfd, int csockfd,char *ip,int port);
		void CloseClient();
		void DoClient();
		void DoPlay();
};

class RtspServer
{
	private:
		static RtspServer* m_instance;
		int m_ServerTcpSockfd;
		int m_ServerRtpSockfd;
		int m_ServerRtcpSockfd;
		int g_OnServer;
		RtspClient m_rtsp_client[10];

		RtspServer();
		~RtspServer();
		int CreateSocket(enum SocketType stype);
		int BindSocketAddr(int sockfd, const char* ip, int port);
		int AcceptClient(int sockfd, char* ip, int* port);
		RtspClient * GetFreeClient();

	public:
		static RtspServer* GetInstance(void);
		int StartRtspServer();
		int StopRtspServer();
};

int StartRtspServer();
int StopRtspServer();
#endif
