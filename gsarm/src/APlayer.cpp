#include "APlayer.h"
//#include "windows.h"
//#include "common.h"
//#include "logFileExFunc.h"
#include "IPCameraBase.h"


APlayer::APlayer()
	: m_audio_stream_index(-1), m_databuf(NULL), m_databufSize(0), m_probesize(-1)
{
	m_dec_ctx = NULL;
	m_fmt_ctx = NULL;
	m_frame = NULL;
	m_ao_device = NULL;
	m_playErrCount = 0;
	m_lastPlayed = timeGetTime();
	m_playPrintTs = m_lastPlayed;
}

APlayer::~APlayer()
{
	ReleaseRes();

	if( m_databuf )
	{
		delete []m_databuf;
		m_databuf = NULL;
		m_databufSize = 0;
	}
}

void APlayer::ReleaseRes()
{
	if( m_dec_ctx )
	{
		defNS_FF::avcodec_close(m_dec_ctx);
		m_dec_ctx = NULL;
	}

	if( m_fmt_ctx )
	{
		defNS_FF::avformat_close_input( &m_fmt_ctx );
		m_fmt_ctx = NULL;
	}

	if( m_frame )
	{
		defNS_FF::av_frame_free( &m_frame );
		m_frame = NULL;
	}

	if( m_ao_device )
	{
		//defNS_AO::ao_close( m_ao_device ); //jyc20170330 remove
		m_ao_device = NULL;
	}

	if( !m_devlst.empty() )
	{
		for( defmapDev::iterator it=m_devlst.begin(); it!=m_devlst.end(); ++it )
		{
			if( it->second && it->second->getControl() )
			{
				IPCameraBase *cam_ctl = (IPCameraBase*)it->second->getControl();
				cam_ctl->OnTalkSend( defTalkSendCmd_Close, NULL, 0 );
			}
		}
		m_devlst.clear();
	}
}

// 
bool APlayer::Init(void)
{
	defNS_FF::avcodec_register_all();
	defNS_FF::av_register_all();
	defNS_FF::avformat_network_init();

	//defNS_AO::ao_initialize();  //jyc20170330 remove

	return true;
}

unsigned char *APlayer::new_buf( unsigned int needsize )
{
	if( m_databuf && needsize > m_databufSize )
	{
		delete []m_databuf;
		m_databuf = NULL;
		m_databufSize = 0;
	}

	if( !m_databuf )
	{
		m_databufSize = needsize;
		m_databuf = new unsigned char[m_databufSize];
	}

	return m_databuf;
}

// url or filename
int APlayer::open_input_file( const std::string &filename )
{
	if( filename.empty() )
	{
		return -1;
	}

	int ret;
	defNS_FF::AVCodec *dec;

	if ((ret = defNS_FF::avformat_open_input(&m_fmt_ctx, filename.c_str(), NULL, NULL)) < 0) {
		LOGMSG( "APlayer::Open : Cannot open input file! url=\"%s\"\r\n", filename.c_str() );
		return ret;
	}

	if( m_probesize >= 0 )
	{
		LOGMSG( "APlayer::Open : useparam : probesize old=%d, new=%d", m_fmt_ctx->probesize, m_probesize );
		m_fmt_ctx->probesize = m_probesize;
	}

	const int new_fps_probe_size = 10;//...
	LOGMSG( "APlayer::Open : useparam : fps_probe_size old=%d, new=%d", m_fmt_ctx->fps_probe_size, new_fps_probe_size );
	m_fmt_ctx->fps_probe_size = new_fps_probe_size;

	const int new_max_delay = (int)(0.7*AV_TIME_BASE);//0.7
	LOGMSG( "APlayer::Open : useparam : max_delay old=%d, new=%d", m_fmt_ctx->max_delay, new_max_delay );
	m_fmt_ctx->max_delay = new_max_delay;

	if ((ret = defNS_FF::avformat_find_stream_info(m_fmt_ctx, NULL)) < 0) {
		LOGMSG( "APlayer::Open : Cannot open audio decoder! url=\"%s\"\r\n", filename.c_str() );
		return ret;
	}

	/* select the audio stream */
	ret = defNS_FF::av_find_best_stream(m_fmt_ctx, defNS_FF::AVMEDIA_TYPE_AUDIO, -1, -1, &dec, 0);
	if (ret < 0) {
		LOGMSG( "APlayer::Open : Cannot find a audio stream in the input file! url=\"%s\"\r\n", filename.c_str() );
		return ret;
	}
	m_audio_stream_index = ret;
	m_dec_ctx = m_fmt_ctx->streams[m_audio_stream_index]->codec;
	//av_opt_set_int(m_dec_ctx, "refcounted_frames", 1, 0);

	/* init the audio decoder */
	if ((ret = defNS_FF::avcodec_open2(m_dec_ctx, dec, NULL)) < 0) {
		LOGMSG( "APlayer::Open : Cannot open audio decoder! url=\"%s\"\r\n", filename.c_str() );
		return ret;
	}

	return 0;
}

