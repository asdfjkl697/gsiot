#include "GSRemoteTalkMgr.h"
#include "common.h"
//#include "APlayer.h" //jyc20170606 remove
#include "RunCode.h"
#include "IPCameraBase.h"
#include <pthread.h>

#include "Aacdecoder.h" //jyc20170601 add
#include "librtmp/rtmp_sys.h" //jyc20170531 add
#include "player.h"

#define defValidTime_newPlay_param 15000


void struNewPlay_Param::settick()
{
	this->ts = timeGetTime();
}

void struTalkState::set_play( bool in_play )
{
	this->play = in_play;
	this->ts = timeGetTime();
	if( 0 == this->ts )
		this->ts += 1;

	if( this->play )
	{
		IsPlaying_ts = timeGetTime();
		keepalive_ts = timeGetTime();
	}
}

void struTalkState::set_IsPlaying_ts()
{
	if( this->play )
		IsPlaying_ts = timeGetTime();
}

GSRemoteTalkMgr::GSRemoteTalkMgr(void)
	: m_last_checktime(0), m_last_key(0), m_ts_newPlay_param(0)
{
}

GSRemoteTalkMgr::~GSRemoteTalkMgr(void)
{
	LOGMSG( "~GSRemoteTalkMgr()" );

	StopTalk_All();

	bool isOnlyOneTalk_cur = false;
	DWORD dwStart = ::timeGetTime();
	while( isPlaying(isOnlyOneTalk_cur) && ::timeGetTime()-dwStart < 2000 )
	{
		usleep(50000); //Sleep(50);
	}
}

std::string GSRemoteTalkMgr::GetStateInfoForMap( const defmapTalk &mapTalk )
{
	if( mapTalk.empty() )
		return std::string("��ǰͨ��״̬: 0ͨ��");

	unsigned long QueueCount = 0;
	unsigned long PlayCount = 0;
	bool isOnlyOneTalk = false;
	GetCountInfoForMap( mapTalk, QueueCount, PlayCount, isOnlyOneTalk );

	char buf[1024] = {0};
	snprintf( buf, sizeof(buf), "��ǰͨ��״̬: %luͨ����, ��������%lu, ��ռͨ��=%d", PlayCount, QueueCount, isOnlyOneTalk );

	std::string strState = buf;

	defmapTalk::const_iterator it = mapTalk.begin();
	for( int i=1; it != mapTalk.end(); ++it, ++i )
	{
		unsigned long second = (timeGetTime()-it->second.ts)/1000;
		if( second > 60 )
		{
			snprintf( buf, sizeof(buf), "%lu��%lu��", second/60, second%60 );
		}
		else
		{
			snprintf( buf, sizeof(buf), "%lu��", second );
		}
		std::string strTime = buf;

		snprintf( buf, sizeof(buf), "\r\n(%d) %s, ʱ��=%s, ��ռ=%d, from=%s, url=\"%s\"", 
			i,
			it->second.play ? "ͨ����":"ֹͣ",
			strTime.c_str(),
			it->second.isOnlyOneTalk,
			it->second.from_Jid.c_str(),
			it->first.c_str() );

		strState += buf;
	}

	return strState;
}

void GSRemoteTalkMgr::GetCountInfoForMap( const defmapTalk &mapTalk, unsigned long &QueueCount, unsigned long &PlayCount, bool &isOnlyOneTalk )
{
	QueueCount = mapTalk.size();
	PlayCount = 0;
	isOnlyOneTalk = false;

	defmapTalk::const_iterator it = mapTalk.begin();
	for( ; it != mapTalk.end(); ++it )
	{
		if( it->second.play )
		{
			PlayCount++;

			if( it->second.isOnlyOneTalk )
			{
				isOnlyOneTalk = true;
			}
		}
	}
}

//unsigned __stdcall APlayThread(LPVOID lpPara)
void *APlayThread(LPVOID lpPara)
{
	GSRemoteTalkMgr *p = (GSRemoteTalkMgr*)lpPara;

	static int s_lastAPlayThread_id_ = 0;
	const int Play_id = ++s_lastAPlayThread_id_;

	LOGMSG( "TalkMgr::APlayThread(%d) begin...", Play_id );

	if( p )
	{
		p->OnProc( Play_id );
	}

	LOGMSG( "TalkMgr::APlayThread(%d) end.", Play_id );
	pthread_detach(pthread_self()); //jyc20170722 add
	//return 0; //jyc20170526 remove
}

