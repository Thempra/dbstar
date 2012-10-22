#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <semaphore.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>

#include "common.h"
#include "sqlite3.h"
#include "sqlite.h"
#include "porting.h"

/*���ݿ���뼰ʱ�򿪡���ʱ�رգ����ܲ��ô򿪼������ķ�ʽ����Ϊ�ϲ�Ӧ��Ҳ������ʹ��*/

///used in this file
static sqlite3* g_db = NULL;												///the pointer of database created or opened
static int s_sqlite_init_flag = 0;

static int createTable(char* name);
static void closeDatabase();

static int createDatabase()
{
	char	*errmsgOpen=NULL;
	int		ret = 0;
	
	if(g_db!=NULL){
		DEBUG("the database has opened\n");
		ret = 0;
	}
	else
	{
		char database_uri[64];
		memset(database_uri, 0, sizeof(database_uri));
		if(-1==database_uri_get(database_uri, sizeof(database_uri))){
			DEBUG("get database uri failed\n");
			return -1;
		}
		
		DEBUG("database_uri: %s\n", database_uri);
		if(-1==dir_exist_ensure(database_uri)){
			return -1;
		}

		if(SQLITE_OK!=sqlite3_open(database_uri,&g_db)){
			ERROROUT("can't open database: %s\n", database_uri);
			ret += -1;
		}
		else{
			/// open foreign key support
			if(sqlite3_exec(g_db,"PRAGMA foreign_keys=ON;",NULL,NULL,&errmsgOpen)
				|| NULL!=errmsgOpen){
				ERROROUT("can't open foreign_keys\n");
				DEBUG("database errmsg: %s\n", errmsgOpen);
				ret += -1;
			}
			else{
				if(createTable("Global")){
					ERROROUT("can not create table \"Global\"\n");
					ret += -1;
				}
				else{
					DEBUG("create table \"Global\" OK\n");
					ret += 0;
				}
				
				if(createTable("Initialize")){
					ERROROUT("can not create table \"Initialize\"\n");
					ret += -1;
				}
				else{
					DEBUG("create table \"Initialize\" OK\n");
					ret += 0;
				}
				if(createTable("Channel")){
					ERROROUT("can not create table \"Channel\"\n");
					ret += -1;
				}
				else{
					DEBUG("create table \"Channel\" OK\n");
					ret += 0;
				}
				
				if(createTable("Service")){
					ERROROUT("can not create table \"Service\"\n");
					ret += -1;
				}
				else{
					DEBUG("create table \"Service\" OK\n");
					ret += 0;
				}
				
				if(createTable("ResStr")){
					ERROROUT("can not create table \"ResStr\"\n");
					ret += -1;
				}
				else{
					DEBUG("create table \"ResStr\" OK\n");
					ret += 0;
				}
				
				if(createTable("ResPoster")){
					ERROROUT("can not create table \"ResPoster\"\n");
					ret += -1;
				}
				else{
					DEBUG("create table \"ResPoster\" OK\n");
					ret += 0;
				}
				if(createTable("ResTrailer")){
					ERROROUT("can not create table \"ResTrailer\"\n");
					ret += -1;
				}
				else{
					DEBUG("create table \"ResTrailer\" OK\n");
					ret += 0;
				}
				if(createTable("ResSubTitle")){
					ERROROUT("can not create table \"ResSubTitle\"\n");
					ret += -1;
				}
				else{
					DEBUG("create table \"ResSubTitle\" OK\n");
					ret += 0;
				}
				if(createTable("ResExtension")){
					ERROROUT("can not create table \"ResExtension\"\n");
					ret += -1;
				}
				else{
					DEBUG("create table \"ResExtension\" OK\n");
					ret += 0;
				}
				if(createTable("ResExtensionFile")){
					ERROROUT("can not create table \"ResExtensionFile\"\n");
					ret += -1;
				}
				else{
					DEBUG("create table \"ResExtensionFile\" OK\n");
					ret += 0;
				}
				if(createTable("Column")){
					ERROROUT("can not create table \"Column\"\n");
					ret += -1;
				}
				else{
					DEBUG("create table \"Column\" OK\n");
					ret += 0;
				}
				if(createTable("ColumnEntity")){
					ERROROUT("can not create table \"ColumnEntity\"\n");
					ret += -1;
				}
				else{
					DEBUG("create table \"ColumnEntity\" OK\n");
					ret += 0;
				}
				if(createTable("Product")){
					ERROROUT("can not create table \"Product\"\n");
					ret += -1;
				}
				else{
					DEBUG("create table \"Product\" OK\n");
					ret += 0;
				}
				if(createTable("PublicationsSet")){
					ERROROUT("can not create table \"PublicationsSet\"\n");
					ret += -1;
				}
				else{
					DEBUG("create table \"PublicationsSet\" OK\n");
					ret += 0;
				}
				if(createTable("Publication")){
					ERROROUT("can not create table \"Publication\"\n");
					ret += -1;
				}
				else{
					DEBUG("create table \"Publication\" OK\n");
					ret += 0;
				}
				if(createTable("MultipleLanguageInfoVA")){
					ERROROUT("can not create table \"MultipleLanguageInfoVA\"\n");
					ret += -1;
				}
				else{
					DEBUG("create table \"MultipleLanguageInfoVA\" OK\n");
					ret += 0;
				}
				if(createTable("MultipleLanguageInfoRM")){
					ERROROUT("can not create table \"MultipleLanguageInfoRM\"\n");
					ret += -1;
				}
				else{
					DEBUG("create table \"MultipleLanguageInfoRM\" OK\n");
					ret += 0;
				}
				if(createTable("MultipleLanguageInfoApp")){
					ERROROUT("can not create table \"MultipleLanguageInfoApp\"\n");
					ret += -1;
				}
				else{
					DEBUG("create table \"MultipleLanguageInfoApp\" OK\n");
					ret += 0;
				}
				if(createTable("MFile")){
					ERROROUT("can not create table \"MFile\"\n");
					ret += -1;
				}
				else{
					DEBUG("create table \"MFile\" OK\n");
					ret += 0;
				}
				if(createTable("Message")){
					ERROROUT("can not create table \"Message\"\n");
					ret += -1;
				}
				else{
					DEBUG("create table \"Message\" OK\n");
					ret += 0;
				}
				if(createTable("GuideList")){
					ERROROUT("can not create table \"GuideList\"\n");
					ret += -1;
				}
				else{
					DEBUG("create table \"GuideList\" OK\n");
					ret += 0;
				}
				if(createTable("ProductDesc")){
					ERROROUT("can not create table \"ProductDesc\"\n");
					ret += -1;
				}
				else{
					DEBUG("create table \"ProductDesc\" OK\n");
					ret += 0;
				}
				if(createTable("Preview")){
					ERROROUT("can not create table \"Preview\"\n");
					ret += -1;
				}
				else{
					DEBUG("create table \"Preview\" OK\n");
					ret += 0;
				}
				if(createTable("RejectRecv")){
					ERROROUT("can not create table \"RejectRecv\"\n");
					ret += -1;
				}
				else{
					DEBUG("create table \"RejectRecv\" OK\n");
					ret += 0;
				}
				if(createTable("SProduct")){
					ERROROUT("can not create table \"SProduct\"\n");
					ret += -1;
				}
				else{
					DEBUG("create table \"SProduct\" OK\n");
					ret += 0;
				}
			}
			sqlite3_free(errmsgOpen);
		}
		closeDatabase();
	}
	
	return (ret<0?-1:0);
}

