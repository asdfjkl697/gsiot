#include "GSIOTConfig.h"
#include "common.h"
#include "RunCode.h"
#include "gloox/util.h"
#include "DBUpdate.h"

bool operator< ( const struRelationMainDev &key1, const struRelationMainDev &key2 )
{
	if( key1.device_type != key2.device_type )
		return (key1.device_type < key2.device_type);

	return (key1.device_id < key2.device_id);
}

GSIOTConfig::GSIOTConfig(void)
	:SQLiteHelper(), m_serialport(0), m_DoInterval_ForSMS(120)
{
	m_UserMgr.SetNotifyHandler(this);
}


GSIOTConfig::~GSIOTConfig(void)
{
	m_UserMgr.DelNotifyHandler();

	if( !m_mapRelation.empty() )
	{
		m_mapRelation.clear();
	}

	m_NotifyHandlerList.clear();
}

bool GSIOTConfig::doDBUpdate()
{
	return g_doDBUpdate( db );
}

#include "SQLiteHelper.h"
#include "common.h"

void GSIOTConfig::LoadDB_cfg() {
	std::string cfgpath;
	static SQLite::Database *cfgdb;
	cfgpath.append((std::string) ROOTDIR + "gscfg.db");
	try {
		if (!cfgdb) {
			cfgdb = new SQLite::Database(cfgpath.c_str(),
					SQLITE_OPEN_READWRITE);
		}
	} catch (...) {
		cfgdb = NULL;
		printf("Open DB failed!\n");
	}

	SQLite::Statement query(*cfgdb, "select * from config");
	while (query.executeStep()) {
		uint32_t code = (uint32_t)query.getColumn(1).getInt();
		string tmp1 = query.getColumn(2);
		switch(code){
		case 101:
			this->m_jid = tmp1;
			break;
		case 102:
			this->m_password = tmp1;
			break;
		default:
			break;
		}
	}
}

bool GSIOTConfig::PreInit( const std::string &RunParam )
{
	LoadDB_cfg();

	if (!db) {
		return false;
	}

	const int cur_db_ver = g_GetDBVer(db);
	if ( GSIOT_DBVER != cur_db_ver) {
		return false;
	}
	//LOGMSGEX( defLOGNAME, defLOG_SYS, "db cur db_ver=%d", cur_db_ver );

	SQLite::Statement query(*this->db,
			"SELECT * FROM config ORDER BY id DESC LIMIT 1");
	if (query.executeStep()) {

		//this->m_serialnumber = query.getColumn(1);
		//this->m_jid = query.getColumn(2);
		//this->m_password = query.getColumn(3);
		string tmp1 = query.getColumn(1); //jyc20160823 add string tmpx
		this->m_serialnumber = tmp1;
		//string tmp2 = query.getColumn(2);
		//this->m_jid = tmp2;
		//string tmp3 = query.getColumn(3);
		//this->m_password = tmp3;
		//this->m_serialport = query.getColumn(4);
		//this->m_smtpserver = query.getColumn(5);
		//this->m_smtpuser = query.getColumn(6);
		//this->m_smtppassword = query.getColumn(7);
		//this->m_noticejid = query.getColumn(8);
		string tmp5 = query.getColumn(5);
		this->m_smtpserver = tmp5;
		string tmp6 = query.getColumn(6);
		this->m_smtpuser = tmp6;
		string tmp7 = query.getColumn(7);
		this->m_smtppassword = tmp7;
		string tmp8 = query.getColumn(8);
		this->m_noticejid = tmp8;

		if (!query.isColumnNull(9)) {
			//this->m_phone = query.getColumn(9); 
			string tmp9 = query.getColumn(9);
			this->m_phone = tmp9;
		}
		if (!query.isColumnNull(10)) {
			this->m_DoInterval_ForSMS = (uint32_t) query.getColumn(10).getInt();
		}
	}

	m_UserMgr.Init(db);
	defmapGSIOTUser &mapUser = m_UserMgr.GetList_User();
	for (defmapGSIOTUser::const_iterator it = mapUser.begin();
			it != mapUser.end(); ++it) {
		GSIOTUser *pUser = it->second;
		this->RefreshAuthOverview(pUser);
	}

	LoadRelation();
	LoadDB_vobj_config();

	return true;
}

void GSIOTConfig::FixOwnerAuth( GSIOTUser *pUser )
{
	if( !pUser )
	{
		return;
	}

	if( pUser->IsAll_Auth() )
	{
		// have authority, do not modify
		return;
	}

	if( this->isOwner(pUser->GetJid()) )
	{
		pUser->SetAll_Auth();
	}
}

