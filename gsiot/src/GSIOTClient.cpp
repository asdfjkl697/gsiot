#include "GSIOTClient.h"
#include "GSIOTInfo.h"
#include "GSIOTDevice.h"
#include "GSIOTControl.h"
#include "GSIOTDeviceInfo.h"
#include "GSIOTHeartbeat.h"


#include "XmppGSMessage.h"

#include <stdio.h>
#include <unistd.h>
#include <time.h>   


std::string g_IOTGetVersion()
{
	return std::string(GSIOT_VERSION);
}

std::string g_IOTGetBuildInfo()
{
	std::string str;
	str += __DATE__;
	str += " ";
	str += __TIME__;
	return str;
}

GSIOTClient::GSIOTClient( const std::string &RunParam )
	//: m_parser(this), m_PreInitState(false), xmppClient(NULL), m_running(false), timeCount(0), serverPingCount(0)
	: m_parser(this), m_PreInitState(false), m_cfg(NULL), xmppClient(NULL), m_running(false), timeCount(0), serverPingCount(0) //20160603
{
	this->PreInit( RunParam );
}

void GSIOTClient::PreInit( const std::string &RunParam )
{
    	//CoInitialize(NULL);
	
	m_cfg = new GSIOTConfig(); //20160604

	m_PreInitState = true;
	m_isThreadExit = true;
	m_isPlayBackThreadExit = true;
	m_isPlayMgrThreadExit = true;
	
}

void GSIOTClient::ResetNoticeJid()
{

}

void GSIOTClient::RunCodeInit()
{
	m_RunCodeMgr.Init();
}



void GSIOTClient::Stop(void)
{
	DWORD dwStart = ::timeGetTime();
	m_running = false;
	
	dwStart = ::timeGetTime();
	while( ::timeGetTime()-dwStart < 10*1000 )
	{
		if( m_isThreadExit 
			//&& m_isPlayBackThreadExit
			//&& m_isPlayMgrThreadExit
			)
		{
			break;
		}

		sleep(1);
	}

	//printf( "~GSIOTClient: thread exit wait usetime=%dms\r\n", ::timeGetTime()-dwStart );
}

GSIOTClient::~GSIOTClient(void)
{
	if(xmppClient){
		xmppClient->disconnect();
		xmppClient->removeStanzaExtension(ExtIot);
	   	xmppClient->removeIqHandler(this, ExtIot);
	   	xmppClient->removeStanzaExtension(ExtIotControl);
		xmppClient->removeIqHandler(this, ExtIotControl);
	   	xmppClient->removeStanzaExtension(ExtIotDeviceInfo);
       	xmppClient->removeIqHandler(this, ExtIotDeviceInfo);
	   	xmppClient->removeSubscriptionHandler(this);
	   	xmppClient->removeMessageHandler(this);
	   	xmppClient->removeIqHandler(this,ExtPing);
	   	delete(xmppClient);
	}
	
	//CoUninitialize();
}

void GSIOTClient::onConnect()
{
	printf( "GSIOTClient::onConnect\r\n" );
}

void GSIOTClient::onDisconnect( ConnectionError e )
{
	printf( "GSIOTClient::onDisconnect(err=%d)\r\n", e );

}

bool GSIOTClient::onTLSConnect( const CertInfo& info )
{
	printf( "GSIOTClient::onTLSConnect\r\n" );

	return true;
}

void GSIOTClient::handleMessage( const Message& msg, MessageSession* session)
{
	std::string subject = msg.subject();
	std::string body = msg.body();
	
	if(body == "help"){
	    
	}
}

void GSIOTClient::handleIqID( const IQ& iq, int context )
{
}

