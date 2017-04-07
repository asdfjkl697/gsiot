#ifndef IPCAMERABASE_H_
#define IPCAMERABASE_H_

#include "gloox/jid.h"
#include "RTMPSession.h"
#include "MediaStream.h"
#include "CameraControl.h"
#include "IPublishHandler.h"
#include "gloox/mutexguard.h"
#include "AudioCapBase.h"
#include <bitset>
#include "PresetManager.h"

//无效通道号
#define defInvalidCamCh		0
#define isInvalidCamCh(ch)	(defInvalidCamCh==ch)

// 视频编码参数质量标准设置等级
enum defCamCompress
{
	defCamCompress_720PLow = 1,		// 720P 低
	defCamCompress_720PNorm,		// 720P 适中
	defCamCompress_720PRelaHigh,	// 720P 较好
	defCamCompress_720PHigh,		// 720P 好
	defCamCompress_1080PLow,		// 1080P 低
	defCamCompress_1080PNorm,		// 1080P 中
};

// 摄像机参数通用缓存，用于将一个复制给另一个中间转换时使用，等等
struct struCamParamBuf
{
	IPCameraType CamType;	// 摄像机类型
	uint32_t ParamType;		// 根据不同摄像机类型参数类型内部区分
	uint16_t ParamLen;
	uint8_t *ParamBuf;

	struCamParamBuf()
	{
		CamType= CameraType_Unkown;
		ParamType = 0;
		ParamLen = 0;
		ParamBuf = NULL;
	}

	~struCamParamBuf()
	{
		ReleaseBuf();
	}

	void ReleaseBuf()
	{
		if( ParamLen>0 && ParamBuf )
		{
			delete[]ParamBuf;
		}

		ParamLen = 0;
		ParamBuf = NULL;
	}

	bool IsValidBuf( const IPCameraType in_CamType, const uint32_t in_ParamType ) const
	{
		return ( in_CamType==CamType && in_ParamType==ParamType && ParamLen>0 && ParamBuf );
	}

	void SetBuf( const IPCameraType in_CamType, const uint32_t in_ParamType, const uint16_t in_ParamLen, const uint8_t *in_ParamBuf )
	{
		ReleaseBuf();

		CamType = in_CamType;
		ParamType = in_ParamType;
		ParamLen = in_ParamLen;
		ParamBuf = new uint8_t[ParamLen];
		memcpy( ParamBuf, in_ParamBuf, ParamLen );
	}

	bool GetBuf( const IPCameraType in_CamType, const uint32_t in_ParamType, const uint16_t in_ParamLen, uint8_t *in_ParamBuf ) const
	{
		if( !IsValidBuf( in_CamType, in_ParamType ) )
			return false;

		if( in_ParamLen < ParamLen )
			return false;

		//in_ParamLen = ParamLen;
		memcpy( in_ParamBuf, ParamBuf, in_ParamLen );
	}
};

struct struChInfo
{
	uint32_t channel;
	uint32_t streamfmt;
	std::string strCh;
	std::string name;
	std::string desc;

	struChInfo()
	{
		channel = -1;
		streamfmt = 0;
	}
};
typedef std::vector<struChInfo> defvecChInfo;

// 时间戳修正模式
enum defFixTSMod
{
	defFixTSMod_NoFix	= 0, // 不修正
	defFixTSMod_Manual	= 1, // 手动固定参数进行修正
	defFixTSMod_Auto	= 2  // 自动计算参数进行修正
};

static const char* s_name_defFixTSMod[] =
{
	"NoFix",
	"Manual",
	"Auto"
};


#define defDefault_CameraFixParm_UPPER	1000
#define defDefault_CameraFixParm_LOWER	10

// 绝对门限默认值，以10帧100为正常间隔
#define defDefault_CameraFixParm_span_Min	50
#define defDefault_CameraFixParm_span_Max	120
#define defDefault_CameraFixParm_fix_Min	40
#define defDefault_CameraFixParm_fix_Max	110
#define defDefault_CameraFixParm_spanWarn	350

