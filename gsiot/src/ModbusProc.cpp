#include "ModbusProc.h"

// modbus��ַƫ��
// �����ڲ���1��ʼ��ͨѶʱ����ƫ��
#define defModbusAddressOffset_Decode (1)
#define defModbusAddressOffset_Encode (-1)

/***
1���ܸ�ʽ��
���豸��ַ		(1�ֽ�, 0x01-0xFF)
������			(1�ֽ�, 0x03/0x06�ȵ�)
������			(*)
CRCУ����		(2�ֽ�)

CRC�������λ��ǰ����λ�ں�,�Ĵ�����ַ, �Ĵ�������,���ݾ�Ϊ��λ��ǰ����λ�ں�;

2����Ӧ�����

2.1) �����������Ӧ
��Ӧ������=��������

2.2) ������쳣��Ӧ
��Ӧ������=��������+0x80
���Һ����ṩһ���쳣��
***/


ModbusProc::ModbusProc()
{
}

ModbusProc::~ModbusProc(void)
{
}

/***
ֻ�������豸��ַ�͹�����
��ʽ�������£�
���豸��ַ		(0x01-0xFF  1�ֽ�)
������			(1�ֽ�)
...
CRCУ����		(2�ֽ�)
***/
bool ModbusProc::Decode_dev_cmd( uint8_t* const originalBuf, const uint16_t buflen, uint8_t &devid, uint8_t &cmdcode )
{
	devid = 0;
	cmdcode = 0;

	if( buflen < 4 )
	{
		//LOGMSGEX( defLOGNAME, defLOG_ERROR, "ModbusProc::Decode_dev_cmd buflen error!!!\r\n");
		return false;
	}

	uint16_t crcFromDecode;
	memcpy( &crcFromDecode, originalBuf+buflen-2, 2 );
	uint16_t crcFromCal = crc16_verify( originalBuf, buflen-2 );

	if( crcFromDecode != crcFromCal )
	{
		//LOGMSGEX( defLOGNAME, defLOG_ERROR, "ModbusProc::Decode_dev_cmd crc error!!! crcFromDecode=%d, crcFromCal=%d\r\n", crcFromDecode, crcFromCal );
		return false;
	}

	devid = originalBuf[0];
	cmdcode = originalBuf[1];

	return true;
}

/***
������ ������Ӧ ������
��ʽ�������£�
���豸��ַ		(1�ֽ�, 0x01-0xFF)
������			(1�ֽ�, ������(�����)=0x8?)
�쳣��			(1�ֽ�)
CRCУ����		(2�ֽ�)
***/
bool ModbusProc::Decode_Err_Response( uint8_t* const originalBuf, const uint16_t buflen, uint8_t &ErrCode )
{
	ErrCode = *(originalBuf+2);
	return true;
}

/***
ֻ������ ����Ĵ���ֵ
��ʽ�������£�
���豸��ַ		(1�ֽ�, 0x01-0xFF)
������			(1�ֽ�)
�������ֽ���		(1�ֽ�, ȡֵ=2*�Ĵ�������)
�Ĵ���ֵ			(�Ĵ�������*2�ֽ�)
CRCУ����		(2�ֽ�)
***/
bool ModbusProc::Decode_Response_addressData( uint8_t* const originalBuf, const uint16_t buflen, std::vector<uint16_t> &vecAddressData, char *loginfo )
{
	uint8_t *ptemp = originalBuf+2;
	uint8_t *ptempEnd = originalBuf+buflen-2;

	uint8_t AddressCount = *ptemp;
	ptemp++;

	if( ptemp > ptempEnd )
	{
		//LOGMSGEX( defLOGNAME, defLOG_ERROR, "ModbusProc::Decode_Response_addressData(%s) ptempEnd error!!!\r\n", loginfo?loginfo:"" );
		return false;
	}

	if( AddressCount > (ptempEnd-ptemp) )
	{
		//LOGMSGEX( defLOGNAME, defLOG_ERROR, "ModbusProc::Decode_Response_addressData(%s) address count error!!! decoded count=%d\r\n", loginfo?loginfo:"", AddressCount );
		return false;
	}

	uint8_t *paddressData = ptemp;
	for( ; paddressData<ptempEnd; )
	{
		vecAddressData.push_back( Big_ByteToInt16( paddressData ) );

		paddressData +=2;
	}

	return true;
}

