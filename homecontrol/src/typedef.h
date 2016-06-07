#ifndef _TYPEDEF_H_
#define _TYPEDEF_H_

#include "packet.h"
#include "event2/listener.h"

#ifndef SOCKET
#define SOCKET int
#endif

#ifndef INVALID_SOCKET
#define INVALID_SOCKET -1
#endif

#ifndef SOCKET_ERROR
#define SOCKET_ERROR -1
#endif

#define UDP_BUFFER_SIZE      1024 //1024


#define VERSION_HARDWARE "20131212"
#define VERSION_SOFT "20150119"


#define OS_UBUNTU 1

typedef void task_callback(void *arg);
typedef void (task_socket_callback)(void *arg, uint8_t *data, int size, struct sockaddr_in *addr);

typedef struct _network_adapter
{
	uint8_t ipaddr[18];
	uint8_t netmask[18];
	uint8_t gateway[18];
	uint8_t dns[18];
	uint8_t mac[18];
}NETWORK_ADAPTER;

typedef struct _task_sock
{
	struct event *evt;
	int socket_fd;
	uint8_t *version;
	NETWORK_ADAPTER *adapter;
	NETWORK_ADAPTER *adapter2;
	task_socket_callback *callback;
}TASK_SOCK;

typedef struct _task_queue
{
	void *handler;
	task_callback *callback;
	struct _task_queue *next;
}TASK_QUEUE;

typedef struct _task_time
{
	struct event *evt;
	void *arg;
	task_callback *callback;
}TASK_TIME;

typedef struct _task
{
	struct event_base *evbase;
	//struct event *timeout_event;
	TASK_SOCK sock;
	TASK_QUEUE *queue;
	uint8_t waitexit;
}TASK;

typedef struct _radio_child{	
	uint8_t addr;
	uint8_t channel;
	uint8_t role;
	uint8_t recv_rssi;
	uint8_t send_rssi;
	struct _radio_child *next;
}RADIO_CHILD;

typedef struct _radio_network
{
	uint8_t state;
	uint8_t addr;
	uint8_t channel;
	uint8_t role;
	uint8_t dis_count;
	uint8_t dis_addr;
	uint8_t dis_channel;
	uint8_t dis_state;
	RADIO_CHILD *child;
	TASK_TIME *radiotask;
}RADIO_NETWORK;

typedef struct _service {
	TASK *task;
	TASK_TIME *timetask;
	char username[16];
	char password[16];
	uint8_t logged;
	uint8_t service_type;
	RADIO_NETWORK *radio;
}SERVICE;

typedef struct _clients {
	uint8_t logged;
	SERVICE *serv;
	serial_packet *prve_packet;
	struct bufferevent *buf_ev;
	struct _clients *next;
}clients;

typedef struct _borad {
	TASK *task;
	uint8_t cache[255];
	uint8_t cache_length;
	serial_packet *prve_packet;
	struct event *serial_evt;
	RADIO_NETWORK *radio;
}BOARD;

#endif
