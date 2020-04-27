#ifndef __BUFFER_MANAGE_H__
#define __BUFFER_MANAGE_H__

#include <stdio.h>
#include <pthread.h>
#include "video.h"
#include "sample_comm.h"
#include "zmdconfig.h"

#ifdef ZMD_APP_DEBUG_BUF
#define BUFLOG(format, ...)     fprintf(stdout, "[BUFLOG][Func:%s][Line:%d], " format, __FUNCTION__,  __LINE__, ##__VA_ARGS__)
#else
#define BUFLOG(format, ...)
#endif
#define BUFERR(format, ...)     fprintf(stdout, "\033[31m[BUFERR][Func:%s][Line:%d], " format"\033[0m", __FUNCTION__,  __LINE__, ##__VA_ARGS__)

#define MAX_FRAME_USER  	(16)		
#define MAX_FRM_NUM			(30*150)
#define MAX_I_F_NUM			(30)
#define BUFFER_SIZE_720P	(0x180000)
#define BUFFER_SIZE_1080P   (0x240000)
#define BUFFER_SIZE_4K  	(0x1680000)
#define AES_KEY             "4D1FAC83E8F049CA9D68360219F98EAE"

#ifndef ZMD_APP_ENCODE_VIDEO_MAX_CH_SRTEAM
#define ZMD_APP_ENCODE_VIDEO_MAX_CH_SRTEAM (3)
#endif

//系统日期时间结构体定义
typedef struct
{
	char         year;
	char         month;
	char         mday;
	char         hour;
	char         minute;
	char         second;
	char         week;      
	char         reserve;
}SystemDateTime;

//帧类型
typedef enum
{
	I_FRAME = 1,
	P_FRAME,
	A_FRAME,
	B_FRAME
}FrameType_E;

//分辨率
typedef enum{
	RES_D1 = 0,
	RES_HD1,
	RES_CIF,
	RES_QCIF,
	RES_4K,
	RES_1080P,
	RES_720P,
	RES_VGA,
	RES_QVGA,
}Resolution_E;

//一个视频帧或音频帧在缓冲池的信息结构体
typedef struct 
{
	unsigned long			FrmStartPos;/*此帧在buffer中的偏移*/
	unsigned long			FrmLength;  /*此帧的有效数据长度*/
	long long				Pts;			/*如果是视频帧，则为此帧的时间戳*/
	unsigned char			Flag;		/* 1 I 帧, 2 P 帧, 3 音频帧*/
	unsigned char			hour;		/*产生此帧的时间*/
	unsigned char			min;
	unsigned char			sec;
	bool                    talk;
}FrameInfo;

typedef struct 
{
	unsigned int	m_nVHeaderFlag; // 帧标识，00dc, 01dc, 01wb
	unsigned int 	m_nVFrameLen;  // 帧的长度
	unsigned char	m_u8Hour;
	unsigned char	m_u8Minute;
	unsigned char	m_u8Sec;
	unsigned char	m_u8Pad0;// 代表附加消息的类型，根据这个类型决定其信息结构0 代表没有1.2.3 各代表其信息
	unsigned int	m_nILastOffset;// 此帧相对上一个I FRAME 的偏移只对Iframe 有用
	long long		m_lVPts;		// 时间戳
	unsigned int	m_bMdAlarm:1;/*bit0 移动侦测报警1:报警，0:没有报警*/
	unsigned int	m_FrameType:4;/*帧类型*/
	unsigned int 	m_Lost:1;
	unsigned int 	m_FrameRate:5;
	unsigned int    m_bPirAlarm:1; /*bit11*/   
	unsigned int    m_bMicroWaveAlarm:1; /*bit12*/ 
	unsigned int    m_b915Alarm:1; /*bit13*/ 
	unsigned int    m_bEncrypt:1; /*bit14*/  
	unsigned int    m_bPirAlarm2:1; /*bit15 第二个pir告警*/   
	unsigned int    m_bPirAlarm3:1; /*bit16 第三个pir告警*/ 
	unsigned int    m_b3CloudRecord:3; /*bit17~bit19 云存储标记0:未触发云存储1:触发云存储2~7未定义*/ 
	unsigned int    m_Res:12; /*bit20-bit31 暂时保留*/   
	unsigned int    m_nReserved;
}VideoFrameHeader;