bool APlayer::Open( const std::string &filename )
{
	m_playErrCount = 0;
	if( filename.empty() )
	{
		return false;
	}
	m_url = filename;
	
	int ret = -1;
	if( (ret = open_input_file(m_url)) < 0 )
	{
		char buf[1024] = {0};
		defNS_FF::av_strerror(ret, buf, sizeof(buf));

		LOGMSG( "APlayer::Open failed! url=\"%s\", ret=%d, err=%s\r\n", m_url.c_str(), ret, buf );
		return false;
	}

	LOGMSG( "APlayer::Open ch=%d, bits=%d, rate=%d", m_dec_ctx->channels, m_dec_ctx->bits_per_coded_sample, m_dec_ctx->sample_rate );

	m_frame = defNS_FF::av_frame_alloc();
	if( !m_frame )
	{
		LOGMSG( "APlayer::Open : Could not allocate m_frame! url=\"%s\"\r\n", m_url.c_str() );
		return false;
	}

	m_lastPlayed = timeGetTime();
	LOGMSG( "APlayer::Open success. url=\"%s\"\r\n", m_url.c_str() );

	return true;
}

defGSReturn APlayer::AddPlayOut( GSIOTDevice *pDev )
{
	if( pDev )
	{
		GSIOTDeviceKey devkey( pDev->getType(), pDev->getId() );
		if( m_devlst.find(devkey) != m_devlst.end() )
		{
			return defGSReturn_IsExist;
		}

		if( pDev->GetEnable() && pDev->getControl() )
		{
			IPCameraBase *cam_ctl = (IPCameraBase*)pDev->getControl();
			if( cam_ctl->isTalkUseCam() ) // 是否使用摄像头对讲
			{
				if( cam_ctl->isTalkSupport() )
				{
					if( !cam_ctl->OnTalkSend( defTalkSendCmd_Open, NULL, 0 ) )
					{
						return defGSReturn_Err;
					}

					m_devlst[devkey] = pDev;
					//LOGMSG( "APlayer::AddPlayOut : open cam TalkSend success. url=\"%s\"\r\n", m_url.c_str() );
				}
				else
				{
					LOGMSG( "APlayer::AddPlayOut : cam(type=%d, id=%d) isTalkSupport=false", pDev->getType(), pDev->getId() );
					return defGSReturn_Err;
				}
			}
			else
			{
				LOGMSG( "APlayer::AddPlayOut : cam(type=%d, id=%d) isTalkUseCam=false", pDev->getType(), pDev->getId() );
				return defGSReturn_Err;
			}
		}
		else
		{
			LOGMSG( "APlayer::AddPlayOut : cam(type=%d, id=%d) enable=false or cfg err", pDev->getType(), pDev->getId() );
			return defGSReturn_Err;
		}
	}
	else
	{
		if( m_ao_device )
		{
			return defGSReturn_IsExist;
		}

		defNS_AO::ao_sample_format format;
		memset(&format, 0, sizeof(format));
		format.bits = m_dec_ctx->bits_per_coded_sample;
		format.channels = m_dec_ctx->channels;
		format.rate = m_dec_ctx->sample_rate;
		format.byte_format = AO_FMT_LITTLE;

		/* -- Open driver -- */ //jyc20170330 remove
		//m_ao_device = defNS_AO::ao_open_live( defNS_AO::ao_default_driver_id(), &format, NULL );/* no options */
		if( !m_ao_device )
		{
			LOGMSG( "APlayer::AddPlayOut : Error opening ao_device! url=\"%s\"\r\n", m_url.c_str() );
			return defGSReturn_Err;
		}

		LOGMSG( "APlayer::AddPlayOut : open ao_device success. url=\"%s\"\r\n", m_url.c_str() );
	}

	m_lastPlayed = timeGetTime();
	return defGSReturn_Success;
}

