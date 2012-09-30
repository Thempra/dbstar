/*
* example.cpp
*
*  Created on: Aug 11, 2011
*      Author: YJQ
*/

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/types.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/socket.h>
#include <net/if.h>
#include <string.h>
#include <arpa/inet.h>
#include <netpacket/packet.h>
#include <linux/if_ether.h>
#include <net/if_arp.h>
#include <stdlib.h>

#include "common.h"
#include "push.h"
#include "mid_push.h"
#include "porting.h"
#include "xmlparser.h"
#include "sqlite.h"

#define MAX_PACK_LEN (1500)
#define MAX_PACK_BUF (200000)		//定义缓冲区大小，单位：包	1500*200000=280M


#define XML_NUM			8
static char s_xml_name[XML_NUM][256];

static pthread_mutex_t mtx_xml = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond_xml = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mtx_push_monitor = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond_push_monitor = PTHREAD_COND_INITIALIZER;

//数据包结构
typedef struct tagDataBuffer
{
    short	m_len;
    unsigned char	m_buf[MAX_PACK_LEN];
}DataBuffer;

typedef struct tagPRG
{
	char		id[32];
	char		prog_uri[128];
	long long	cur;
	long long	total;
}PROG_S;

static int mid_push_regist(char *id, char *content_uri, long long content_len);
static int push_monitor_regist(int regist_flag);

#define PROGS_NUM 64
static PROG_S s_prgs[PROGS_NUM];
static char s_push_data_dir[256];
/*************接收缓冲区定义***********/
DataBuffer *g_recvBuffer;	//[MAX_PACK_BUF]
static int g_wIndex = 0;
static int g_rIndex = 0;
/**************************************/

static pthread_t tidDecodeData;
static int s_xmlparse_running = 0;
static int s_monitor_running = 0;
static int s_decoder_running = 0;
static char *s_dvbpush_info = NULL;
static int s_dvbpush_info_readed = 0;

/*
当向push中写数据时才有必要监听进度，否则直接使用数据库中记录的进度即可。
考虑到缓冲，在无数据后多查询几轮再停止查询，因此push数据时，置此值为3。
考虑到开机最好给一次显示的机会，初始化为1。
*/
static int s_push_has_data = 0;

int send_mpe_sec_to_push_fifo(uint8_t *pkt, int pkt_len)
{
	int res = -1;
	int snap = 0;
	int offset;
	unsigned char *eth;
	DataBuffer *revbuf;
	int revbufw;
	
	/*	static unsigned int rx_errors = 0;
	static unsigned int rx_length_errors = 0;
	static unsigned int rx_crc_errors = 0;
	static unsigned int rx_dropped = 0;
	static unsigned int rx_frame_errors = 0;
	static unsigned int rx_fifo_dropped = 0;
	*/
	
	if (pkt_len < 16) {
		printf("IP/MPE packet length = %d too small.\n", pkt_len);
		//		rx_errors++;
		//		rx_length_errors++;
		return res;
	}
	
	if (pkt[5] & 0x3e)
	{
		printf("lxy add for youhua,too many!!!!!!!!\n");
		if ((pkt[5] & 0x3c) != 0x00) {
			/* drop scrambled */
			//	rx_errors++;
			//	rx_crc_errors++;
			return res;
		}
		if (pkt[5] & 0x02) {
			if (pkt_len < 24 || memcmp(&pkt[12], "\xaa\xaa\x03\0\0\0", 6)) {
				//	rx_dropped++;
				return res;
			}
			snap = 8;
		}
	}
	/*	if (pkt[7]) {
	//		rx_errors++;
	//		rx_frame_errors++;
	return res;
	}*/
	offset = pkt_len - 16 - snap; 
	//if (pkt_len - 12 - 4 + 14 - snap <= 0) {
	if (offset + 14 <= 0) {
		printf("IP/MPE packet length = %d too small.\n", pkt_len);
		//rx_errors++;
		//rx_length_errors++;
		return res;
	}
	
	if(NULL==g_recvBuffer)
		return res;
	
	/*	if (g_wIndex == g_rIndex 
	&& g_recvBuffer[g_wIndex].m_len) {
	rx_fifo_dropped++;
	printf("Push FIFO is full. lost pkt %d\n", rx_fifo_dropped);
	return res;
	}*/
	revbufw = g_wIndex;
	revbuf = &g_recvBuffer[revbufw];
	//eth = g_recvBuffer[g_wIndex].m_buf;
	eth = revbuf->m_buf;
	
	memcpy(eth + 14, pkt + 12 + snap, offset);//pkt_len - 12 - 4 - snap);
	eth[0]=pkt[0x0b];
	eth[1]=pkt[0x0a];
	eth[2]=pkt[0x09];
	eth[3]=pkt[0x08];
	eth[4]=pkt[0x04];
	eth[5]=pkt[0x03];
	
	eth[6]=eth[7]=eth[8]=eth[9]=eth[10]=eth[11]=0;
	
	if (snap) {
		eth[12] = pkt[18];
		eth[13] = pkt[19];
	} else {
		if (pkt[12] >> 4 == 6) {
			eth[12] = 0x86;	
			eth[13] = 0xdd;
		} else {
			eth[12] = 0x08;	
			eth[13] = 0x00;
		}
	}
	
	
	//g_recvBuffer[g_wIndex].m_len = offset;//pkt_len - 12 - 4 - snap;
	revbuf->m_len = offset;
	revbufw++;
	if (revbufw < MAX_PACK_BUF)
		g_wIndex = revbufw;
	else
		g_wIndex = 0;
	
	//DEBUG("g_wIndex=%d\n", g_wIndex);
	//g_wIndex++;
	//g_wIndex %= MAX_PACK_BUF;
	
	return 0;
}

