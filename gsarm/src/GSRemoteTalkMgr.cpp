#include "GSRemoteTalkMgr.h"
//#include <process.h>   //jyc20170330 modify to pthread.h
#include <pthread.h>
#include "common.h"
#include "APlayer.h"
#include "RunCode.h"
#include "IPCameraBase.h"


#define defValidTime_newPlay_param 15000


void struNewPlay_Param::settick()
{
	this->ts = timeGetTime();
}

void g_setAPlayerParam( APlayer &ap )
{
	if( IsRUNCODEEnable(defCodeIndex_APlay_ProbeSize) )
	{
		const int c_probesize = RUNCODE_Get(defCodeIndex_APlay_ProbeSize,defRunCodeValIndex_2);
		if( c_probesize >= 0 )
		{
			ap.m_probesize = c_probesize;
		}
	}
}

bool g_AddPlayOut( APlayer &ap, const GSIOTDeviceKey &devkey, ITalkNotify *pITalkNotify )
{
	// 指定播放设备
	GSIOTDevice *pDev = NULL;
	defGSReturn ret = defGSReturn_Null;
	if( IOT_DEVICE_Unknown != devkey.m_type && devkey.m_id>0 )
	{
		if( pITalkNotify )
		{
			const defvecDevKey c_vecdev;
			pDev = (GSIOTDevice*)pITalkNotify->OnTalkNotify( XmppGSTalk::defTalkSelfCmd_GetDevice, "", "", "", true, c_vecdev, true, devkey.m_type, devkey.m_id );
			ret = ap.AddPlayOut( pDev );
		}
	}
	else
	{
		ret = ap.AddPlayOut( NULL );
	}

	if( defGSReturn_Success == ret || defGSReturn_IsExist == ret ) // 已打开也认为成功
	{
		return true;
	}

	return false;
}