/***
ֻ�������Ĵ�����ַ�ͼĴ���ֵ
��ʽ�������£�
���豸��ַ		(0x01-0xFF  1�ֽ�)
������			(1�ֽ�)
�Ĵ�����ַ		(2�ֽ�)
�Ĵ���ֵ			(2�ֽ�)
CRCУ����		(2�ֽ�)
***/
bool ModbusProc::Decode_address_value( uint8_t* const originalBuf, const uint16_t buflen, uint16_t &address, uint16_t &addressValue, char *loginfo )
{
	address = 0;
	addressValue = 0;

	if( buflen < 4 )
	{
		//LOGMSGEX( defLOGNAME, defLOG_ERROR, "ModbusProc::Decode_address_value(%s) buflen error!!!\r\n", loginfo?loginfo:"" );
		return false;
	}

	address = Big_ByteToInt16( originalBuf+2 )+defModbusAddressOffset_Decode;
	addressValue = Big_ByteToInt16( originalBuf+4 );
	return true;
}


/***
ͨ�ñ��뺯��
��ʽ�������£�
���豸��ַ		(1�ֽ�, 0x01-0xFF)
������			(1�ֽ�, 0x06)
2�ֽ�����1		(2�ֽ�)
2�ֽ�����2		(2�ֽ�)
CRCУ����		(2�ֽ�)
***/
bool ModbusProc::Encode_data_2short( uint8_t* destBuf, uint16_t &buflen, const uint8_t devid, const uint8_t cmd, const uint16_t short1, const uint16_t short2, char *loginfo )
{
	if( buflen < 8 )
	{
		//LOGMSGEX( defLOGNAME, defLOG_ERROR, "ModbusProc::Encode_data_2short(%s) buflen=%d error!!! devid=%d, cmd=%d\r\n", loginfo?loginfo:"", buflen, devid, cmd );
		return false;
	}

	uint8_t *enc = destBuf;

	*enc = devid; enc++;
	*enc = cmd; enc++;
	
	enc = Big_Int16ToByte( enc, short1 );
	enc = Big_Int16ToByte( enc, short2 );

	uint16_t crcFromCal = crc16_verify( destBuf, enc-destBuf );
	memcpy( enc, &crcFromCal, 2 );

	buflen = 8;
	return true;
}

/***
ͨ�ñ��뺯��
��ʽ�������£�
���豸��ַ		(1�ֽ�, 0x01-0xFF)
������			(1�ֽ�, 0xXX)
1�ֽ�����1		(1�ֽ�)
1�ֽ�����2		(1�ֽ�)
CRCУ����		(2�ֽ�)
***/
bool ModbusProc::Encode_data_1Byte( uint8_t* destBuf, uint16_t &buflen, const uint8_t devid, const uint8_t cmd, const uint8_t byte1, const uint8_t byte2, char *loginfo )
{
	if( buflen < 6 )
	{
		//LOGMSGEX( defLOGNAME, defLOG_ERROR, "ModbusProc::Encode_data_1Byte(%s) buflen=%d error!!! devid=%d, cmd=%d\r\n", loginfo?loginfo:"", buflen, devid, cmd );
		return false;
	}

	uint8_t *enc = destBuf;

	*enc = devid; enc++;
	*enc = cmd; enc++;

	*enc = byte1; enc++;
	*enc = byte2; enc++;

	uint16_t crcFromCal = crc16_verify( destBuf, enc-destBuf );
	memcpy( enc, &crcFromCal, 2 );

	buflen = 6;
	return true;
}

//////////////////////////////////////////////////////////

/***
03(0x03)�����ּĴ��� ����
���豸��ַ		(1�ֽ�, 0x01-0xFF)
������			(1�ֽ�, 0x03)
��ʼ�Ĵ�����ַ	(2�ֽ�)
�Ĵ�������		(2�ֽ�)
CRCУ����		(2�ֽ�)
***/
bool ModbusProc::Encode_Read03_Request( uint8_t* destBuf, uint16_t &buflen, const uint8_t devid, const uint16_t addressStart, const uint16_t addressCount )
{
	return Encode_data_2short( destBuf, buflen, devid, 0x03, addressStart+defModbusAddressOffset_Encode, addressCount, "Encode_Read03_Request" );
}

/***
03(0x03)�����ּĴ��� ��Ӧ
���豸��ַ		(1�ֽ�, 0x01-0xFF)
������			(1�ֽ�, 0x03)
�������ֽ���		(1�ֽ�, ȡֵ=2*�Ĵ�������)
�Ĵ���ֵ			(�Ĵ�������*2�ֽ�)
CRCУ����		(2�ֽ�)
***/
bool ModbusProc::Decode_Read03_Response( uint8_t* const originalBuf, const uint16_t buflen, std::vector<uint16_t> &vecAddressData )
{
	return Decode_Response_addressData( originalBuf, buflen, vecAddressData );
}

