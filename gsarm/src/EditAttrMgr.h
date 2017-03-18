#pragma once

#include "typedef.h"
#include "common.h"
#include "gloox/tag.h"

using namespace gloox;

typedef std::map<std::string,std::string> defmapEditAttr; // <������,����ֵ> 

// �༭���Թ����࣬�����������ݿ��ֶ�����ȫһ��
class EditAttrMgr
{
public:
	EditAttrMgr(void);
	~EditAttrMgr(void);

	virtual bool doEditAttrFromAttrMgr( const EditAttrMgr &attrMgr, defCfgOprt_ oprt = defCfgOprt_Modify ) { return false; };

	void tagEditAttr( Tag* iDest, const struTagParam &TagParam ) const;
	void UntagEditAttr( const Tag* iSrc );
	void AddEditAttr( const std::string &AttrName, const std::string &AttrValue );
	bool FindEditAttr( const std::string &AttrName, std::string &outAttrValue ) const;
	void ClearEditAttr();
	
	const defmapEditAttr& GetEditAttrMap() const
	{
		return m_AttrMap;
	}

	void SetResult( defGSReturn result )
	{
		m_result = result;
	}

	defGSReturn GetResult() const
	{
		return m_result;
	}

protected:
	defGSReturn m_result;		// Զ�̲������
	defmapEditAttr m_AttrMap;
};
