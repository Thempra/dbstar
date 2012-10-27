//ȫ��ʹ�õ�ͷ�ļ����������������ļ���������

#ifndef __COMMON_H__
#define __COMMON_H__
#include <errno.h>

extern int debug_level_get(void);

#define DVBPUSH_DEBUG_ANDROID 1
#if DVBPUSH_DEBUG_ANDROID
#include <android/log.h>
#define LOG_TAG "dvbpush"
#define DEBUG(x...) do { \
	__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, "[%s:%s:%d] ", __FILE__, __FUNCTION__, __LINE__); \
	__android_log_print(ANDROID_LOG_DEBUG, LOG_TAG, x); \
} while(0)
#define ERROROUT(x...) do { \
	__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "[%s:%s:%d] ", __FILE__, __FUNCTION__, __LINE__); \
	if (errno != 0) \
		__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, "[err note: %s]",strerror(errno)); \
	__android_log_print(ANDROID_LOG_ERROR, LOG_TAG, x); \
} while(0)
#else
#define DEBUG(x...) do{printf("[%s:%s:%d] ", __FILE__, __FUNCTION__, __LINE__);printf(x);}while(0)

#define ERROROUT(ERRSTR...) \
			do{printf("[%s:%s:%d] ", __FILE__, __FUNCTION__, __LINE__);\
			if(errno!=0) printf("[err note: %s]",strerror(errno));\
			printf(ERRSTR);}while(0)
#endif

#define MIN_LOCAL(a,b) ((a)>(b)?(b):(a))

typedef enum{
	BOOL_FALSE = 0,
	BOOL_TRUE
}BOOL_E;

/*
��������ʹ�õ����á�fifo�ļ���
*/
#define	WORKING_DATA_DIR	"/data/dbstar"
#define	MSG_FIFO_ROOT_DIR	WORKING_DATA_DIR"/msg_fifo"
#define SETTING_BASE		WORKING_DATA_DIR"/dbstar.conf"
#define PUSH_CONF			WORKING_DATA_DIR"/push.conf"
#define PUSH_CONF_DF		"/etc/push.conf"

/*
�������й����в��������ݣ����������ص�ƬԴ����Ӧ�����ݿ�
*/
#define USR_DATA_ROOT_DIR	"/mnt/sda1/dbstar"
#define PUSH_DATA_DIR_DF	USR_DATA_ROOT_DIR		// �ο�push.conf��DATA_DIR���弰ʱˢ�£��Ա�Ӧ��ʹ��
#define DATABASE			"/data/dbstar/Dbstar.db"

#define	SERVICE_ID			"01"
#define PROG_DATA_PID_DF	(102)	// 0X66
#define ROOT_CHANNEL		(400)
#define ROOT_PUSH_FILE		"Initialize.xml"
#define ROOT_PUSH_FILE_SIZE	(1024)			/* Is this len right??? */
#define DATA_SOURCE			"igmp://239.1.7.5:5000"
#define MULTI_BUF_SIZE		(12*1024*1316)	/* larger than 16M */

#define XML_ROOT_ELEMENT	"RootElement"
#define EXTENSION_STR_FILL	"Extension"
#define OBJID_PAUSE			"_|_"
#define OBJ_SERVICE			"Service"
#define OBJ_PRODUCT			"Product"
#define OBJ_PUBLICATIONSSET	"PublicationsSet"
#define OBJ_PUBLICATION		"Publication"
#define OBJ_MFILE			"MFile"
#define OBJ_COLUMN			"Column"
#define OBJ_GUIDELIST		"GuideList"
#define OBJ_PRODUCTDESC		"ProductDesc"
#define OBJ_MESSAGE			"Message"
#define OBJ_PREVIEW			"Preview"

#define GLB_NAME_SERVICEID			"serviceID"
#define GLB_NAME_PUSHDIR			"PushDir"
#define GLB_NAME_COLUMNRES			"ColumnRes"
#define GLB_NAME_PREVIEWPATH		"PreviewPath"
#define GLB_NAME_CURLANGUAGE		"CurLanguage"
#define GLB_NAME_OPERATIONBUSINESS	"OperationBusiness"
#define GLB_NAME_SMARTCARDID		"SmartCardID"
#define GLB_NAME_ORDERPRODUCT		"OrderProduct"
#define GLB_NAME_DATASOURCE			"PushSource"
#define GLB_NAME_HELPINFO			"HelpInfo"
#define GLB_NAME_STBID				"StbID"
#define GLB_NAME_HARDWARE_VERSION	"HardwareVersion"
#define GLB_NAME_SOFTWARE_VERSION	"SoftwareVersion"

#define INITIALIZE_MIDPATH	"servicegroup/initialize"
#define DBSTAR_PREVIEWPATH	"/mnt/sda1/dbstar/PreView"
#define LOCALCOLUMN_RES		"/data/dbstar/ColumnRes"