bool GSIOTClient::handleIq( const IQ& iq )
{
	printf( "rec message form server!!\n" );
	if( iq.from() == this->xmppClient->jid() )
	{
#ifdef _DEBUG
		//LOGMSGEX( defLOGNAME, defLOG_ERROR, "handleIq iq.from() == this->jid()!!!" );
#endif
		return true;
	}

	printf("IQ::Get=%d\n",IQ::Get);
	printf("IQ::Set=%d\n",IQ::Set);
	printf("IQ::Result=%d\n",IQ::Result);
	printf("iq.subtype=%d\n",iq.subtype());

	switch( iq.subtype() ){
        	case IQ::Get:
			{
				// 与服务器的心跳总是通过
				const StanzaExtension *Ping= iq.findExtension(ExtPing);
				if(Ping){
					XmppPrint( iq, "handleIq recv" );

					if(iq.from().full() == XMPP_SERVER_DOMAIN){
						serverPingCount++;
					}
				    return true;
				}

				/*author control*/
				GSIOTUser *pUser = m_cfg->m_UserMgr.check_GetUser( iq.from().bare() );
				this->m_cfg->FixOwnerAuth(pUser);

				XmppGSAuth_User *pExXmppGSAuth_User = (XmppGSAuth_User*)iq.findExtension(ExtIotAuthority_User);
				if( pExXmppGSAuth_User )
				{
					handleIq_Get_XmppGSAuth_User( pExXmppGSAuth_User, iq, pUser );
					return true;
				}
#if defined(defTest_defCfgOprt_GetSelf)
				XmppGSAuth *pExXmppGSAuth_Test = (XmppGSAuth*)iq.findExtension(ExtIotAuthority);
				if( pExXmppGSAuth_Test )
				{
					handleIq_Get_XmppGSAuth( pExXmppGSAuth_Test, iq, pUser );
					return true;
				}
#endif
				/*defGSReturn ret = m_cfg->m_UserMgr.check_User(pUser);
				if( macGSFailed(ret) )
				{
					//LOGMSGEX( defLOGNAME, defLOG_INFO, "(%s)IQ::Get: Not found userinfo. no auth.", iq.from().bare().c_str() );

					IQ re( IQ::Result, iq.from(), iq.id());
					re.addExtension( new XmppGSResult( "all", defGSReturn_NoAuth ) );
					XmppClientSend(re,"handleIq Send(all Get ACK)");
					return true;
				}*/
				
				GSIOTInfo *iotInfo = (GSIOTInfo *)iq.findExtension(ExtIot);
				if(iotInfo){
					
					//GSIOTDevice *iotdevice = new GSIOTDevice (23,"testdev23",IOT_DEVICE_RS485,"","","",NULL);
					GSIOTDevice *iotdevice = new GSIOTDevice ();
					iotdevice->setControl (NULL);
					iotdevice->setId ( 27 );
					iotdevice->setName ( "testdev29" );
					iotdevice->setType (IOT_DEVICE_RS485);
					iotdevice->setVer ("v1.0");

					IotDeviceList.push_back(iotdevice); //20160606 test
					GSIOTDevice *iotdevice2 = new GSIOTDevice (27,"testdev27",IOT_DEVICE_Remote,"","","",NULL);
					IotDeviceList.push_back(iotdevice2); //20160606 test
														
					std::list<GSIOTDevice *> tempDevGetList;
					//tempDevGetList.push_back(iotdevice); //20160612 test
					std::list<GSIOTDevice *>::const_iterator	it = IotDeviceList.begin();
							
					for(;it!=IotDeviceList.end();it++)
					{
						GSIOTDevice *pTempDev = (*it);
						printf("test1122\n");
						if( !iotInfo->isAllType() )
						{
							if( !iotInfo->isInGetType( pTempDev->getType() ) )
							{
								continue;
							}
						}

						if( !pTempDev->GetEnable() )
						{
							continue;
						}

						/*/20160604
						defUserAuth curAuth = m_cfg->m_UserMgr.check_Auth( pUser, pTempDev->getType(), pTempDev->getId() );

						if( GSIOTUser::JudgeAuth( curAuth, defUserAuth_RO ) )
						{
							tempDevGetList.push_back(pTempDev);
						}*/
						tempDevGetList.push_back(pTempDev); //20160608 test
					}
					

					
					IQ re( IQ::Result, iq.from(), iq.id());
					re.addExtension(new GSIOTInfo(tempDevGetList));
					//re.addExtension(new GSIOTInfo(IotDeviceList,true)); //20160614
						
					XmppClientSend(re,"handleIq Send(Get ExtIot ACK)");

					tempDevGetList.clear();
					return true;
				}
				
				
				printf("test21\n");
				GSIOTDeviceInfo *deviceInfo = (GSIOTDeviceInfo *)iq.findExtension(ExtIotDeviceInfo);
				if(deviceInfo){
					printf("test211\n");
					GSIOTDevice *device = deviceInfo->GetDevice();
					if(device){
						std::list<GSIOTDevice *>::const_iterator it = IotDeviceList.begin();
						for(;it!=IotDeviceList.end();it++){
							if((*it)->getId() == device->getId() && (*it)->getType() == device->getType()){

								if( !(*it)->GetEnable() )
								{
									IQ re( IQ::Result, iq.from(), iq.id() );
									re.addExtension( new XmppGSResult( XMLNS_GSIOT_DEVICE, defGSReturn_NoExist ) );
									XmppClientSend( re, "handleIq Send(Get ExtIotDeviceInfo ACK)" );
									return true;
								}
								
								defUserAuth curAuth = m_cfg->m_UserMgr.check_Auth( pUser, device->getType(), device->getId() );

								if( !GSIOTUser::JudgeAuth( curAuth, defUserAuth_RO ) )
								{
									//LOGMSGEX( defLOGNAME, defLOG_INFO, "(%s)IQ::Get ExtIotDeviceInfo: no auth., curAuth=%d, devType=%d, devID=%d", iq.from().bare().c_str(), curAuth, device->getType(), device->getId() );

									IQ re( IQ::Result, iq.from(), iq.id());
									re.addExtension( new XmppGSResult( XMLNS_GSIOT_DEVICE, defGSReturn_NoAuth ) );
									XmppClientSend(re,"handleIq Send(Get ExtIotDeviceInfo ACK)");
									return true;
								}

								if( deviceInfo->isShare() )
								{
									const defUserAuth guestAuth = m_cfg->m_UserMgr.check_Auth( m_cfg->m_UserMgr.GetUser(XMPP_GSIOTUser_Guest), device->getType(), device->getId() );
									curAuth = ( defUserAuth_RW==guestAuth ) ? defUserAuth_RW : defUserAuth_RO;
								}
								
								//找到设备,返回设备详细信息
								IQ re( IQ::Result, iq.from(), iq.id());
								re.addExtension(new GSIOTDeviceInfo(*it, curAuth, deviceInfo->isShare()?defRunCodeVal_Spec_Enable:0) );
								XmppClientSend(re,"handleIq Send(Get ExtIotDeviceInfo ACK)");
								printf("test219\n");
								return true;
							}
						}
					}
				    return true;
				}
				break;
			}
		case IQ::Set:
			{

			}
			break;

		case IQ::Result:
			{
				printf("test31\n");
				XmppGSMessage *pExXmppGSMessage = (XmppGSMessage*)iq.findExtension(ExtIotMessage);
				if( pExXmppGSMessage )
				{
					if( defGSReturn_Success == pExXmppGSMessage->get_state() )
					{
						EventNoticeMsg_Remove( pExXmppGSMessage->get_id() );
					}

					return true;
				}
			}
			break;
	}
	return true;
}