void *push_decoder_thread()
{
	unsigned char *pBuf = NULL;
	int rindex = 0;
	int read_nothing_count = 0;
    short len;
	
	s_decoder_running = 1;
	while (1==s_decoder_running && NULL!=g_recvBuffer)
	{
		len = g_recvBuffer[rindex].m_len;
		if (len)
		{
			pBuf = g_recvBuffer[rindex].m_buf;
			/*
			* 调用PUSH数据解析接口解析数据，该函数是阻塞的，所以应该使用一个较大
			* 的缓冲区来暂时存储源源不断的数据。
			*/
			push_parse((char *)pBuf, len);
			s_push_has_data = 3;
			
			g_recvBuffer[rindex].m_len = 0;
			rindex++;
			if(rindex >= MAX_PACK_BUF)
				rindex = 0;
			g_rIndex = rindex;
		}
		else
		{
			usleep(20000);
			read_nothing_count++;
			if(read_nothing_count>=500)
			{
				DEBUG("read nothing, read index %d\n", rindex);
				read_nothing_count = 0;
			}
		}
	}
	DEBUG("exit from push decoder thread\n");
	
	return NULL;
}

static void push_progs_finish(char *id)
{
	char sqlite_cmd[256+128];
	
	snprintf(sqlite_cmd, sizeof(sqlite_cmd), "UPDATE content SET ready=1 WHERE id='%s';", id);
	sqlite_execute(sqlite_cmd);
}

#if 0
static void push_progs_process_refresh(char *regist_dir, long long cur_size)
{
	char sqlite_cmd[256+128];
	
	memset(sqlite_cmd, 0, sizeof(sqlite_cmd));
	snprintf(sqlite_cmd, sizeof(sqlite_cmd), "UPDATE brand SET download=%lld WHERE regist_dir='%s';", cur_size, regist_dir);
	sqlite_execute(sqlite_cmd);
}
#endif

static time_t s_recv_status_refresh_pin = 0;
static int push_monitor_resume()
{
	pthread_mutex_lock(&mtx_push_monitor);
	if(time(NULL)-s_recv_status_refresh_pin>=5){
		pthread_cond_signal(&cond_push_monitor); //send sianal
		s_recv_status_refresh_pin = time(NULL);
	}
	pthread_mutex_unlock(&mtx_push_monitor);
	return 0;
}

void dvbpush_getinfo_start()
{
	DEBUG("here start\n");
}