static int openDatabase()
{
	int ret = -1;
	
	if(g_db!=NULL){
		DEBUG("the database has opened\n");
		ret = 0;
	}
	else
	{
		char database_uri[64];
		memset(database_uri, 0, sizeof(database_uri));
		if(-1==database_uri_get(database_uri, sizeof(database_uri))){
			DEBUG("get database uri failed\n");
			return -1;
		}
		if(SQLITE_OK!=sqlite3_open(database_uri,&g_db)){
			ERROROUT("can't open database\n");
			ret = -1;
		}
		else
			ret = 0;
	}
	
	return ret;
}

static void closeDatabase()
{
	if(g_db!=NULL)
	{
		sqlite3_close(g_db);
		g_db=NULL;
	}
	else{
		DEBUG("g_db is NULL, can not do database close action\n");
	}
	
	return;
}

static int createTable(char* name)
{
	char* errmsg=NULL;
	char ** l_result=NULL;									    	///result of tables in database
	int l_row=0;                                            	///the row of result
	int l_column=0;									        	///the column of result
	char sqlite_cmd[1024];
	int ret = -1;
	
	memset(sqlite_cmd, 0, sizeof(sqlite_cmd));
	snprintf(sqlite_cmd, sizeof(sqlite_cmd), "SELECT name FROM sqlite_master WHERE type='table' AND name='%s';", name);
	if(sqlite3_get_table(g_db,sqlite_cmd,&l_result,&l_row,&l_column,&errmsg))
	{
		ERROROUT("read tables from database failed.");
		ret = -1;
	}
	else{
		if(l_row>0){
			DEBUG("tabel \"%s\" is exist\n", name);
			ret = 0;
		}
		else{
			sqlite3_free(errmsg);
			ret = 0;
			/*���ｨ�����Ŀ���ǲ�ѯ�����Ǵ洢�����Բ������ڲ�ѯ��ͼƬ���ṹ�塢������*/
			if(!strcmp(name,"Global"))
			{
				snprintf(sqlite_cmd, sizeof(sqlite_cmd),\
					"CREATE TABLE %s(\
Name	NVARCHAR(64) PRIMARY KEY,\
Value	NVARCHAR(128),\
Param	NVARCHAR(1024));", name);
			}
			else if(!strcmp(name,"Initialize"))
			{
				snprintf(sqlite_cmd, sizeof(sqlite_cmd),\
					"CREATE TABLE %s(\
PushFlag	NVARCHAR(64) PRIMARY KEY,\
XMLName	NVARCHAR(64),\
Version	NVARCHAR(64),\
StandardVersion	NVARCHAR(32),\
URI		NVARCHAR(256));", name);
			}
			else if(!strcmp(name,"Channel"))
			{
				snprintf(sqlite_cmd, sizeof(sqlite_cmd),\
					"CREATE TABLE %s(\
pid	NVARCHAR(64) PRIMARY KEY,\
pidtype	NVARCHAR(64),\
multiURI	NVARCHAR(64));", name);
			}
			else if(!strcmp(name,"Service"))
			{
				snprintf(sqlite_cmd, sizeof(sqlite_cmd),\
					"CREATE TABLE %s(\
ServiceID	NVARCHAR(64) PRIMARY KEY,\
RegionCode	NVARCHAR(64),\
OnlineTime	RCHAR(32),\
OfflineTime	RCHAR(32));", name);
			}
			else if(!strcmp(name,"ResStr"))
			{
				snprintf(sqlite_cmd, sizeof(sqlite_cmd),\
					"CREATE TABLE %s(\
ObjectName	NVARCHAR(64),\
EntityID	NVARCHAR(64),\
StrLang		NVARCHAR(32),\
StrName		NVARCHAR(64),\
Extension	NVARCHAR(64),\
StrValue	NVARCHAR(1024),\
PRIMARY KEY (ObjectName,EntityID,StrLang,StrName,Extension));", name);
			}
			else if(!strcmp(name,"ResPoster"))
			{
				snprintf(sqlite_cmd, sizeof(sqlite_cmd),\
					"CREATE TABLE %s(\
ObjectName	NVARCHAR(64),\
EntityID	NVARCHAR(64),\
PosterID	NVARCHAR(64),\
PosterName	NVARCHAR(64),\
PosterURI	NVARCHAR(256),\
PRIMARY KEY (ObjectName,EntityID,PosterID));", name);
			}
			else if(!strcmp(name,"ResTrailer"))
			{
				snprintf(sqlite_cmd, sizeof(sqlite_cmd),\
					"CREATE TABLE %s(\
ObjectName	NVARCHAR(64),\
EntityID	NVARCHAR(64),\
TrailerID	NVARCHAR(64),\
TrailerName	NVARCHAR(64),\
TrailerURI	NVARCHAR(256),\
PRIMARY KEY (ObjectName,EntityID,TrailerID));", name);
			}
			else if(!strcmp(name,"ResSubTitle"))
			{
				snprintf(sqlite_cmd, sizeof(sqlite_cmd),\
					"CREATE TABLE %s(\
ObjectName	NVARCHAR(64),\
EntityID	NVARCHAR(64),\
SubTitleID	NVARCHAR(64),\
SubTitleName	NVARCHAR(64),\
SubTitleLanguage	NVARCHAR(64),\
SubTitleURI	NVARCHAR(256),\
PRIMARY KEY (ObjectName,EntityID,SubTitleID));", name);
			}
			else if(!strcmp(name,"ResExtension"))
			{
				snprintf(sqlite_cmd, sizeof(sqlite_cmd),\
					"CREATE TABLE %s(\
ObjectName	NVARCHAR(256),\
EntityID	NVARCHAR(64),\
Name	NVARCHAR(64),\
Type	NVARCHAR(64),\
PRIMARY KEY (ObjectName,EntityID,Name));", name);
			}
			else if(!strcmp(name,"ResExtensionFile"))
			{
				snprintf(sqlite_cmd, sizeof(sqlite_cmd),\
					"CREATE TABLE %s(\
ObjectName	NVARCHAR(256),\
EntityID	NVARCHAR(64),\
FileID	NVARCHAR(64),\
FileName	NVARCHAR(64),\
FileURI	NVARCHAR(256),\
PRIMARY KEY (ObjectName,EntityID,FileID));", name);
			}
			else if(!strcmp(name,"Column"))
			{
				snprintf(sqlite_cmd, sizeof(sqlite_cmd),\
					"CREATE TABLE %s(\
ColumnID	NVARCHAR(64) PRIMARY KEY,\
ParentID	NVARCHAR(64),\
Path	NVARCHAR(256),\
ColumnType	NVARCHAR(256),\
ColumnIcon_losefocus	NVARCHAR(256),\
ColumnIcon_getfocus	NVARCHAR(256),\
ColumnIcon_onclick	NVARCHAR(256),\
SequenceNum	INTEGER,\
URI	NVARCHAR(256));", name);
			}
			else if(!strcmp(name,"ColumnEntity"))
			{
				snprintf(sqlite_cmd, sizeof(sqlite_cmd),\
					"CREATE TABLE %s(\
ColumnID	NVARCHAR(64),\
EntityID	NVARCHAR(64),\
EntityType	NVARCHAR(64));", name);
			}
			else if(!strcmp(name,"Product"))
			{
				snprintf(sqlite_cmd, sizeof(sqlite_cmd),\
					"CREATE TABLE %s(\
ProductID	NVARCHAR(64) PRIMARY KEY,\
ProductType	NVARCHAR(64),\
Flag	NVARCHAR(64),\
OnlineDate	CHAR(32),\
OfflineDate	CHAR(32),\
IsReserved	CHAR(32),\
Price	NVARCHAR(32),\
CurrencyType	NVARCHAR(32),\
DRMFile	NVARCHAR(256),\
ColumnID	NVARCHAR(64),\
IsAuthorized	NVARCHAR(64),\
VODNum	NVARCHAR(64),\
VODPlatform	NVARCHAR(256));", name);
			}
			else if(!strcmp(name,"PublicationsSet"))
			{
				snprintf(sqlite_cmd, sizeof(sqlite_cmd),\
					"CREATE TABLE %s(\
SetID	NVARCHAR(64) PRIMARY KEY,\
ColumnID	NVARCHAR(64),\
ProductID	NVARCHAR(64),\
URI	NVARCHAR(256),\
TotalSize	NVARCHAR(64),\
ProductDescID	NVARCHAR(64),\
ReceiveStatus	NVARCHAR(64),\
PushTime	NVARCHAR(128),\
IsReserved	NVARCHAR(64),\
Visible	NVARCHAR(64),\
Favorite	NVARCHAR(64),\
IsAuthorized	NVARCHAR(64),\
VODNum	NVARCHAR(64),\
VODPlatform	NVARCHAR(256));", name);
			}
			else if(!strcmp(name,"Publication"))
			{
				snprintf(sqlite_cmd, sizeof(sqlite_cmd),\
					"CREATE TABLE %s(\
PublicationID	NVARCHAR(64) PRIMARY KEY,\
ColumnID	NVARCHAR(64),\
ProductID	NVARCHAR(64),\
URI	NVARCHAR(256),\
DescURI	NVARCHAR(256),\
TotalSize	NVARCHAR(64),\
ProductDescID	NVARCHAR(64),\
ReceiveStatus	NVARCHAR(64),\
PushTime	NVARCHAR(128),\
PublicationType	NVARCHAR(64),\
IsReserved	CHAR(32),\
Visible	CHAR(32),\
DRMFile	NVARCHAR(256),\
SetID	NVARCHAR(64),\
IndexInSet	NVARCHAR(32),\
Favorite	NVARCHAR(32),\
Bookmark	NVARCHAR(32),\
IsAuthorized	NVARCHAR(64),\
VODNum	NVARCHAR(64),\
VODPlatform	NVARCHAR(256),\
FileID	NVARCHAR(64),\
FileSize	NVARCHAR(64),\
FileURI	NVARCHAR(256),\
FileType	NVARCHAR(64),\
FileFormat	NVARCHAR(32),\
Duration	NVARCHAR(32),\
Resolution	NVARCHAR(32),\
BitRate	NVARCHAR(32),\
CodeFormat	NVARCHAR(32));", name);
			}
/*

*/
			else if(!strcmp(name,"MultipleLanguageInfoVA"))
			{
				snprintf(sqlite_cmd, sizeof(sqlite_cmd),\
					"CREATE TABLE %s(\
PublicationID	NVARCHAR(64),\
infolang	NVARCHAR(64),\
PublicationDesc	NVARCHAR(1024),\
ImageDefinition	NVARCHAR(32),\
Keywords	NVARCHAR(256),\
Area	NVARCHAR(64),\
Language	NVARCHAR(64),\
Episode	NVARCHAR(32),\
AspectRatio	NVARCHAR(32),\
AudioChannel	NVARCHAR(32),\
Director	NVARCHAR(128),\
Actor	NVARCHAR(256),\
Audience	NVARCHAR(64),\
Model	NVARCHAR(32),\
PRIMARY KEY (PublicationID,infolang));", name);
			}
			else if(!strcmp(name,"MultipleLanguageInfoRM"))
			{
				snprintf(sqlite_cmd, sizeof(sqlite_cmd),\
					"CREATE TABLE %s(\
PublicationID	NVARCHAR(64),\
infolang	NVARCHAR(64),\
PublicationDesc	NVARCHAR(1024),\
Keywords	NVARCHAR(256),\
Publisher	NVARCHAR(128),\
Area	NVARCHAR(64),\
Language	NVARCHAR(64),\
Episode	NVARCHAR(32),\
AspectRatio	NVARCHAR(32),\
VolNum	NVARCHAR(64),\
ISSN	NVARCHAR(64),\
PRIMARY KEY (PublicationID,infolang));", name);
			}
			else if(!strcmp(name,"MultipleLanguageInfoApp"))
			{
				snprintf(sqlite_cmd, sizeof(sqlite_cmd),\
					"CREATE TABLE %s(\
PublicationID	NVARCHAR(64) PRIMARY KEY,\
infolang	NVARCHAR(64),\
PublicationDesc	NVARCHAR(1024),\
Keywords	NVARCHAR(256),\
Category	NVARCHAR(64),\
Released	NVARCHAR(64),\
AppVersion	NVARCHAR(64),\
Language	NVARCHAR(64),\
Developer	NVARCHAR(64),\
Rated	NVARCHAR(64),\
Requirements	NVARCHAR(64));", name);
			}
			else if(!strcmp(name,"Message"))
			{
				snprintf(sqlite_cmd, sizeof(sqlite_cmd),\
					"CREATE TABLE %s(\
MessageID	NVARCHAR(64),\
type	NVARCHAR(64),\
displayForm	NVARCHAR(64),\
StartTime	CHAR(32),\
EndTime		CHAR(32),\
Interval	CHAR(32));", name);
			}
			else if(!strcmp(name,"GuideList"))
			{
				snprintf(sqlite_cmd, sizeof(sqlite_cmd),\
					"CREATE TABLE %s(\
DateValue	NVARCHAR(64),\
GuideListID	NVARCHAR(64),\
productID	NVARCHAR(64),\
PublicationID	NVARCHAR(64),\
URI	NVARCHAR(256),\
TotalSize	NVARCHAR(64),\
ProductDescID	NVARCHAR(64),\
ReceiveStatus	NVARCHAR(64),\
PushTime	NVARCHAR(128),\
UserStatus	NVARCHAR(64),\
PRIMARY KEY (DateValue,PublicationID));", name);
			}
			else if(!strcmp(name,"ProductDesc"))
			{
				snprintf(sqlite_cmd, sizeof(sqlite_cmd),\
					"CREATE TABLE %s(\
ReceiveType	NVARCHAR(64),\
ProductDescID	NVARCHAR(64),\
ID	NVARCHAR(64),\
TotalSize	NVARCHAR(64),\
URI	NVARCHAR(256),\
ReceiveStatus	NVARCHAR(64),\
PushTime	NVARCHAR(64),\
PRIMARY KEY (ReceiveType,ID));", name);
			}
			else if(!strcmp(name,"Preview"))
			{
				snprintf(sqlite_cmd, sizeof(sqlite_cmd),\
					"CREATE TABLE %s(\
PreviewID	NVARCHAR(64) PRIMARY KEY,\
ProductID	NVARCHAR(64),\
PreviewType	NVARCHAR(64),\
PreviewSize	NVARCHAR(64),\
ShowTime	NVARCHAR(64),\
PreviewURI	NVARCHAR(256),\
PreviewFormat	NVARCHAR(64),\
Duration	NVARCHAR(64),\
Resolution	NVARCHAR(64),\
BitRate	NVARCHAR(64),\
CodeFormat	NVARCHAR(64),\
URI	NVARCHAR(256),\
TotalSize	NVARCHAR(64),\
ProductDescID	NVARCHAR(64),\
ReceiveStatus	NVARCHAR(64),\
PushTime	NVARCHAR(128),\
StartTime	CHAR(32),\
EndTime	CHAR(32),\
PlayMode	NVARCHAR(64));", name);
			}
			else if(!strcmp(name,"RejectRecv"))
			{
				snprintf(sqlite_cmd, sizeof(sqlite_cmd),\
					"CREATE TABLE %s(\
ServiceID	NVARCHAR(64),\
ID	NVARCHAR(64),\
URI	NVARCHAR(512),\
Type	NVARCHAR(64),\
PRIMARY KEY (ServiceID,ID));", name);
			}
			else if(!strcmp(name,"SProduct"))
			{
				snprintf(sqlite_cmd, sizeof(sqlite_cmd),\
					"CREATE TABLE %s(\
SType	NVARCHAR(64),\
Name	NVARCHAR(64),\
URI	NVARCHAR(512));", name);
			}
			
			else
				memset(sqlite_cmd, 0, sizeof(sqlite_cmd));
			
			if(strlen(sqlite_cmd)>0){
				if(sqlite3_exec(g_db,sqlite_cmd,NULL,NULL,&errmsg))
				{
					ERROROUT("create '%s' failed: %s\n", name, sqlite_cmd);
					DEBUG("sqlite errmsg: %s\n", errmsg);
					ret = -1;
				}
			}
		}
	}

	sqlite3_free_table(l_result);
	sqlite3_free(errmsg);
	return ret;
}
/***sqlite_init() brief init sqlite, include open database, create table, and so on.
 * param null
 *
 * retval int,0 if successful or -1 failed
 ***/