void GSRemoteTalkMgr::end_OnProc(struNewPlay_Param param)
{
	UrlKey_Del(param.url, param.key);

// �첽֪ͨfrom_Jid�������˳�
	if (!param.from_Jid.empty()) {
		if (m_ITalkNotify) {
			const defvecDevKey c_vecdev;
			m_ITalkNotify->OnTalkNotify(XmppGSTalk::defTalkCmd_quit, param.url,
					param.from_Jid, param.from_id, false, c_vecdev);
		}
	}
}

void GSRemoteTalkMgr::OnProc( const int Play_id )
{
	struNewPlay_Param param;
	if( !NewPlay_ParamPop( param ) )
	{
		//LOGMSGEX( defLOGNAME, defLOG_ERROR, "TalkMgr::OnProc(%d) NewPlay_ParamPop failed!", Play_id );
		return;
	}

	if( param.url.empty() )
	{
		LOGMSG( "TalkMgr::OnProc(%d) url is null!", Play_id );
		return;
	}

	// ��¼���ڲ��ŵ�url
	//const unsigned long key = UrlKey_Add( param );

	defvecTalkParam vecTalkParam;
	defvecDevKey successDevlst;
	defvecDevKey failedDevlst;
	const std::string play_url = param.url + " live=1";
	LOGMSG( "TalkMgr::OnProc(%d) APlayer.Open(%s), from_Jid=\"%s\", from_id=\"%s\"", Play_id, play_url.c_str(), param.from_Jid.c_str(), param.from_id.c_str() );

	bool doOpenLocal = false;
	bool hasSuccess = false;

	RTMP *rtmp = RTMP_Alloc();  //jyc20170532 add
	RTMP_Init(rtmp);
	if (!RTMP_SetupURL(rtmp, (char*) play_url.c_str())) {
		RTMP_Free(rtmp);
	}
    if(!RTMP_Connect(rtmp,NULL)){
        RTMP_Free(rtmp);
    }
    if(!RTMP_ConnectStream(rtmp,0)){
        RTMP_Close(rtmp);
        RTMP_Free(rtmp);
    }
    int nRead,buflen=0;

    int buflvlen = 1024;//1024*10->100
    char *buflv=(char*)malloc(buflvlen);
    char adtsbuf[7];
    unsigned char *m_freamebuf_aac=(unsigned char*)malloc(MAXONFREAMSIZE_AAC);

    nRead = RTMP_Read(rtmp, buflv, buflvlen);

    NeAACDecFrameInfo frame_info;
	NeAACDecHandle hdecoder = 0;
	unsigned long samplerate = 0;
	unsigned char channels = 0;
	ADTS_HEADER adtsheader;
	ADTS_HEADER adtsheader_t;
	int ret = 0;
	unsigned char * pcm_buf;

	//////////////////////////////////////////////////////////////////////////
	memset(&frame_info, 0, sizeof(frame_info));

	//分帧
	if((*(buflv+13)!=0x08)||((*(buflv+24)&0xf0)!=0xa0)){
		end_OnProc(param);
	    RTMP_Close(rtmp);
	    RTMP_Free(rtmp);
		return;
	}

	unsigned char audioObjectType;
	if(*(buflv+25)==0){
		audioObjectType = (*(buflv+26)>>3)- 1; //jyc20170612 notice -1
		adtsheader_t.sf_index = (*(buflv+26) &0x07)<<1 | (*(buflv+27) &0x80)>>7;
		adtsheader_t.channel_configuration = (*(buflv+27)>>3)&0x0f;
	}


	unsigned int temp=14; //jyc20170612 modify +7-2
	adtsbuf[0] = 0xff;adtsbuf[1] = 0xf1;
	adtsbuf[2] = (audioObjectType) << 6 | adtsheader_t.sf_index << 2
			| adtsheader_t.channel_configuration >> 2;
	adtsbuf[3] = adtsheader_t.channel_configuration << 6| (temp & 0x1800) >> 11;
	adtsbuf[4] = (temp & 0x07f8) >> 3;
	adtsbuf[5] = (temp & 0x07) << 5 | 0x1f;
	adtsbuf[6] = 0xfc | 0x00;

	Detach_Head_Aac(&adtsheader, adtsbuf);
	memcpy(m_freamebuf_aac, adtsbuf, 7);
	memcpy(m_freamebuf_aac + 7, adtsbuf, adtsheader.aac_frame_length-7); //add just for number
	if(buflv)free(buflv);
	//* 打开解码器
	hdecoder = NeAACDecOpen();
#if 1
	NeAACDecConfigurationPtr conf = NeAACDecGetCurrentConfiguration(hdecoder);
	conf->dontUpSampleImplicitSBR = 1;
	NeAACDecSetConfiguration(hdecoder, conf);
#endif

	ret = NeAACDecInit(hdecoder, m_freamebuf_aac, adtsheader.aac_frame_length,
			&samplerate, &channels);
	if (ret < 0) {
		printf("NeAACDecInit error\n");
		end_OnProc(param); //jyc20170612 add
		NeAACDecClose(hdecoder);
	    RTMP_Close(rtmp);
	    RTMP_Free(rtmp);
		return;
	} else {
		printf("frame size %d\n", adtsheader.aac_frame_length);
		printf("samplerate %d, channels %d\n", samplerate, channels);
	}

	if( !param.vecdev.empty() )
	{
		for( size_t i=0; i<param.vecdev.size(); ++i )
		{
			//if( g_AddPlayOut( ap, param.vecdev[i], m_ITalkNotify ) )
			if(1) //jyc20170602 debug
			{
				hasSuccess = true;
				successDevlst.push_back( param.vecdev[i] );
			}
			else
			{
				failedDevlst.push_back( param.vecdev[i] );
			}
		}
	}
	// �첽֪ͨfrom_Jid�Ѳ���
	if( !param.from_Jid.empty() )
	{
		if( m_ITalkNotify )
		{
			m_ITalkNotify->OnTalkNotify( hasSuccess?XmppGSTalk::defTalkCmd_session:XmppGSTalk::defTalkCmd_quit, param.url, param.from_Jid, param.from_id, true, successDevlst, hasSuccess );
		}
	}
	if( !successDevlst.empty() )
	{
		successDevlst.clear();
	}

	if( !failedDevlst.empty() )
	{
		if( m_ITalkNotify )
		{
			m_ITalkNotify->OnTalkNotify( XmppGSTalk::defTalkCmd_adddev, param.url, param.from_Jid, param.from_id, true, failedDevlst, false );
		}

		failedDevlst.clear();
	}

	int bufsize=1024*100;//1024*10->100
	char *buf=(char*)malloc(bufsize);
    char *pcmplaybuf = (char*)malloc(1024*1024*2);
    int pcmplaylen=0;
//	pcm_buf = m_onfreamebuf_pcm;
//	memset(m_freamebuf_aac, 0, MAXONFREAMSIZE_AAC);
//	memset(m_onfreamebuf_pcm, 0, MAXONFREAMSIZE_PCM);

	while (nRead = RTMP_Read(rtmp, buf, bufsize))
	{
		if(*buf!=8)continue; //flv
		temp=(*(buf+1)<<16 | (*(buf+2))<<8 | *(buf+3)) + 5 ; //+7-2 notice
		if(temp<=7)continue;
		adtsbuf[0]=0xff;adtsbuf[1]=0xf1;
		adtsbuf[2]=(audioObjectType)<<6 | adtsheader_t.sf_index<<2 | adtsheader_t.channel_configuration>>2;
		adtsbuf[3]=adtsheader_t.channel_configuration<<6 | (temp&0x1800)>>11 ;
		adtsbuf[4]=(temp&0x07f8)>>3;
		adtsbuf[5]=(temp&0x07)<<5 | 0x1f;
		adtsbuf[6]=0xfc|0x00;

		if((*(buf+11) &0xf0)!=0xa0)continue; //aac

		Detach_Head_Aac(&adtsheader, adtsbuf);
		memcpy(m_freamebuf_aac,adtsbuf,7);
		memcpy(m_freamebuf_aac+7,buf+13,adtsheader.aac_frame_length-7);
//		buf += adtsheader.aac_frame_length-7+13 ;

		//decode ADTS frame
		pcm_buf = (unsigned char *) NeAACDecDecode(hdecoder, &frame_info,
				m_freamebuf_aac, adtsheader.aac_frame_length);

		if (frame_info.error > 0) {
//			printf("%s\n", NeAACDecGetErrorMessage(frame_info.error));
		}
		else if (pcm_buf && frame_info.samples > 0) //jyc20170613 add Isplayflag
		{
//			printf(
//					"frame info: bytesconsumed %d, channels %d, header_type %d\object_type %d, samples %d, samplerate %d\n",
//					frame_info.bytesconsumed, frame_info.channels,
//					frame_info.header_type, frame_info.object_type,
//					frame_info.samples, frame_info.samplerate);
			memcpy(&pcmplaybuf[pcmplaylen], pcm_buf,frame_info.samples * frame_info.channels);
			pcmplaylen += frame_info.samples * frame_info.channels;
		}
		memset(m_freamebuf_aac, 0, MAXONFREAMSIZE_AAC);
//		memset(m_onfreamebuf_pcm, 0, MAXONFREAMSIZE_PCM);
		if(pcmplaylen>1024*61){ //jyc20170612 debug
			break;
		}
	}

	if(buf)free(buf);
	if(m_freamebuf_aac)free(m_freamebuf_aac);
	Player playtest;
	playtest.initPlayer(samplerate);
	playtest.play(pcmplaybuf, pcmplaylen);
	if(pcmplaybuf)free(pcmplaybuf);

	NeAACDecClose(hdecoder);
    RTMP_Close(rtmp);
    RTMP_Free(rtmp);

	LOGMSG( "TalkMgr::OnProc(%d) APlayer.Close(%s), from_Jid=\"%s\", from_id=\"%s\"", Play_id, play_url.c_str(), param.from_Jid.c_str(), param.from_id.c_str() );

	end_OnProc(param); //jyc20170612 modify
}