bool g_OnProc_TalkParam( const int Play_id, APlayer &ap, defvecTalkParam &vecTalkParam, ITalkNotify *pITalkNotify )
{
	for( defvecTalkParam::const_iterator it=vecTalkParam.begin(); it!=vecTalkParam.end(); ++it )
	{
		if( timeGetTime()-it->ts > 20000 )
		{
			LOGMSG( "TalkMgr::OnProc(%d) Talk running cmd=%d overtime! from_Jid=\"%s\", from_id=\"%s\"", Play_id, it->cmd, it->from_Jid.c_str(), it->from_id.c_str() );
			continue;
		}

		const struNewPlay_Param &param = *it;
		//jyc20170330 remove
		//LOGMSG( "TalkMgr::g_OnProc_TalkParam(%d) cmd=\"%s\", from_Jid=\"%s\", from_id=\"%s\"", Play_id, XmppGSTalk::TalkCmd2String(it->cmd).c_str(), it->from_Jid.c_str(), it->from_id.c_str() );

		switch( it->cmd )
		{
		case XmppGSTalk::defTalkCmd_adddev:
			{
				defvecDevKey successDevlst;
				defvecDevKey failedDevlst;
				if( !param.vecdev.empty() )
				{
					for( size_t i=0; i<param.vecdev.size(); ++i )
					{
						if( g_AddPlayOut( ap, param.vecdev[i], pITalkNotify ) )
						{
							successDevlst.push_back( param.vecdev[i] );
						}
						else
						{
							failedDevlst.push_back( param.vecdev[i] );
						}
					}
				}

				// 都没有时也发送失败
				if( successDevlst.empty() && failedDevlst.empty() )
				{
					if( pITalkNotify )
					{
						pITalkNotify->OnTalkNotify( XmppGSTalk::defTalkCmd_adddev, param.url, param.from_Jid, param.from_id, true, successDevlst, false );
					}
				}

				if( !successDevlst.empty() )
				{
					if( pITalkNotify )
					{
						pITalkNotify->OnTalkNotify( XmppGSTalk::defTalkCmd_adddev, param.url, param.from_Jid, param.from_id, true, successDevlst, true );
					}

					successDevlst.clear();
				}

				//if( !failedDevlst.empty() )
				//{
				//	if( pITalkNotify )
				//	{
				//		pITalkNotify->OnTalkNotify( XmppGSTalk::defTalkCmd_adddev, param.url, param.from_Jid, param.from_id, true, failedDevlst, false );
				//	}

				//	failedDevlst.clear();
				//}
			}
			break;

		case XmppGSTalk::defTalkCmd_removedev:
			{
				defvecDevKey successDevlst;
				defvecDevKey failedDevlst;
				if( !param.vecdev.empty() )
				{
					for( size_t i=0; i<param.vecdev.size(); ++i )
					{
						if( ap.RemovePlayOut( param.vecdev[i] ) )
						{
							successDevlst.push_back( param.vecdev[i] );
						}
						else
						{
							failedDevlst.push_back( param.vecdev[i] );
						}
					}
				}

				// 都没有时也发送失败
				if( successDevlst.empty() && failedDevlst.empty() )
				{
					if( pITalkNotify )
					{
						pITalkNotify->OnTalkNotify( XmppGSTalk::defTalkCmd_removedev, param.url, param.from_Jid, param.from_id, true, successDevlst, false );
					}
				}
				else if( !successDevlst.empty() )
				{
					if( pITalkNotify )
					{
						pITalkNotify->OnTalkNotify( XmppGSTalk::defTalkCmd_removedev, param.url, param.from_Jid, param.from_id, true, successDevlst, true );
					}

					successDevlst.clear();
				}
				else if( !failedDevlst.empty() )
				{
					if( pITalkNotify )
					{
						pITalkNotify->OnTalkNotify( XmppGSTalk::defTalkCmd_removedev, param.url, param.from_Jid, param.from_id, true, failedDevlst, false );
					}

					failedDevlst.clear();
				}
			}
			break;

		case XmppGSTalk::defTalkCmd_keepalive:
			{
				defvecDevKey successDevlst;
				defvecDevKey failedDevlst;
				if( !param.vecdev.empty() )
				{
					if( !ap.Keepalive( param.vecdev, successDevlst, failedDevlst ) )
					{
						if( pITalkNotify )
						{
							pITalkNotify->OnTalkNotify( XmppGSTalk::defTalkCmd_quit, param.url, param.from_Jid, param.from_id, true, failedDevlst, true );
						}

						break;
					}
					else if( !successDevlst.empty() )
					{
						if( pITalkNotify )
						{
							pITalkNotify->OnTalkNotify( XmppGSTalk::defTalkCmd_keepalive, param.url, param.from_Jid, param.from_id, true, successDevlst, true );
						}

						successDevlst.clear();
					}
					else if( !failedDevlst.empty() )
					{
						if( pITalkNotify )
						{
							pITalkNotify->OnTalkNotify( XmppGSTalk::defTalkCmd_removedev, param.url, param.from_Jid, param.from_id, true, failedDevlst, true );
						}

						failedDevlst.clear();
					}
				}
				else
				{
					if( pITalkNotify )
					{
						pITalkNotify->OnTalkNotify( XmppGSTalk::defTalkCmd_keepalive, param.url, param.from_Jid, param.from_id, true, param.vecdev, true );
					}
				}
			}
			break;
		}
	}

	return true;
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
		return std::string("当前通话状态: 0通话");

	unsigned long QueueCount = 0;
	unsigned long PlayCount = 0;
	bool isOnlyOneTalk = false;
	GetCountInfoForMap( mapTalk, QueueCount, PlayCount, isOnlyOneTalk );

	char buf[1024] = {0};
	snprintf( buf, sizeof(buf), "当前通话状态: %lu通话中, 队列数量%lu, 独占通话=%d", PlayCount, QueueCount, isOnlyOneTalk );

	std::string strState = buf;

	defmapTalk::const_iterator it = mapTalk.begin();
	for( int i=1; it != mapTalk.end(); ++it, ++i )
	{
		unsigned long second = (timeGetTime()-it->second.ts)/1000;
		if( second > 60 )
		{
			snprintf( buf, sizeof(buf), "%lu分%lu秒", second/60, second%60 );
		}
		else
		{
			snprintf( buf, sizeof(buf), "%lu秒", second );
		}
		std::string strTime = buf;

		snprintf( buf, sizeof(buf), "\r\n(%d) %s, 时间=%s, 独占=%d, from=%s, url=\"%s\"", 
			i,
			it->second.play ? "通话中":"停止",
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

unsigned __stdcall APlayThread(LPVOID lpPara)
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
	return 0;
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

	// 记录正在播放的url
	//const unsigned long key = UrlKey_Add( param );

	defvecTalkParam vecTalkParam;
	defvecDevKey successDevlst;
	defvecDevKey failedDevlst;
	const std::string play_url = param.url + " live=1";
	APlayer ap;
	g_setAPlayerParam( ap );
	LOGMSG( "TalkMgr::OnProc(%d) APlayer.Open(%s), from_Jid=\"%s\", from_id=\"%s\"", Play_id, play_url.c_str(), param.from_Jid.c_str(), param.from_id.c_str() );

	bool doOpenLocal = false;
	bool hasSuccess = false;
	if( !ap.Open( play_url ) )
	{
		goto end_OnProc;
	}

	//bool doOpenLocal = false; //jyc20170330 move up goto
	//bool hasSuccess = false;
	if( !param.vecdev.empty() )
	{
		for( size_t i=0; i<param.vecdev.size(); ++i )
		{
			if( g_AddPlayOut( ap, param.vecdev[i], m_ITalkNotify ) )
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
	else
	{
		if( g_AddPlayOut( ap, GSIOTDeviceKey(), m_ITalkNotify ) )
		{
			hasSuccess = true;
			successDevlst.push_back( GSIOTDeviceKey() );
		}
		else
		{
			failedDevlst.push_back( GSIOTDeviceKey() );
		}
	}

	// 异步通知from_Jid已播放
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

	// 一直循环播放
	while( UrlKey_IsPlaying( param.url, param.key, vecTalkParam, true ) )
	{
		if( !ap.Play( failedDevlst ) )
		{
			break;
		}

		if( !failedDevlst.empty() )
		{
			if( m_ITalkNotify )
			{
				m_ITalkNotify->OnTalkNotify( XmppGSTalk::defTalkCmd_removedev, param.url, param.from_Jid, param.from_id, false, failedDevlst, true );
			}

			failedDevlst.clear();
		}

		if( !vecTalkParam.empty() )
		{
			g_OnProc_TalkParam( Play_id, ap, vecTalkParam, m_ITalkNotify );
			vecTalkParam.clear();
		}

		UrlKey_Check( false );
	}

	LOGMSG( "TalkMgr::OnProc(%d) APlayer.Close(%s), from_Jid=\"%s\", from_id=\"%s\"", Play_id, play_url.c_str(), param.from_Jid.c_str(), param.from_id.c_str() );
	ap.Close();

end_OnProc:

	UrlKey_Del( param.url, param.key );

	// 异步通知from_Jid播放已退出
	if( !param.from_Jid.empty() )
	{
		if( m_ITalkNotify )
		{
			const defvecDevKey c_vecdev;
			m_ITalkNotify->OnTalkNotify( XmppGSTalk::defTalkCmd_quit, param.url, param.from_Jid, param.from_id, false, c_vecdev );
		}
	}
}

// 创建播放线程
// isOnlyOneTalk: 只播放一个，此时先停止其它所有播放，再创建新播放
// doforce: 当已经在播放，或达到上限时，true表示强制覆盖建立新播放
defGSReturn GSRemoteTalkMgr::StartTalk( const std::string &url, const std::string &from_Jid, const std::string &from_id, const defvecDevKey &vecdev, const bool doforce, const bool isOnlyOneTalk )
{
	if( url.empty() )
	{
		LOGMSG( "TalkMgr::StartTalk url is null!" );
		return defGSReturn_Err;
	}

	UrlKey_Check( true );

	// 指定播放设备
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
				if( cam_ctl->isTalkUseCam() ) // 是否使用摄像头对讲
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

	 // 是否已经有在播放
	if( isPlaying(isOnlyOneTalk_cur) )
	{
		// 将要设置成只播放一个
		if( isOnlyOneTalk || isOnlyOneTalk_cur )
		{
			if( !doforce )
			{
				LOGMSG( "TalkMgr::StartTalk have isPlaying, failed!" );
				return defGSReturn_IsLock;
			}

			StopTalk_All();
		}
		else // 允许多个播放
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
		// 当前没有在播放，则直接播放
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
    int i,ret;  
    //ret=pthread_create(&id_1,NULL,APlayThread, this);  //jyc20170330 trouble
    if(ret!=0)  
    {  
        printf("Create timemanager_pthread error!\n");  
		return defGSReturn_Err;  
    } 

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

// 指定符合 url等参数的停止播放，只设置停止标志，不马上删除，线程停止后调用删除
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

// 都停止
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

// 保留一个，其它都停止
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
				// 第一个不停止
				isFoundPlay = true;
			}
		}
	}

	return true;
}

// 停止一个，其它都保留
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

// 是否有在播放
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

// 指定符合 url 的是否在播放
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

// 获取统计信息
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

// 存储新播放参数
bool GSRemoteTalkMgr::NewPlay_ParamPush( const struNewPlay_Param &param )
{
	gloox::util::MutexGuard mutexguard( m_mutex_TalkMgr );

	// 已存在正在执行的操作
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

// 取走新播放参数
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

// 添加一个播放记录
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

// 播放中增加命令
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

// 播放线程停止后执行删除播放记录，必须符合key一致。
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

// 指定符合 url 和 key 的是否在播放
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

// 检查是否只播放一个，如果是，则保留一个停止其它
void GSRemoteTalkMgr::UrlKey_Check( bool forceCheck )
{
	const uint32_t curtime = timeGetTime();
	gloox::util::MutexGuard mutexguard( m_mutex_TalkMgr );

	// 每间隔检测一次
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
			// 保留一个停止其它
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
						// 第一个不停止
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