int sqlite_init()
{
	if(0==s_sqlite_init_flag){
		g_db = NULL;
		if(-1==createDatabase()){						///open database
			return -1;
		}
		s_sqlite_init_flag = 1;
	}

	return 0;						/// quit
}

int sqlite_uninit()
{
	return 0;
}

/***getGlobalPara() brief get some global variables from sqlite, such as 'version'.
 * param name[in], the name of global param
 *
 * retval int,0 if successful or -1 failed
 ***/
int sqlite_execute(char *exec_str)
{
	char* errmsg=NULL;
	int ret = -1;
	
	//open database
	if(-1==openDatabase())
	{
		ERROROUT("Open database failed\n");
		ret = -1;
	}
	else{
		DEBUG("%s\n", exec_str);
		if(sqlite3_exec(g_db,exec_str,NULL,NULL,&errmsg)){
			DEBUG("sqlite3 errmsg: %s\n", errmsg);
			ret = -1;
		}
		else{
			//DEBUG("sqlite3_exec success\n");
			ret = 0;
		}
		
		sqlite3_free(errmsg);								///	release the memery possessed by error message
		closeDatabase();									///	close database
	}
	
	return ret;	
}

/*
���ܣ�	ִ��SELECT���
���룺	sqlite_cmd				����sql SELECT���
		receiver				�������ڴ���SELECT����Ĳ��������sqlite_read_callbackΪNULL����receiverҲ����ΪNULL
		sqlite_read_callback	�������ڴ���SELECT����Ļص������ֻ����֪����ѯ��������¼����˻ص�����ΪNULL
���أ�	-1����ʧ�ܣ�����ֵ������ѯ���ļ�¼��
*/
int sqlite_read(char *sqlite_cmd, void *receiver, int (*sqlite_read_callback)(char **result, int row, int column, void *receiver))
{
	char* errmsg=NULL;
	char** l_result = NULL;
	int l_row = 0;
	int l_column = 0;
	int ret = 0;
	int (*sqlite_callback)(char **,int,int,void *) = sqlite_read_callback;

	DEBUG("sqlite read: %s\n", sqlite_cmd);
	
	///open database
	if(-1==openDatabase())
	{
		ERROROUT("Open database failed\n");
		ret = -1;
	}
	else{	// open database ok
		
		if(sqlite3_get_table(g_db,sqlite_cmd,&l_result,&l_row,&l_column,&errmsg)
			|| NULL!=errmsg)
		{
			ERROROUT("sqlite cmd: %s\n", sqlite_cmd);
			DEBUG("errmsg: %s\n", errmsg);
			ret = -1;
		}
		else{ // inquire table ok
			if(0==l_row){
				DEBUG("no row, l_row=0, l_column=%d", l_column);
				int i = 0;
				for(i=0;i<l_column;i++)
					printf("\t\t%s", l_result[i]);
				printf("\n");
			}
			else{
				DEBUG("sqlite select OK, %s\n", NULL==sqlite_callback?"no callback fun":"do callback fun");
				if(sqlite_callback)
					sqlite_callback(l_result, l_row, l_column, receiver);
				else{
					DEBUG("l_row=%d, l_column=%d\n", l_row, l_column);
//					int i = 0;
//					for(i=0;i<(l_column+1);i++)
//						printf("\t\t%s\n", l_result[i]);
				}
			}
			ret = l_row;
		}
		sqlite3_free_table(l_result);
		sqlite3_free(errmsg);
		closeDatabase();
	}
	
	return ret;
}

