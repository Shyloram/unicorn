#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <sys/select.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>

#include "audio.h"
#include "sample_comm.h"
#include "buffermanage.h"
#include "acodec.h"
#include "zmdconfig.h"

void *AudioEncoderThreadEntry(void *para)
{
	AUDLOG("threadid %d\n",(unsigned)pthread_self()); 
	ZMDAudio *paudio = (ZMDAudio *)para;
	paudio->AudioEncStreamThreadBody();
	return NULL;
} 

ZMDAudio* ZMDAudio::m_instance = new ZMDAudio;

ZMDAudio* ZMDAudio::GetInstance()
{
	return m_instance;
}

ZMDAudio::ZMDAudio()
{
}

ZMDAudio::~ZMDAudio()
{
}

int ZMDAudio::AudioInit()
{
    HI_S32 i, j, s32Ret;
    AUDIO_DEV   AiDev = SAMPLE_AUDIO_INNER_AI_DEV;
    AUDIO_DEV   AoDev = SAMPLE_AUDIO_INNER_AO_DEV;
    AI_CHN      AiChn = 0;
    AO_CHN      AoChn = 0;
    AENC_CHN    AeChn = 0;
    ADEC_CHN    AdChn = 0;
    HI_S32      s32AiChnCnt;
    HI_S32      s32AoChnCnt;
    HI_S32      s32AencChnCnt;
    AIO_ATTR_S stAioAttr;
	PAYLOAD_TYPE_E gs_enPayloadType = PT_G711A;
    HI_BOOL gs_bAioReSample = HI_FALSE;
    AUDIO_SAMPLE_RATE_E enInSampleRate  = AUDIO_SAMPLE_RATE_BUTT;
    AUDIO_SAMPLE_RATE_E enOutSampleRate = AUDIO_SAMPLE_RATE_BUTT;

    stAioAttr.enSamplerate   = AUDIO_SAMPLE_RATE_8000;
    stAioAttr.enBitwidth     = AUDIO_BIT_WIDTH_16;
    stAioAttr.enWorkmode     = AIO_MODE_I2S_MASTER;
    stAioAttr.enSoundmode    = AUDIO_SOUND_MODE_MONO;
    stAioAttr.u32EXFlag      = 0;
    stAioAttr.u32FrmNum      = 30;
    stAioAttr.u32PtNumPerFrm = NumPerFrm;
    stAioAttr.u32ChnCnt      = 1;
    stAioAttr.u32ClkSel      = 0;
    stAioAttr.enI2sType      = AIO_I2STYPE_INNERCODEC;

    /********************************************
      step 1: start Ai
    ********************************************/
    s32AiChnCnt = stAioAttr.u32ChnCnt;
    s32Ret = SAMPLE_COMM_AUDIO_StartAi(AiDev, s32AiChnCnt, &stAioAttr, enOutSampleRate, gs_bAioReSample, NULL, 0, -1);
    if (s32Ret != HI_SUCCESS)
    {
		AUDERR("s32Ret=%#x\n", s32Ret);
		return s32Ret;
    }

    /********************************************
      step 2: config audio codec
    ********************************************/
    s32Ret = SAMPLE_COMM_AUDIO_CfgAcodec(&stAioAttr);
    if (s32Ret != HI_SUCCESS)
    {
		AUDERR("s32Ret=%#x\n", s32Ret);
		return s32Ret;
    }

    /********************************************
      step 3: start Aenc
    ********************************************/
    s32AencChnCnt = stAioAttr.u32ChnCnt >> stAioAttr.enSoundmode;
    s32Ret = SAMPLE_COMM_AUDIO_StartAenc(s32AencChnCnt, &stAioAttr, gs_enPayloadType);
    if (s32Ret != HI_SUCCESS)
    {
		AUDERR("s32Ret=%#x\n", s32Ret);
		return s32Ret;
    }

    /********************************************
      step 4: Aenc bind Ai Chn
    ********************************************/
	for (i = 0; i < s32AencChnCnt; i++)
	{
		AeChn = i;
		AiChn = i;

		s32Ret = SAMPLE_COMM_AUDIO_AencBindAi(AiDev, AiChn, AeChn);
		if (s32Ret != HI_SUCCESS)
		{
			AUDERR("s32Ret=%#x\n", s32Ret);
			for (j=0; j<i; j++)
			{
				SAMPLE_COMM_AUDIO_AencUnbindAi(AiDev, j, j);
			}
			return s32Ret;
		}
		AUDLOG("Ai(%d,%d) bind to AencChn:%d ok!\n",AiDev , AiChn, AeChn);
	}

	/********************************************
	  step 5: start Adec & Ao. ( if you want )
	 ********************************************/
	s32Ret = SAMPLE_COMM_AUDIO_StartAdec(AdChn, gs_enPayloadType);
	if (s32Ret != HI_SUCCESS)
	{
		AUDERR("s32Ret=%#x\n", s32Ret);
		return s32Ret;
	}

	s32AoChnCnt = stAioAttr.u32ChnCnt;
	s32Ret = SAMPLE_COMM_AUDIO_StartAo(AoDev, s32AoChnCnt, &stAioAttr, enInSampleRate, gs_bAioReSample);
	if (s32Ret != HI_SUCCESS)
	{
		AUDERR("s32Ret=%#x\n", s32Ret);
		return s32Ret;
	}

	s32Ret = SAMPLE_COMM_AUDIO_AoBindAdec(AoDev, AoChn, AdChn);
	if (s32Ret != HI_SUCCESS)
	{
		AUDERR("s32Ret=%#x\n", s32Ret);
		return s32Ret;
	}
	AUDLOG("bind adec:%d to ao(%d,%d) ok \n", AdChn, AoDev, AoChn);

	return s32Ret;
}