// ���������߳�
// isOnlyOneTalk: ֻ����һ������ʱ��ֹͣ�������в��ţ��ٴ����²���
// doforce: ���Ѿ��ڲ��ţ���ﵽ����ʱ��true��ʾǿ�Ƹ��ǽ����²���
defGSReturn GSRemoteTalkMgr::StartTalk( const std::string &url, const std::string &from_Jid, const std::string &from_id, const defvecDevKey &vecdev, const bool doforce, const bool isOnlyOneTalk )
{
	if( url.empty() )
	{
		LOGMSG( "TalkMgr::StartTalk url is null!" );
		return defGSReturn_Err;
	}

	UrlKey_Check( true );

	// ָ�������豸
	if( vecdev.size() <= 1 )
	{
		IOTDeviceType dev_type = IOT_DEVICE_Unknown;
		int dev_id = 0;

		if( vecdev.size() > 0 )
		{
			dev_type = vecdev[0].m_type;
			dev_id = vecdev[0].m_id;
		}

		if( IOT_DEVICE_Unknown != dev_type && dev_id>0 )
		{
			GSIOTDevice *pDev = NULL;
			if( m_ITalkNotify )
			{
				const defvecDevKey c_vecdev;
				pDev = (GSIOTDevice*)m_ITalkNotify->OnTalkNotify( XmppGSTalk::defTalkSelfCmd_GetDevice, "", "", "", true, c_vecdev, true, dev_type, dev_id );
			}

			if( pDev && pDev->GetEnable() && pDev->getControl() )
			{
				IPCameraBase *cam_ctl = (IPCameraBase*)pDev->getControl();
				if( cam_ctl->isTalkUseCam() ) // �Ƿ�ʹ������ͷ�Խ�
				{
					if( !cam_ctl->isTalkSupport() )
					{
						LOGMSG( "TalkMgr::StartTalk faild! cam(type=%d, id=%d) isTalkSupport=false", dev_type, dev_id );
						return defGSReturn_NoExist;
					}
				}
				else
				{
					return defGSReturn_NoExist;
				}
			}
			else
			{
				return defGSReturn_NoExist;
			}
		}
	}

	bool isOnlyOneTalk_cur = false;
	unsigned long curPlayCount = 0;
	unsigned long curQueueCount = 0;
	GetCountInfo( curQueueCount, curPlayCount, isOnlyOneTalk_cur );

	LOGMSG( "TalkMgr::StartTalk curInfo(PlayCnt=%lu, QueueCnt=%lu, isOnlyOneTalk_cur=%d), new: isOnlyOneTalk=%d, doforce=%d, devnum=%d, url=\"%s\", from_Jid=\"%s\", from_id=\"%s\"", 
		curPlayCount, curQueueCount, isOnlyOneTalk_cur, 
		isOnlyOneTalk, doforce, vecdev.size(), url.c_str(), from_Jid.c_str(), from_id.c_str() );

	if( curQueueCount >= MAX_TalkQueueSize )
	{
		LOGMSG( "TalkMgr::StartTalk MAX_TalkQueue(%d) is full, faild!", curQueueCount );

		return defGSReturn_IsLock;
	}

	 // �Ƿ��Ѿ����ڲ���
	if( isPlaying(isOnlyOneTalk_cur) )
	{
		// ��Ҫ���ó�ֻ����һ��
		if( isOnlyOneTalk || isOnlyOneTalk_cur )
		{
			if( !doforce )
			{
				LOGMSG( "TalkMgr::StartTalk have isPlaying, failed!" );
				return defGSReturn_IsLock;
			}

			StopTalk_All();
		}
		else // ����������
		{
			bool isOnlyOneTalk_byurl = false;
			const bool isplaying_url = isPlaying_url( url, isOnlyOneTalk_byurl );
			if( isplaying_url )
			{
				if( !doforce )
				{
					LOGMSG( "TalkMgr::StartTalk isPlaying this url=\"%s\"", url.c_str() );
					return defGSReturn_IsLock;
				}
			}

			if( curPlayCount >= MAX_TALK )
			{
				if( !doforce )
				{
					LOGMSG( "TalkMgr::StartTalk isPlaying MAX_TALK, failed!" );
					return defGSReturn_IsLock;
				}

				if( isplaying_url )
				{
					LOGMSG( "TalkMgr::StartTalk is MAX_TALK, call StopTalk(%s).", url.c_str() );
					StopTalk( url );
				}
				else
				{
					LOGMSG( "TalkMgr::StartTalk is MAX_TALK, call StopTalk_AnyOne." );
					StopTalk_AnyOne();
				}
			}
		}
	}
	else
	{
		// ��ǰû���ڲ��ţ���ֱ�Ӳ���
	}

	if( !NewPlay_ParamPush( struNewPlay_Param( XmppGSTalk::defTalkCmd_session, url, from_Jid, from_id, isOnlyOneTalk, vecdev ) ) )
	{
		return defGSReturn_IsLock;
	}

	/*
	HANDLE   hth1;
	unsigned  uiThread1ID;
	hth1 = (HANDLE)_beginthreadex( NULL, 0, APlayThread, this, 0, &uiThread1ID );
	CloseHandle(hth1);
	*/

	pthread_t id_1;
    int ret=pthread_create(&id_1,NULL,APlayThread,this);
    if(ret!=0)  
    {  
        printf("Create APlayThread error!\n");
		return defGSReturn_Err;  
    } 
    //pthread_join(id_1,NULL); //jyc20170722 add have trouble

	return defGSReturn_Success;
}