int sqlite_table_clear(char *table_name)
{
	DEBUG("CAUTION: will clear table '%s'\n", table_name);
	char sqlite_cmd[256];	
	
	memset(sqlite_cmd, 0, sizeof(sqlite_cmd));
	snprintf(sqlite_cmd,sizeof(sqlite_cmd),"DELETE FROM %s;", table_name);
	DEBUG("sqlite cmd str: %s\n", sqlite_cmd);

	int ret = sqlite_execute(sqlite_cmd);
	if(0==ret){
		DEBUG("table '%s' clear successfully\n", table_name);
		return 0;
	}
	else{
		DEBUG("table '%s' reset failed\n", table_name);
		return -1;
	}
}

#if 0
/*
 ��һϵ��sqlite����װΪһ�����񣬸���������'\n'��β��
*/
int sqlite_transaction(char *sqlite_cmd)
{
	if(NULL==sqlite_cmd || 0==strlen(sqlite_cmd)){
		DEBUG("invalid argument\n");
		return -1;
	}
	DEBUG("%s\n", sqlite_cmd);
	int ret = -1;
	
	if(-1==openDatabase())
	{
		ERROROUT("Open database failed\n");
		ret = -1;
	}
	else{
		ret = sqlite3_exec(g_db, "begin transaction", NULL, NULL, NULL);
		if(SQLITE_OK == ret){
			char *p_cmd = sqlite_cmd;
			char *p = NULL;
			unsigned int killed_len = 0;
			do{
				killed_len = 0;
				p = strchr(p_cmd, '\n');
				if(p){
					*p = '\0';
					killed_len += 1;
				}
				killed_len += strlen(p_cmd);
				
				DEBUG("sqlite cmd: %s, p=%s, killed_len=%d\n", p_cmd, p, killed_len);
				if(strlen(p_cmd)>0){
					ret = sqlite3_exec(g_db, p_cmd, NULL, NULL, NULL);
					if(SQLITE_OK == ret){
						;
					}
					else{
						DEBUG("sqlite3 errmsg: %s\n", sqlite3_errmsg(g_db));
						ret = -1;
						break;
					}
				}
				p_cmd += killed_len;
			}while(p_cmd && strlen(p_cmd)>0);
			
			if(SQLITE_OK == ret){
				ret = sqlite3_exec(g_db, "commit transaction", NULL, NULL, NULL);
			}
			else{
				DEBUG("rollback transaction\n");
				ret = sqlite3_exec(g_db, "rollback transaction", NULL, NULL, NULL);
			}
			
			if(SQLITE_OK == ret){
				ret = 0;
			}
			else{
				DEBUG("%s\n", sqlite3_errmsg(g_db));
				ret = -1;
			}
		}
		else{
			DEBUG("sqlite3 errmsg: %s\n", sqlite3_errmsg(g_db));
			ret = -1;
		}
		
		closeDatabase();									///	close database
	}
	
	return ret;
}
#endif