bool GSIOTConfig::isOwner( const std::string &UserJID )
{
	if( GSIOTUser::isSame(GetNoticeJid(),UserJID) )
	{
		return true;
	}

	return false;
}

void GSIOTConfig::RefreshAuthOverview( GSIOTUser *pUser )
{
	if( isOwner( pUser->GetJid() ) )
	{
		pUser->Set_AuthOverview_UserType( defAuthOverview_UserType_Owner );
		pUser->Set_AuthOverview_Device( defAuthOverview_Auth_All );
		pUser->Set_AuthOverview_Module( defAuthOverview_Auth_All );
		return;
	}
	else if( GSIOTUserMgr::IsAdmin(pUser) )
	{
		pUser->Set_AuthOverview_UserType( defAuthOverview_UserType_SysAdmin );
	}
	else if( GSIOTUserMgr::IsGuest(pUser) )
	{
		pUser->Set_AuthOverview_UserType( defAuthOverview_UserType_Guest );
		pUser->SetName( defGuestName );
	}
	else
	{
		pUser->Set_AuthOverview_UserType( defAuthOverview_UserType_User );
	}

	pUser->Refresh_AuthOverview_Auth();
}

void GSIOTConfig::AddNotifyHandler( INotifyHandler_UserMgr *handler )
{
	if(handler)
		m_NotifyHandlerList.push_back(handler);
}

void GSIOTConfig::RemoveNotifyHandler( INotifyHandler_UserMgr *handler )
{
	if(handler)
		m_NotifyHandlerList.remove(handler);
}

void GSIOTConfig::OnNotify_UserMgr( defNotify_ notify, GSIOTUser *pUser )
{
	this->RefreshAuthOverview( pUser );

	std::list<INotifyHandler_UserMgr*>::const_iterator it = m_NotifyHandlerList.begin();
	for( ; it!=m_NotifyHandlerList.end(); ++it )
	{
		(*it)->OnNotify_UserMgr( notify, pUser );
	}
}
void GSIOTConfig::SaveToFile()
{
	SQLite::Statement query(*db,"SELECT id FROM config ORDER BY id DESC LIMIT 1");
	if(query.executeStep()){
		int id = query.getColumn(0);
		SQLite::Statement query(*this->db,"UPDATE config SET serialport=?,smtpserver=?,"\
               "smtpuser=?,smtppassword=?,noticejid=?,phone=?,do_interval_sms=?");
		query.bind(1, this->m_serialport);
		query.bind(2, this->m_smtpserver);
		query.bind(3, this->m_smtpuser);
		query.bind(4, this->m_smtppassword);
		query.bind(5, this->m_noticejid);
		query.bind(6, this->m_phone);
		query.bind(7, (int)this->m_DoInterval_ForSMS);
		query.exec();
	}else{
	    SQLite::Statement query(*this->db,"INSERT INTO config VALUES(NULL,:serialnumber,:jid,:password,:serialport,:smtpserver,"\
               ":smtpuser,:smtppassword,:noticejid,:phone,:do_interval_sms)");
		query.bind(1, this->m_serialnumber);
		query.bind(2, this->m_jid);
		query.bind(3, this->m_password);
		query.bind(4, this->m_serialport);
		query.bind(5, this->m_smtpserver);
		query.bind(6, this->m_smtpuser);
		query.bind(7, this->m_smtppassword);
		query.bind(8, this->m_noticejid);
		query.bind(9, this->m_phone);
		query.bind(10, (int)this->m_DoInterval_ForSMS);
		query.exec();
	}

	defmapGSIOTUser &mapUser = m_UserMgr.GetList_User();
	for( defmapGSIOTUser::const_iterator it=mapUser.begin(); it!=mapUser.end(); ++it )
	{
		GSIOTUser *pUser = it->second;

		this->RefreshAuthOverview( pUser );
	}

	g_Changed( defCfgOprt_Modify, IOT_Obj_SYS, 0 );
}


//=====================================================
// Relation cfg

bool GSIOTConfig::SetRelation( IOTDeviceType device_type, int device_id, const deflstRelationChild &ChildList )
{
	m_mapRelation[struRelationMainDev(device_type,device_id)] = ChildList;

	return SaveRelation( device_type, device_id, ChildList );
}

bool GSIOTConfig::GetRelation( IOTDeviceType device_type, int device_id, deflstRelationChild &ChildList )
{
	if( !ChildList.empty() )
	{
		ChildList.clear();
	}

	defmapRelation::const_iterator it = m_mapRelation.find( struRelationMainDev(device_type,device_id) );
	if( it != m_mapRelation.end() )
	{
		if( !ChildList.empty() )
		{
			ChildList.clear();
		}

		ChildList = it->second;
		return true;
	}

	return false;
}