/***
03(0x03)�����ּĴ��� ����
���豸��ַ		(1�ֽ�, 0x01-0xFF)
������			(1�ֽ�, ������(�����)=0x83)
�쳣��			(1�ֽ�)
CRCУ����		(2�ֽ�)
***/
bool ModbusProc::Decode_Read03_Err_Response( uint8_t* const originalBuf, const uint16_t buflen, uint8_t &ErrCode )
{
	return Decode_Err_Response( originalBuf, buflen, ErrCode );
}

/***
04(0x04)�����ּĴ��� ����
���豸��ַ		(1�ֽ�, 0x01-0xFF)
������			(1�ֽ�, 0x04)
��ʼ�Ĵ�����ַ	(2�ֽ�)
�Ĵ�������		(2�ֽ�)
CRCУ����		(2�ֽ�)
***/
bool ModbusProc::Encode_Read04_Request( uint8_t* destBuf, uint16_t &buflen, const uint8_t devid, const uint16_t addressStart, const uint16_t addressCount )
{
	return Encode_data_2short( destBuf, buflen, devid, 0x04, addressStart+defModbusAddressOffset_Encode, addressCount, "Encode_Read04_Request" );
}

/***
04(0x04)�����ּĴ��� ��Ӧ
���豸��ַ		(1�ֽ�, 0x01-0xFF)
������			(1�ֽ�, 0x04)
�������ֽ���		(1�ֽ�, ȡֵ=2*�Ĵ�������)
�Ĵ���ֵ			(�Ĵ�������*2�ֽ�)
CRCУ����		(2�ֽ�)
***/
bool ModbusProc::Decode_Read04_Response( uint8_t* const originalBuf, const uint16_t buflen, std::vector<uint16_t> &vecAddressData )
{
	return Decode_Response_addressData( originalBuf, buflen, vecAddressData );
}

/***
04(0x04)�����ּĴ��� ����
���豸��ַ		(1�ֽ�, 0x01-0xFF)
������			(1�ֽ�, ������(�����)=0x84)
�쳣��			(1�ֽ�)
CRCУ����		(2�ֽ�)
***/
bool ModbusProc::Decode_Read04_Err_Response( uint8_t* const originalBuf, const uint16_t buflen, uint8_t &ErrCode )
{
	return Decode_Err_Response( originalBuf, buflen, ErrCode );
}

/***
06(0x06)д�����Ĵ��� ����
���豸��ַ		(1�ֽ�, 0x01-0xFF)
������			(1�ֽ�, 0x06)
��ʼ�Ĵ�����ַ	(2�ֽ�)
д��Ĵ�����ֵ	(2�ֽ�)
CRCУ����		(2�ֽ�)
***/
bool ModbusProc::Encode_Write06_Request( uint8_t* destBuf, uint16_t &buflen, const uint8_t devid, const uint16_t address, const uint16_t addressValue )
{
	return Encode_data_2short( destBuf, buflen, devid, 0x06, address+defModbusAddressOffset_Encode, addressValue, "Encode_Write06_Request" );
}

/***
06(0x06)д�����Ĵ��� ��Ӧ
���豸��ַ		(1�ֽ�, 0x01-0xFF)
������			(1�ֽ�, 0x06)
�Ĵ�����ַ		(2�ֽ�)
�Ĵ���ֵ			(2�ֽ�)
CRCУ����		(2�ֽ�)
***/
bool ModbusProc::Decode_Write06_Response( uint8_t* const originalBuf, const uint16_t buflen, uint16_t &address, uint16_t &addressValue )
{
	return Decode_address_value( originalBuf, buflen, address, addressValue );
}

/***
06(0x06)д�����Ĵ��� ����
���豸��ַ		(1�ֽ�, 0x01-0xFF)
������			(1�ֽ�, ������(�����)=0x86)
�쳣��			(1�ֽ�)
CRCУ����		(2�ֽ�)
***/
bool ModbusProc::Decode_Write06_Err_Response( uint8_t* const originalBuf, const uint16_t buflen, uint8_t &ErrCode )
{
	return Decode_Err_Response( originalBuf, buflen, ErrCode );
}


/***
16(0x10)д�Ĵ���FC16���ֽ� ���� (���ܷǱ�׼)
���豸��ַ		(1�ֽ�, 0x01-0xFF)
������			(1�ֽ�, 0x10)
��ʼ�Ĵ�����ַ	(1�ֽ�)
д��Ĵ�����ֵ	(1�ֽ�)
CRCУ����		(2�ֽ�)
***/
bool ModbusProc::Encode_Write1601_Request( uint8_t* destBuf, uint16_t &buflen, const uint8_t devid, const uint16_t address, const uint16_t addressValue )
{
	return Encode_data_1Byte( destBuf, buflen, devid, 0x10, address+defModbusAddressOffset_Encode, addressValue, "Encode_Write1601_Request" );
}