//动态门限，正常值的百分比，例如120表示正常值的百分之120
#define defDefault_CameraFixParm_mfactor_span_Min	50
#define defDefault_CameraFixParm_mfactor_span_Max	120
#define defDefault_CameraFixParm_mfactor_fix_Min	40
#define defDefault_CameraFixParm_mfactor_fix_Max	110
#define defDefault_CameraFixParm_mfactor_spanWarn	500


// 视频采集修正参数，默认值见defDefault_CameraFixParm_
struct struCameraFixParm
{
	defFixTSMod	FixTSMod;

	uint32_t	parmFps; // 参数对应的fps值，未知写0
	uint32_t	lasttime_Recal; // 最后一次计算的时间

	// 手动模式时，设置固定值，自动模式时根据乘系数动态计算
	uint32_t	span_Min; // 帧间隔最大下限
	uint32_t	span_Max; // 帧间隔最大上限
	uint32_t	fix_Min;  // 帧间隔修正最小值
	uint32_t	fix_Max;  // 帧间隔修正最大值
	uint32_t	spanWarn; // 帧间隔比较大时打印警告日志，上限值

	// 自动模式时用于动态计算
	uint32_t	mfactor_span_Min; // 帧间隔最大下限乘系数百分比，例如120表示百分之120
	uint32_t	mfactor_span_Max; // 帧间隔最大上限乘系数百分比
	uint32_t	mfactor_fix_Min;  // 帧间隔修正最小值乘系数百分比
	uint32_t	mfactor_fix_Max;  // 帧间隔修正最大值乘系数百分比
	uint32_t	mfactor_spanWarn; // 帧间隔比较大时打印警告日志，上限值乘系数百分比

	void		*exparam; // 扩展参数

	struCameraFixParm(void)
	{
		Reset();
	}

	~struCameraFixParm(void)
	{
	}

	void Reset();
	void Recal( uint32_t cur_fps, uint32_t timespan, const char *loginfo=NULL, bool isForceRecal=false );
	void CheckParam();
	void print( const char *pinfo=NULL );
};

// 视频采集状态
struct struCameraState
{
	bool		isSetFirst;
	uint32_t	startTime;	// 开始时间
	uint32_t	sno;		// 帧序号计数，从收到的第一个帧开始计数，无论是否有效
	uint32_t	validPktCount;  // 有效帧的包总计数
	uint32_t	prev_FPS;	// 上一帧FPS
	uint32_t	prev_ts;	// 上一帧时间戳
	uint32_t	prev_tskey;	// 上一关键帧时间戳
	uint32_t	prev_time;	// 上一帧时间
	void		*exparam;	// 扩展参数

	struCameraState(void)
	{
		Reset();
	}

	~struCameraState(void)
	{
	}

	void Reset();
};

using namespace gloox;

class CRecCfg
{
public:
	CRecCfg(void) 
		: m_rec_mod(defRecMod_NoRec), m_rec_svrtype(CameraType_Unkown), m_rec_port(0), m_rec_channel(0)
	{};

	~CRecCfg(void){};


	void setrec_mod( defRecMod rec_mod )
	{
		this->m_rec_mod = rec_mod;
	}
	defRecMod getrec_mod() const
	{
		return this->m_rec_mod;
	}
	void setrec_svrtype( IPCameraType rec_svrtype )
	{
		this->m_rec_svrtype = rec_svrtype;
	}
	IPCameraType getrec_svrtype() const
	{
		return this->m_rec_svrtype;
	}
	const std::string& getrec_ip() const
	{
		return this->m_rec_ip;
	}
	void setrec_ip( const std::string &rec_ip )
	{
		m_rec_ip = rec_ip;
	}

	void setrec_port( uint32_t rec_port )
	{
		this->m_rec_port = rec_port;
	}
	uint32_t getrec_port() const
	{
		return this->m_rec_port;
	}

	const std::string& getrec_username() const
	{
		return this->m_rec_username;
	}
	void setrec_username( const std::string &rec_username )
	{
		m_rec_username = rec_username;
	}