void GSIOTClient::handleSubscription( const Subscription& subscription )
{
	if(subscription.subtype() == Subscription::Subscribe){
		//xmppClient->rosterManager()->ackSubscriptionRequest(subscription.from(),true);
	}
}

void GSIOTClient::handleTag( Tag* tag )
{
	std::string strPath = getAppPath();

	Tag *tmgr = tag;
	if( tag->name() != "update" )
	{
		//LOGMSGEX( defLOGNAME, defLOG_INFO, "not fond update tag." );
		return;
	}

	if( tmgr->findChild("version") )
	{
		m_strVerNew = tmgr->findChild("version")->cdata();
	}
	else
	{
		//LOGMSGEX( defLOGNAME, defLOG_ERROR, "not get new ver!" );
		return;
	}

	m_retCheckUpdate = true;

	//LOGMSGEX( defLOGNAME, defLOG_INFO, "localver=%s, newver=%s", m_strVerLocal.c_str(), m_strVerNew.c_str() );
}

void GSIOTClient::OnTimer( int TimerID )
{
	if( !m_running )
		return ;

	if( 4 == TimerID )
	{
		xmppClient->whitespacePing();
		return ;
	}
	
	if( 1 != TimerID )
		return ;

	char strState_xmpp[256] = {0};
	gloox::ConnectionState state = xmppClient->state();
	switch( state )
	{
	case StateDisconnected:
		snprintf( strState_xmpp, sizeof(strState_xmpp), "xmpp curstate(%d) Disconnected", state );
		this->m_xmppReconnect = true;
		break;

	case StateConnecting:
		snprintf( strState_xmpp, sizeof(strState_xmpp), "xmpp curstate(%d) Connecting", state );
		break;

	case StateConnected:
		snprintf( strState_xmpp, sizeof(strState_xmpp), "xmpp curstate(%d) Connected", state );
		break;

	default:
		snprintf( strState_xmpp, sizeof(strState_xmpp), "xmpp curstate(%d)", state );
		break;
	}
	
	printf( "Heartbeat: %s\r\n", strState_xmpp );

	timeCount++;
	if(timeCount>10){

		printf( "GSIOT Version %s (build %s)\r\n", g_IOTGetVersion().c_str(), g_IOTGetBuildInfo().c_str() );

		//5分钟内检测一次服务器连接
		xmppClient->whitespacePing();
		if(serverPingCount==0){
			printf( "xmppClient serverPingCount=0\r\n" );
			//this->m_xmppReconnect = true;
		}
		serverPingCount = 0;
	    timeCount = 0;
	}

}