defGSReturn GSRemoteTalkMgr::AdddevTalk( const std::string &url, const std::string &from_Jid, const std::string &from_id, const defvecDevKey &vecdev )
{
	if( !UrlKey_push_cmd( url, struNewPlay_Param( XmppGSTalk::defTalkCmd_adddev, url, from_Jid, from_id, false, vecdev ) ) )
	{
		defGSReturn_Err;
	}

	return defGSReturn_Success;
}

defGSReturn GSRemoteTalkMgr::RemovedevTalk( const std::string &url, const std::string &from_Jid, const std::string &from_id, const defvecDevKey &vecdev )
{
	if( !UrlKey_push_cmd( url, struNewPlay_Param( XmppGSTalk::defTalkCmd_removedev, url, from_Jid, from_id, false, vecdev ) ) )
	{
		defGSReturn_Err;
	}

	return defGSReturn_Success;
}

// ָ����� url�Ȳ����ֹͣ���ţ�ֻ����ֹͣ��־��������ɾ���߳�ֹͣ�����ɾ��
bool GSRemoteTalkMgr::StopTalk( const std::string &url )
{
	if( url == "all" )
	{
		return StopTalk_All();
	}
	else
	{
		gloox::util::MutexGuard mutexguard( m_mutex_TalkMgr );

		if( m_mapTalk.empty() )
		{
			LOGMSG( "TalkMgr::StopTalk mapTalk is empty" );
			return true;
		}

		defmapTalk::iterator it = m_mapTalk.find( url );
		if( it != m_mapTalk.end() )
		{
			LOGMSG( "TalkMgr::StopTalk url=\"%s\"", url.c_str() );

			it->second.set_play( false );
			return true;
		}

		LOGMSG( "TalkMgr::StopTalk not found url=\"%s\"", url.c_str() );
	}

	return false;
}

