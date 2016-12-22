#include <iostream>
#include <cstdlib>

#include <stdio.h>
#include <string.h>
#include <stdlib.h>


#include <sqlite3.h>

#include <lzal.h>
#include <lzsql.h>

using namespace std;

int lz_sql_insert(string msg){
	sqlite3 *db;  
    FILE *fd;  
	int ret;
	char* errmsg;
	std::string sql;
    sqlite3_open("test.db", &db);  //打开（或新建）一个数据库 
	sql = "insert into tb(id,data) values((SELECT max(id) FROM tb)+1,'"+msg+"')";
	ret = sqlite3_exec(db,sql.data(),NULL,NULL,&errmsg);
    printf("Insert a record %s\n",ret == SQLITE_OK ? "OK":"FAIL");

	//fclose(fd); 
    sqlite3_close(db); //关闭数据库 

	return 1;
}

  
/* callback函数中： 
 * arg: 是从主函数传进来的参数 
 * values: 是查询结果的值 
 * names: 查询结果列的名字 
 */  
int wf_callback(void *arg, int nr, char **values, char **names)  
{  
    int i;  
    FILE *fd;  
    char str[128];  
    fd = (FILE *)arg; //将void *的参数强制转换为FILE *   
    for (i=0; i<nr; i++) { //将查询结果写入到文件中   
        memset(str, '\0', 128);  
        sprintf(str, "\t%s\t", values[i]);  
        fwrite(str, sizeof(char), sizeof(str), fd);  
    }  
    memset(str, '\0', 128);  
    fwrite("\n", sizeof(char), 2, fd); //写完一条记录换行   
    return 0;  //callback函数正常返回0   
}  
  
int test_sql()  
{  
    char sql[128];  
    sqlite3 *db;  
    FILE *fd;  
  
    sqlite3_open("test.db", &db);  //打开（或新建）一个数据库   
    memset(sql, '\0', 128);  
    /* 新建一张表 */  
    strcpy(sql, "create table tb(id INTEGER PRIMARY KEY, data TEXT)");  
    sqlite3_exec(db, sql, NULL, NULL, NULL);  
  
    /* 新建一个文件，把数据库的查询结果保存在该文件中 */  
    fd = fopen("test.txt", "w");  
    fwrite("Result: \n", sizeof(char), 10, fd);  
    memset(sql, '\0', 128);  
    strcpy(sql, "select * from tb");  
    sqlite3_exec(db, sql, wf_callback, fd, NULL);  
    fclose(fd);  
  
    sqlite3_close(db); //关闭数据库   
    return 0;  
} 