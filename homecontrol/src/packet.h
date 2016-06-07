#ifndef _PACKET_H_
#define _PACKET_H_

#include <stdint.h>

#define BUFFER_SIZE 1024

#define PROTOCOL_FLAG 0x47534b4a //"GSKJ"
#define PROTOCOL_VERSION 65536


enum COMMAND
{
	CMD_BROADCAST_INFO = 0x10,
	CMD_SET_NETWORK_IP = 0x20,
	CMD_SET_WIFI_IP    = 0x30,
	CMD_LOGIN          = 0x40,
	CMD_SERIAL         = 0x50,
	CMD_DEVICE_INFO    = 0x60,
	CMD_SCHEDULE_TASK  = 0x70,
	CMD_TRIGGER        = 0x80,
	CMD_RADIONETWORK   = 0x90,
	CMD_SYSTEM         = 0xF0	
};

enum ACK
{
	ACK_NOP = 0xDA,
	ACK_ASK = 0xAC,
	ACK_REPLY = 0xE0
};

enum DATA_TYPE
{
	DATA_NONE = 0x0,
	DATA_INT32,
	DATA_STRING,
	DATA_BOOLEAN,
	DATA_BYTE,
	DATA_INT16,
	DATA_INT64,
	DATA_DOUBLE,
	DATA_FLOAT,
	DATA_OBJECT,
	DATA_KEYVALUE
};

enum OLD_SERIAL_PACKET
{
	OLD_RXB8_315 = 0x01,
	OLD_RXB8_433,
	OLD_RXB8_315_TX,
	OLD_RXB8_433_TX,
	OLD_CC1101_433,
	OLD_CAN_Chip,
	OLD_RS485,
	OLD_TX315_RAW,
	OLD_TX433_RAW,
	OLD_GSM,
	OLD_Module_Set
};
	
enum SERIAL_PACKET
{
	RF_Receiver = 0x10,
	RF_Transmit = 0x1A,
	IR_Receiver = 0x30,
	IR_Transmit = 0x3A,
	RS485       = 0x20,
	CC1101      = 0x40,
	SI5351      = 0x50,
	CAN         = 0X60,
	SYSTEM_INFO = 0x80,
	SYSTEM_SET  = 0xE0,
	SYSTEM_RETURN = 0xEA,
};

enum SYSTEM_ACTION
{
	SYSTEM_READ = 0x0D,
	SYSTEM_WIRTE = 0x1D,
	SYSTEM_QUERY = 0x0A,
	SYSTEM_SETTING = 0x1A,
	SYSTEM_FAIL  = 0xEE	
};

typedef struct _packet
{
	int      flag;
	int      version;
	uint32_t id;
	uint8_t  command;
	uint8_t  ack;
	uint32_t length;
	uint8_t  *data;
}packet;

typedef struct _serial_packet
{
	uint8_t head;
	uint8_t ack;
	uint8_t data_size;
	uint8_t read_size;
	uint8_t data[260];
	uint8_t end_ack;
	struct _serial_packet *next;
}serial_packet;

typedef struct _str_st
{
	uint8_t *str;
	uint16_t len;
}str_st;

int16_t packet_readInt16(uint8_t *data);
int32_t packet_readInt32(uint8_t *data);
uint8_t *packet_writeInt16(uint8_t *data_ptr, uint16_t value);
uint8_t *packet_writeInt32(uint8_t *data_ptr, uint32_t value);

uint8_t *packet_encodeInt16(uint8_t *data_ptr, uint16_t value);
uint8_t *packet_encodeInt32(uint8_t *data_ptr, uint32_t value);
uint8_t *packet_encodeBoolean(uint8_t *data_ptr, uint8_t value);
uint8_t *packet_encodeString(uint8_t *data_ptr, const char *value);
uint8_t *packet_decodeString(uint8_t *data_ptr, str_st *value);
uint8_t *packet_decodeByte(uint8_t *data_ptr, uint8_t *value);
uint8_t *packet_decodeBoolean(uint8_t *data_ptr, uint8_t *value);

int packet_decode(packet *pkt, uint8_t *data, int size);
int packet_readBuffer(packet *pkt, uint8_t *data);

void packet_init_serial(serial_packet *pkt);
void packet_free_serial(serial_packet *pkt);

uint8_t *packet_to_hardware(uint8_t *buffer, uint8_t *data, uint8_t size);
uint8_t *packet_setfreq(uint8_t *buffer, uint8_t module, uint32_t freq);

#endif