typedef enum{
	NAVIGATIONTYPE_NOCOLUMN = 0,
	NAVIGATIONTYPE_COLUMN
}NAVIGATIONTYPE_E;


typedef enum{
	COLUMN_MOVIE = 1,
	COLUMN_TV = 2,
	COLUMN_GUIDE = 3,
	COLUMN_SG = 4,
	COLUMN_LOCAL = 99	// �������õĲ˵������������á��͡��������ġ�
}COLUMN_TYPE_E;

/*
 ��ǰ��Ч��ǡ����µ�Channel�����·�ʱ�������е�pid���Ϊ0�������·���pid���Ϊ1��
 ������Ϊ�˷�����������һ����Channel
*/
typedef enum{
	CHANNEL_INEFFECTIVE = 0,
	CHANNEL_EFFECTIVE = 1
}CHANNEL_EFFECTIVE_E;

/*
Ĭ�ϵĳ�ʼ���ļ�uri�������push��·����uri��������Initialize.xml��Channel.xml��·��
�������й����п��ܻᱻ����
*/
typedef enum{
	PUSH_XML_FLAG_UNDEFINED = -1,
	PUSH_XML_FLAG_MINLINE = 0,
	
	INITIALIZE_XML = 100,
	COLUMN_XML = 101,
	GUIDELIST_XML = 102,
	COMMANDS_XML = 104,
	MESSAGE_XML = 105,
	PRODUCTDESC_XML = 106,
	SERVICE_XML = 107,
	SPRODUCT_XML = 108,
	
	PUSH_XML_FLAG_MAXLINE = 1000
}PUSH_XML_FLAG_E;


/*
	���ֵܽڵ��У�ֻ��Ҫ�����ڵ��ڲ���Ϣ����Ϻ���Ҫ����ɨ�������ֵܽڵ㣬����process_overΪ1��
	�������������д����ж��������Ϸ�ʱ�ݹ�parseNode�����ڲ����ڲ�������Ϻ������ڵ������
	1���Ѿ��ҵ��Ϸ��ķ�֧��ʣ�µķ�֧�����������ǰ�˳�
	2����ҵ���߼����жϲ���Ҫ���������磬ServiceID��ƥ�䣬���߰汾���
	3��������������ǰ�˳�
*/
typedef enum{
	XML_EXIT_NORMALLY = 0,
	XML_EXIT_MOVEUP = 1,
	XML_EXIT_UNNECESSARY = 2,
	XML_EXIT_ERROR = 3,
}XML_EXIT_E;

/*
���ز���pushʱʹ�ã����hytd.ts������������������¹رմ˺ꡣ
*/
//#define PUSH_LOCAL_TEST

typedef struct{
	char	Name[64];
	char	Value[128];
	char	Param[256];
}DBSTAR_GLOBAL_S;

typedef struct{
	char	productID[64];
	char	productName[64];
	char	serviceID[64];
}DBSTAR_PRODUCT_SERVICE_S;

typedef struct{
	char	PushFlag[64];
	char	XMLName[64];
	char	Version[64];
	char	StandardVersion[64];
	char	URI[256];
	char	ServiceID[64];
}DBSTAR_XMLINFO_S;

typedef struct{
	char	pid[64];
	char	pidtype[64];
	char	multiURI[64];
}DBSTAR_CHANNEL_S;

typedef struct{
	char	ObjectName[64];
	char	EntityID[64];
	char	StrLang[32];
	char	StrName[64];
	char	*StrValue;
	char	Extension[64];	// "Extension" or ""
}DBSTAR_RESSTR_S;

typedef struct{
	char	ObjectName[64];
	char	EntityID[64];
	char	SubTitleID[64];
	char	SubTitleName[64];
	char	SubTitleLanguage[64];
	char	SubTitleURI[256];
}DBSTAR_RESSUBTITLE_S;

typedef struct{
	char	ObjectName[64];
	char	EntityID[64];
	char	TrailerID[64];
	char	TrailerName[64];
	char	TrailerURI[256];
}DBSTAR_RESTRAILER_S;

typedef struct{
	char	ObjectName[64];
	char	EntityID[64];
	char	PosterID[64];
	char	PosterName[64];
	char	PosterURI[256];
}DBSTAR_RESPOSTER_S;

typedef struct{
	char	ObjectName[64];
	char	EntityID[64];
	char	Name[64];
	char	Type[64];
}DBSTAR_RESEXTENSION_S;

typedef struct{
	char	ObjectName[256];
	char	EntityID[64];
	char	FileID[64];
	char	FileName[64];
	char	FileURI[256];
}DBSTAR_RESEXTENSIONFILE_S;

typedef struct{
	char	ServiceID[64];
	char	RegionCode[64];
	char	OnlineTime[32];
	char	OfflineTime[32];
}DBSTAR_SERVICE_S;