void dvbpush_getinfo_stop()
{
	DEBUG("here stop\n");
}

int dvbpush_getinfo(char **p, unsigned int *len)
{
#define OLNY_FOR_DEBUG 1
#if OLNY_FOR_DEBUG
	static char buff[] = "1001|taska|23932|23523094823\n1002|任务2|234239|12349320\n";
	unsigned int bufflen = sizeof(buff);

	*p = buff;
	*len = bufflen;
	return 0;
#endif

	if(NULL!=s_dvbpush_info){
		free(s_dvbpush_info);
		s_dvbpush_info = NULL;
	}
	s_dvbpush_info_readed = 0;
	push_monitor_resume();
	
	int i = 0;
	for(i=0; i<32; i++){
		if(s_dvbpush_info_readed>0)
			break;
		else
			usleep(100000);
	}
	if(32==i)
		return -1;
	
	/* 形如：1001|aaaaaaname|23932|23523094823\n1002|bbbbbbname|234239|12349320\n1003|cccccname|0|213984902943
	 每条记录预留长度：64位id + strlen(prog_uri) + 20位当前长度 + 20位总长 + 4位分隔符
	 其中：long long型转为10进制后最大长度为20
	*/
	unsigned int info_len = 0;
	for(i=0; i<PROGS_NUM; i++)
	{
		//循环结束的条件是遇到节目路径为空串时。
		if(strlen(s_prgs[i].prog_uri)>0 && s_prgs[i].total>0LL)
			info_len += (strlen(s_prgs[i].prog_uri)+64+20+20+4);
		else
			break;
	}
	if(i>0){
		int progs_num = i;
		
		info_len ++;
		s_dvbpush_info = malloc(info_len);
		if(s_dvbpush_info){
			DEBUG("malloc %d Bs for push info\n", info_len);
			memset(s_dvbpush_info, 0, info_len);
			for(i=0; i<progs_num; i++){
				snprintf(s_dvbpush_info+strlen(s_dvbpush_info), info_len-strlen(s_dvbpush_info),
					"%s%s|%s|%lld|%lld", strlen(s_dvbpush_info)>0?"\n":"",s_prgs[i].id,s_prgs[i].prog_uri,s_prgs[i].cur,s_prgs[i].total);
			}
			*p = s_dvbpush_info;
			*len = strlen(s_dvbpush_info);
			DEBUG("%s\n", s_dvbpush_info);
			return 0;
		}
		else
			DEBUG("can not malloc %d Bs for push info\n", info_len);
	}
	return -1;
}

/*
为避免无意义的查询硬盘，应完成下面两个工作：
1、当节目接收完毕后不应再查询，数据库中记录的是100%
2、只有UI上进入查看进度的界面后，通知底层去查询，其他时间查询没有意义。
3、当push无数据后，再轮询若干遍（等待缓冲数据写入硬盘）后就不再轮询。
*/
void *push_monitor_thread()
{
	int i;
	
	s_monitor_running = 1;
	while (1==s_monitor_running)
	{
		/*
		需要以较低的频率解锁，原因：
		1、解锁后将先注册各个监控文件，然后查询状态，最后反注册各个监控文件
			（之所以要每次查询前后都进行注册、反注册，是由于push模块导致的；即使注册以后不主动查询，push模块内部也要15秒查询一次，加重硬盘负担）；
		2、查询接收状态涉及到硬盘扫描，硬避免过于频繁的硬盘扫描动作。
		*/
		pthread_mutex_lock(&mtx_push_monitor);
		/*
		需要本线程先运行到这里，再在其他非父线程中执行pthread_cond_signal(&cond_push_monitor)才能生效。
		*/
		pthread_cond_wait(&cond_push_monitor,&mtx_push_monitor); //wait
		if(1==s_monitor_running && s_push_has_data>0){
			if(0==push_monitor_regist(1)){
				/*
				监测节目接收进度
				*/
				for(i=0; i<PROGS_NUM; i++)
				{
					if(strlen(s_prgs[i].prog_uri)<=0 || s_prgs[i].total<=0LL)
						break;
					/*
					* 获取指定节目的已接收字节大小，可算出百分比
					*/
					long long rxb = push_dir_get_single(s_prgs[i].prog_uri);
					
					DEBUG("PROG_S:%s %s %lld/%lld %-3lld%%\n",
						s_prgs[i].id,
						s_prgs[i].prog_uri,
						rxb,
						s_prgs[i].total,
						rxb*100/s_prgs[i].total);
					
					if(s_prgs[i].cur != rxb){
						/*
						采用JNI接口读取，不再写入数据库
						push_progs_process_refresh(s_prgs[i].prog_uri, rxb);
						*/
					
						if(rxb>=s_prgs[i].total){
							DEBUG("%s download finished, wipe off from monitor, and set 'ready'\n", s_prgs[i].prog_uri);
							push_progs_finish(s_prgs[i].id);
							push_dir_unregister(s_prgs[i].prog_uri);
						}
						
						s_prgs[i].cur = rxb;
					}
				}
				
				push_monitor_regist(0);
			}
			
			s_push_has_data --;
		}
		
		pthread_mutex_unlock(&mtx_push_monitor);
		s_dvbpush_info_readed = 1;
		//sleep(5);// 在触发控制信号处进行时间限制。
	}
	DEBUG("exit from push monitor thread\n");
	
	return NULL;
}