	const std::string& getrec_password() const
	{
		return this->m_rec_password;
	}
	void setrec_password( const std::string &rec_password )
	{
		m_rec_password = rec_password;
	}

	void setrec_channel( uint32_t rec_channel )
	{
		this->m_rec_channel = rec_channel;
	}
	uint32_t getrec_channel() const
	{
		return this->m_rec_channel;
	}

protected:
	// 录像及录像机通道映射信息
	defRecMod m_rec_mod;
	IPCameraType m_rec_svrtype; // 录像机类型
	std::string m_rec_ip;
	uint32_t m_rec_port;
	std::string m_rec_username;
	std::string m_rec_password;
	uint32_t m_rec_channel;

private:
};


// 属性有效位位置
// 在m_aAttr中的位号
// 注：顺序与配置存储有关，版本确定后，只能往后增加，不能随便调整原有顺序
enum defCamAdvAttr
{
	defCamAdvAttr_Min_ = 0,
	defCamAdvAttr_motion_track = 0,		// 是否支持智能跟踪
	defCamAdvAttr_manual_trace,			// 是否支持手动跟踪
	defCamAdvAttr_manual_ptzsel,		// 是否支持手动定位
	defCamAdvAttr_CamTalk,				// 是否支持摄像机对讲
	defCamAdvAttr_AutoConnect,			// 是否保持自动连接
	defCamAdvAttr_PTZ_ParkAction,		// 是否支持守望功能
	defCamAdvAttr_PTZ_ParkActionKeep,	// 是否在空闲时保持守望
	defCamAdvAttr_PTZ_Preset,			// 是否支持预置点
	defCamAdvAttr_manual_zoomrng,		// 是否支持区域选择缩放
	defCamAdvAttr_SupportAlarm,			// 是否具有告警功能
	defCamAdvAttr_PTZ_Ang,				// 是否具有云台斜角方向控制功能
	defCamAdvAttr_AutoPublish,			// 是否保持发布/自动发布
	defCamAdvAttr_Max_
};

class CCamAdvAttr
{
public:
	CCamAdvAttr(void)
		: m_AdvAttribute(0)
	{};

	~CCamAdvAttr(void){};

	// 高级属性操作 m_AdvAttribute
	static bool isShow_AdvAttr( defCamAdvAttr attr );
	static const char* get_AdvAttr_Name( defCamAdvAttr attr );

	bool get_AdvAttr( defCamAdvAttr attr ) const;
	bool set_AdvAttr( defCamAdvAttr attr, bool val );
	uint32_t get_AdvAttr_uintfull() const;
	bool set_AdvAttr_uintfull( uint32_t fullval );

private:
	std::bitset<defCamAdvAttr_Max_> m_AdvAttribute;
};

class IPCameraBase:public IPublishHandler,public CameraControl, public AudioCapCallBack, public CPresetManager, public AGRunStateBase
{
protected:
	CRecCfg m_RecCfg;
	CAudioCfg m_AudioCfg;
	CCamAdvAttr m_AdvAttr;

	AudioCapBase* m_pLocalCapAudio;

	std::string m_name;
	std::string m_deviceID;

	uint32_t m_port;
	std::string m_ipaddress;
	std::string m_username;
	std::string m_password;

	uint32_t m_channel;
	uint32_t m_streamfmt;

	long m_camera_hwnd;
	long m_playhandle;
	bool m_isPlayBack;
	MediaStream *stream;

	struCameraFixParm m_fixparm;
	struCameraState m_state;
	
	uint32_t m_prev_tick_PrintFrame;
	uint32_t m_prev_tick_PrintFrame_Audio;

	// PrePic
	std::string m_PrePicChangeCode;

	// state
protected:
	BYTE m_EnableTrack;   //启用跟踪，0-否，1-是
	BYTE m_PTZ_ParkAction; // 启用守望，0-否，1-是
	
public:
	IPCameraBase(const std::string& deviceID, const std::string& name,
		const std::string& ipaddress,uint32_t port,const std::string& username,
		const std::string& password, const std::string& ver, GSPtzFlag PTZFlag, GSFocalFlag FocalFlag,
		uint32_t channel, uint32_t streamfmt );
	~IPCameraBase(void);