typedef struct{
	char	ProductID[64];
	char	ProductType[64];
	char	Flag[64];
	char	OnlineDate[64];
	char	OfflineDate[64];
	char	IsReserved[64];
	char	Price[64];
	char	CurrencyType[64];
	char	DRMFile[256];
}DBSTAR_PRODUCT_S;

typedef struct{
	char	type[64];
	char	uri[256];
}COLUMNICON_S;

typedef struct{
	char	ColumnID[64];
	char	ParentID[64];
	char	Path[256];
	char	ColumnType[256];
	char	ColumnIcon_losefocus[256];
	char	ColumnIcon_getfocus[256];
	char	ColumnIcon_onclick[256];
}DBSTAR_COLUMN_S;

typedef struct{
	char	DateValue[64];
	char	GuideListID[64];
	char	productID[64];
	char	PublicationID[64];
}DBSTAR_GUIDELIST_S;

typedef struct{
	char	serviceID[64];
	char	ReceiveType[64];
	char	ProductDescID[64];
	char	rootPath[256];
	char	productID[64];
	char	SetID[64];
	char	ID[64];
	char	TotalSize[64];
	char	URI[256];
	char	PushStartTime[64];
	char	PushEndTime[64];
	char	Columns[1024];	// it's better to use malloc and relloc
}DBSTAR_PRODUCTDESC_S;

typedef struct{
	char	serviceID[64];
	char	navigationType[64];
}DBSTAR_NAVIGATION_S;

typedef struct{
	char	columnID[64];
	char	EntityID[64];
}DBSTAR_COLUMNENTITY_S;

typedef struct{
	char	SetID[64];
	char	ProductID[64];
	char	PublicationID[64];
	char	IndexInSet[64];
}DBSTAR_PUBLICATIONSSET_S;




typedef struct{
	char	PublicationID[64];
	char	FileID[64];
	char	FileSize[64];
	char	FileURI[256];
	char	FileType[64];
	char	FileFormat[32];
	char	Duration[32];
	char	Resolution[32];
	char	BitRate[32];
	char	CodeFormat[32];
}DBSTAR_MFILE_S;


typedef struct{
	char	PublicationID[64];
	char	PublicationType[64];
	char	IsReserved[32];
	char	Visible[32];
	char	DRMFile[256];
}DBSTAR_PUBLICATION_S;

typedef struct{
	char	PublicationID[64];
	char	infolang[64];
	char	*PublicationDesc;
	char	Keywords[256];
	char	ImageDefinition[32];
	char	Area[64];
	char	Language[64];
	char	Episode[32];
	char	AspectRatio[32];
	char	AudioChannel[32];
	char	Director[128];
	char	Actor[256];
	char	Audience[64];
	char	Model[32];
}DBSTAR_MULTIPLELANGUAGEINFOVA_S;

typedef struct{
	char	PublicationID[64];
	char	infolang[64];
	char	*PublicationDesc;
	char	Keywords[256];
	char	Publisher[128];
	char	Area[64];
	char	Language[64];
	char	Episode[64];
	char	AspectRatio[64];
	char	VolNum[64];
	char	ISSN[64];
}DBSTAR_MULTIPLELANGUAGEINFORM_S;

typedef struct{
	char	PublicationID[64];
	char	infolang[64];
	char	*PublicationDesc;
	char	Keywords[256];
	char	Category[64];
	char	Released[64];
	char	AppVersion[64];
	char	Language[64];
	char	Developer[64];
	char	Rated[64];
}DBSTAR_MULTIPLELANGUAGEINFOAPP_S;


typedef struct{
	char	MessageID[64];
	char	type[64];
	char	displayForm[64];
	char	StartTime[64];
	char	EndTime[64];
	char	Interval[64];
}DBSTAR_MESSAGE_S;

typedef struct{
	char	PreviewID[64];
	char	PreviewType[64];
	char	PreviewSize[64];
	char	ShowTime[64];
	char	PreviewURI[256];
	char	PreviewFormat[64];
	char	Duration[64];
	char	Resolution[64];
	char	BitRate[64];
	char	CodeFormat[64];
}DBSTAR_PREVIEW_S;



int appoint_str2int(char *str, unsigned int str_len, unsigned int start_position, unsigned int appoint_len, int base);
unsigned int randint();
int dir_exist_ensure(char *dir);

void print_timestamp(int show_s_ms, int show_str);
int dir_ensure(char *dir);
int phony_div(unsigned int div_father, unsigned int div_son);
void ms_sleep(unsigned int ms);
char *strrstr_s(const char *str_dad, char *str_son, char signchr);
char *time_serial();
int ipv4_simple_check(const char *ip_addr);
int distill_file(char *path, char *file, unsigned int file_size, char *filefmt, char *preferential_file);
int check_tail(const char *str_dad, char *str_tail, int case_cmp);

#endif