/*
 ���ݿ�����ʹ�ù���
 1������Ŀ�ʼ�ͽ��������ǳɶԳ��ֵġ���������Ƕ�ס�
 2�����������滻�����ݱ�Ӧ�����ݱ����������¼�빤����װ��һ�������С�
 3������Res��ͷ�����ݱ����ǵ����ݶ����������������ݱ�������壬
 	�����Щ���ݱ�Ĳ��롢ɾ�������µȶ���������ĳ�����������һ���֣�������������һ������������
*/
typedef enum{
	SQL_TRAN_STATUS_END = 0,
	SQL_TRAN_STATUS_BEGIN,
	SQL_TRAN_STATUS_LOADING
}SQL_TRAN_STATUS_E;
static SQL_TRAN_STATUS_E s_sql_tran_status = SQL_TRAN_STATUS_END;


/*
 ����1������ʼ
*/
int sqlite_transaction_begin()
{
	int ret = -1;
	
	DEBUG("sqlite_transaction_begin>>\n");
	if(SQL_TRAN_STATUS_BEGIN==s_sql_tran_status || SQL_TRAN_STATUS_LOADING==s_sql_tran_status){
		DEBUG("######### SQLITE TRANSACTION STATUS is abnormally #########\n");
		DEBUG("expect SQL_TRAN_STATUS_END but %d\n", s_sql_tran_status);
		DEBUG("###########################################################\n");
		return -1;
		// ret = sqlite_transaction_end(0);
	}
	
	if(SQL_TRAN_STATUS_END == s_sql_tran_status){
		if(-1==openDatabase())
		{
			ERROROUT("Open database failed\n");
			ret = -1;
		}
		else{
			ret = sqlite3_exec(g_db, "begin transaction", NULL, NULL, NULL);
			if(SQLITE_OK == ret){
				s_sql_tran_status = SQL_TRAN_STATUS_BEGIN;
				ret = 0;
				//DEBUG(">>>>>>>>>>>>>>>> 	ok\n");
			}
			else{
				DEBUG("sqlite3 errmsg: %s\n", sqlite3_errmsg(g_db));
				ret = -1;
			}
		}
	}
	
	return ret;
}
/*
 ����2�������е�sqlite��䡣
*/
int sqlite_transaction_exec(char *sqlite_cmd)
{
	if(NULL==sqlite_cmd || 0==strlen(sqlite_cmd)){
		DEBUG("invalid argument\n");
		return -1;
	}
	DEBUG("%s\n", sqlite_cmd);
	
	int ret = -1;
	
	if(SQL_TRAN_STATUS_END == s_sql_tran_status){
		DEBUG("######### SQLITE TRANSACTION STATUS is abnormally #########\n");
		DEBUG("expect SQL_TRAN_STATUS_BEGIN but %d\n", s_sql_tran_status);
		DEBUG("###########################################################\n");
		return -1;
		//ret = sqlite_transaction_begin();
	}
	
	if(SQL_TRAN_STATUS_BEGIN == s_sql_tran_status || SQL_TRAN_STATUS_LOADING == s_sql_tran_status){
		ret = sqlite3_exec(g_db, sqlite_cmd, NULL, NULL, NULL);
		if(SQLITE_OK == ret){
			s_sql_tran_status = SQL_TRAN_STATUS_LOADING;
			ret = 0;
		}
		else{
			DEBUG("sqlite3 errmsg: %s\n", sqlite3_errmsg(g_db));
			ret = -1;
		}
	}
	
	return ret;
}
/*
 ����3���������ݱ���������ִ�С�
*/
int sqlite_transaction_table_clear(char *table_name)
{
	DEBUG("CAUTION: will clear table '%s'\n", table_name);
	char sqlite_cmd[256];	
	
	snprintf(sqlite_cmd,sizeof(sqlite_cmd),"DELETE FROM %s;", table_name);
	
	return sqlite_transaction_exec(sqlite_cmd);
}

