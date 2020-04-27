#include <stdio.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include "buffermanage.h"        
#include "modinterfacedef.h"
#include "aes.h"
#include "zmdconfig.h"

int GetSysTime(SystemDateTime *pSysTime)
{
	struct tm *t ;  
	time_t timer = 0;

	timer = time(0);
	t = localtime(&timer);

	/*需要注意的是年份加上1900，月份加上1才是实际的时间*/
	/*把时间的年换算成以2000年为基准的时间*/
	if( t->tm_year > 99) // 1900 为基准
		pSysTime->year = t->tm_year -100;
	else 
		pSysTime->year = 9;


	pSysTime->month = t->tm_mon + 1;
	pSysTime->mday = t->tm_mday;
	pSysTime->hour = t->tm_hour;
	pSysTime->minute = t->tm_min;
	pSysTime->second = t->tm_sec;
	pSysTime->week= t->tm_wday;

	return 0;
}

#ifdef ZMD_APP_ENCODE_BUFFERMANAGE_IFRAME_ENCRYPTION
int AesEncrypt(unsigned char *input, unsigned char *output)                                                                                                                               
{ 
	int ret = 0; 
	mbedtls_aes_context ctx; 
	unsigned char iv_str[100]={0};
	mbedtls_aes_init(&ctx);
	mbedtls_aes_setkey_enc(&ctx, (unsigned char*)AES_KEY, 256);
	ret = mbedtls_aes_crypt_cbc(&ctx, MBEDTLS_AES_ENCRYPT, 256, iv_str, input, output);
	if(ret != 0)
	{
		BUFERR("enc error ret %d\n",ret); 
	}
	mbedtls_aes_free( &ctx );
	return ret;
}
#endif
		
BufferManage::BufferManage()
{
	memset(&m_FrameBufferPool, 0, sizeof(FrameBufferPool));
	InitBufferLock();
}

BufferManage::~BufferManage()
{
	DestroyBufferLock();
}

int BufferManage::ResetUserInfo(int userid)
{ 
	if(userid >= MAX_FRAME_USER) 
	{ 
		return -1; 
	} 
	AddBufferLock(); 
	
	if(m_FrameBufferPool.ICurIndex <= m_FrameBufferPool.CurFrmIndex) 
	{ 
		m_FrameBufferUser[userid].ReadCircleNum = m_FrameBufferPool.circlenum; 
	} 
	else 
	{ 
		m_FrameBufferUser[userid].ReadCircleNum = m_FrameBufferPool.circlenum-1; 
	} 
	m_FrameBufferUser[userid].ReadFrmIndex = m_FrameBufferPool.ICurIndex; 
	m_FrameBufferUser[userid].reserve = 0; 
	m_FrameBufferUser[userid].diffpos = 0; 
	m_FrameBufferUser[userid].throwframcount = 0; 
	ReleaseBufferLock(); 
	return 0; 
}

int BufferManage::CreateBufferPool(int resolution, unsigned long bufsize)
{
	if(bufsize == 0)
	{     
		if(RES_4K == resolution)
		{
			bufsize = BUFFER_SIZE_4K;
		}
		else if(RES_1080P == resolution)
		{
			bufsize = BUFFER_SIZE_1080P;
		}
		else if(RES_720P == resolution)
		{
			bufsize = BUFFER_SIZE_720P;
		}
		else if(RES_VGA == resolution)
		{
			bufsize = BUFFER_SIZE_720P/4;
		}
		else if(RES_QVGA == resolution)
		{
			bufsize = BUFFER_SIZE_720P/8;			
		}
	}

	m_FrameBufferPool.bufferstart = new unsigned char[bufsize];
	if(NULL == m_FrameBufferPool.bufferstart)
	{
		BUFERR("CreateBufferPool: new failed!\n");
		return -1;
	}

	m_FrameBufferPool.buffersize = bufsize;
	memset(m_FrameBufferPool.bufferstart, 0, bufsize);	
	m_FrameBufferPool.circlenum = 0;
	m_FrameBufferPool.CurFrmIndex = 0;
	m_FrameBufferPool.TotalFrm = 0;
	m_FrameBufferPool.writepos = 0;
	m_FrameBufferPool.IFrmIndex = 0;     
	m_FrameBufferPool.TotalIFrm = 0;
	memset(m_FrameBufferPool.FrmList, 0, sizeof(FrameInfo)*MAX_FRM_NUM);
	memset(m_FrameBufferPool.IFrmList, 0, sizeof(unsigned short)*MAX_I_F_NUM);
	m_u32IFrameOffset = 0;
	memset(m_FrameBufferUser, 0, sizeof(FrameBufferUser)*MAX_FRAME_USER);
	return 0;
}