// ��ֹͣ
bool GSRemoteTalkMgr::StopTalk_All()
{
	gloox::util::MutexGuard mutexguard( m_mutex_TalkMgr );

	if( m_mapTalk.empty() )
		return true;

	defmapTalk::iterator it = m_mapTalk.begin();
	for( ; it != m_mapTalk.end(); ++it )
	{
		LOGMSG( "TalkMgr::StopTalk_All url=\"%s\"", it->first.c_str() );

		it->second.set_play( false );
	}

	return true;
}

// ����һ��������ֹͣ
bool GSRemoteTalkMgr::StopTalk_AllButOneTalk()
{
	gloox::util::MutexGuard mutexguard( m_mutex_TalkMgr );

	if( m_mapTalk.empty() )
		return true;

	bool isFoundPlay = false;
	defmapTalk::iterator it = m_mapTalk.begin();
	for( ; it != m_mapTalk.end(); ++it )
	{
		if( it->second.play )
		{
			if( isFoundPlay )
			{
				LOGMSG( "TalkMgr::StopTalk_AllButOneTalk url=\"%s\"", it->first.c_str() );

				it->second.set_play( false );
			}
			else
			{
				// ��һ����ֹͣ
				isFoundPlay = true;
			}
		}
	}

	return true;
}

// ֹͣһ����������
bool GSRemoteTalkMgr::StopTalk_AnyOne()
{
	gloox::util::MutexGuard mutexguard( m_mutex_TalkMgr );

	if( m_mapTalk.empty() )
		return true;

	defmapTalk::iterator it = m_mapTalk.begin();
	for( ; it != m_mapTalk.end(); ++it )
	{
		if( it->second.play )
		{
			LOGMSG( "TalkMgr::StopTalk_AnyOne url=\"%s\"", it->first.c_str() );

			it->second.set_play( false );
			return true;
		}
	}

	return true;
}

