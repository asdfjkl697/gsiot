#pragma once

#include "typedef.h"
#include "common.h"

enum defRFSignalType
{
	defRFSignalType_invalid		= 0,	// ��Ч
	defRFSignalType_code		= 1,	// �����������źŸ�ʽRF
	defRFSignalType_original	= 2,	// ԭʼ�źŸ�ʽRF
	defRFSignalType_IR_code		= 3,	// ��������ʽ
	defRFSignalType_IR_original	= 4,	// ����ԭʼ�źŸ�ʽ

	defRFSignalType_Ex_Spec_	= 9000,	// �ź����ͷָ���

	defRFSignalType_RemoteObj_ExType	= 9001,	// Զ�̶��� - ��չ���ͣ���Ϊ��չ����ʱ����չ���ʹ洢��code
};

enum defFreq
{
	defFreq_Null = 0,
	defFreq_315 = 315,
	defFreq_433 = 433
};

#define defRFSignalOriginalMax 125

struct RFSignal
{
	int id;
	defRFSignalType signal_type;// �źŸ�ʽ����
	defFreq freq;				// 315 or 443

	uint32_t code;				// ������ʱ��
	uint8_t codeValidLen;		// (������ʱ��)ָ���		ָ����Чλ����(λ��)

	uint8_t headlen;			// �����볤�ȣ�0��ʾû��������
	uint16_t headcode[7];		// �����뻺��

	uint16_t one_high_time;		// ���ݶ�BIT 1������ʱ��		ʱ�䵥λ��us
	uint16_t one_low_time;		// ���ݶ�BIT 1�½���ʱ��		ʱ�䵥λ��us
	uint16_t zero_high_time;	// ���ݶ�BIT 0������ʱ��		ʱ�䵥λ��us
	uint16_t zero_low_time;		// ���ݶ�BIT 0�½���ʱ��		ʱ�䵥λ��us

	uint16_t silent_interval;	// silent���ʱ��			ʱ�䵥λ��us

	uint8_t taillen;			// �����볤�ȣ�0��ʾû�н�����
	uint16_t tailcode[7];		// �����뻺��

	uint8_t signal_count;		//����������ʱ�����ж��յ������Ƿ�ﵽ������ʱ���ڷ��ʹ���

	// * Ϊԭʼ�ź�ʱ�洢
	uint8_t original_headflag;	// ԭʼ�ź� ������������������־ 0x00�����壬0x01��
	uint16_t original_headtime;	// ԭʼ�ź� ������ʱ��
	uint8_t original_len;		// ԭʼ�ź� ���ݳ���
	uint16_t original[defRFSignalOriginalMax];		// ԭʼ�ź� ��������

	RFSignal()
	{
		RFSignal::Init( *this );
	}

	static void Init( RFSignal &rfsignal )
	{
		memset( &rfsignal, 0, sizeof(RFSignal) );
	}

	static std::string Trans_Type2Str( const defRFSignalType signal_type );

	bool isValid() const
	{
		return ( defRFSignalType_invalid != this->signal_type );
	}

	bool isSignal() const
	{
		return ( this->signal_type < defRFSignalType_Ex_Spec_ );
	}

	bool operator == ( const RFSignal &other ) const;
	bool IsNear( const RFSignal &other ) const;
	void Update( const RFSignal &src, bool onlysignal );
	SerialPortModule GetModuleIndex( bool isSend=true );

	std::string Get_original( int format=1, const bool isByteStr=false ) const;
	bool Set_original( const std::string &stroriginal, const bool isByteStr=false );
	std::string Print( const char *info=NULL, bool doPrint=true ) const;
	std::string GetUIShow( const uint8_t spec_count=0 ) const;

	bool SaveToFile( const std::string &filename ) const;
	bool LoadFromFile( const std::string &filename );

	uint32_t GetSendNeedTime( const uint8_t spec_count=0 ) const;
	int GetFixUnit() const;
	std::string Get_DecodeDesc( bool showint=false ) const;
};