// clear all dev_relation
bool GSIOTConfig::ClearAllRelation_SpecDev( IOTDeviceType device_type, int device_id )
{
	char sqlbuf[256] = {0};
	snprintf( sqlbuf, sizeof(sqlbuf), "DELETE FROM dev_relation where device_type=%d and device_id=%d", device_type, device_id );
	db->exec( sqlbuf );

	g_Changed( defCfgOprt_Modify, device_type, device_id, defIOTDeviceAddr_Other );
	return true;
}

// clear all relation type
bool GSIOTConfig::ClearAllRelation_SpecType( IOTDeviceType device_type )
{
	if( !m_mapRelation.empty() )
	{
		for( defmapRelation::iterator //const_iterator //jyc20160922 modify
		    it = m_mapRelation.begin(); it != m_mapRelation.end(); )
		{
			if( it->first.device_type == device_type )
			{
				m_mapRelation.erase(it); 
				it = m_mapRelation.begin();
				continue;
			}

			it++;
		}
	}

	char sqlbuf[256] = {0};
	snprintf( sqlbuf, sizeof(sqlbuf), "DELETE FROM dev_relation where device_type=%d", device_type );
	db->exec( sqlbuf );

	g_Changed( defCfgOprt_Modify, device_type, 0, defIOTDeviceAddr_Other );
	return true;
}

// clear all relation 
bool GSIOTConfig::ClearAllRelation_All()
{
	if( !m_mapRelation.empty() )
	{
		m_mapRelation.clear();
	}

	db->exec( "DELETE FROM dev_relation" );

	g_Changed( defCfgOprt_Modify, IOT_DEVICE_All, 0, defIOTDeviceAddr_Other );
	return true;
}


//=====================================================
// Relation db

bool GSIOTConfig::LoadRelation()
{
	if( !m_mapRelation.empty() )
	{
		m_mapRelation.clear();
	}
	
	SQLite::Statement query( *this->db, "select * from dev_relation where enable=1 order by device_type, device_id, sort_no" );
	while( query.executeStep() )
	{
		int col = 0;
		int id = query.getColumn(col++);
		uint32_t  enable = query.getColumn(col++).getInt();
		IOTDeviceType  device_type = (IOTDeviceType)query.getColumn(col++).getInt();
		int device_id = query.getColumn(col++).getInt();
		uint32_t  sort_no = query.getColumn(col++).getInt();
		IOTDeviceType  child_dev_type = (IOTDeviceType)query.getColumn(col++).getInt();
		uint32_t  child_dev_id = query.getColumn(col++).getInt();
		uint32_t  child_dev_subid = query.getColumn(col++).getInt();
		uint32_t  flag = query.getColumn(col++).getInt();

		if( enable )
		{
			defmapRelation::iterator it = m_mapRelation.find( struRelationMainDev(device_type,device_id) );
			if( it != m_mapRelation.end() )
			{
				it->second.push_back( struRelationChild( child_dev_type, child_dev_id, child_dev_subid ) );
			}
			else
			{
				deflstRelationChild ChildList;
				ChildList.push_back( struRelationChild( child_dev_type, child_dev_id, child_dev_subid ) );
				m_mapRelation[struRelationMainDev(device_type,device_id)] = ChildList;
			}
		}
	}

	return true;
}

bool GSIOTConfig::SaveRelation( IOTDeviceType device_type, int device_id, const deflstRelationChild &ChildList )
{
	if( ChildList.empty() )
	{
		return ClearAllRelation_SpecDev( device_type, device_id );
	}
	
	UseDbTransAction dbta(db);

	//set to zero
	char sqlbuf[1024] = {0};
	snprintf( sqlbuf, sizeof(sqlbuf), "update dev_relation set enable=0 where device_type=%d and device_id=%d", device_type, device_id );
	db->exec( sqlbuf );

	int sort_no = 1;
	for( deflstRelationChild::const_iterator it=ChildList.begin(); it!=ChildList.end(); ++it, ++sort_no )
	{
		if( IsInDB_Relation( device_type, device_id, it->child_dev_type, it->child_dev_id, it->child_dev_subid ) )
		{
			snprintf( sqlbuf, sizeof(sqlbuf), "update dev_relation set enable=1, sort_no=%d, flag=%d where device_type=%d and device_id=%d and child_dev_type=%d and child_dev_id=%d and child_dev_subid=%d", 
				sort_no, int(0), device_type, device_id, it->child_dev_type, it->child_dev_id, it->child_dev_subid );
			db->exec( sqlbuf );
		}
		else
		{
			SQLite::Statement query( *this->db, "INSERT INTO dev_relation VALUES(NULL,:enable,:device_type,:device_id,:sort_no,"\
				":child_dev_type,:child_dev_id,:child_dev_subid,:flag)");
			query.bind(1, int(1)); // enable
			query.bind(2, device_type);
			query.bind(3, device_id);
			query.bind(4, sort_no);
			query.bind(5, it->child_dev_type);
			query.bind(6, it->child_dev_id);
			query.bind(7, it->child_dev_subid);
			query.bind(8, int(0)); // flag
			query.exec();
		}
	}

	g_Changed( defCfgOprt_Modify, device_type, device_id, defIOTDeviceAddr_Other );
	return true;
}