int BufferManage::DestroyBufferPool()
{
	if(NULL != m_FrameBufferPool.bufferstart)
	{
		delete []m_FrameBufferPool.bufferstart;
		m_FrameBufferPool.bufferstart = (unsigned char*)NULL;
		memset(&m_FrameBufferPool, 0, sizeof(FrameBufferPool));
		return 0;
	}
	return -1;
}

int BufferManage::GetAudioFrameInfo(AUDIO_STREAM_S *Astream, int *framelen, unsigned long long *pts)
{
	AUDIO_STREAM_S *stream = Astream;
	*pts = stream->u64TimeStamp;
 	*framelen = stream->u32Len ;	
	return 0;
}

int BufferManage::GetVideoFrameInfo(VENC_STREAM_S *Vstream, int *frametype, int *framelen, unsigned long long *pts)
{
	VENC_STREAM_S *stream =Vstream;
	unsigned int i =0;
	if(stream)
	{	
		if(stream->pstPack[0].DataType.enH264EType == H264E_NALU_PSLICE)
		{
			*frametype = P_FRAME;
		}
		else
		{
			*frametype = I_FRAME;		
		}

		*pts = stream->pstPack[0].u64PTS;

		*framelen = 0;	
		for (i = 0; i < stream->u32PackCount; i++)
		{
			*framelen+=(stream->pstPack[i].u32Len-stream->pstPack[i].u32Offset);
		}		
		return 0;
	}
	return -1;
}