// �Ƿ����ڲ���
bool GSRemoteTalkMgr::isPlaying( bool &isOnlyOneTalk )
{
	gloox::util::MutexGuard mutexguard( m_mutex_TalkMgr );

	isOnlyOneTalk = false;
	if( m_mapTalk.empty() )
		return false;

	defmapTalk::const_iterator it = m_mapTalk.begin();
	for( ; it != m_mapTalk.end(); ++it )
	{
		if( it->second.play )
		{
			isOnlyOneTalk = it->second.isOnlyOneTalk;
			return true;
		}
	}

	return false;
}

// ָ����� url ���Ƿ��ڲ���
bool GSRemoteTalkMgr::isPlaying_url( const std::string &url, bool &isOnlyOneTalk, bool iskeepalive )
{
	gloox::util::MutexGuard mutexguard( m_mutex_TalkMgr );

	isOnlyOneTalk = false;
	if( url.empty() )
		return false;

	defmapTalk::iterator it = m_mapTalk.find( url );
	if( it != m_mapTalk.end() )
	{
		isOnlyOneTalk = it->second.isOnlyOneTalk;
		if( iskeepalive )
		{
			it->second.keepalive_ts = timeGetTime();
		}
		return it->second.play;
	}

	return false;
}

// ��ȡͳ����Ϣ
void GSRemoteTalkMgr::GetCountInfo( unsigned long &QueueCount, unsigned long &PlayCount, bool &isOnlyOneTalk )
{
	gloox::util::MutexGuard mutexguard( m_mutex_TalkMgr );

	GetCountInfoForMap( m_mapTalk, QueueCount, PlayCount, isOnlyOneTalk );
}