bool GSIOTConfig::IsInDB_Relation( IOTDeviceType device_type, int device_id, IOTDeviceType child_dev_type, int child_dev_id, int child_dev_subid )
{
	char sqlbuf[1024] = {0};
	snprintf( sqlbuf, sizeof(sqlbuf), "SELECT id FROM dev_relation where device_type=%d and device_id=%d and child_dev_type=%d and child_dev_id=%d and child_dev_subid=%d", 
		device_type, device_id, child_dev_type, child_dev_id, child_dev_subid );
	
	SQLite::Statement query( *db, sqlbuf );
	if( query.executeStep() )
	{
		return true;
	}

	return false;
}


//=====================================================
// VObj Cfg

//get Vobj cfg message
bool GSIOTConfig::VObj_Get( int id, stru_vobj_config &cfgobj ) const
{
	defmapVObjConfig::const_iterator it = this->m_mapVObj.find(id);
	if( it != this->m_mapVObj.end() )
	{
		cfgobj = it->second;
		return true;
	}

	return false;
}

// Vobj check exist
defGSReturn GSIOTConfig::VObj_CheckExist( const stru_vobj_config &cfgobj, std::string *strerr )
{
	for( defmapVObjConfig::const_iterator it = m_mapVObj.begin(); it!= m_mapVObj.end(); ++it )
	{
		if( cfgobj.id == it->second.id )
			continue;

		if( cfgobj.name == it->second.name )
		{
			if( strerr ) *strerr = "name is exist";
			return defGSReturn_SameName;
		}
	}

	return defGSReturn_Success;
}

// Vobj add message
defGSReturn GSIOTConfig::VObj_Add( stru_vobj_config &cfgobj, std::string *strerr )
{
	assert(IOT_DEVICE_Unknown!=cfgobj.vobj_type);
	if( IOT_DEVICE_Unknown==cfgobj.vobj_type )
	{
		return defGSReturn_ErrParam;
	}

	defGSReturn ret = VObj_CheckExist( cfgobj, strerr );
	if( macGSFailed(ret) )
	{
		return ret;
	}

	cfgobj.id = db_Insert_vobj_config( cfgobj );
	if( cfgobj.id < 0 )
	{
		if( strerr ) *strerr = "save failed";
		return defGSReturn_SaveFailed;
	}

	m_mapVObj[cfgobj.id] = cfgobj;

	LOGMSG( "VObj_Add id=%d, aftercount(%d)", cfgobj.id, m_mapVObj.size() );

	return defGSReturn_Success;
}

// Vobj delete message
defGSReturn GSIOTConfig::VObj_Delete( const IOTDeviceType vobj_type, const int id )
{
	db_Delete_vobj_config( vobj_type, id );

	defmapVObjConfig::iterator it = this->m_mapVObj.find(id);
	if( it != this->m_mapVObj.end() )
	{
		ClearAllRelation_SpecDev( vobj_type, id );

		m_mapVObj.erase(it);

		LOGMSG( "VObj_Delete id=%d, aftercount(%d)", id, m_mapVObj.size() );
		return defGSReturn_Success;
	}

	return defGSReturn_NoExist;
}

// Vobj modify message
defGSReturn GSIOTConfig::VObj_Modify( const stru_vobj_config &cfgobj, std::string *strerr )
{
	assert(IOT_DEVICE_Unknown!=cfgobj.vobj_type);
	if( IOT_DEVICE_Unknown==cfgobj.vobj_type )
	{
		return defGSReturn_ErrParam;
	}

	defGSReturn ret = VObj_CheckExist( cfgobj, strerr );
	if( macGSFailed(ret) )
	{
		return ret;
	}

	defmapVObjConfig::iterator it = this->m_mapVObj.find(cfgobj.id);
	if( it != this->m_mapVObj.end() )
	{
		it->second = cfgobj;

		db_Update_vobj_config( cfgobj );

		return defGSReturn_Success;
	}

	return defGSReturn_NoExist;
}

