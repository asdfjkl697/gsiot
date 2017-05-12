#include "GsCamera.h"


/*静态函数*/
void GsCamera::InitSDK( ICameraHandler *handler )
{
//	g_ICameraAlarmRecv = hAlarmRecv;
//
//	defNS_HIK::NET_DVR_Init();
//    //defNS_HIK::NET_DVR_SetDVRMessageCallBack_V30(MessageCallback,(unsigned long)handler);
//	defNS_HIK::NET_DVR_SetDVRMessageCallBack_V30(MessageCallback,NULL);
//	defNS_HIK::NET_DVR_SetConnectTime(3000, 3);
//	defNS_HIK::NET_DVR_SetReconnect( 10000, TRUE );
//
//	//---------------------------------------
//	//设置异常消息回调函数
//	defNS_HIK::NET_DVR_SetExceptionCallBack_V30(0, NULL,g_ExceptionCallBack, NULL);
}

void GsCamera::Cleanup()
{
	//defNS_HIK::NET_DVR_Cleanup();
}

bool GsCamera::GetChannelNum( char *sDVRIP, uint16_t wDVRPort, char *sUserName, char *sPassword, uint8_t &byChanNum, uint8_t &byStartChan, std::string &strErr )
{
	char chbuf[256] = {0};

	byChanNum = 0;
	byStartChan = 0;
	strErr = "";



	return true;
}

bool GsCamera::GetCameraDevInfo(
	char *sDVRIP, uint16_t wDVRPort, char *sUserName, char *sPassword, const std::string &devname,
	std::string &strCamSerialNo, defvecChInfo &vecChInfo, std::string &strErr, const uint32_t streamfmt )
{
	char chbuf[256] = {0};

	strErr = "";



	return true;
}

defGSReturn GsCamera::SetCamTime_Spec( const struGSTime &newtime, const std::string &name, const std::string &sIP, uint16_t wPort, const std::string &sUserName, const std::string &sPassword )
{
	defGSReturn gsret = defGSReturn_Err;

	return gsret;
}

defGSReturn GsCamera::SetCamTime_ForLoginID( const struGSTime &newtime, const std::string &name, const long LoginID, const std::string &sIP, const uint16_t wPort )
{
	defGSReturn gsret = defGSReturn_Err;

	return defGSReturn_ConnectObjErr;
}

GsCamera::GsCamera(const std::string& deviceID, const std::string& name,
		const std::string& ipaddress,uint32_t port,const std::string& username,
		const std::string& password, const std::string& ver, GSPtzFlag PTZFlag, GSFocalFlag FocalFlag,
		uint32_t channel, uint32_t streamfmt )
		:IPCameraBase(deviceID, name,ipaddress, port, username,
		password, ver, PTZFlag, FocalFlag, channel, streamfmt )
{
	this->m_camera_hwnd = -1;
	this->m_playhandle = -1;
	this->m_AlarmChan = -1;
	this->m_isFirstAudio = true;
	this->m_AudioEncType = -1;
	this->m_pDecHandle = (void*)-1;

	this->m_lVoiceComHandle = -1;

	//memset( &m_sptr, 0, sizeof(m_sptr) );
	//memset( &m_frame, 0, sizeof(m_frame) );

	m_timeMakeKeyFrame = timeGetTime()-60000;
}

GsCamera::~GsCamera(void)
{

}

void GsCamera::PreinitBufCheck()
{

}

bool GsCamera::TestConnect( std::string &strErr )
{


	return true;
}

defGSReturn GsCamera::SetCamTime( const struGSTime &newtime )
{
	this->Login( false );

	return SetCamTime_ForLoginID( newtime, m_name, m_camera_hwnd, this->ConnUse_ip(), this->ConnUse_port() );
}

defGSReturn GsCamera::Login( bool playback )
{


	return defGSReturn_Success;
}

// 连接建立告警通道
void GsCamera::ConnectAlarmGuard()
{

}

// 连接建立告警通道
void GsCamera::DisconnectAlarmGuard()
{

}

void GsCamera::Logout()
{

}