void *push_xml_parse_thread()
{
	s_xmlparse_running = 1;
	while (1==s_xmlparse_running)
	{
		pthread_mutex_lock(&mtx_xml);
		pthread_cond_wait(&cond_xml,&mtx_xml); //wait
		if(1==s_xmlparse_running){
			int i = 0;
			for(i=0; i<XML_NUM; i++){
				if(strlen(s_xml_name[i])>0){
					parseDoc(s_xml_name[i]);
					memset(s_xml_name[i], 0, sizeof(s_xml_name[i]));
				}
			}
		}
		pthread_mutex_unlock(&mtx_xml);
		if(1==s_xmlparse_running)
			push_monitor_resume();
	}
	DEBUG("exit from xml parse thread\n");
	
	return NULL;
}

void usage()
{
	printf("-i	interface name, default value is eth0.\n");
	printf("-h	print out this message.\n");
	
	printf("\n");
	exit(0);
}

int push_data_root_dir_get(char *buf, unsigned int size)
{
	if(NULL==buf || 0==size){
		DEBUG("some arguments are invalid\n");
		return -1;
	}
	
	strncpy(buf, s_push_data_dir, size);
	return 0;
}

void callback(const char *path, long long size, int flag)
{
	DEBUG("path:%s, size:%lld, flag:%d\n", path, size, flag);
	
	char xml_absolute_name[256+128];
	snprintf(xml_absolute_name, sizeof(xml_absolute_name), "%s/%s", s_push_data_dir, path);
	/* 由于涉及到解析和数据库操作，这里不直接调用parseDoc，避免耽误push任务的运行效率 */
	// settings/allpid/allpid.xml
	if(	0==filename_check(path, "allpid.xml")
		|| 0==filename_check(path, "column.xml")
		|| 0==filename_check(path, "ProductTag.xml")
		|| 0==filename_check(path, "brand_0001.xml")
		|| 0==filename_check(path, "PreProductTag.xml")){
			
		pthread_mutex_lock(&mtx_xml);
		
		int i = 0;
		for(i=0; i<XML_NUM; i++){
			if(0==strlen(s_xml_name[i])){
				strcpy(s_xml_name[i], xml_absolute_name);
				break;
			}
		}
		if(XML_NUM<=i)
			DEBUG("xml name space is full\n");
		else
			pthread_cond_signal(&cond_xml); //send sianal
			
		pthread_mutex_unlock(&mtx_xml);
	}
}

