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

//ϵͳ����ʱ��ṹ�嶨��
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

//֡����
typedef enum
{
	I_FRAME = 1,
	P_FRAME,
	A_FRAME,
	B_FRAME
}FrameType_E;

//�ֱ���
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

//һ����Ƶ֡����Ƶ֡�ڻ���ص���Ϣ�ṹ��
typedef struct 
{
	unsigned long			FrmStartPos;/*��֡��buffer�е�ƫ��*/
	unsigned long			FrmLength;  /*��֡����Ч���ݳ���*/
	long long				Pts;			/*�������Ƶ֡����Ϊ��֡��ʱ���*/
	unsigned char			Flag;		/* 1 I ֡, 2 P ֡, 3 ��Ƶ֡*/
	unsigned char			hour;		/*������֡��ʱ��*/
	unsigned char			min;
	unsigned char			sec;
	bool                    talk;
}FrameInfo;

typedef struct 
{
	unsigned int	m_nVHeaderFlag; // ֡��ʶ��00dc, 01dc, 01wb
	unsigned int 	m_nVFrameLen;  // ֡�ĳ���
	unsigned char	m_u8Hour;
	unsigned char	m_u8Minute;
	unsigned char	m_u8Sec;
	unsigned char	m_u8Pad0;// ��������Ϣ�����ͣ�����������;�������Ϣ�ṹ0 ����û��1.2.3 ����������Ϣ
	unsigned int	m_nILastOffset;// ��֡�����һ��I FRAME ��ƫ��ֻ��Iframe ����
	long long		m_lVPts;		// ʱ���
	unsigned int	m_bMdAlarm:1;/*bit0 �ƶ���ⱨ��1:������0:û�б���*/
	unsigned int	m_FrameType:4;/*֡����*/
	unsigned int 	m_Lost:1;
	unsigned int 	m_FrameRate:5;
	unsigned int    m_bPirAlarm:1; /*bit11*/   
	unsigned int    m_bMicroWaveAlarm:1; /*bit12*/ 
	unsigned int    m_b915Alarm:1; /*bit13*/ 
	unsigned int    m_bEncrypt:1; /*bit14*/  
	unsigned int    m_bPirAlarm2:1; /*bit15 �ڶ���pir�澯*/   
	unsigned int    m_bPirAlarm3:1; /*bit16 ������pir�澯*/ 
	unsigned int    m_b3CloudRecord:3; /*bit17~bit19 �ƴ洢���0:δ�����ƴ洢1:�����ƴ洢2~7δ����*/ 
	unsigned int    m_Res:12; /*bit20-bit31 ��ʱ����*/   
	unsigned int    m_nReserved;
}VideoFrameHeader;


typedef struct 
{
	unsigned int		m_nAHeaderFlag; // ֡��ʶ��00dc, 01dc, 01wb
	unsigned int 		m_nAFrameLen;  // ֡�ĳ���
	long long			m_lAPts;		// ʱ���
}AudioFrameHeader;

//֡����صĽṹ����
typedef struct 
{
	unsigned char  			*bufferstart;				/*ý������buf ��ʼ��ַ*/
	unsigned long       	buffersize;				/*buf �ռ��С*/
	unsigned long	   		writepos;				/*дָ��ƫ��*/
	unsigned long			readpos;				/*��ָ���ƫ��*/
	FrameInfo				FrmList[MAX_FRM_NUM];	/*buf �д洢��֡�б���Ϣ*/
	unsigned short		 	CurFrmIndex;			/*֡�б��е��±�*/			
	unsigned short		 	TotalFrm;				/*buffer �е���֡��*/
	unsigned short 			IFrmList[MAX_I_F_NUM];	/*���n ��i ֡��FrmList�е������±���Ϣ*/
	unsigned short			IFrmIndex;				/*��ǰI ֡�б�����*/
	unsigned short			TotalIFrm;				/*�ܵ�I ֡��Ŀ*/
	unsigned short			ICurIndex;				//��ǰI֡����
	unsigned long			circlenum;				/*buf���ǵ�Ȧ��*/
	unsigned long			m_u32MaxWpos;			/*���дָ��λ��*/
}FrameBufferPool;

//һ��FrameBufferPool �û��ṹ�嶨��
typedef struct
{
	unsigned short		ReadFrmIndex;			/*���û���֡����ط������õ�֡����*/
	unsigned short		reserve;
	unsigned long		ReadCircleNum;			/*���û���֡����صķ���Ȧ������ʼʱ����֡������е�circlenum*/
	unsigned int		diffpos;				/*��ָ���дָ��λ�ò�ֵ����λΪ֡*/
	unsigned int 		throwframcount;			/*�ӿ�ʼ������֡�ĸ���*/
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