/*
 ����4�����������
 ������commin_flag����0���ύ����-1���ع�����
*/
int sqlite_transaction_end(int commit_flag)
{
	int ret = -1;
	
	DEBUG("sqlite_transaction_end<<\n");
	if(SQL_TRAN_STATUS_BEGIN==s_sql_tran_status || SQL_TRAN_STATUS_LOADING==s_sql_tran_status){
		if(1==commit_flag){
			DEBUG("commit transaction\n");
			ret = sqlite3_exec(g_db, "commit transaction", NULL, NULL, NULL);
		}
		else{
			DEBUG("rollback transaction\n");
			ret = sqlite3_exec(g_db, "rollback transaction", NULL, NULL, NULL);
		}
		
		if(SQLITE_OK == ret){
			s_sql_tran_status = SQL_TRAN_STATUS_END;
			ret = 0;
			//DEBUG("<<<<<<<<<<<<<<<<<<<< 	ok\n");
		}
		else{
			DEBUG("%s\n", sqlite3_errmsg(g_db));
			ret = -1;
		}
		closeDatabase();
	}
	else{
		DEBUG("sqlite transaction is end, do nothing\n");
		ret = -1;
	}
	
	return ret;
}


/*
 ������Ŀ�ĳ�ʼ����Ŀǰ�ǡ����á��͡��������ġ�
 ע����·���Column.xml�ֶ�ʶ�𱣳�һ��
*/
int localcolumn_init()
{
	char sqlite_cmd[512];
	
	DEBUG("aaaaaaaaaaaaaaaaaaa\n");
	
	sqlite_transaction_begin();
	
	
	/*
	 һ���˵����������ġ�
	*/
	snprintf(sqlite_cmd, sizeof(sqlite_cmd), "REPLACE INTO Column(ColumnID,ParentID,Path,ColumnType,ColumnIcon_losefocus,ColumnIcon_getfocus,ColumnIcon_onclick,SequenceNum) VALUES('%s','%s','%s','%d','%s','%s','%s',1);",
		"01","-1","01",COLUMN_LOCAL,"LocalColumnIcon/MyCenter_losefocus.png","LocalColumnIcon/MyCenter_getfocus.png","MyCenter_onclick.png");
	sqlite_transaction_exec(sqlite_cmd);
	snprintf(sqlite_cmd, sizeof(sqlite_cmd), "REPLACE INTO ResStr(ObjectName,EntityID,StrLang,StrName,StrValue,Extension) VALUES('%s','%s','%s','%s','%s','%s');",
		"Column","01","chi","DisplayName","��������","");
	sqlite_transaction_exec(sqlite_cmd);
	snprintf(sqlite_cmd, sizeof(sqlite_cmd), "REPLACE INTO ResStr(ObjectName,EntityID,StrLang,StrName,StrValue,Extension) VALUES('%s','%s','%s','%s','%s','%s');",
		"Column","01","eng","DisplayName","My Center","");
	sqlite_transaction_exec(sqlite_cmd);
	/*
	 �����˵���ѡ����ա�
	*/
	snprintf(sqlite_cmd, sizeof(sqlite_cmd), "REPLACE INTO Column(ColumnID,ParentID,Path,ColumnType,ColumnIcon_losefocus,ColumnIcon_getfocus,ColumnIcon_onclick,SequenceNum) VALUES('%s','%s','%s','%d','%s','%s','%s',1);",
		"0101","01","01/0101",COLUMN_LOCAL,"LocalColumnIcon/Receiving_losefocus.png","LocalColumnIcon/Receiving_getfocus.png","Receiving_onclick.png");
	sqlite_transaction_exec(sqlite_cmd);
	snprintf(sqlite_cmd, sizeof(sqlite_cmd), "REPLACE INTO ResStr(ObjectName,EntityID,StrLang,StrName,StrValue,Extension) VALUES('%s','%s','%s','%s','%s','%s');",
		"Column","0101","chi","DisplayName","ѡ�����","");
	sqlite_transaction_exec(sqlite_cmd);
	snprintf(sqlite_cmd, sizeof(sqlite_cmd), "REPLACE INTO ResStr(ObjectName,EntityID,StrLang,StrName,StrValue,Extension) VALUES('%s','%s','%s','%s','%s','%s');",
		"Column","0101","eng","DisplayName","Receiving","");
	sqlite_transaction_exec(sqlite_cmd);
	/*
	 �����˵�������״̬��
	*/
	snprintf(sqlite_cmd, sizeof(sqlite_cmd), "REPLACE INTO Column(ColumnID,ParentID,Path,ColumnType,ColumnIcon_losefocus,ColumnIcon_getfocus,ColumnIcon_onclick,SequenceNum) VALUES('%s','%s','%s','%d','%s','%s','%s',2);",
		"0102","01","01/0102",COLUMN_LOCAL,"LocalColumnIcon/Download_losefocus.png","LocalColumnIcon/Download_getfocus.png","Download_onclick.png");
	sqlite_transaction_exec(sqlite_cmd);
	snprintf(sqlite_cmd, sizeof(sqlite_cmd), "REPLACE INTO ResStr(ObjectName,EntityID,StrLang,StrName,StrValue,Extension) VALUES('%s','%s','%s','%s','%s','%s');",
		"Column","0102","chi","DisplayName","����״̬","");
	sqlite_transaction_exec(sqlite_cmd);
	snprintf(sqlite_cmd, sizeof(sqlite_cmd), "REPLACE INTO ResStr(ObjectName,EntityID,StrLang,StrName,StrValue,Extension) VALUES('%s','%s','%s','%s','%s','%s');",
		"Column","0102","eng","DisplayName","Download","");
	sqlite_transaction_exec(sqlite_cmd);
	
	
	/*
	 һ���˵������á�
	*/
	snprintf(sqlite_cmd, sizeof(sqlite_cmd), "REPLACE INTO Column(ColumnID,ParentID,Path,ColumnType,ColumnIcon_losefocus,ColumnIcon_getfocus,ColumnIcon_onclick,SequenceNum) VALUES('%s','%s','%s','%d','%s','%s','%s',2);",
		"02","-1","02",COLUMN_LOCAL,"LocalColumnIcon/Setting_losefocus.png","LocalColumnIcon/Setting_getfocus.png","Setting_onclick.png");
	sqlite_transaction_exec(sqlite_cmd);
	snprintf(sqlite_cmd, sizeof(sqlite_cmd), "REPLACE INTO ResStr(ObjectName,EntityID,StrLang,StrName,StrValue,Extension) VALUES('%s','%s','%s','%s','%s','%s');",
		"Column","02","chi","DisplayName","����","");
	sqlite_transaction_exec(sqlite_cmd);
	snprintf(sqlite_cmd, sizeof(sqlite_cmd), "REPLACE INTO ResStr(ObjectName,EntityID,StrLang,StrName,StrValue,Extension) VALUES('%s','%s','%s','%s','%s','%s');",
		"Column","02","eng","DisplayName","Setting","");
	sqlite_transaction_exec(sqlite_cmd);
	/*
	 �����˵������ã��������á�
	*/
	snprintf(sqlite_cmd, sizeof(sqlite_cmd), "REPLACE INTO Column(ColumnID,ParentID,Path,ColumnType,ColumnIcon_losefocus,ColumnIcon_getfocus,ColumnIcon_onclick,SequenceNum) VALUES('%s','%s','%s','%d','%s','%s','%s',1);",
		"0201","02","02/0201",COLUMN_LOCAL,"LocalColumnIcon/Network_losefocus.png","LocalColumnIcon/Network_getfocus.png","Network_onclick.png");
	sqlite_transaction_exec(sqlite_cmd);
	snprintf(sqlite_cmd, sizeof(sqlite_cmd), "REPLACE INTO ResStr(ObjectName,EntityID,StrLang,StrName,StrValue,Extension) VALUES('%s','%s','%s','%s','%s','%s');",
		"Column","0201","chi","DisplayName","��������","");
	sqlite_transaction_exec(sqlite_cmd);
	snprintf(sqlite_cmd, sizeof(sqlite_cmd), "REPLACE INTO ResStr(ObjectName,EntityID,StrLang,StrName,StrValue,Extension) VALUES('%s','%s','%s','%s','%s','%s');",
		"Column","0201","eng","DisplayName","Network","");
	sqlite_transaction_exec(sqlite_cmd);
	/*
	 �����˵������ã���Ƶ���á�
	*/
	snprintf(sqlite_cmd, sizeof(sqlite_cmd), "REPLACE INTO Column(ColumnID,ParentID,Path,ColumnType,ColumnIcon_losefocus,ColumnIcon_getfocus,ColumnIcon_onclick,SequenceNum) VALUES('%s','%s','%s','%d','%s','%s','%s',2);",
		"0202","02","02/0202",COLUMN_LOCAL,"LocalColumnIcon/Video_losefocus.png","LocalColumnIcon/Video_getfocus.png","Video_onclick.png");
	sqlite_transaction_exec(sqlite_cmd);
	snprintf(sqlite_cmd, sizeof(sqlite_cmd), "REPLACE INTO ResStr(ObjectName,EntityID,StrLang,StrName,StrValue,Extension) VALUES('%s','%s','%s','%s','%s','%s');",
		"Column","0202","chi","DisplayName","��Ƶ����","");
	sqlite_transaction_exec(sqlite_cmd);
	snprintf(sqlite_cmd, sizeof(sqlite_cmd), "REPLACE INTO ResStr(ObjectName,EntityID,StrLang,StrName,StrValue,Extension) VALUES('%s','%s','%s','%s','%s','%s');",
		"Column","0202","eng","DisplayName","Video","");
	sqlite_transaction_exec(sqlite_cmd);
	/*
	 �����˵������ã���Ƶ���á�
	*/
	snprintf(sqlite_cmd, sizeof(sqlite_cmd), "REPLACE INTO Column(ColumnID,ParentID,Path,ColumnType,ColumnIcon_losefocus,ColumnIcon_getfocus,ColumnIcon_onclick,SequenceNum) VALUES('%s','%s','%s','%d','%s','%s','%s',3);",
		"0203","02","02/0203",COLUMN_LOCAL,"LocalColumnIcon/Audio_losefocus.png","LocalColumnIcon/Audio_getfocus.png","Audio_onclick.png");
	sqlite_transaction_exec(sqlite_cmd);
	snprintf(sqlite_cmd, sizeof(sqlite_cmd), "REPLACE INTO ResStr(ObjectName,EntityID,StrLang,StrName,StrValue,Extension) VALUES('%s','%s','%s','%s','%s','%s');",
		"Column","0203","chi","DisplayName","��Ƶ����","");
	sqlite_transaction_exec(sqlite_cmd);
	snprintf(sqlite_cmd, sizeof(sqlite_cmd), "REPLACE INTO ResStr(ObjectName,EntityID,StrLang,StrName,StrValue,Extension) VALUES('%s','%s','%s','%s','%s','%s');",
		"Column","0203","eng","DisplayName","Audio","");
	sqlite_transaction_exec(sqlite_cmd);
	/*
	 �����˵������ã��û���Ϣ��
	*/
	snprintf(sqlite_cmd, sizeof(sqlite_cmd), "REPLACE INTO Column(ColumnID,ParentID,Path,ColumnType,ColumnIcon_losefocus,ColumnIcon_getfocus,ColumnIcon_onclick,SequenceNum) VALUES('%s','%s','%s','%d','%s','%s','%s',4);",
		"0204","02","02/0204",COLUMN_LOCAL,"LocalColumnIcon/UserInfo_losefocus.png","LocalColumnIcon/UserInfo_getfocus.png","UserInfo_onclick.png");
	sqlite_transaction_exec(sqlite_cmd);
	snprintf(sqlite_cmd, sizeof(sqlite_cmd), "REPLACE INTO ResStr(ObjectName,EntityID,StrLang,StrName,StrValue,Extension) VALUES('%s','%s','%s','%s','%s','%s');",
		"Column","0204","chi","DisplayName","�û���Ϣ","");
	sqlite_transaction_exec(sqlite_cmd);
	snprintf(sqlite_cmd, sizeof(sqlite_cmd), "REPLACE INTO ResStr(ObjectName,EntityID,StrLang,StrName,StrValue,Extension) VALUES('%s','%s','%s','%s','%s','%s');",
		"Column","0204","eng","DisplayName","User Info","");
	sqlite_transaction_exec(sqlite_cmd);
	/*
	 �����˵������ã�������Ϣ��
	*/
	snprintf(sqlite_cmd, sizeof(sqlite_cmd), "REPLACE INTO Column(ColumnID,ParentID,Path,ColumnType,ColumnIcon_losefocus,ColumnIcon_getfocus,ColumnIcon_onclick,SequenceNum) VALUES('%s','%s','%s','%d','%s','%s','%s',5);",
		"0205","02","02/0205",COLUMN_LOCAL,"LocalColumnIcon/DeviceInfo_losefocus.png","LocalColumnIcon/DeviceInfo_getfocus.png","DeviceInfo_onclick.png");
	sqlite_transaction_exec(sqlite_cmd);
	snprintf(sqlite_cmd, sizeof(sqlite_cmd), "REPLACE INTO ResStr(ObjectName,EntityID,StrLang,StrName,StrValue,Extension) VALUES('%s','%s','%s','%s','%s','%s');",
		"Column","0205","chi","DisplayName","������Ϣ","");
	sqlite_transaction_exec(sqlite_cmd);
	snprintf(sqlite_cmd, sizeof(sqlite_cmd), "REPLACE INTO ResStr(ObjectName,EntityID,StrLang,StrName,StrValue,Extension) VALUES('%s','%s','%s','%s','%s','%s');",
		"Column","0205","eng","DisplayName","Device Info","");
	sqlite_transaction_exec(sqlite_cmd);
	/*
	 �����˵������ã�������
	*/
	snprintf(sqlite_cmd, sizeof(sqlite_cmd), "REPLACE INTO Column(ColumnID,ParentID,Path,ColumnType,ColumnIcon_losefocus,ColumnIcon_getfocus,ColumnIcon_onclick,SequenceNum) VALUES('%s','%s','%s','%d','%s','%s','%s',6);",
		"0206","02","02/0206",COLUMN_LOCAL,"LocalColumnIcon/Help_losefocus.png","LocalColumnIcon/Help_getfocus.png","Help_onclick.png");
	sqlite_transaction_exec(sqlite_cmd);
	snprintf(sqlite_cmd, sizeof(sqlite_cmd), "REPLACE INTO ResStr(ObjectName,EntityID,StrLang,StrName,StrValue,Extension) VALUES('%s','%s','%s','%s','%s','%s');",
		"Column","0206","chi","DisplayName","����","");
	sqlite_transaction_exec(sqlite_cmd);
	snprintf(sqlite_cmd, sizeof(sqlite_cmd), "REPLACE INTO ResStr(ObjectName,EntityID,StrLang,StrName,StrValue,Extension) VALUES('%s','%s','%s','%s','%s','%s');",
		"Column","0206","eng","DisplayName","Help","");
	sqlite_transaction_exec(sqlite_cmd);
	
	
	return sqlite_transaction_end(1);
}
