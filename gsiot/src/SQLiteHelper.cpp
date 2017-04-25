#include "SQLiteHelper.h"
#include "common.h"

SQLite::Database* SQLiteHelper::db = NULL;
SQLite::Database* SQLiteHelper::cfgdb = NULL; //jyc20170407 add
SQLiteHelper::SQLiteHelper(void)
{
	//path = getAppPath();
//jyc20170224 UBUNTU DIFF OPENWRT
/*if(OS_UBUNTU_FLAG)
	path.append("/home/chen/gsiot.db");
else
	path.append("/root/gsiot.db");*/
	
	//jyc20170227 modify
	path.append((std::string)ROOTDIR+"gsiot.db");
	try
	{
		if( !db )
		{
			db = new SQLite::Database( path.c_str(), SQLITE_OPEN_READWRITE );
		}
	}
	catch(...)
	{
		db = NULL;
		//LOGMSGEX( defLOGNAME, defLOG_ERROR, "Open DB failed!" );
		printf("Open DB failed!\n");
	}

	cfgpath.append((std::string) ROOTDIR + "gscfg.db");
	try {
		if (!cfgdb) {
			cfgdb = new SQLite::Database(path.c_str(), SQLITE_OPEN_READWRITE);
		}
	} catch (...) {
		cfgdb = NULL;
		//LOGMSGEX( defLOGNAME, defLOG_ERROR, "Open DB failed!" );
		printf("Open CFGDB failed!\n");
	}
}

SQLiteHelper::~SQLiteHelper(void)
{
}

void SQLiteHelper::FinalRelease()
{
	if(db)
	{
		delete db;
		db = NULL;
	}
}