	// enable字段值拆解
	static void uncode_enable_col_save( const int enable_col_save, int &enable_val, int &AGRunState_val )
	{
		enable_val = enable_col_save & 0x01;					// 启用标志
		AGRunState_val = (enable_col_save & 0x02) >> 1;	//摄像机布防标志
	}

	// 合并获得enable字段值
	static int encode_enable_col_save( const int enable_val, const int AGRunState_val )
	{
		const int AGRunState_comb = (AGRunState_val & 0x01) << 1;
		const int enable_comb = enable_val & 0x01;

		return (AGRunState_comb+enable_comb);
	}

	bool doEditAttrFromAttrMgr( const EditAttrMgr &attrMgr, defCfgOprt_ oprt = defCfgOprt_Modify );

	virtual void OnAudioBufferCB( double SampleTime, unsigned char *pBuffer, long BufferLen );

	CRecCfg& GetRecCfg()
	{
		return m_RecCfg;
	};

	void UpdateReccfg( const CRecCfg& RecCfg )
	{
		m_RecCfg = RecCfg;
	};

	CAudioCfg& GetAudioCfg()
	{
		return m_AudioCfg;
	};
	
	void UpdateAudioCfg()
	{
		//this->GetStreamObj()->GetRTMPSendObj()->UpdateAudioCfg( m_AudioCfg );
	};

	void UpdateAudioCfg( const CAudioCfg& AudioCfg )
	{
		m_AudioCfg = AudioCfg;
		this->UpdateAudioCfg();
	};

	CCamAdvAttr& GetAdvAttr()
	{
		return m_AdvAttr;
	};

	const CCamAdvAttr& GetAdvAttrSafe() const
	{
		return m_AdvAttr;
	};

	void UpdateAdvAttr( const CCamAdvAttr& AdvAttr )
	{
		m_AdvAttr = AdvAttr;
	};

	bool isChanged( 
		const std::string &old_ipaddress, 
		const uint32_t old_port, 
		const std::string &old_username, 
		const std::string &old_password, 
		const uint32_t old_channel, 
		const uint32_t old_streamfmt,
		const CAudioCfg &old_AudioCfg ) const;

	bool isRight_manual_trace();

	bool ReCreateLocalCapAudio();
	bool StartLocalCapAudio();
	bool StopLocalCapAudio();

    virtual void OnPublishStart();
	virtual void OnPublishStop(defGSReturn code);
	virtual uint32_t OnPublishUpdateSession( const std::string &strjid, bool *isAdded=NULL );
	
	void SetDeviceId( const std::string &deviceID )
	{
		this->m_deviceID = deviceID;
	}

	const std::string& GetDeviceId()
	{
		return this->m_deviceID;
	}

	const std::string& GetName() const
	{
		return this->m_name;
	}

	void SetName( const std::string &name )
	{
		m_name = name;
	}

	void SetIPAddress( std::string& ipaddress )
	{
		this->m_ipaddress = ipaddress;
	}
	const std::string& GetIPAddress() const
	{
		return this->m_ipaddress;
	}

	void setPort( uint32_t port )
	{
		this->m_port = port;
	}
	uint32_t GetPort() const
	{
		return this->m_port;
	}

	void SetUsername( std::string &username )
	{
		this->m_username = username;
	}
	const std::string& GetUsername() const
	{
		return this->m_username;
	}

	void SetPassword( std::string &password )
	{
		this->m_password = password;
	}
	const std::string& GetPassword() const
	{
		return this->m_password;
	}


	long GetCameraHWND()
	{
		return this->m_camera_hwnd;
	}

	MediaStream* GetStreamObj( )
	{
		return  this->stream;
	}

	struCameraFixParm& GetFixParmObj()
	{
		return  this->m_fixparm;
	}

