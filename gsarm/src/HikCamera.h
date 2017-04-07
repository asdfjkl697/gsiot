#ifndef HikCamera_H
#define HikCamera_H

#include "common.h"
//#include "windows.h"  //jyc20170322 remove
#include "MediaStream.h"
#include "ICameraHandler.h"
#include "IPCameraBase.h"
//#include "mpegdemux.h" //jyc20170322 debug
//#include "BufferMerge.h"
#include "Audio_WAV2AAC.h"

#pragma comment(lib,"libmpeg.lib")
#pragma comment(lib,"HCNetSDK.lib")

class HikCamera:public IPCameraBase, public IPlayBackControl
{
private:
	defGSReturn Login( bool playback );
	void Logout();
	defGSReturn SetChannelStream(int channel, bool reconnectAudio, void* RealPlayWnd=NULL);
	defGSReturn PlayBack( void* RealPlayWnd, const struGSTime *pdtBegin=NULL, const struGSTime *pdtEnd=NULL );
	bool InitAudioEncType();
	static bool isSupportAudioEncType( int AudioEncType );

public:
	HikCamera(const std::string& deviceID, const std::string& name,
		const std::string& ipaddress,uint32_t port,const std::string& username,
		const std::string& password, const std::string& ver, GSPtzFlag PTZFlag, GSFocalFlag FocalFlag,
		uint32_t channel=1, uint32_t streamfmt=0 );
	~HikCamera(void);
	
	IPCameraType GetCameraType() const {
		return CameraType_hik;
	};

	static void InitSDK( ICameraHandler *handler, ICameraAlarmRecv *hAlarmRecv );
	static void Cleanup();

    static void SearchInit(ICameraHandler *handler);
	static void SearchStart();
	static void SearchStop();
	static bool GetChannelNum( char *sDVRIP, uint16_t wDVRPort, char *sUserName, char *sPassword, uint8_t &byChanNum, uint8_t &byStartChan, std::string &strErr );
	static bool GetCameraDevInfo( 
		char *sDVRIP, uint16_t wDVRPort, char *sUserName, char *sPassword, const std::string &devname, 
		std::string &strCamSerialNo, defvecChInfo &vecChInfo, std::string &strErr, const uint32_t streamfmt=0 );
	static defGSReturn SetCamTime_Spec( const struGSTime &newtime, const std::string &name, const std::string &sIP, uint16_t wPort, const std::string &sUserName, const std::string &sPassword );
	static defGSReturn SetCamTime_ForLoginID( const struGSTime &newtime, const std::string &name, const long LoginID, const std::string &sIP, const uint16_t wPort );
	
	virtual defGSReturn GetCamShowInfo( int &showtime, int &showname, std::string &name );
	virtual defGSReturn SetCamShowInfo( const int showtime, const int showname, const char *pname );
	
	virtual defGSReturn GetCamCompressCfg( std::string *cfginfo, struCamParamBuf *getCamParamBuf=NULL );
	virtual defGSReturn SetCamCompressCfg( const defCamCompress CamCompress, const struCamParamBuf *setuseCamParamBuf=NULL );

	void PreinitBufCheck();
	bool DealwithAlarm(char* pBuf, unsigned long dwLen);
	bool IsConnect();
	bool TestConnect( std::string &strErr );
	virtual defGSReturn SetCamTime( const struGSTime &newtime );
	defGSReturn Connect( bool reconnectAudio, void* RealPlayWnd=NULL, bool playback=false, const struGSTime *pdtBegin=NULL, const struGSTime *pdtEnd=NULL );
	int PlayBackControl( GSPlayBackCode_ ControlCode, void *pInBuffer = NULL, uint32_t InLen = 0, void *pOutBuffer = NULL, uint32_t *pOutLen = NULL );
	void OnDisconnct();
	void ResetBuf();
	bool SendPTZ( GSPTZ_CtrlCmd command, int param1=0, int param2=0, int speed=0, const char *callinfo=NULL );
	int GetPTZState(GSPTZ_CtrlCmd command, bool refresh_device);
	bool Set_motion_track( const bool enable, const bool srccall=true );
	bool Set_PTZ_ParkAction( const bool enable, const bool srccall=true );

	void OnPublishStart();
	void MakeKeyFrame( bool forcenow=true );

	bool isTalkSupport() const;
	bool OnTalkSend( defTalkSendCmd_ TalkSendCmd, unsigned char *pdata, uint32_t data_size );

	void OnData( LONG lRealHandle, DWORD dwDataType, BYTE *pBuffer, DWORD dwBufSize );

	void check_NetUseable( bool *isChanged=NULL );
	
	defGSReturn QuickSearchPlayback( const struGSTime *pdtBegin=NULL, const struGSTime *pdtEnd=NULL );

	virtual bool SetAGRunState( bool AGRunState, bool onlyvalue=false ) // ��������������ø澯����״̬
	{
		if( onlyvalue )
		{
			m_AGRunState = AGRunState;	// �޸Ĳ�����־
			return true;
		}

		// ���������ñ�Ϊ����ʱ���澯����
		if( m_AGRunState != AGRunState && !AGRunState  )
		{
			this->SetCurAlarmState( defAlarmState_UnInit );
		}

		m_AGRunState = AGRunState;	// �޸Ĳ�����־

		if( m_AGRunState )
		{
			this->Login(false); //this->ConnectAlarmGuard();		// ����ʱ�����Ӹ澯ͨ��
		}
		else
		{
			this->DisconnectAlarmGuard();	// ����ʱ���Ͽ��澯ͨ��
		}

		return true;
	}
	virtual bool IsAGRunStateFinished() const // ����������Ƿ���ɣ�����������־�͸澯ͨ��
	{
		return ( m_AlarmChan>=0 && GetAGRunState() );
	}
	
	bool CapturePic( char *buf, DWORD bufsize, DWORD &outsize, DWORD PicSize );

protected:
	bool PresetControl( DWORD dwPTZPresetCmd, DWORD dwPresetIndex, const std::string &callinfo );

private:
	//MpegDemux m_demux;  //jyc20170322 remove
	//mpeg2ps_stream_t m_sptr;
	//MPEGPESFrame m_frame;
	//CBufferMerge m_bufMerge;
	bool m_isFirstAudio;
	
	int m_AudioEncType;
	void *m_pDecHandle;
	Audio_WAV2AAC m_accen;
	//CBufferMerge m_accen_bufMerge;
	void doAudioSource_CameraReEnc( unsigned char *buffer, int size, const uint32_t sendFrameTS );

	LONG m_lVoiceComHandle;
	//CBufferMerge m_talkde_bufMerge;

	gloox::util::Mutex m_mutex;

	uint32_t m_timeMakeKeyFrame;

	void ConnectAlarmGuard(); // �����澯ͨ��
	void DisconnectAlarmGuard(); // ����澯ͨ��
	long m_AlarmChan; // �澯ͨ��
};

#endif

