#pragma once
#include "common.h"
#include "gloox/mutexguard.h"
#include "DeviceAddress.h"
#include "ControlBase.h"

enum defMODSysSetInfo
{
	defMODSysSetInfo_TXCtl = 0,		// IR �շ����� - �Ƿ�Ϊ��
	defMODSysSetInfo_RXMod,			// IR ��ģʽ���Ƿ�Ϊԭʼ����
	defMODSysSetInfo_RF_RX_freq,	// RF ���߽���Ƶ������315/433

	defMODSysSetInfo_MAX,			// defMODSysSetInfo ����
};

typedef struct _sdatabuffer
{
public:
	_sdatabuffer();
	~_sdatabuffer();

	uint8_t *data;
	uint32_t size;

	defLinkID LinkID;
	IOTDeviceType DevType;
	uint32_t DevID;
	ControlBase *ctl;
	DeviceAddress *address;
	uint32_t overtime;			// �ȴ���Ӧʱ������
	uint32_t QueueOverTime;		// �����Ŷ�����ʱ������
	uint32_t nextInterval;		// ���´η��͵ļ��ʱ��

public:
	void SetNowTime();
	bool IsOverTime() const;
	bool IsQueueOverTime() const;
	uint32_t get_nextInterval() const;
	void Print( const char *info ) const;
	
	bool IsSameOnlyDev( const _sdatabuffer &other );
	bool IsSameOnlyDev(
		const defLinkID other_LinkID, const IOTDeviceType other_DevType, const uint32_t other_DevID,
		const ControlBase *other_ctl
		);

	bool IsSame( const _sdatabuffer &other );
	bool IsSame(
		const defLinkID other_LinkID, const uint8_t *other_data, const uint32_t other_size, const IOTDeviceType other_DevType, const uint32_t other_DevID,
		const ControlBase *other_ctl, const DeviceAddress *other_address,
		const uint32_t other_overtime, const uint32_t other_QueueOverTime, const uint32_t other_nextInterval
		);

private:
	uint32_t m_time;

}SERIALDATABUFFER;


class CMsgCurCmd
{
public:
	CMsgCurCmd();
	~CMsgCurCmd(void);

	uint32_t m_ts_lastSend;
	uint32_t m_prevInterval;		// ���ϴη��͵ļ��ʱ��

	static void Delete_CurCmd_Content_spec( SERIALDATABUFFER *CurCmd );
	static std::string FormatShowForTS( const std::string &strver, const uint32_t *last_ts=NULL );

	void SetGSIOTBoardVer( const std::string &ver );
	std::string GetGSIOTBoardVer( uint32_t *last_ts=NULL );

	void Set_MODSysSetInfo( const defMODSysSetInfo MODSysSetInfo, const int val );
	std::string Get_MODSysSetInfo( const defMODSysSetInfo MODSysSetInfo, uint32_t *last_ts, bool commboget, int *getval=NULL );

	bool Is_CurCmd( ControlBase *DoCtrl );
	bool Set_CurCmd( ControlBase *DoCtrl, SERIALDATABUFFER *CurCmd );
	void Get_CurCmd( ControlBase *DoCtrl, SERIALDATABUFFER **CurCmd );
	void Delete_CurCmd_Content( bool lock );

private:
	// ��ǰ����ִ�е�RS485������
	gloox::util::Mutex m_mutex_CurCmd;
	SERIALDATABUFFER *m_CurCmd;
	SERIALDATABUFFER *m_CurCmd_Prev;

	uint32_t m_GSIOTBoardVer_ts;
	std::string m_GSIOTBoardVer;

	uint32_t m_MODSysSetInfo_TXCtl_ts;
	int m_MODSysSetInfo_TXCtl; // IR �շ����� - �Ƿ�Ϊ��
	
	uint32_t m_MODSysSetInfo_RXMod_ts;
	int m_MODSysSetInfo_RXMod; // IR ��ģʽ���Ƿ�Ϊԭʼ����

	uint32_t m_MODSysSetInfo_RF_RX_freq_ts;
	int m_MODSysSetInfo_RF_RX_freq; // RF ���߽���Ƶ������315/433
};