bool GSRemoteTalkMgr::CopyMapTalk( defmapTalk &mapTalk )
{
	gloox::util::MutexGuard mutexguard( m_mutex_TalkMgr );

	mapTalk = m_mapTalk;
	return true;
}

// �洢�²��Ų���
bool GSRemoteTalkMgr::NewPlay_ParamPush( const struNewPlay_Param &param )
{
	gloox::util::MutexGuard mutexguard( m_mutex_TalkMgr );

	// �Ѵ�������ִ�еĲ���
	if( !m_newPlay_param.url.empty() )
	{
		if( timeGetTime()-m_ts_newPlay_param <= defValidTime_newPlay_param )
		{
			//LOGMSGEX( defLOGNAME, defLOG_ERROR, "TalkMgr::NewPlay_ParamPush failed! isbeing ts delay=%dms", timeGetTime()-m_ts_newPlay_param );
			return false;
		}
	}

	m_newPlay_param = param;
	m_newPlay_param.key = UrlKey_Add_nolock( m_newPlay_param );
	m_ts_newPlay_param = timeGetTime();
	return true;
}

// ȡ���²��Ų���
bool GSRemoteTalkMgr::NewPlay_ParamPop( struNewPlay_Param &param )
{
	gloox::util::MutexGuard mutexguard( m_mutex_TalkMgr );

	if( m_newPlay_param.url.empty() )
	{
		//LOGMSGEX( defLOGNAME, defLOG_ERROR, "TalkMgr::NewPlay_ParamPop failed! url is null" );
		return false;
	}

	if( timeGetTime()-m_ts_newPlay_param > defValidTime_newPlay_param )
	{
		//LOGMSGEX( defLOGNAME, defLOG_ERROR, "TalkMgr::NewPlay_ParamPop failed! ts over delay=%dms", timeGetTime()-m_ts_newPlay_param );
		m_newPlay_param.url = "";
		m_newPlay_param.from_Jid = "";
		m_newPlay_param.from_id = "";
		return false;
	}

	param = m_newPlay_param;

	m_newPlay_param.url = "";
	m_newPlay_param.from_Jid = "";
	m_newPlay_param.from_id = "";
	return true;
}

// ���һ�����ż�¼
unsigned long GSRemoteTalkMgr::UrlKey_Add_nolock( const struNewPlay_Param &param )
{
	//gloox::util::MutexGuard mutexguard( m_mutex_TalkMgr );

	if( param.url.empty() )
		return 0;

	m_last_key++;
	m_mapTalk[param.url] = struTalkState( m_last_key, true, param.isOnlyOneTalk, param.from_Jid, param.from_id );

	LOGMSG( "TalkMgr::UrlKey_Add aftersize=%d, isOnlyOneTalk=%d, key=%lu, url=\"%s\", from_Jid=\"%s\", from_id=\"%s\"",
		m_mapTalk.size(), param.isOnlyOneTalk, m_last_key, param.url.c_str(), param.from_Jid.c_str(), param.from_id.c_str() );

	return m_last_key;
}

// ��������������
bool GSRemoteTalkMgr::UrlKey_push_cmd( const std::string &url, const struNewPlay_Param &param )
{
	gloox::util::MutexGuard mutexguard( m_mutex_TalkMgr );

	if( url.empty() )
		return false;

	defmapTalk::iterator it = m_mapTalk.find( url );
	if( it != m_mapTalk.end() )
	{
		if( it->second.play )
		{
			if( it->second.vecTalkParam.size() < 99 )
			{
				it->second.vecTalkParam.push_back(param);
				//jyc20170330 remove
				//LOGMSG( "TalkMgr::UrlKey_push_cmd success, cmd=\"%s\", from_Jid=\"%s\", url=\"%s\"", XmppGSTalk::TalkCmd2String(param.cmd).c_str(), param.from_Jid.c_str(), url.c_str() );
				return true;
			}
			else
			{
				LOGMSG( "TalkMgr::UrlKey_push_cmd vec full, url=\"%s\"", url.c_str() );
				return false;
			}
		}
		else
		{
			LOGMSG( "TalkMgr::UrlKey_push_cmd not play, url=\"%s\"", url.c_str() );
			return false;
		}
	}

	LOGMSG( "TalkMgr::UrlKey_push_cmd not found, url=\"%s\"", url.c_str() );
	return false;
}

