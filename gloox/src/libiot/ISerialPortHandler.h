#ifndef ISERIALPORTHANDLER_H_
#define ISERIALPORTHANDLER_H_

#include "CommLink.h"

#define defSerialBufPreLen 32

#define MIAN_PORT_Pkt_LEN	(255)
#define MIAN_PORT_UNPkt_LEN (1+1+1+1) // ������ �����ݵĳ��ȣ���ͷ+ȷ��λ+�����ȴ洢+����ȷ��λ
#define MIAN_PORT_BUF_MAX (MIAN_PORT_Pkt_LEN+MIAN_PORT_UNPkt_LEN) // �����Ȼ����С����  +��ͷ+ȷ��λ+�����ȴ洢+����ȷ��λ

class ISerialPortHandler
{
public:
	virtual int SendData(CCommLinkRun *CommLink, unsigned char *buf,int size){ return 0; };
	virtual void OnDataReceived( CCommLinkRun *CommLink, uint8_t* const srcbuf, const uint32_t srcbufsize ){};
	virtual int RecvData(CCommLinkRun *CommLink, unsigned char *buf,int size){ return 0; };
};

#endif