std::string GSIOTClient::GetConnectStateStr() const
{
	if( !xmppClient )
	{
		return std::string("未注册服务");
	}

	switch( xmppClient->state() )
	{
	case StateDisconnected:
		return std::string("连接中断");

	case StateConnecting:
		return std::string("连接中");

	case StateConnected:
		return std::string("正常");

	default:
		break;
	}

	return std::string("");
}

void GSIOTClient::Run()
{
	printf( "GSIOTClient::Run()\r\n" );
}

void GSIOTClient::Connect()
{
	printf( "GSIOTClient::Connect()\r\n" );
	/*
	std::string strmac = m_cfg->getSerialNumber();
	std::string strjid = m_cfg->getSerialNumber()+"@"+XMPP_SERVER_DOMAIN;

	if(!CheckRegistered()){
		m_cfg->setJid(strjid);
		m_cfg->setPassword(getRandomCode());
		XmppRegister *reg = new XmppRegister(m_cfg->getSerialNumber(),m_cfg->getPassword());
		reg->start();
		bool state = reg->getState();
		delete(reg);
		if(!state){	
			printf( "GSIOTClient::Connect XmppRegister failed!!!" );
		    return;
		}

		m_cfg->SaveToFile();

		SetJidToServer( strjid, strmac );
	}
	*/

	/*推送流定时器*/
	//timer = new TimerManager();
	//timer->registerTimer(this,1,30);
	//timer->registerTimer(this,2,2);		// 通知检测
	//timer->registerTimer(this,3,15);	// 回放检测
	//timer->registerTimer(this,4,60);	// 间隔发ping
	//timer->registerTimer(this,5,300);	// check system
	
	JID jid("000c2988989d@gsss.cn");
	jid.setResource("gsiot");
	xmppClient = new Client(jid,"cfqgleukxk");
	//注册物联网协议
	xmppClient->disco()->addFeature(XMLNS_GSIOT);
	xmppClient->disco()->addFeature(XMLNS_GSIOT_CONTROL);
	xmppClient->disco()->addFeature(XMLNS_GSIOT_DEVICE);

	xmppClient->registerStanzaExtension(new GSIOTInfo());
	xmppClient->registerStanzaExtension(new GSIOTControl());
	xmppClient->registerStanzaExtension(new GSIOTDeviceInfo());
	xmppClient->registerIqHandler(this, ExtIot);
	xmppClient->registerIqHandler(this, ExtIotControl);
	xmppClient->registerIqHandler(this, ExtIotDeviceInfo);

	xmppClient->registerConnectionListener( this );
	//订阅请求直接同意
	xmppClient->registerSubscriptionHandler(this);
	//消息帮助
	xmppClient->registerMessageHandler(this);
	//服务器心跳
	xmppClient->registerIqHandler(this,ExtPing);
	
	m_running = true;
	m_isThreadExit = false;
	
	unsigned long reconnect_tick = timeGetTime();
	printf( "GSIOTClient Running...\r\n\r\n" );
		
	while(m_running){
		//hbGuard.alive();
#if 1
		ConnectionError ce = ConnNoError;
		if( xmppClient->connect( false ) )
		{
			m_xmppReconnect = false;
			while( ce == ConnNoError && m_running )
			{
				//hbGuard.alive();
				if( m_xmppReconnect )
				{
					printf( "m_xmppReconnect is true, disconnect\n" );
					xmppClient->disconnect();
					break;
				}

				ce = xmppClient->recv(1000);

			}
			printf( "xmppClient->recv() return %d, m_xmppReconnect=%s\n", ce, m_xmppReconnect?"true":"false" );
		}
#else
	    xmppClient->connect(); // 阻塞式连接
#endif

		uint32_t waittime= 10000;//RUNCODE_Get(defCodeIndex_xmpp_ConnectInterval);

		if( waittime<6000 )
			waittime=6000;

		const unsigned long prev_span = timeGetTime()-reconnect_tick;
		if( prev_span > waittime*5 )
		{
			waittime=500;
		}
		reconnect_tick = timeGetTime();

		printf( ">>>>> xmppClient->connect() return. waittime=%d, prev_span=%lu\r\n", waittime, prev_span );

		DWORD dwStart = ::timeGetTime();
		while( m_running && ::timeGetTime()-dwStart < waittime )
		{
			sleep(1);
		}
	}
	m_isThreadExit = true;
}