typedef struct 
{
	unsigned int		m_nAHeaderFlag; // 帧标识，00dc, 01dc, 01wb
	unsigned int 		m_nAFrameLen;  // 帧的长度
	long long			m_lAPts;		// 时间戳
}AudioFrameHeader;

//帧缓冲池的结构定义
typedef struct 
{
	unsigned char  			*bufferstart;				/*媒体数据buf 起始地址*/
	unsigned long       	buffersize;				/*buf 空间大小*/
	unsigned long	   		writepos;				/*写指针偏移*/
	unsigned long			readpos;				/*读指针的偏移*/
	FrameInfo				FrmList[MAX_FRM_NUM];	/*buf 中存储的帧列表信息*/
	unsigned short		 	CurFrmIndex;			/*帧列表中的下标*/			
	unsigned short		 	TotalFrm;				/*buffer 中的总帧数*/
	unsigned short 			IFrmList[MAX_I_F_NUM];	/*最近n 个i 帧在FrmList中的数组下标信息*/
	unsigned short			IFrmIndex;				/*当前I 帧列表索引*/
	unsigned short			TotalIFrm;				/*总的I 帧数目*/
	unsigned short			ICurIndex;				//当前I帧序列
	unsigned long			circlenum;				/*buf覆盖的圈数*/
	unsigned long			m_u32MaxWpos;			/*最大写指针位置*/
}FrameBufferPool;

//一个FrameBufferPool 用户结构体定义
typedef struct
{
	unsigned short		ReadFrmIndex;			/*此用户对帧缓冲池访问所用的帧索引*/
	unsigned short		reserve;
	unsigned long		ReadCircleNum;			/*此用户对帧缓冲池的访问圈数，初始时等于帧缓冲池中的circlenum*/
	unsigned int		diffpos;				/*读指针和写指针位置差值，单位为帧*/
	unsigned int 		throwframcount;			/*从开始计数丢帧的个数*/
}FrameBufferUser;

//Encode para
typedef struct 
{
	int				ch;
	int				userid;
	unsigned char   **buffer;
	FrameInfo		*frameinfo;
}ParaEncUserInfo;

class BufferManage
{
	private:
		FrameBufferPool		m_FrameBufferPool;
		FrameBufferUser		m_FrameBufferUser[MAX_FRAME_USER];
		unsigned int 		m_u32IFrameOffset;
		pthread_mutex_t		BufManageMutex;

	private:
		inline void InitBufferLock()
		{
			pthread_mutex_init(&BufManageMutex, 0);
		}

		inline void AddBufferLock()
		{
			pthread_mutex_lock(&BufManageMutex);
		}

		inline void ReleaseBufferLock()
		{
			pthread_mutex_unlock(&BufManageMutex);
		}

		inline void DestroyBufferLock()
		{
			pthread_mutex_destroy(&BufManageMutex);
		}
		int GetAudioFrameInfo(AUDIO_STREAM_S *Astream, int *framelen, unsigned long long *pts);
		int GetVideoFrameInfo(VENC_STREAM_S *Vstream, int *frametype, int *framelen, unsigned long long *pts);

	public:
		BufferManage();
		~BufferManage();
		int CreateBufferPool(int resolution, unsigned long bufsize = 0);
		int DestroyBufferPool();
		int PutOneVFrameToBuffer(VENC_STREAM_BUF_INFO_S *Vbuffinfo, VENC_STREAM_S *Vstream);
		int PutOneAFrameToBuffer(AUDIO_STREAM_S *Astream,bool talk);
		int ResetUserInfo(int userid);
		int GetOneFrameFromBuffer(int userid, unsigned char **buffer, FrameInfo *pFrameInfo);
};

class BufferManageCtrl
{
	private:
		static BufferManageCtrl* m_instance;
		BufferManage* m_pBufferManage[ZMD_APP_ENCODE_VIDEO_MAX_CH_SRTEAM];

	private:
		BufferManageCtrl();
		~BufferManageCtrl();

	public:
		static BufferManageCtrl* GetInstance(void);
		int BufferManageInit(void);
		int BufferManageRelease(void);
		BufferManage* GetBMInstance(int ch);
		int GetFrame(void* para);
		int ResetUser(void* para);
		int GetAesKey(void* para);
};

#endif //__BUFFER_MANAGE_H__
