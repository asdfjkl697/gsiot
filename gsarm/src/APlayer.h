#pragma once
#include "common.h"
#include <string>
//#include <Windows.h>
#include "GSIOTDevice.h"

//#pragma comment(lib,"winmm.lib")

#define defNS_AO ns_AO
namespace ns_AO
{
#include "ao/ao.h"
//#pragma comment(lib,"libao.lib")
}

#define defNS_FF ns_FF
namespace ns_FF
{
	extern "C"
	{
		#include "libavcodec/avcodec.h"
		#include "libavformat/avformat.h"
		#include "libavutil/avutil.h"
	}
	//#pragma comment(lib,"avcodec.lib")		
	//#pragma comment(lib,"avformat.lib")
	//#pragma comment(lib,"avutil.lib")
}

/*
播放声音
  来源支持url, rtmp等
*/
class APlayer
{
public:
	APlayer();
	virtual ~APlayer();

	static bool Init();

	int m_probesize;

	bool Open( const std::string &filename );
	defGSReturn AddPlayOut( GSIOTDevice *pDev );
	defGSReturn RemovePlayOut( const GSIOTDeviceKey &devkey );
	bool isPlaying() const;
	bool Play( defvecDevKey &failedDevlst );
	bool Keepalive( const GSIOTDeviceKey &devkey );
	bool Keepalive( const defvecDevKey &vecdev, defvecDevKey &successDevlst, defvecDevKey &failedDevlst );
	void Close(void);

private:
	int open_input_file( const std::string &filename );
	unsigned char *new_buf( unsigned int needsize );
	void ReleaseRes();

private:
	std::string m_url;
	int m_audio_stream_index;
	uint32_t m_lastPlayed;
	uint32_t m_playPrintTs;
	defNS_FF::AVFormatContext *m_fmt_ctx;
	defNS_FF::AVCodecContext *m_dec_ctx;
	defNS_FF::AVFrame *m_frame;

	defNS_AO::ao_device *m_ao_device;
	defmapDev m_devlst;
	uint32_t m_playErrCount;

	unsigned char *m_databuf;
	unsigned int m_databufSize;
};