int BufferManage::PutOneVFrameToBuffer(VENC_STREAM_BUF_INFO_S *Vbuffinfo,VENC_STREAM_S *Vstream)
{
	VENC_STREAM_S *stream = Vstream;
	VENC_STREAM_BUF_INFO_S *buffinfo = Vbuffinfo;
	HI_U32 u64SrcPhyAddr;
	HI_U32 u32Left;
	HI_U32 i,j;
	int  frametype = 0;
	int FrameLen = 0;
	unsigned long long pts = 0;
	unsigned int IFrameheader = 0x63643030; 
	unsigned int PFrameheader = 0x63643130;
	unsigned int TimeInfo = 0;	
	SystemDateTime systime;
	AddBufferLock();

	//解析此帧的信息
	if(GetVideoFrameInfo(Vstream, &frametype, &FrameLen, &pts) < 0)
	{
		BUFERR("GetVideoFrameInfo failed!\n");
		ReleaseBufferLock();
		return -1;
	}

	//判断剩余空间是否足够放下一帧数据
	if((m_FrameBufferPool.buffersize - m_FrameBufferPool.writepos) < ((unsigned long)FrameLen + sizeof(VideoFrameHeader)))
	{
		memset(m_FrameBufferPool.bufferstart+m_FrameBufferPool.writepos,0x0,m_FrameBufferPool.buffersize - m_FrameBufferPool.writepos);
		m_FrameBufferPool.TotalFrm = m_FrameBufferPool.CurFrmIndex;
		m_FrameBufferPool.CurFrmIndex = 0;
		m_FrameBufferPool.circlenum += 1;
		m_FrameBufferPool.writepos = 0;
	}

	//填写一帧索引信息
	m_FrameBufferPool.FrmList[m_FrameBufferPool.CurFrmIndex].FrmStartPos = m_FrameBufferPool.writepos;
	m_FrameBufferPool.FrmList[m_FrameBufferPool.CurFrmIndex].FrmLength = FrameLen + sizeof(VideoFrameHeader);
	m_FrameBufferPool.FrmList[m_FrameBufferPool.CurFrmIndex].Pts = pts;
	m_FrameBufferPool.FrmList[m_FrameBufferPool.CurFrmIndex].Flag = frametype;
	GetSysTime(&systime);
	m_FrameBufferPool.FrmList[m_FrameBufferPool.CurFrmIndex].hour = systime.hour;
	m_FrameBufferPool.FrmList[m_FrameBufferPool.CurFrmIndex].min = systime.minute;
	m_FrameBufferPool.FrmList[m_FrameBufferPool.CurFrmIndex].sec = systime.second;

	/******************************start write frame head********************************************/
	//帧标识
	if(I_FRAME == frametype)
	{
		*(unsigned int*)(m_FrameBufferPool.bufferstart + m_FrameBufferPool.writepos) = IFrameheader;
	}
	else 
	{
		*(unsigned int*)(m_FrameBufferPool.bufferstart + m_FrameBufferPool.writepos) = PFrameheader;
	}
	m_FrameBufferPool.writepos += 4;

	// 帧长度
	*(unsigned int*)(m_FrameBufferPool.bufferstart + m_FrameBufferPool.writepos) = FrameLen;
	m_FrameBufferPool.writepos += 4;

	// 时间信息
	TimeInfo = systime.hour|(systime.minute<<8)|(systime.second<<16);
	*(unsigned int*)(m_FrameBufferPool.bufferstart + m_FrameBufferPool.writepos) = TimeInfo;
	m_FrameBufferPool.writepos += 4;

	// 上一个I帧的偏移
	*(unsigned int*)(m_FrameBufferPool.bufferstart + m_FrameBufferPool.writepos) = m_u32IFrameOffset;
	m_FrameBufferPool.writepos += 4;

	// 时间戳
	*(long long *)(m_FrameBufferPool.bufferstart + m_FrameBufferPool.writepos) = pts;
	m_FrameBufferPool.writepos += 8;

	//报警信息
	unsigned int otherinfo =0;
	if(I_FRAME == frametype)
	{
		otherinfo|=(1<<14);//加密
	}
	otherinfo |= (1 << 20);//支持年月日信息
	*(unsigned int*)(m_FrameBufferPool.bufferstart + m_FrameBufferPool.writepos) = otherinfo;
	m_FrameBufferPool.writepos += 4;

	// 保留信息
	unsigned int 	m_nReserved = 0;
	m_nReserved |= (systime.year);
	m_nReserved |= (systime.month-1) << 7;
	m_nReserved |= (systime.mday - 1) << 11;
	*(unsigned int*)(m_FrameBufferPool.bufferstart + m_FrameBufferPool.writepos) = m_nReserved;
	m_FrameBufferPool.writepos += 4;	
	/******************************write frame head end********************************************/

	if(I_FRAME == frametype)
	{
		m_u32IFrameOffset = 0;
	}

	m_u32IFrameOffset +=((m_FrameBufferPool.FrmList[m_FrameBufferPool.CurFrmIndex].FrmLength +7)&(~7));


	//拷贝码流数据
	if(stream && stream->pstPack)
	{
		for(i = 0; i < stream->u32PackCount; i++)
		{
			if(buffinfo == NULL)//LINUX
			{
				memcpy(m_FrameBufferPool.bufferstart + m_FrameBufferPool.writepos, 
						(void*)(stream->pstPack[i].pu8Addr + stream->pstPack[i].u32Offset),
						stream->pstPack[i].u32Len - stream->pstPack[i].u32Offset);
			}
			else//LITEOS
			{
				for(j=0; j<MAX_TILE_NUM; j++)
				{
					if((stream->pstPack[i].u64PhyAddr > buffinfo->u64PhyAddr[j]) &&
							(stream->pstPack[i].u64PhyAddr <= buffinfo->u64PhyAddr[j] + buffinfo->u64BufSize[j]))
						break;
				}

				if(stream->pstPack[i].u64PhyAddr + stream->pstPack[i].u32Len >=
						buffinfo->u64PhyAddr[j] + buffinfo->u64BufSize[j])
				{
					if(stream->pstPack[i].u64PhyAddr + stream->pstPack[i].u32Offset >=
							buffinfo->u64PhyAddr[j] + buffinfo->u64BufSize[j])
					{
						/* physical address retrace in offset segment */
						u64SrcPhyAddr = buffinfo->u64PhyAddr[j] +
							((stream->pstPack[i].u64PhyAddr + stream->pstPack[i].u32Offset) - 
							 (buffinfo->u64PhyAddr[j] + buffinfo->u64BufSize[j]));

						memcpy(m_FrameBufferPool.bufferstart + m_FrameBufferPool.writepos, 
								(void*)(HI_UL)u64SrcPhyAddr,
								stream->pstPack[i].u32Len - stream->pstPack[i].u32Offset);
					}
					else
					{
						/* physical address retrace in data segment */
						u32Left = (buffinfo->u64PhyAddr[j] + buffinfo->u64BufSize[j]) - stream->pstPack[i].u64PhyAddr;

						memcpy(m_FrameBufferPool.bufferstart + m_FrameBufferPool.writepos, 
								(void*)(HI_UL)(stream->pstPack[i].u64PhyAddr + stream->pstPack[i].u32Offset),
								u32Left - stream->pstPack[i].u32Offset);
						memcpy(m_FrameBufferPool.bufferstart + m_FrameBufferPool.writepos + u32Left - stream->pstPack[i].u32Offset, 
								(void*)(HI_UL)buffinfo->u64PhyAddr[j],
								stream->pstPack[i].u32Len - u32Left);
					}
				}
				else
				{
					/* physical address retrace does not happen */
					memcpy(m_FrameBufferPool.bufferstart + m_FrameBufferPool.writepos, 
							(void*)(HI_UL)(stream->pstPack[i].u64PhyAddr + stream->pstPack[i].u32Offset),
							stream->pstPack[i].u32Len - stream->pstPack[i].u32Offset);
				}
			}
			m_FrameBufferPool.writepos+=(stream->pstPack[i].u32Len - stream->pstPack[i].u32Offset);
		}
	}

#ifdef ZMD_APP_ENCODE_BUFFERMANAGE_IFRAME_ENCRYPTION
	if(FrameLen>0 && I_FRAME == frametype)
	{
		unsigned char encinput[256]={0};
		memcpy(encinput,m_FrameBufferPool.bufferstart + m_FrameBufferPool.writepos - FrameLen,256);
		AesEncrypt(encinput,m_FrameBufferPool.bufferstart + m_FrameBufferPool.writepos - FrameLen);
	}
#endif

	m_FrameBufferPool.writepos = (m_FrameBufferPool.writepos + 7)&(~7);  // 采用数据位8字节对齐

	//如果是I 帧则还需填充i 帧列表
	if(I_FRAME == frametype)
	{
		m_FrameBufferPool.IFrmList[m_FrameBufferPool.IFrmIndex] = m_FrameBufferPool.CurFrmIndex;
		m_FrameBufferPool.ICurIndex = m_FrameBufferPool.CurFrmIndex;
		m_FrameBufferPool.IFrmIndex++;
		if(m_FrameBufferPool.TotalIFrm < m_FrameBufferPool.IFrmIndex)
		{
			m_FrameBufferPool.TotalIFrm = m_FrameBufferPool.IFrmIndex;
		}

		if(m_FrameBufferPool.IFrmIndex >= MAX_I_F_NUM)
		{
			m_FrameBufferPool.IFrmIndex = 0;
		}
	}

	//修改帧索引下标
	m_FrameBufferPool.CurFrmIndex++;
	if(m_FrameBufferPool.CurFrmIndex > m_FrameBufferPool.TotalFrm)
	{
		m_FrameBufferPool.TotalFrm = m_FrameBufferPool.CurFrmIndex;
	}

	if(m_FrameBufferPool.CurFrmIndex >= MAX_FRM_NUM)
	{
		m_FrameBufferPool.TotalFrm = m_FrameBufferPool.CurFrmIndex;
		m_FrameBufferPool.CurFrmIndex = 0;
		m_FrameBufferPool.circlenum += 1;
		m_FrameBufferPool.writepos = 0;
	}

	ReleaseBufferLock();	 

	return FrameLen;
}