int ZMDAudio::AudioRelease()
{
#if 0
	AUDIO_DEV   AiDev = SAMPLE_AUDIO_AI_DEV;
	AUDIO_DEV   AoDev = SAMPLE_AUDIO_AO_DEV;
	AI_CHN      AiChn = 0;
	AO_CHN      AoChn = 0;
	ADEC_CHN    AdChn = 0;
	AENC_CHN    AeChn = 0;
	HI_BOOL gs_bAioReSample = HI_FALSE;
	HI_S32      s32AiChnCnt = 1;
	HI_S32      s32AoChnCnt = 1;
	HI_S32      s32AencChnCnt = 1;

	AudioStop();
	AUDLOG("audio stop is ok!\n");

	//stop adec
	SAMPLE_COMM_AUDIO_AoUnbindAdec(AoDev, AoChn, AdChn);
	SAMPLE_COMM_AUDIO_StopAo(AoDev, s32AoChnCnt, gs_bAioReSample, HI_FALSE);
	SAMPLE_COMM_AUDIO_StopAdec(AdChn);

	//stop aenc
	SAMPLE_COMM_AUDIO_AencUnbindAi(AiDev, AiChn, AeChn);
	SAMPLE_COMM_AUDIO_StopAi(AiDev, s32AiChnCnt, gs_bAioReSample, HI_FALSE);
	SAMPLE_COMM_AUDIO_StopAenc(s32AencChnCnt);
#endif

	return 0;
}

int ZMDAudio::AudioEncStreamThreadBody(void)
{
    HI_S32 s32Ret,i;
    HI_S32 AencFd;
    AUDIO_STREAM_S stStream;
    fd_set read_fds;
    struct timeval TimeoutVal;
	BufferManage* pbuffer;
	AENC_CHN    AeChn = 0;
	bool        talk = false;

    FD_ZERO(&read_fds);
    AencFd = HI_MPI_AENC_GetFd(AeChn);
    FD_SET(AencFd, &read_fds);

    while (1 == m_ThreadStat)
    {
        TimeoutVal.tv_sec = 1;
        TimeoutVal.tv_usec = 0;

        FD_ZERO(&read_fds);
        FD_SET(AencFd, &read_fds);

        s32Ret = select(AencFd + 1, &read_fds, NULL, NULL, &TimeoutVal);
        if (s32Ret < 0)
        {
            break;
        }
        else if (0 == s32Ret)
        {
            AUDERR("get aenc stream select time out\n");
            break;
        }

        if (FD_ISSET(AencFd, &read_fds))
        {
            /* get stream from aenc chn */
            s32Ret = HI_MPI_AENC_GetStream(AeChn, &stStream, HI_FALSE);
            if (HI_SUCCESS != s32Ret )
            {
                AUDERR("HI_MPI_AENC_GetStream(%d), failed with %#x!\n", AeChn, s32Ret);
				m_ThreadStat = 0;
                return s32Ret;
            }

            /* send stream to buffermanage */
			if(m_TalkDelay > 0)
			{
				talk = true;
				m_TalkDelay--;
			}
			else
			{
				talk = false;
			}
			for(i = 0;i < ZMD_APP_ENCODE_VIDEO_MAX_CH_SRTEAM;i++)
			{
				pbuffer = BufferManageCtrl::GetInstance()->GetBMInstance(i);
				if(pbuffer != NULL)
				{
					pbuffer->PutOneAFrameToBuffer(&stStream,talk);
				}
			}

            /* finally you must release the stream */
            s32Ret = HI_MPI_AENC_ReleaseStream(AeChn, &stStream);
            if (HI_SUCCESS != s32Ret )
            {
                AUDERR("HI_MPI_AENC_ReleaseStream(%d), failed with %#x!\n", AeChn, s32Ret);
                m_ThreadStat = 0;
                return s32Ret;
            }
        }
    }
    return 0;
}