//bool GSIOTClient::CheckRegistered()
//{
//	if(m_cfg->getJid().empty() || m_cfg->getPassword().empty()){
//		return false;
//	}
//	return true;
//}

/*
bool GSIOTClient::SetJidToServer( const std::string &strjid, const std::string &strmac )
{
	char chreq_setjid[256] = {0};
	snprintf( chreq_setjid, sizeof(chreq_setjid), "api.gsss.cn/gsiot.ashx?method=SetJID&jid=%s&mac=%s", strjid.c_str(), strmac.c_str() );
	httpreq::Request req_setjid;
	std::string psHeaderSend;
	std::string psHeaderReceive;
	std::string psMessage;
	if( req_setjid.SendRequest( false, chreq_setjid, psHeaderSend, psHeaderReceive, psMessage ) )
	{
		printf( "SetJID to server send success. HeaderReceive=\"%s\", Message=\"%s\"", UTF8ToASCII(psHeaderReceive).c_str(), UTF8ToASCII(psMessage).c_str() );
		return true;
	}

	printf( "SetJID to server send failed." );
	return false;
}
*/

void GSIOTClient::XmppClientSend( const IQ& iq, const char *callinfo )
{
	if( xmppClient )
	{
		XmppPrint( iq, callinfo );
		xmppClient->send( iq );
	}
}

void GSIOTClient::XmppPrint( const Message& msg, const char *callinfo )
{
	XmppPrint( msg.tag(), callinfo, NULL );
}

void GSIOTClient::XmppPrint( const IQ& iq, const char *callinfo )
{
	XmppPrint( iq.tag(), callinfo, NULL, false); //20160612 add false
}

void GSIOTClient::XmppPrint( const Tag *ptag, const char *callinfo, const Stanza *stanza, bool dodel )
{
	std::string strxml;
	if( ptag )
	{
		strxml = ptag->xml();
		//strxml = UTF8ToASCII( strxml );
	}
	else
	{
		strxml = "<no tag>";
	}
	printf( "GSIOT %s from=\"%s\", xml=\"%s\"\r\n", callinfo?callinfo:"", stanza?stanza->from().full().c_str():"NULL", strxml.c_str() );
	if( ptag && dodel )
	{
		delete ptag; //20160608
	}
}