int BufferManage::PutOneAFrameToBuffer(AUDIO_STREAM_S *Astream,bool talk)
{
	AUDIO_STREAM_S *stream = Astream;
	unsigned long long pts = 0;
	int framelen = 0;
	unsigned int AFrameheader = 0x62773130;
	
	SystemDateTime systime;
	AddBufferLock();

	//解析音频帧信息
	if(GetAudioFrameInfo(Astream, &framelen, &pts) < 0)
	{
		ReleaseBufferLock();
		return -1;
	}	

	//判断剩余空间是否足够放下一帧数据
	if((m_FrameBufferPool.buffersize - m_FrameBufferPool.writepos) < ((unsigned long)framelen + sizeof(AudioFrameHeader)))
	{
		memset(m_FrameBufferPool.bufferstart+m_FrameBufferPool.writepos,0x0,m_FrameBufferPool.buffersize - m_FrameBufferPool.writepos);
		m_FrameBufferPool.TotalFrm = m_FrameBufferPool.CurFrmIndex;
		m_FrameBufferPool.CurFrmIndex = 0;
		m_FrameBufferPool.circlenum += 1;
		m_FrameBufferPool.writepos = 0;
	}

	//填写一帧索引信息
	m_FrameBufferPool.FrmList[m_FrameBufferPool.CurFrmIndex].FrmStartPos = m_FrameBufferPool.writepos;
	m_FrameBufferPool.FrmList[m_FrameBufferPool.CurFrmIndex].FrmLength = framelen + sizeof(AudioFrameHeader);
	m_FrameBufferPool.FrmList[m_FrameBufferPool.CurFrmIndex].Pts = pts;
	m_FrameBufferPool.FrmList[m_FrameBufferPool.CurFrmIndex].Flag = A_FRAME;
    m_FrameBufferPool.FrmList[m_FrameBufferPool.CurFrmIndex].talk = talk;

	GetSysTime(&systime);
	m_FrameBufferPool.FrmList[m_FrameBufferPool.CurFrmIndex].hour = systime.hour;
	m_FrameBufferPool.FrmList[m_FrameBufferPool.CurFrmIndex].min = systime.minute;
	m_FrameBufferPool.FrmList[m_FrameBufferPool.CurFrmIndex].sec = systime.second;
	
	// 帧标识
	*(unsigned int*)(m_FrameBufferPool.bufferstart + m_FrameBufferPool.writepos) = AFrameheader;
	m_FrameBufferPool.writepos += 4;

	// 帧长度
	*(unsigned int*)(m_FrameBufferPool.bufferstart + m_FrameBufferPool.writepos) = framelen;
	m_FrameBufferPool.writepos += 4;

	// 时间戳
	*(long long *) (m_FrameBufferPool.bufferstart + m_FrameBufferPool.writepos) = pts;
	m_FrameBufferPool.writepos += 8;
	
	m_u32IFrameOffset +=((m_FrameBufferPool.FrmList[m_FrameBufferPool.CurFrmIndex].FrmLength +7)&(~7));

	//拷贝码流数据到缓存
	memcpy(m_FrameBufferPool.bufferstart + m_FrameBufferPool.writepos, stream->pStream, framelen);
	m_FrameBufferPool.writepos += framelen;
	m_FrameBufferPool.writepos = (m_FrameBufferPool.writepos + 7)&(~7);  // 采用数据位8字节对齐

	//修改帧索引下标
	m_FrameBufferPool.CurFrmIndex++;
	if(m_FrameBufferPool.CurFrmIndex > m_FrameBufferPool.TotalFrm)
	{
		m_FrameBufferPool.TotalFrm = m_FrameBufferPool.CurFrmIndex;
	}

	if(m_FrameBufferPool.CurFrmIndex >= MAX_FRM_NUM)
	{
		m_FrameBufferPool.TotalFrm = m_FrameBufferPool.CurFrmIndex;
		m_FrameBufferPool.CurFrmIndex = 0;
		m_FrameBufferPool.circlenum += 1;
		m_FrameBufferPool.writepos = 0;
	}
	ReleaseBufferLock();
	return 0;
}