/*
如果传入参数为空，则寻找“/etc/push.conf”文件。详细约束参考push_init()说明
*/
static void push_root_dir_init(char *push_conf)
{
	FILE* fp = NULL;
	char tmp_buf[256];
	char *p_value;
	
	if(NULL==push_conf)
		fp = fopen("/etc/push.conf", "r");
	else
		fp = fopen(push_conf, "r");
	
	memset(s_push_data_dir, 0, sizeof(s_push_data_dir));
	if(NULL==fp){
		DEBUG("waring: open push.conf to get push data dir failed\n");
		strncpy(s_push_data_dir, PUSH_DATA_DIR_DF, sizeof(s_push_data_dir)-1);
	}
	else{
		memset(tmp_buf, 0, sizeof(tmp_buf));
		while(NULL!=fgets(tmp_buf, sizeof(tmp_buf), fp)){
			p_value = setting_item_value(tmp_buf, strlen(tmp_buf));
			if(NULL!=p_value)
			{
				DEBUG("setting item: %s, value: %s\n", tmp_buf, p_value);
				if(strlen(tmp_buf)>0 && strlen(p_value)>0){
					if(0==strcmp(tmp_buf, "DATA_DIR")){
						strncpy(s_push_data_dir, p_value, sizeof(s_push_data_dir)-1);
						break;
					}
				}
			}
			memset(tmp_buf, 0, sizeof(tmp_buf));
		}
		fclose(fp);
	}
}

/*
根据s_prgs数组对push接收进度查询进行注册和反注册。
regist_flag: 1表示注册；0表示反注册。
注意：必须降低push接收进度查询，因为在大量数据接收时，硬盘本身有很大的写数据压力。
同时，如果注册了接收进度查询，但是没有主动查询时，push模块自身会15秒左右查询一次，这应当避免。
因此，每次查询前注册，查询后立即反注册。
*/
static int push_monitor_regist(int regist_flag)
{
	int i = 0;
	int ret = -1;
	
	if(1==regist_flag || 0==regist_flag){
		for(i=0; i<PROGS_NUM; i++){
			if(strlen(s_prgs[i].prog_uri)>0 && s_prgs[i].total>0LL){
				ret = 0;
				if(1==regist_flag)
					push_dir_register(s_prgs[i].prog_uri, s_prgs[i].total, 0);
				else if(0==regist_flag)
					push_dir_unregister(s_prgs[i].prog_uri);
			}
		}
	}
	else
		DEBUG("invalid regist flag: %d\n", regist_flag);
	
	return ret;
}

static int brand_sqlite_callback(char **result, int row, int column, void *receiver)
{
	DEBUG("sqlite callback, row=%d, column=%d, receiver addr: %p\n", row, column, receiver);
	if(row<1){
		DEBUG("no record in table, return\n");
		return 0;
	}
	
	int i = 0;
	long long totalsize = 0LL;
	for(i=1;i<row+1;i++)
	{
		//DEBUG("==%s:%s:%ld==\n", result[i*column], result[i*column+1], strtol(result[i*column+1], NULL, 0));
		sscanf(result[i*column+2],"%lld", &totalsize);
		mid_push_regist(result[i*column], result[i*column+1], totalsize);
	}
	
	return 0;
}

/*
初始化push monitor数组，一般情况是从数据库brand表中读取需要监控的节目。
如果下发了新的brand.xml，解析xml并入库后，需要调用此函数刷新数组。
需要确保刷新数组前就已经将数组中非空条目从push模块中反注册，避免监控垃圾信息。
*/
int push_monitor_reset()
{
	int ret = -1;
	char sqlite_cmd[256+128];
	int (*sqlite_callback)(char **, int, int, void *) = brand_sqlite_callback;

	pthread_mutex_lock(&mtx_push_monitor);
#ifdef PUSH_LOCAL_TEST	
	mid_push_regist("1","prog/video", 206237980LL);
	mid_push_regist("2","prog/file", 18816360LL);
	mid_push_regist("3","prog/audio", 38729433LL);
	ret = 0;
#else
	snprintf(sqlite_cmd,sizeof(sqlite_cmd),"SELECT id, regist_dir, totalsize FROM brand;");
	ret = sqlite_read(sqlite_cmd, NULL, sqlite_callback);
#endif
	pthread_mutex_unlock(&mtx_push_monitor);

	return ret;
}