void GSIOTClient::handleIq_Get_XmppGSAuth_User( const XmppGSAuth_User *pExXmppGSAuth_User, const IQ& iq, const GSIOTUser *pUser )
{
	defmapGSIOTUser mapUserDest;
	const defmapGSIOTUser &mapUser = m_cfg->m_UserMgr.GetList_User();

	if( defCfgOprt_GetSelf == pExXmppGSAuth_User->GetMethod()
#if defined(defTest_defCfgOprt_GetSelf)
		|| true //--temptest
#endif
		)
	{

		const std::string selfJid = iq.from().bare();

		GSIOTUser *pUserRet = NULL;
		const GSIOTUser *pUserSelf = m_cfg->m_UserMgr.GetUser( selfJid );
		if( pUserSelf )
		{
			pUserRet = pUserSelf->clone();
		}
		else
		{
			const GSIOTUser *pUserGuest = m_cfg->m_UserMgr.GetUser( XMPP_GSIOTUser_Guest );
			if( pUserGuest )
			{

				pUserRet = pUserGuest->clone();
				pUserRet->SetName( "guest" );
			}
			else
			{
				pUserRet = new GSIOTUser();
				pUserRet->SetJid(selfJid);
				pUserRet->SetID(0);
				pUserRet->SetName( "(un add user)" );
				pUserRet->SetEnable(defDeviceDisable);
			}
		}

		if( pUserRet )
		{
			this->m_cfg->FixOwnerAuth(pUserRet);

			pUserRet->RemoveUnused();
			GSIOTUserMgr::usermapInsert( mapUserDest, pUserRet );
		}
		else
		{
			return;
		}
	}
	else
	{
		const defUserAuth curAuth = m_cfg->m_UserMgr.check_Auth( pUser, IOT_Module_authority, defAuth_ModuleDefaultID );
		if( !GSIOTUser::JudgeAuth( curAuth, defUserAuth_RO ) )
		{
			IQ re( IQ::Result, iq.from(), iq.id());
			re.addExtension( new XmppGSResult( XMLNS_GSIOT_AUTHORITY_USER, defGSReturn_NoAuth ) );
			XmppClientSend(re,"handleIq Send(Get ExtIotAuthority_User ACK)");
			return ;
		}

		const std::string keyjid_owner = this->m_cfg->GetOwnerKeyJid();

		const defmapGSIOTUser& needGetUser = pExXmppGSAuth_User->GetList_User();

		for( defmapGSIOTUser::const_iterator it=mapUser.begin(); it!=mapUser.end(); ++it )
		{
			const GSIOTUser *pUser = it->second;
			const std::string keyjid_user = pUser->GetKeyJid();

			if( needGetUser.find( keyjid_user ) != needGetUser.end() )
			{
				GSIOTUser *pUserRet = pUser->clone();

				this->m_cfg->FixOwnerAuth(pUserRet);

				if( defCfgOprt_GetSimple == pExXmppGSAuth_User->GetMethod()
					|| this->m_cfg->isOwnerForKeyJid(keyjid_owner,keyjid_user) )
				{
					pUserRet->RemoveUnused( true );
				}

				GSIOTUserMgr::usermapInsert( mapUserDest, pUserRet );
			}
		}
	}

	IQ re( IQ::Result, iq.from(), iq.id());
	re.addExtension( new XmppGSAuth_User(pExXmppGSAuth_User->GetSrcMethod(), mapUserDest, struTagParam(), true) );
	XmppClientSend(re,"handleIq Send(Get ExtIotAuthority_User ACK)");
}

void GSIOTClient::EventNoticeMsg_Remove( const std::string &id )
{
	/*
	gloox::util::MutexGuard mutexguard( m_mutex_lstEventNoticeMsg );

	std::map<std::string,struEventNoticeMsg*>::iterator it = m_lstEventNoticeMsg.find( id );
	if( it != m_lstEventNoticeMsg.end() )
	{
		delete it->second;
		m_lstEventNoticeMsg.erase( it );
	}*/
}