	struCameraState& GetStateObj()
	{
		return  this->m_state;
	}
	
	void SetChannel( uint32_t channel )
	{
		m_channel = channel;
	}

	uint32_t GetChannel() const
	{
		return this->m_channel;
	}

	void SetStreamfmt( uint32_t streamfmt )
	{
		m_streamfmt = streamfmt;
	}

	uint32_t GetStreamfmt() const
	{
		return this->m_streamfmt;
	}


	bool isGetSelf( bool playback ) const
	{
		if( playback )
		{
			if( defRecMod_OnReordSvr==this->m_RecCfg.getrec_mod() )
			{
				return false;
			}

			return true;
		}

		return true;
	}
	const std::string& ConnUse_ip( bool playback=false ) const
	{
		if( isGetSelf(playback) )
			return m_ipaddress;

		return this->m_RecCfg.getrec_ip();
	}
	uint32_t ConnUse_port( bool playback=false ) const
	{
		if( isGetSelf(playback) )
			return m_port;

		return this->m_RecCfg.getrec_port();
	}
	const std::string& ConnUse_username( bool playback=false ) const
	{
		if( isGetSelf(playback) )
			return m_username;

		return this->m_RecCfg.getrec_username();
	}
	const std::string& ConnUse_password( bool playback=false ) const
	{
		if( isGetSelf(playback) )
			return m_password;

		return this->m_RecCfg.getrec_password();
	}
	uint32_t ConnUse_channel( bool playback=false ) const
	{
		if( isGetSelf(playback) )
			return m_channel;

		return this->m_RecCfg.getrec_channel();
	}

	bool GetFrameState();
	void SetFrameState( bool state );

	void setUrl( const std::string& url )
	{
		this->stream->GetRTMPSendObj()->setUrl( url );
	}

	const std::string getUrl()
	{
		return this->stream->GetRTMPSendObj()->getUrl();
	}

	// 基本可用状态
	defUseable set_NetUseable( defUseable NetUseable, bool *isChanged=NULL );
	defUseable get_NetUseable();
	virtual void check_NetUseable( bool *isChanged=NULL ){};
	bool InvalidVideoBitrate() const;

	// 综合分析可用状态
	defUseable get_all_useable_state();

	bool IsPublish();
	defGSReturn SendToRTMPServer( const JID& jid, const std::string& url, const std::vector<std::string> &url_backup );
	void StopRTMPSend(const JID& jid);
	void StopRTMPSendAll();
	bool CheckSession( const uint32_t enable, const bool isPlayBack, const bool CheckNow );
	uint32_t UpdateSession( const JID& jid, bool *isAdded=NULL );
	uint32_t GetSessionStartTime( const std::string& strjid );
	uint32_t GetSessionLastUpdateTime( const std::string& strjid );
	uint32_t GetSessionCount();
	uint32_t GetSessionInfoList( deflstRTMPSessionInfo &outSessionInfoList );
	void PushVideo( const bool keyframe, const uint32_t rate, char *data, int size, uint32_t timestamp, const bool use_input_ts=true, const bool isAudio=false );

	bool isAutoConnect() const;