// �����߳�ֹͣ��ִ��ɾ��ż�¼��������keyһ�¡�
bool GSRemoteTalkMgr::UrlKey_Del( const std::string &url, const unsigned long key )
{
	gloox::util::MutexGuard mutexguard( m_mutex_TalkMgr );

	if( m_mapTalk.empty() )
		return true;

	defmapTalk::iterator it = m_mapTalk.find( url );
	if( it != m_mapTalk.end() )
	{
		if( key == it->second.key )
		{
			const unsigned long usems = timeGetTime()-it->second.ts;
			m_mapTalk.erase(it);

			LOGMSG( "TalkMgr::UrlKey_Del aftersize=%d, key=%lu, quitusetime=%lums, url=\"%s\"", m_mapTalk.size(), key, usems, url.c_str() );
			return true;
		}
	}

	return false;
}

// ָ����� url �� key ���Ƿ��ڲ���
bool GSRemoteTalkMgr::UrlKey_IsPlaying( const std::string &url, const unsigned long key, defvecTalkParam &vecTalkParam, bool popcmd )
{
	if( !vecTalkParam.empty() )
	{
		vecTalkParam.clear();
	}

	gloox::util::MutexGuard mutexguard( m_mutex_TalkMgr );

	if( url.empty() )
		return false;

	defmapTalk::iterator it = m_mapTalk.find( url );
	if( it != m_mapTalk.end() )
	{
		if( key == it->second.key )
		{
			if( popcmd )
			{
				vecTalkParam.swap( it->second.vecTalkParam );
			}
			it->second.set_IsPlaying_ts();
			return it->second.play;
		}
		else
		{
			LOGMSG( "TalkMgr::UrlKey_IsPlaying false key diff, in_key=%lu, play_key=%lu, url=\"%s\"", key, it->second.key, url.c_str() );
			return false;
		}
	}

	return false;
}

// ����Ƿ�ֻ����һ��������ǣ�����һ��ֹͣ����
void GSRemoteTalkMgr::UrlKey_Check( bool forceCheck )
{
	const uint32_t curtime = timeGetTime();
	gloox::util::MutexGuard mutexguard( m_mutex_TalkMgr );

	// ÿ������һ��
	if( !forceCheck
		 && (curtime-m_last_checktime) < 5000
		)
	{
		return;
	}

	m_last_checktime = curtime;
	if( m_mapTalk.empty() )
		return;

	defmapTalk::iterator it = m_mapTalk.begin();
	for( ; it != m_mapTalk.end(); ++it )
	{
		if( it->second.isOnlyOneTalk
			&& it->second.play )
		{
			// ����һ��ֹͣ����
			bool isFoundPlay = false;
			it = m_mapTalk.begin();
			for( ; it != m_mapTalk.end(); ++it )
			{
				if( it->second.play )
				{
					if( isFoundPlay )
					{
						LOGMSG( "TalkMgr::UrlKey_Check StopTalk key=%lu, url=\"%s\"", it->second.key, it->first.c_str() );

						it->second.set_play( false );
					}
					else
					{
						// ��һ����ֹͣ
						isFoundPlay = true;
					}
				}
			}

			break;
		}
	}

	it = m_mapTalk.begin();
	for( ; it != m_mapTalk.end(); ++it )
	{
		if( !it->second.play )
		{
			if( curtime - it->second.ts > 60000 )
			{
				LOGMSG( "TalkMgr::UrlKey_Check play set false timeout key=%lu, url=\"%s\"", it->second.key, it->first.c_str() );

				m_mapTalk.erase(it);
				it = m_mapTalk.begin();
			}
		}
		else
		{
			if( curtime - it->second.IsPlaying_ts > 60000 )
			{
				LOGMSG( "TalkMgr::UrlKey_Check IsPlaying_ts timeover key=%lu, url=\"%s\"", it->second.key, it->first.c_str() );

				m_mapTalk.erase(it);
				it = m_mapTalk.begin();
			}

			if( curtime - it->second.keepalive_ts > 60000 )
			{
				LOGMSG( "TalkMgr::UrlKey_Check keepalive_ts timeover key=%lu, url=\"%s\"", it->second.key, it->first.c_str() );

				m_mapTalk.erase(it);
				it = m_mapTalk.begin();
			}
		}
	}
}