int BufferManage::GetOneFrameFromBuffer(int userid, unsigned char **buffer, FrameInfo *pFrameInfo)
{
	if((userid > MAX_FRAME_USER)||(NULL == buffer)||(NULL == pFrameInfo))
	{
		BUFERR("invalid user id = %d\n", userid);
		return -1;
	}

	AddBufferLock();
	
	/*
	大部分情况下，读进程比写进程快，所以读圈数和写圈数一样，读指针和写指针一样
	*/
	if(m_FrameBufferUser[userid].ReadFrmIndex == m_FrameBufferPool.CurFrmIndex
		&&(m_FrameBufferUser[userid].ReadCircleNum == m_FrameBufferPool.circlenum))
	{
		ReleaseBufferLock();
		return -2;
	}

	/*
	典型的读换圈:写的圈数，比读的圈数大；读指针达到缓存的末尾。
	*/
	if((m_FrameBufferUser[userid].ReadFrmIndex >= m_FrameBufferPool.TotalFrm) &&
		(m_FrameBufferUser[userid].ReadCircleNum < m_FrameBufferPool.circlenum))
	{		
		m_FrameBufferUser[userid].ReadFrmIndex = 0;
		m_FrameBufferUser[userid].ReadCircleNum = m_FrameBufferPool.circlenum;
		if(m_FrameBufferUser[userid].ReadFrmIndex == m_FrameBufferPool.CurFrmIndex)
		{
			//*表示无数据可读
			ReleaseBufferLock();
			return -3;
		}		 
	}

	/*
	典型的数据覆盖:读比写小一圈,读位置大于写位置小
	*/
	if((m_FrameBufferPool.circlenum - m_FrameBufferUser[userid].ReadCircleNum == 1)
	  && m_FrameBufferPool.FrmList[m_FrameBufferUser[userid].ReadFrmIndex].FrmStartPos < m_FrameBufferPool.writepos)
	{
		//*表示读的太慢
		BUFLOG("--------err: date recover,ReadFrmIndex = %d,TotalFrm=%d,ReadCircleNum=%lu,circlenum=%lu\n", 
			m_FrameBufferUser[userid].ReadFrmIndex,m_FrameBufferPool.TotalFrm,
			m_FrameBufferUser[userid].ReadCircleNum,m_FrameBufferPool.circlenum);
		m_FrameBufferUser[userid].ReadFrmIndex = m_FrameBufferPool.ICurIndex;
		m_FrameBufferUser[userid].ReadCircleNum = m_FrameBufferPool.circlenum;
		ReleaseBufferLock();
		return -4;
	}
	
	/*
	相对比较极端的情况:读非常慢,写的圈数,比读的圈数大于2。
	*/
	if(m_FrameBufferPool.circlenum - m_FrameBufferUser[userid].ReadCircleNum >= 2)
	{
		//*表示读的太慢
		m_FrameBufferUser[userid].ReadFrmIndex = m_FrameBufferPool.ICurIndex;
		m_FrameBufferUser[userid].ReadCircleNum = m_FrameBufferPool.circlenum;
		ReleaseBufferLock();
		return -5;
	}
	
	*buffer = m_FrameBufferPool.bufferstart + m_FrameBufferPool.FrmList[m_FrameBufferUser[userid].ReadFrmIndex].FrmStartPos;
	memcpy(pFrameInfo, &(m_FrameBufferPool.FrmList[m_FrameBufferUser[userid].ReadFrmIndex]), sizeof(FrameInfo));
	
	m_FrameBufferUser[userid].ReadFrmIndex++;
	if(m_FrameBufferUser[userid].ReadFrmIndex < m_FrameBufferPool.CurFrmIndex &&\
		m_FrameBufferUser[userid].ReadCircleNum == m_FrameBufferPool.circlenum)
		
	{
		m_FrameBufferUser[userid].diffpos = m_FrameBufferPool.CurFrmIndex -m_FrameBufferUser[userid].ReadFrmIndex;
	}
	else if(m_FrameBufferUser[userid].ReadFrmIndex >= m_FrameBufferPool.CurFrmIndex &&\
		m_FrameBufferUser[userid].ReadCircleNum < m_FrameBufferPool.circlenum)
		
	{
		m_FrameBufferUser[userid].diffpos = m_FrameBufferPool.TotalFrm+m_FrameBufferPool.CurFrmIndex -m_FrameBufferUser[userid].ReadFrmIndex;
	}

	ReleaseBufferLock();
	return 0;
}