	/*虚方法，子类继承*/
	virtual defGSReturn GetCamCompressCfg( std::string *cfginfo, struCamParamBuf *getCamParamBuf=NULL ){ return defGSReturn_UnSupport; };
	virtual defGSReturn SetCamCompressCfg( const defCamCompress CamCompress, const struCamParamBuf *setuseCamParamBuf=NULL ){ return defGSReturn_UnSupport; };
	virtual defGSReturn GetCamShowInfo( int &showtime, int &showname, std::string &name ){ return defGSReturn_UnSupport; };
	virtual defGSReturn SetCamShowInfo( const int showtime, const int showname, const char *pname ){ return defGSReturn_UnSupport; };
	virtual void PreinitBufCheck(){};
	virtual bool TestConnect( std::string &strErr ) = 0;
	virtual bool IsConnect() = 0;
	virtual defGSReturn SetCamTime( const struGSTime &newtime ){ return defGSReturn_UnSupport; };
	void ConnectBefore();
	void ConnectAfter();
	virtual defGSReturn Connect( bool reconnectAudio, void* RealPlayWnd=NULL, bool playback=false, const struGSTime *pdtBegin=NULL, const struGSTime *pdtEnd=NULL ) = 0;
	virtual int PlayBackControl( GSPlayBackCode_ ControlCode, void *pInBuffer = NULL, uint32_t InLen = 0, void *pOutBuffer = NULL, uint32_t *pOutLen = NULL ) = 0;
	virtual bool SendPTZ( GSPTZ_CtrlCmd command, int param1=0, int param2=0, int speed=0, const char *callinfo=NULL ) = 0;
	virtual int GetPTZState(GSPTZ_CtrlCmd command, bool refresh_device) = 0;
	virtual void OnDisconnct(){ this->SetFrameState(false); };
	virtual IPCameraType GetCameraType() const = 0;
	virtual void MakeKeyFrame( bool forcenow=true ) = 0;
	bool isTalkUseCam() const;
	virtual bool isTalkSupport() const = 0;
	virtual bool OnTalkSend( defTalkSendCmd_ TalkSendCmd, unsigned char *pdata, uint32_t data_size ) = 0;
	virtual defGSReturn QuickSearchPlayback( const struGSTime *pdtBegin=NULL, const struGSTime *pdtEnd=NULL ){ return defGSReturn_Success; };

	virtual bool SetAGRunState( bool AGRunState, bool onlyvalue=false ) // 摄像机布防：设置告警布防状态
	{
		if( onlyvalue )
		{
			m_AGRunState = AGRunState;	// 修改布防标志
			return true;
		}

		// 当布防设置变为撤防时，告警重置
		if( m_AGRunState != AGRunState && !AGRunState  )
		{
			this->SetCurAlarmState( defAlarmState_UnInit );
		}

		m_AGRunState = AGRunState;
		return true;
	}
	virtual defAGRunStateFinalST GetAGRunStateFinalST() const
	{
		if( !IsSupportAGRunState() )
		{
			return defAGRunStateFinalST_UnAg;
		}

		if( GetAGRunState() )
		{
			if( IsAGRunStateFinished() )
				return defAGRunStateFinalST_AgFinish;
			else
				return defAGRunStateFinalST_AgFailed;
		}

		return defAGRunStateFinalST_UnAg;
	}
	virtual bool IsSupportAGRunState() const
	{
		return this->GetAdvAttrSafe().get_AdvAttr(defCamAdvAttr_SupportAlarm);
	}

	bool SetCurAlarmState( const defAlarmState CurAlarmState );
	bool CheckResumeCurAlarmState( uint32_t enable );
	defAlarmState GetCurAlarmState() const // 获取当前告警状态
	{
		return m_CurAlarmState;
	}

	// PrePic
	static bool SavePrePicChangeInfo( const int camid, const std::string &PrePicChangeCode );
	static bool LoadPrePicChangeInfo( const int camid, std::string &PrePicChangeCode );
	void SetPrePicChangeCode( const std::string &PrePicChangeCode );
	std::string GetPrePicChangeCode();

private:
	defAlarmState m_CurAlarmState;	// 摄像机当前告警状态，0正常，1告警
	uint32_t m_lastupdate_CurAlarmState;// 摄像机当前告警状态最后更新时间
	std::list<RTMPSession *> sessionList;
	gloox::util::Mutex m_mutex_sessionList;
	
protected:
	// prev state
	bool prev_isState_Publish;
	bool prev_isState_Url;
	bool prev_isState_Session;
	bool prev_FrameState;
	bool prev_IsConnect;
	uint32_t prev_checktime;

	uint32_t m_src_first_ts_video;
	uint32_t m_src_first_ts_audio;
	uint32_t m_src_prev_fix;

	uint32_t m_hasFrameTime;
	defUseable m_NetUseable;
	uint32_t  m_curVideoBitrate; // 设备当前视频码率值，通过接口获取到
};

#endif