defGSReturn APlayer::RemovePlayOut( const GSIOTDeviceKey &devkey )
{
	LOGMSG( "APlayer::RemovePlayOut : close dev(type=%d, id=%d). url=\"%s\"\r\n", devkey.m_type, devkey.m_id, m_url.c_str() );

	if( IOT_DEVICE_Unknown==devkey.m_type && 0==devkey.m_id )
	{
		if( m_ao_device )
		{
			//defNS_AO::ao_close( m_ao_device ); //jyc20170330 remove
			m_ao_device = NULL;
		}
	}
	else
	{
		if( !m_devlst.empty() )
		{
			defmapDev::iterator it = m_devlst.find( devkey );
			if( it != m_devlst.end() )
			{
				if( it->second && it->second->getControl() )
				{
					IPCameraBase *cam_ctl = (IPCameraBase*)it->second->getControl();
					cam_ctl->OnTalkSend( defTalkSendCmd_Close, NULL, 0 );
					m_devlst.erase( devkey );
				}
			}
		}
	}

	return defGSReturn_Success;
}

bool APlayer::isPlaying() const
{
	if( m_ao_device || !m_devlst.empty() )
	{
		return true;
	}

	return false;
}

#define defLastPlayedOverTime	20000
#define defPlayPrintTsTime		60000

bool APlayer::Play( defvecDevKey &failedDevlst )
{
	if( !failedDevlst.empty() )
	{
		failedDevlst.clear();
	}

	//if( !isPlaying() )
	//	return false;

	if( !m_fmt_ctx )
	{
		LOGMSG( "APlayer::Play : failed! m_fmt_ctx=NULL! url=\"%s\"\r\n", m_url.c_str() );
		return false;
	}

	if( timeGetTime()-m_lastPlayed > defLastPlayedOverTime )
	{
		LOGMSG( "APlayer::Play : long time(%ds) not stream played! url=\"%s\"\r\n", defLastPlayedOverTime/1000, m_url.c_str() );
		return false;
	}

	int ret = -1;
	int got_frame;

	defNS_FF::AVPacket packet;
	defNS_FF::av_init_packet(&packet);
	packet.data = NULL; // packet data will be allocated by the encoder
	packet.size = 0;

	ret = defNS_FF::av_read_frame(m_fmt_ctx, &packet);

	if (ret == AVERROR(EAGAIN))
	{
		usleep(1000);
		ret = 0;
		return true;
	}
	else if (ret < 0)
	{
		LOGMSG( "APlayer::Play : failed! read_frame ret=%d! url=\"%s\"\r\n", ret, m_url.c_str() );
		return false;
	}

	if( packet.stream_index == m_audio_stream_index )
	{
		defNS_FF::av_frame_unref(m_frame);//avcodec_get_frame_defaults(m_frame);
		got_frame = 0;
		ret = defNS_FF::avcodec_decode_audio4(m_dec_ctx, m_frame, &got_frame, &packet);
		if (ret < 0)
		{
			goto end;
		}

		if (got_frame) {

			unsigned short *pdata = NULL;
			int data_size = 0;

			if( defNS_FF::AV_SAMPLE_FMT_S16 == m_dec_ctx->sample_fmt 
				|| defNS_FF::AV_SAMPLE_FMT_S16P == m_dec_ctx->sample_fmt
				)
			{
				pdata = (unsigned short*)m_frame->data[0];
				data_size = m_frame->linesize[0];
			}
			else if( defNS_FF::AV_SAMPLE_FMT_FLT == m_dec_ctx->sample_fmt 
				|| defNS_FF::AV_SAMPLE_FMT_FLTP == m_dec_ctx->sample_fmt
				)
			{
				int nb_samples = m_frame->nb_samples;
				int channels = m_frame->channels;
				int outputBufferLen = nb_samples * channels * 2;

				//short* pdata = new short[outputBufferLen/2];
				pdata = (unsigned short*)this->new_buf( sizeof(unsigned short)*(outputBufferLen/2) );
				data_size = outputBufferLen;

				for (int i = 0; i < nb_samples; i++)
				{
					for (int c = 0; c < channels; c++)
					{
						float* extended_data = (float*)m_frame->extended_data[c];
						float sample = extended_data[i];
						if (sample < -1.0f) sample = -1.0f;
						else if (sample > 1.0f) sample = 1.0f;
						pdata[i * channels + c] = (short)(sample * 32767.0f);//(short)round(sample * 32767.0f);
					}
				}
			}
			else
			{
				goto end;
			}

			if( !pdata || data_size<=0 )
			{
				goto end;
			}

			if( timeGetTime()-m_playPrintTs > defPlayPrintTsTime )
			{
				LOGMSG( "APlayer::Play : (print/%ds) audio_stream_index=%d. url=\"%s\"\r\n", defPlayPrintTsTime/1000, m_audio_stream_index, m_url.c_str() );
				m_playPrintTs = timeGetTime();
			}

			bool thisPlayed = false;
			if( m_ao_device )
			{
				thisPlayed = true;
				m_lastPlayed = timeGetTime();

				//defNS_AO::ao_play( m_ao_device, (char*)pdata, data_size);  //jyc20170330 remove
			}

			// 可同时播放
			if( !m_devlst.empty() )
			{
				for( defmapDev::iterator it=m_devlst.begin(); it!=m_devlst.end(); ++it )
				{
					if( it->second && it->second->getControl() )
					{
						IPCameraBase *cam_ctl = (IPCameraBase*)it->second->getControl();
						if( it->second->GetEnable()		// 设备被禁用时停止
							&& cam_ctl->isTalkUseCam()	// 配置发生变化不一致时停止
							&& cam_ctl->isTalkSupport()	// 配置发生变化不一致时停止
							)
						{
							if( cam_ctl->OnTalkSend( defTalkSendCmd_Play, (unsigned char*)pdata, data_size ) )
							{
								thisPlayed = true;
								m_lastPlayed = timeGetTime();

								m_playErrCount = 0;
							}
							else
							{
								m_playErrCount++;
						
								if( m_playErrCount >= 60 )
								{
									LOGMSG( "APlayer::Play failed return! playErrCount=%d. dev(type=%d, id=%d), url=\"%s\"\r\n", m_playErrCount, it->first.m_type, it->first.m_id, m_url.c_str() );
									failedDevlst.push_back( it->first );
								}
								else if( 1==m_playErrCount || 0==(m_playErrCount%10) )
								{
									LOGMSG( "APlayer::Play playErrCount=%d. dev(type=%d, id=%d), url=\"%s\"\r\n", m_playErrCount, it->first.m_type, it->first.m_id, m_url.c_str() );
								}
							}
						}
						else
						{
							LOGMSG( "APlayer::Play dev cfg changed & not play. dev(type=%d, id=%d), url=\"%s\"\r\n", it->first.m_type, it->first.m_id, m_url.c_str() );
							failedDevlst.push_back( it->first );
						}
					}
				}

				if( !failedDevlst.empty() )
				{
					for( defvecDevKey::const_iterator it=failedDevlst.begin(); it!=failedDevlst.end(); ++it )
					{
						this->RemovePlayOut( *it );
					}
					//failedDevlst.clear();// 通知客户端后再清空

					if( !thisPlayed && !m_ao_device && m_devlst.empty() )
					{
						LOGMSG( "APlayer::Play all play dev stoped. url=\"%s\"\r\n", m_url.c_str() );

						defNS_FF::av_free_packet(&packet);
						return false;
					}
				}
			}
		}
	}

end:
	defNS_FF::av_free_packet(&packet);

	return true;
}