BufferManageCtrl* BufferManageCtrl::m_instance = new BufferManageCtrl;

BufferManageCtrl* BufferManageCtrl::GetInstance(void)
{
	return m_instance;
}

BufferManageCtrl::BufferManageCtrl()
{
}

BufferManageCtrl::~BufferManageCtrl()
{
}

int BufferManageCtrl::BufferManageInit(void)
{
	int i;
	Resolution_E resize[3] = {RES_4K, RES_1080P, RES_720P};

	for(i = 0;i < ZMD_APP_ENCODE_VIDEO_MAX_CH_SRTEAM;i++)
	{
		m_pBufferManage[i] = new BufferManage;
		if(m_pBufferManage[i] == NULL)
		{
			return -1;
		}
		if(m_pBufferManage[i]->CreateBufferPool(resize[i]) == -1)
		{
			return -1;
		}
	}
	return 0;
}

int BufferManageCtrl::BufferManageRelease(void)
{
	int i;
	for(i = 0;i < ZMD_APP_ENCODE_VIDEO_MAX_CH_SRTEAM;i++)
	{
		if(0 == m_pBufferManage[i]->DestroyBufferPool())
		{
			BUFLOG("DestroyBufferPool VeChn:%d success \n",i);
		}   
		delete m_pBufferManage[i];
		m_pBufferManage[i] = (BufferManage*)NULL;
	}
	return 0;
}