int mid_push_init(char *push_conf)
{
	int i = 0;
	for(i=0;i<XML_NUM;i++){
		memset(s_xml_name[i], 0, sizeof(s_xml_name[i]));
	}
	
	for(i=0; i<PROGS_NUM; i++){
		memset(s_prgs[i].id, 0, sizeof(s_prgs[i].id));
		memset(s_prgs[i].prog_uri, 0, sizeof(s_prgs[i].prog_uri));
		s_prgs[i].cur = 0LL;
		s_prgs[i].total = 0LL;
	}
	push_monitor_reset();
	
	g_recvBuffer = (DataBuffer *)malloc(sizeof(DataBuffer)*MAX_PACK_BUF);
	if(NULL==g_recvBuffer){
		ERROROUT("can not malloc %d*%d\n", sizeof(DataBuffer), MAX_PACK_BUF);
		return -1;
	}
	for(i=0;i<MAX_PACK_BUF;i++)
		g_recvBuffer[i].m_len = 0;
	
	push_root_dir_init(push_conf);
	/*
	* 初始化PUSH库
	 */
	if (push_init(push_conf) != 0)
	{
		DEBUG("Init push lib failed!\n");
		return -1;
	}
	
	/*
	确保开机后至少有一次扫描机会，获得准确的下载进度。
	*/
	s_push_has_data = 1;
	
	push_set_notice_callback(callback);
	
	//创建数据解码线程
	pthread_create(&tidDecodeData, NULL, push_decoder_thread, NULL);
	//pthread_detach(tidDecodeData);
	
	//创建监视线程
	pthread_t tidMonitor;
	pthread_create(&tidMonitor, NULL, push_monitor_thread, NULL);
	pthread_detach(tidMonitor);
	
	//创建xml解析线程
	pthread_t tidxmlphase;
	pthread_create(&tidxmlphase, NULL, push_xml_parse_thread, NULL);
	pthread_detach(tidxmlphase);
	
	return 0;
}

int mid_push_uninit()
{
	pthread_mutex_lock(&mtx_xml);
	s_xmlparse_running = 0;
	pthread_cond_signal(&cond_xml);
	pthread_mutex_unlock(&mtx_xml);
	
	pthread_mutex_lock(&mtx_push_monitor);
	s_monitor_running = 0;
	pthread_cond_signal(&cond_push_monitor);
	pthread_mutex_unlock(&mtx_push_monitor);
	
	s_push_has_data = 0;
	
	push_destroy();
	
	s_decoder_running = 0;
	pthread_join(tidDecodeData, NULL);
	
	free(g_recvBuffer);
	g_recvBuffer = NULL;
	
	return 0;
}

//注册节目
static int mid_push_regist(char *id, char *content_uri, long long content_len)
{
	if(NULL==id || NULL==content_uri || 0==strlen(content_uri) || content_len<=0LL){
		DEBUG("some arguments are invalid\n");
		return -1;
	}
	
	/*
	* Notice:节目路径是一个相对路径，不要以'/'开头；
	* 若节目单中给出的路径是"/vedios/pushvod/1944"，则去掉最开始的'/'，
	* 用"vedios/pushvod/1944"进行注册。
	*
	* 此处PRG这个结构体是出于示例方便定义的，不一定适用于您的程序中
	*/
	int i;
	for(i=0; i<PROGS_NUM; i++)
	{
		if(0==strlen(s_prgs[i].prog_uri)){
			snprintf(s_prgs[i].id, sizeof(s_prgs[i].id), "%s", id);
			snprintf(s_prgs[i].prog_uri, sizeof(s_prgs[i].prog_uri), "%s", content_uri);
			s_prgs[i].cur = 0;
			s_prgs[i].total = content_len;
			
			/*
			只在需要查询时才注册，并在查询后反注册。避免push模块自身无意义的查询
			push_dir_register(s_prgs[i].prog_uri, s_prgs[i].total, 0);
			*/
			DEBUG("regist to push %s %lld\n", s_prgs[i].prog_uri, s_prgs[i].total);
			break;
		}
	}
	if(i>=PROGS_NUM){
		DEBUG("progs monitor array is overflow\n");
		return -1;
	}
	else
		return 0;
}
