#ifndef GSIOTCLIENT_H
#define GSIOTCLIENT_H

#include "gloox/client.h"
#include "gloox/connectionlistener.h"
#include "gloox/adhoccommandprovider.h"
#include "gloox/disco.h"
#include "gloox/adhoc.h"
#include "gloox/tag.h"
#include "gloox/loghandler.h"
#include "gloox/logsink.h"

#include "gloox/subscriptionhandler.h"
#include "gloox/messagehandler.h"
#include "gloox/message.h"


#include "gloox/iqhandler.h" //20160603
#include "GSIOTDevice.h" //20160603
#include "GSIOTConfig.h" //20160603

#include "XmppGSResult.h"
#include "XmppGSAuth.h"
#include "XmppGSState.h"

#include "RunCode.h"
#include "DataStoreMgr.h"

#include "typedef.h"

#define LOGMSG printf

using namespace gloox;

std::string g_IOTGetVersion();
std::string g_IOTGetBuildInfo();


class GSIOTClient: public IqHandler, SubscriptionHandler, ConnectionListener,
	MessageHandler, public TagHandler
{
private:
	GSIOTConfig *m_cfg; 
	std::list<GSIOTDevice *> IotDeviceList; //20160603
	//DeviceConnection *deviceClient;
	bool serverPingCount;
	int timeCount;
	gloox::Client *xmppClient;
private:
	void Register();

public:
	GSIOTClient( const std::string &RunParam );
	~GSIOTClient(void);
	
	void RunCodeInit();
	CRunCodeMgr m_RunCodeMgr;

	void PreInit( const std::string &RunParam );
	bool GetPreInitState() const
	{
		return m_PreInitState;
	}
	

	/*xmpp handler*/
	virtual void onConnect();
	virtual void onDisconnect( ConnectionError e );
	virtual bool onTLSConnect( const CertInfo& info );
	void handleIqID( const IQ& iq, int context );
	bool handleIq( const IQ& iq );
	void handleSubscription( const Subscription& subscription );
	void handleMessage( const Message& msg, MessageSession* session = 0 );
	virtual void handleTag( Tag* tag );

	void handleIq_Get_XmppGSAuth_User( const XmppGSAuth_User *pExXmppGSAuth_User, const IQ& iq, const GSIOTUser *pUser );

	// --------------------------
public:
	static void XmppPrint( const Tag *ptag, const char *callinfo, const Stanza *stanza, bool dodel=true );
	static void XmppPrint( const IQ& iq, const char *callinfo );
	static void XmppPrint( const Message& msg, const char *callinfo );
	void XmppClientSend( const IQ& iq, const char *callinfo );
	void XmppClientSend_msg( const JID &to_jid, const std::string &strBody, const std::string &strSubject, const char *callinfo );

	/*timer*/
	void OnTimer( int TimerID );
	std::string GetConnectStateStr() const;

	void Run();
	void Connect();
	void Stop(void);
	bool CheckRegistered();
	void LoadConfig();
	bool SetJidToServer( const std::string &strjid, const std::string &strmac );
	void ResetNoticeJid();
	
private:
	bool m_PreInitState;
	bool m_running;

	time_t m_IOT_starttime;
	std::string m_str_IOT_starttime;
	bool m_xmppReconnect;

	Parser m_parser;
	std::string m_strVerLocal;
	std::string m_strVerNew;
	bool m_retCheckUpdate;
	bool m_isThreadExit;
	bool m_isPlayBackThreadExit;
	bool m_isPlayMgrThreadExit;

public:

	void EventNoticeMsg_Remove( const std::string &id );
};

#endif