BufferManage* BufferManageCtrl::GetBMInstance(int ch)
{
	return m_pBufferManage[ch];
}

int BufferManageCtrl::GetFrame(void* para)
{
	int ret;
	ParaEncUserInfo* uinf = (ParaEncUserInfo*)para;
	if(uinf != NULL)
	{
		if(uinf->ch < 0 || uinf->ch >= ZMD_APP_ENCODE_VIDEO_MAX_CH_SRTEAM)
		{
			BUFERR("ch:%d,ch error!\n",uinf->ch);
			return -1;
		}
		if(m_pBufferManage[uinf->ch] != NULL)
		{
			ret = m_pBufferManage[uinf->ch]->GetOneFrameFromBuffer(uinf->userid,uinf->buffer,uinf->frameinfo);
			return ret;
		}
	}
	return -1;
}

int BufferManageCtrl::ResetUser(void* para)
{
	int ret;
	ParaEncUserInfo* uinf = (ParaEncUserInfo*)para;
	if(uinf != NULL)
	{
		if(uinf->ch < 0 || uinf->ch >= ZMD_APP_ENCODE_VIDEO_MAX_CH_SRTEAM)
		{
			BUFERR("ch error!\n");
			return -1;
		}
		ZMDVideo::GetInstance()->RequestIFrame(uinf->ch);
		usleep(200 * 1000);
		if(m_pBufferManage[uinf->ch] != NULL)
		{
			ret = m_pBufferManage[uinf->ch]->ResetUserInfo(uinf->userid);
			return ret;
		}
	}
	return -1;
}

int BufferManageCtrl::GetAesKey(void* para)
{
	if(para == NULL)
	{
		BUFERR("input error!\n");
		return -1;
	}
	memcpy(para,(unsigned char*)AES_KEY,36);
	return 0;
}
