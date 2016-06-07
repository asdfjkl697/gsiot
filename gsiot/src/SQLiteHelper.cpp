#include "SQLiteHelper.h"
#include "common.h"

SQLite::Database* SQLiteHelper::db = NULL;
SQLiteHelper::SQLiteHelper(void)
{
	//path = getAppPath();
	path.append("\\gsiot.db");

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