// load Vobj config
bool GSIOTConfig::LoadDB_vobj_config()
{
	//LOGMSGEX( defLOGNAME, defLOG_SYS, "LoadDB_vobj_config" );

	SQLite::Statement query(*this->db,"select * from vobj_config");
	while( query.executeStep() )
	{
		stru_vobj_config cfg;

		int col = 0;
		cfg.id = query.getColumn(col++).getInt();
		cfg.enable = query.getColumn(col++).getInt();
		cfg.vobj_type = (IOTDeviceType)query.getColumn(col++).getInt();
		cfg.vobj_flag = query.getColumn(col++).getInt();

		//cfg.name = query.getColumn(col++);
		//cfg.ver = query.getColumn(col++);
		string strtmp1 = query.getColumn(col++); //jyc20160823
		cfg.name = strtmp1;
		string strtmp2 = query.getColumn(col++);
		cfg.ver = strtmp2;

		cfg.param[0] = query.getColumn(col++).getInt();
		cfg.param[1] = query.getColumn(col++).getInt();
		cfg.param[2] = query.getColumn(col++).getInt();
		cfg.param[3] = query.getColumn(col++).getInt();
		cfg.param[4] = query.getColumn(col++).getInt();
		cfg.param[5] = query.getColumn(col++).getInt();

		//cfg.param_str[0] = query.getColumn(col++);
		//cfg.param_str[1] = query.getColumn(col++);
		string strtmp3 = query.getColumn(col++);
		cfg.param_str[0] = strtmp3;
		string strtmp4 = query.getColumn(col++);
		cfg.param_str[1] = strtmp4;

		if( IOT_DEVICE_Unknown==cfg.vobj_type )
		{
			continue;
		}

		m_mapVObj[cfg.id] = cfg;
	}

	return true;
}


//=====================================================
// VObj db

// 
int GSIOTConfig::db_Insert_vobj_config( const stru_vobj_config &cfg )
{
	SQLite::Statement  query(*db,"INSERT INTO vobj_config VALUES(NULL,:enable,:vobj_type,:vobj_flag,:name,:ver,"\
		":param1,:param2,:param3,:param4,:param5,:param6,"\
		":param_str1,:param_str2)");

	int col = 1;
	query.bind( col++, cfg.enable );
	query.bind( col++, cfg.vobj_type );
	query.bind( col++, cfg.vobj_flag );
	query.bind( col++, cfg.name );
	query.bind( col++, cfg.ver );
	query.bind( col++, cfg.param[0] );
	query.bind( col++, cfg.param[1] );
	query.bind( col++, cfg.param[2] );
	query.bind( col++, cfg.param[3] );
	query.bind( col++, cfg.param[4] );
	query.bind( col++, cfg.param[5] );
	query.bind( col++, cfg.param_str[0] );
	query.bind( col++, cfg.param_str[1] );
	query.exec();

	int id = db->execAndGet("SELECT id FROM vobj_config ORDER BY id DESC LIMIT 1");
	
	g_Changed( defCfgOprt_Add, cfg.vobj_type, cfg.id );
	return id;
}

// 
bool GSIOTConfig::db_Update_vobj_config( const stru_vobj_config &cfg )
{
	char sqlbuf[4096] = {0};

	snprintf( sqlbuf, sizeof(sqlbuf), "update vobj_config set enable=%d, vobj_type=%d, vobj_flag=%d, name='%s', ver='%s',"\
		"param1=%d, param2=%d, param3=%d, param4=%d, param5=%d, param6=%d, param_str1='%s', param_str2='%s' where vobj_type=%d and id=%d", 
		cfg.enable, cfg.vobj_type, cfg.vobj_flag, cfg.name.c_str(), cfg.ver.c_str(),
		cfg.param[0], cfg.param[1], cfg.param[2], cfg.param[3], cfg.param[4], cfg.param[5], 
		cfg.param_str[0].c_str(), cfg.param_str[1].c_str(), cfg.vobj_type, cfg.id );

	db->exec( sqlbuf );

	g_Changed( defCfgOprt_Modify, cfg.vobj_type, cfg.id );
	return true;
}

// 
bool GSIOTConfig::db_Delete_vobj_config( const IOTDeviceType vobj_type, const int id )
{
	char sqlbuf[1024] = {0};

	snprintf( sqlbuf, sizeof(sqlbuf), "DELETE FROM vobj_config WHERE vobj_type=%d and id=%d", vobj_type, id );
	db->exec( sqlbuf );

	g_Changed( defCfgOprt_Delete, vobj_type, id );
	return true;
}