bool APlayer::Keepalive( const GSIOTDeviceKey &devkey )
{
	if( !m_fmt_ctx )
	{
		return false;
	}

	if( IOT_DEVICE_Unknown==devkey.m_type && 0==devkey.m_id )
	{
		return ( m_ao_device ? true:false );
	}
	else
	{
		if( !m_devlst.empty() )
		{
			return ( m_devlst.find( devkey ) != m_devlst.end() );
		}
	}

	return false;
}

bool APlayer::Keepalive( const defvecDevKey &vecdev, defvecDevKey &successDevlst, defvecDevKey &failedDevlst )
{
	LOGMSG( "APlayer::Keepalive size=%d\r\n", vecdev.size() );

	if( !m_fmt_ctx )
	{
		return false;
	}

	if( timeGetTime()-m_lastPlayed > defLastPlayedOverTime )
	{
		return false;
	}

	if( !successDevlst.empty() )
	{
		successDevlst.clear();
	}

	if( !failedDevlst.empty() )
	{
		failedDevlst.clear();
	}

	if( !vecdev.empty() )
	{
		for( size_t i=0; i<vecdev.size(); ++i )
		{
			if( this->Keepalive( vecdev[i] ) )
			{
				successDevlst.push_back( vecdev[i] );
			}
			else
			{
				failedDevlst.push_back( vecdev[i] );
			}
		}
	}

	return true;
}

void APlayer::Close(void)
{
	ReleaseRes();
	LOGMSG( "APlayer::Close\r\n" );
}