defGSReturn GsCamera::Connect( bool reconnectAudio, void* RealPlayWnd, bool playback, const struGSTime *pdtBegin, const struGSTime *pdtEnd )
{

	return defGSReturn_Success;
}

bool GsCamera::isSupportAudioEncType( int AudioEncType )
{

	return false;
}

bool GsCamera::InitAudioEncType()
{
	return true;
}

defGSReturn GsCamera::SetChannelStream(int channel, bool reconnectAudio, void* RealPlayWnd)
{
	return defGSReturn_Success;
}

defGSReturn GsCamera::PlayBack( void* RealPlayWnd, const struGSTime *pdtBegin, const struGSTime *pdtEnd )
{
	return defGSReturn_Success;
}

defGSReturn GsCamera::QuickSearchPlayback( const struGSTime *pdtBegin, const struGSTime *pdtEnd )
{
	if( !pdtBegin || !pdtEnd )
		return defGSReturn_Err;

	return defGSReturn_Err;
}

int GsCamera::PlayBackControl( GSPlayBackCode_ ControlCode, void *pInBuffer, uint32_t InLen, void *pOutBuffer, uint32_t *pOutLen )
{


	return -1;
}

void GsCamera::OnDisconnct()
{

}

bool GsCamera::IsConnect()
{
	if( m_camera_hwnd>=0 && m_playhandle>=0 )
	{
		return true;
	}

	return false;
}

void GsCamera::ResetBuf()
{

}

void GsCamera::SearchInit(ICameraHandler *handler)
{
}

void GsCamera::SearchStart()
{
}

void GsCamera::SearchStop()
{
}

bool GsCamera::DealwithAlarm(char* pBuf , unsigned long dwLen)
{
	return true;
}

// speed : 1-7
bool GsCamera::SendPTZ( GSPTZ_CtrlCmd command, int param1, int param2, int speed, const char *callinfo )
{


	return true;
}

int GsCamera::GetPTZState(GSPTZ_CtrlCmd command, bool refresh_device)
{


	return 0;
}

bool GsCamera::Set_motion_track( const bool enable, const bool srccall )
{


	return true;
}

bool GsCamera::Set_PTZ_ParkAction( const bool enable, const bool srccall )
{
	return true;
}

bool GsCamera::PresetControl( DWORD dwPTZPresetCmd, DWORD dwPresetIndex, const std::string &callinfo )
{
	return true;
}

void GsCamera::OnPublishStart()
{

}

void GsCamera::MakeKeyFrame( bool forcenow )
{
	if( !IsConnect() )
	{
		return;
	}
}

void GsCamera::OnData(LONG lRealHandle, DWORD dwDataType, BYTE *pBuffer, DWORD dwBufSize )
{

}

void GsCamera::doAudioSource_CameraReEnc( unsigned char *buffer, int size, const uint32_t sendFrameTS )
{

}

bool GsCamera::isTalkSupport() const
{
	return false;
}

bool GsCamera::OnTalkSend( defTalkSendCmd_ TalkSendCmd, unsigned char *pdata, uint32_t data_size )
{
	return true;
}

void GsCamera::check_NetUseable( bool *isChanged )
{

}

defGSReturn GsCamera::GetCamShowInfo( int &showtime, int &showname, std::string &name )
{
	return defGSReturn_Err;
}

defGSReturn GsCamera::SetCamShowInfo( const int showtime, const int showname, const char *pname )
{
	return defGSReturn_Err;
}

defGSReturn GsCamera::GetCamCompressCfg( std::string *cfginfo, struCamParamBuf *getCamParamBuf )
{
	return defGSReturn_Success;
}

// setuseCamParamBuf有值时，使用此覆盖整个结构进行设置
defGSReturn GsCamera::SetCamCompressCfg( const defCamCompress CamCompress, const struCamParamBuf *setuseCamParamBuf )
{
	return defGSReturn_Success;
}

bool GsCamera::CapturePic( char *buf, DWORD bufsize, DWORD &outsize, DWORD PicSize )
{
	return false;
}