int SetInputVolume(unsigned int vol)
{
	HI_S32 fdAcodec = -1;
	unsigned int volume = 0;

	fdAcodec = open(ACODEC_FILE,O_RDWR);
	if(fdAcodec <0 )
	{
		close(fdAcodec);
		AUDERR("open acodec fail.\n");
		return -1;
	}
	if (ioctl(fdAcodec, ACODEC_GET_INPUT_VOL, &volume))
	{
		AUDERR("ACODEC_GET_INPUT_VOL failed\n");
		close(fdAcodec);
		return -1;
	}
	AUDLOG("default input volume is %d\n",volume);
	volume = vol;
	if (ioctl(fdAcodec, ACODEC_SET_INPUT_VOL, &volume))
	{
		AUDERR("ACODEC_SET_INPUT_VOL failed\n");
		close(fdAcodec);
		return -1;
	}
	AUDLOG("after set input volume is %d\n",volume);
	return 0;
}

int ZMDAudio::AudioStart()
{
	HI_S32 s32Ret = HI_SUCCESS;
	m_ThreadStat = 1;

	//init input volume
	SetInputVolume(40);
	
    s32Ret = pthread_create(&m_pid, 0, AudioEncoderThreadEntry, (void*)this);
	if (HI_SUCCESS != s32Ret)
	{
		SAMPLE_PRT("Start Aenc failed!\n");
		return s32Ret;
	}
	return HI_SUCCESS;
}

int ZMDAudio::AudioStop()
{
	if(m_ThreadStat == 1)
	{
		m_ThreadStat = 0;
		pthread_join(m_pid,0);
	}
	return 0;
}

int ZMDAudio::AudioDecHandle(void* para)
{
	ParaEncAdecInfo* padec = (ParaEncAdecInfo*)para;
	if((padec == NULL) || (padec->buffer == NULL)||(padec->len > NumPerFrm + 4)) 
	{
		AUDERR("len is error ,need:%d,now:%d\n", NumPerFrm + 4,padec->len);
		return -1;  
	}               
	HI_S32 s32Ret=0;
	AUDIO_STREAM_S stAudioStream;    
	stAudioStream.u32Len = padec->len;
	stAudioStream.pStream = padec->buffer;
	//AUDLOG("SendAudioStreamToDecode len:%d,%02x,%02x,%02x,%02x,\n",padec->len,padec->buffer[0],padec->buffer[1],padec->buffer[2],padec->buffer[3]);
	s32Ret = HI_MPI_ADEC_SendStream(0, &stAudioStream, (HI_BOOL)padec->block);                                                                                                                                                      
	if (s32Ret != HI_SUCCESS) 
	{
		AUDERR("HI_MPI_ADEC_SendStream failed with %#x!\n", s32Ret);          
		AUDERR("SendAudioStreamToDecode  failed len:%d,%02x,%02x,%02x,%02x,\n",padec->len,padec->buffer[0],padec->buffer[1],padec->buffer[2],padec->buffer[3]);
	}
	m_TalkDelay =20;
	if (s32Ret)
	{
		if((s32Ret&0xf)==0xF)       
		{           
			AUDERR("HI_MPI_ADEC_ClearChnBuf ######## with %#x!,%02x\n", s32Ret,(s32Ret&0xf));         
			s32Ret=HI_MPI_AO_ClearChnBuf(0,0);          
			HI_MPI_ADEC_ClearChnBuf(0);                         
			HI_MPI_AO_ClearChnBuf(0,0);     
		}               
		return -1;
	}

	return 0;
}

int ZMDAudio::AudioDecSwitch(bool onoff)
{
#if 0
	if(onoff == true)
	{
		himm(0x200f0074, 0x00);
		himm(0x20140400, 0x02);
		himm(0x20140008, 0x02);
	}
	else
	{
	}
#endif
	return 0;
}
