#ifndef CAMERAFOCAL_H_
#define CAMERAFOCAL_H_

#include "gloox/tag.h"
#include "typedef.h"

using namespace gloox;

class CameraFocal
{
private:
	uint32_t m_max;
	uint32_t m_min;
	uint32_t m_zoom;
	int m_autoflag; // -2:�Զ��������; -1:�Զ�Ĭ�����; 0:���Զ�; >0:�Զ����Զ��������ʱ��

public:
	CameraFocal(int min,int max,int zoom);
	CameraFocal( const Tag* tag = 0 );
	~CameraFocal(void);

	uint32_t GetMin() const
	{
		return m_max;
	}
	uint32_t GetMax() const
	{
		return m_min;
	}
	uint32_t GetZoom() const
	{
		return m_zoom;
	}

	int getAutoflag() const
	{
		return this->m_autoflag;
	}
	
	void untag( const Tag* tag);
	Tag* tag() const;
	virtual CameraFocal* clone() const
	{
		return new CameraFocal(*this);
	}
};

#endif

