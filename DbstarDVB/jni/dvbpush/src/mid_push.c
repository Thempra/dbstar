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
#include <time.h>

#include "common.h"
#include "push.h"
#include "mid_push.h"
#include "porting.h"
#include "xmlparser.h"
#include "sqlite.h"
#include "dvbpush_api.h"
#include "multicast.h"

#define MAX_PACK_LEN (1500)
#define MAX_PACK_BUF (60000)		//���建������С����λ����	1500*200000=280M
//#define MEMSET_PUSHBUF_SAFE			// if MAX_PACK_BUF<200000 define

/*
 ֻ���ڽ��롰����״̬��ҳ��Ų�ѯ���ȣ��������ڵ�ǰpush���У���Ϊ��Ҫͨ��������ؽ��Ȼ�ȡ��Ŀ������ϡ�
*/
//#define MONITOR_MIN

#define XML_NUM			8
static PUSH_XML_S		s_push_xml[XML_NUM];

static pthread_mutex_t mtx_xml = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond_xml = PTHREAD_COND_INITIALIZER;
static pthread_mutex_t mtx_push_monitor = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond_push_monitor = PTHREAD_COND_INITIALIZER;


static pthread_mutex_t mtx_push_rely_condition = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t cond_push_rely_condition = PTHREAD_COND_INITIALIZER;
static int s_push_rely_condition = 0;
static int push_idle = 0;
static int s_disk_manage_flag = 0;

//���ݰ��ṹ
typedef struct tagDataBuffer
{
    short	m_len;
    unsigned char	m_buf[MAX_PACK_LEN];
}DataBuffer;

typedef struct tagPRG
{
	char			id[32];
	char			uri[256];
	char			descURI[384];
	char			caption[256];
	char			deadline[32];
	RECEIVETYPE_E	type;
	long long		cur;
	long long		total;
	int				parsed;
}PROG_S;

static int mid_push_regist(PROG_S *prog);
static int mid_push_unregist(PROG_S *prog);
static int push_decoder_buf_uninit();
static int prog_name_fill();

#define PROGS_NUM 64
static PROG_S s_prgs[PROGS_NUM];
//static char s_push_data_dir[256];
/*************���ջ���������***********/
DataBuffer *g_recvBuffer = NULL;	//[MAX_PACK_BUF]
static int g_wIndex = 0;
//static int g_rIndex = 0;
/**************************************/

static pthread_t tidDecodeData;
static int s_xmlparse_running = 0;
static int s_monitor_running = 0;
static int s_decoder_running = 0;
static int s_push_monitor_active = 0;
static int s_dvbpush_getinfo_flag = 0;

static int s_column_refresh = 0;
static int s_interface_refresh = 0;


/*
����push��д����ʱ���б�Ҫ�������ȣ�����ֱ��ʹ�����ݿ��м�¼�Ľ��ȼ��ɡ�
���ǵ����壬�������ݺ���ѯ������ֹͣ��ѯ�����push����ʱ���ô�ֵΪ3��
���ǵ�������ø�һ����ʾ�Ļ��ᣬ��ʼ��Ϊ1��
*/
static int s_push_has_data = 3;

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
	
//	PRINTF("g_recvBuffer=%p\n", g_recvBuffer);
	//return 0;
	
	if (pkt_len < 16) {
		PRINTF("IP/MPE packet length = %d too small.\n", pkt_len);
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
	
//	if(NULL==g_recvBuffer){
//		DEBUG("g_recvBuffer is NULL\n");
//		return res;
//	}
	
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
    short len;
    int push_loop_idle_cnt = 0;
	
	DEBUG("push decoder thread will goto main loop\n");
	s_decoder_running = 1;
rewake:	
	DEBUG("go to push main loop\n");
	while (1==s_decoder_running && NULL!=g_recvBuffer)
	{
		len = g_recvBuffer[rindex].m_len;
		if (len)
		{
			pBuf = g_recvBuffer[rindex].m_buf;
			/*
			* ����PUSH���ݽ����ӿڽ������ݣ��ú����������ģ�����Ӧ��ʹ��һ���ϴ�
			* �Ļ���������ʱ�洢ԴԴ���ϵ����ݡ�
			*/
			push_parse((char *)pBuf, len);
			s_push_has_data = 3;
			
			g_recvBuffer[rindex].m_len = 0;
			rindex++;
			if(rindex >= MAX_PACK_BUF)
				rindex = 0;
			//g_rIndex = rindex;
		}
		else
		{
			usleep(20000);
			push_loop_idle_cnt++;
			if(push_loop_idle_cnt > 1024)
			{
				DEBUG("read nothing, read index %d\n", rindex);
				push_loop_idle_cnt = 0;
			}
		}
	}
	
	if (s_decoder_running == 2)
	{
		push_idle = 1;
		while (s_decoder_running == 2)
		{
			DEBUG("push thread in idle\n");
			sleep(15);
		}
#ifdef MEMSET_PUSHBUF_SAFE
		memset(g_recvBuffer,0 ,sizeof(DataBuffer)*MAX_PACK_BUF);
		DEBUG("g_recvBuffer=%p\n", g_recvBuffer);
#else
		g_recvBuffer[0].m_len = 0;
		g_recvBuffer[1].m_len = 0;
#endif
		g_wIndex = 0;
		rindex = 0;
		push_idle = 0;
		goto rewake;
	}
	DEBUG("exit from push decoder thread\n");
	
	return NULL;
}

void push_rely_condition_set(int cmd)
{
	pthread_mutex_lock(&mtx_push_rely_condition);
	int tmp_cond = s_push_rely_condition;
	if(CMD_NETWORK_DISCONNECT==cmd){
		s_push_rely_condition = s_push_rely_condition & (~RELY_CONDITION_NET);
	}
	else if(CMD_NETWORK_CONNECT==cmd){
		s_push_rely_condition = s_push_rely_condition | RELY_CONDITION_NET;
	}
	else if(CMD_DISK_UNMOUNT==cmd){
		s_push_rely_condition = s_push_rely_condition & (~RELY_CONDITION_HD);
	}
	else if(CMD_DISK_MOUNT==cmd){
		s_push_rely_condition = s_push_rely_condition | RELY_CONDITION_HD;
	}
	else{
		DEBUG("this cmd 0x%x is ignored\n", cmd);
		pthread_mutex_unlock(&mtx_push_rely_condition);
	}
	DEBUG("push origine %d, cmd 0x%x, so s_push_rely_condition is %d\n", tmp_cond, cmd, s_push_rely_condition);
	pthread_cond_signal(&cond_push_rely_condition);
	pthread_mutex_unlock(&mtx_push_rely_condition);
}

//#define PARSE_XML_VIA_THREAD
static int push_prog_finish(char *id, RECEIVETYPE_E type,char *DescURI)
{
	if(NULL==DescURI || NULL==DescURI){
		DEBUG("invalid args\n");
		return -1;
	}
	
	DEBUG("should parse: %s\n", DescURI);
	if(RECEIVETYPE_PUBLICATION==type){
#ifdef PARSE_XML_VIA_THREAD
		send_xml_to_parse(DescURI,PRODUCTION_XML,id);
#else
/*
ֱ�ӵ���parse_xml��Ŀ���Ǳܿ��̣߳��������parse_xml��push_recv_manage_refreshͬʱʹ��sqlite�⣬�����쳣
���Ƿ��������
��������ֱ�ӵ���parse_xml�ᵼ�����е����񽻲�����sqlite����out of memory
զ���أ�����Ŀǰֻ����sqlite��������ô���һ�����Ᵽ����
*/
		parse_xml(DescURI,PRODUCTION_XML,id);
#endif
	}
	else if(RECEIVETYPE_COLUMN==type){
#ifdef PARSE_XML_VIA_THREAD
		send_xml_to_parse(DescURI,COLUMN_XML,NULL);
#else
		parse_xml(DescURI,COLUMN_XML,NULL);
#endif
	}
	else if(RECEIVETYPE_SPRODUCT==type){
#ifdef PARSE_XML_VIA_THREAD
		send_xml_to_parse(DescURI,SPRODUCT_XML,NULL);
#else
		parse_xml(DescURI,SPRODUCT_XML,NULL);
#endif
	}
	else
		DEBUG("this type can not be distinguish\n");
	
	return 0;
}

static int productdesc_parsed_set(char *xml_uri)
{
	char sqlite_cmd[512];
	snprintf(sqlite_cmd, sizeof(sqlite_cmd), "UPDATE ProductDesc SET Parsed='1' WHERE DescURI='%s';", xml_uri);
	return sqlite_execute(sqlite_cmd);
}


/*
 �����ж��Ƿ��ǺϷ���Ŀ
 ����ֵ��	-1��ʾ�Ƿ���1��ʾ�Ϸ���
*/
static int prog_is_valid(PROG_S *prog)
{
	if(NULL==prog)
		return -1;
		
	if(strlen(prog->uri)>0 || (prog->total)>0LL){
		//DEBUG("valid prog\n");
		return 1;
	}
	else
		return -1;
}

void dvbpush_getinfo_start()
{
	s_dvbpush_getinfo_flag = 1;
	
	DEBUG("dvbpush getinfo start >>\n");
}

void dvbpush_getinfo_stop()
{
	DEBUG("dvbpush getinfo stop <<\n");
	s_dvbpush_getinfo_flag = 0;
}


/*
��Ŀǰ���Ե��������ֻ���ڼ���̵߳���prog_monitorʱ��������ϵĽ�Ŀ����ֱ�ӵ���xml������
����Ǵ�JNI��dvbpush_getinfo�ӿڵ��ù��������ܽ���xml��������Launcher������
*/
static int prog_monitor(PROG_S *prog, char *time_stamp)
{
	if(NULL==prog)
		return -1;
	
	if(NULL!=time_stamp && strcmp(time_stamp,prog->deadline)>0){
		DEBUG("this prog[%s:%s] is overdue, compare with %s\n", prog->id,prog->deadline,time_stamp);
		mid_push_unregist(prog);
		return 0;
	}
	
	if(s_push_has_data>0 && (prog->cur)<(prog->total)){
		/*
		* ��ȡָ����Ŀ���ѽ����ֽڴ�С��������ٷֱ�
		���ڹ����ϵĴ����п��ܳ��ֵ�ǰ���ȴ����ܳ������������������ʵ��¼rxb��ֵ��ֻ����UIִ��getinfoʱ�ٿ���cur<=total
		*/
		long long rxb = push_dir_get_single(prog->uri);
		
		DEBUG("PROG_S[%s]:%s %s %lld/%lld %-3lld%%\n",
			prog->id,
			prog->uri,
			prog->descURI,
			rxb,
			prog->total,
			rxb*100/prog->total);
		
		prog->cur = rxb;
	}
	else
		DEBUG("s_push_has_data=%d, prog->cur=%lld, prog->total=%lld, no need to monitor\n", s_push_has_data,prog->cur,prog->total);

	return 0;
}


int dvbpush_getinfo(char *buf, unsigned int size)
{
	if(NULL==buf || 0==size){
		DEBUG("invalid args\n");
		return -1;
	}
	
	int i = 0;
	if(s_push_monitor_active>0){
		/*
		����Ŀ���ս���
		*/
		pthread_mutex_lock(&mtx_push_monitor);
		for(i=0; i<PROGS_NUM; i++)
		{
			if(-1==prog_is_valid(&s_prgs[i]))
				continue;
			
			prog_monitor(&s_prgs[i],NULL);
			
			if(RECEIVETYPE_PUBLICATION==s_prgs[i].type){
				if(0==i){
					snprintf(buf, size,
						"%s\t%s\t%lld\t%lld", s_prgs[i].id,s_prgs[i].caption,s_prgs[i].cur>s_prgs[i].total?s_prgs[i].total:s_prgs[i].cur,s_prgs[i].total);
				}
				else{
					snprintf(buf+strlen(buf), size-strlen(buf),
						"%s%s\t%s\t%lld\t%lld", "\n",s_prgs[i].id,s_prgs[i].caption,s_prgs[i].cur>s_prgs[i].total?s_prgs[i].total:s_prgs[i].cur,s_prgs[i].total);
				}
			}
		}
		s_push_has_data --;
		pthread_mutex_unlock(&mtx_push_monitor);
	
		DEBUG("%s\n", buf);
	}
	else
		DEBUG("no program in monitor\n");
	
	return 0;
}

#if 0
/*
 ����1��ʾ���ڣ�0��ʾʱ����ȣ�-1��ʾ�����ڣ�-2��ʾ��������
*/
static int prog_overdue(char *my_time, char *deadline_time)
{
	if(NULL==my_time || NULL==deadline)
		return -2;
	
	struct tm my_tm;
	struct tm deadline_tm;	// short for deadline
	
	memset(my_tm, 0, sizeof(my_tm));
	memset(deadline_tm, 0, sizeof(deadline_tm));
	
	int ret = -2;
	if(		4!=sscanf(my_time, "%d-%d-%d %d:%d:%d", &my_tm.tm_year, &my_tm.tm_mon, &my_tm.tm_mday, &my_tm.tm_hour, &my_tm.tm_min, &my_tm.tm_sec)
		||	4!=sscanf(deadline_time, "%d-%d-%d %d:%d:%d", &deadline_tm.tm_year, &deadline_tm.tm_mon, &deadline_tm.tm_mday, &deadline_tm.tm_hour, &deadline_tm.tm_min, &deadline_tm.tm_sec))
	{
		DEBUG("sscanf for time str failed, my_time: %s, deadline_time: %s\n", my_time, deadline_time);
	}
	else{
		if(my_tm.tm_year>deadline_tm.tm_year)
			return 1;
		else if(my_tm.tm_year<deadline_tm.tm_year)
			return -1;
		else{
			if(my_tm.tm_mon>deadline.tm_mon)
				return 1;
			else if(my_tm.tm_mon<deadline.tm_mon)
				return -1;
			else{
				.....
			}
		}
	}
}
#endif

void disk_manage_flag_set(int flag)
{
	s_disk_manage_flag = flag;
	DEBUG("s_disk_manage_flag=%d\n\n\n", s_disk_manage_flag);
}

/*
 ����push�����ݣ����Ҽ�صĽ�Ŀ��������0��������Ҫmonitor��ء�
 return 1������Ҫ���
 return 0��������Ҫ���
*/
static int need_push_monitor()
{
	if(s_push_has_data>0 && s_push_monitor_active>0)
		return 1;
	else{
		DEBUG("s_push_has_data: %d, s_push_monitor_active: %d\n", s_push_has_data, s_push_monitor_active);
		return 0;
	}
}

void column_refresh_flag_set(int flag)
{
	s_column_refresh = flag;
}

void interface_refresh_flag_set(int flag)
{
	s_interface_refresh = flag;
}

/*
Ϊ����������Ĳ�ѯӲ�̣�Ӧ�����������������
1������Ŀ������Ϻ�Ӧ�ٲ�ѯ�����ݿ��м�¼����100%
2��ֻ��UI�Ͻ���鿴���ȵĽ����֪ͨ�ײ�ȥ��ѯ������ʱ���ѯû�����塣
3����push�����ݺ�����ѯ���ɱ飨�ȴ���������д��Ӳ�̣���Ͳ�����ѯ��
*/
void *push_monitor_thread()
{
	s_monitor_running = 1;
	
	struct timeval now;
	struct timespec outtime;
	int retcode = 0;
	char sqlite_cmd[256];
	char time_stamp[32];
	int monitor_interval = 61;
	unsigned int loop_cnt = 0;
	int i = 0;
	
	while (1==s_monitor_running)
	{
		pthread_mutex_lock(&mtx_push_monitor);
		PRINTF("2 s_push_has_data:%d,s_push_monitor_active:%d\n",s_push_has_data,s_push_monitor_active);
		
		gettimeofday(&now, NULL);
		outtime.tv_sec = now.tv_sec + monitor_interval;
		outtime.tv_nsec = now.tv_usec;
		retcode = pthread_cond_timedwait(&cond_push_monitor, &mtx_push_monitor, &outtime);
		
		if(need_push_monitor()>0 && 0==s_dvbpush_getinfo_flag){
			memset(time_stamp, 0, sizeof(time_stamp));
			snprintf(sqlite_cmd,sizeof(sqlite_cmd),"select datetime('now','localtime');");
			if(-1==str_sqlite_read(time_stamp,sizeof(time_stamp),sqlite_cmd)){
				DEBUG("can not generate DATETIME for prog monitor\n");
				memset(time_stamp, 0, sizeof(time_stamp));
			}
			
			for(i=0; i<PROGS_NUM; i++)
			{
				if(-1==prog_is_valid(&s_prgs[i]) || s_prgs[i].cur>=s_prgs[i].total)
					continue;
				
				prog_monitor(&s_prgs[i],time_stamp);
			}
			s_push_has_data--;
		}
		
		for(i=0; i<PROGS_NUM; i++)
		{
			if(-1==prog_is_valid(&s_prgs[i]))
				continue;
			
			if(0==s_prgs[i].parsed && (s_prgs[i].cur) >= (s_prgs[i].total)){
				DEBUG("%s download finished, parse %s\n", s_prgs[i].uri,s_prgs[i].descURI);
				push_prog_finish(s_prgs[i].id,s_prgs[i].type,s_prgs[i].descURI);
				productdesc_parsed_set(s_prgs[i].descURI);
				s_prgs[i].parsed = 1;
			}
		}
		
		pthread_mutex_unlock(&mtx_push_monitor);
		
		if(ETIMEDOUT!=retcode){
			DEBUG("push monitor thread is awaked by external signal\n");
		}
		else{
//			if(need_push_monitor()>0)
//				push_recv_manage_refresh(2,time_stamp);
		}
		
		// ÿ��12��Сʱ����tdt pid����ʱ��ͬ��������ֻ�ǽ�����monitor�����Ƶѭ����
		loop_cnt ++;
		if(loop_cnt>(720)){
			time_t timep;
			struct tm *p_tm;
			timep = time(NULL);
			p_tm = localtime(&timep); /*��ȡ����ʱ��ʱ��*/ 
			DEBUG("it's time to awake tdt time sync, %4d-%2d-%2d %2d:%2d:%2d\n", (p_tm->tm_year+1900), (p_tm->tm_mon+1), p_tm->tm_mday, p_tm->tm_hour, p_tm->tm_min, p_tm->tm_sec);
			tdt_time_sync_awake();
			loop_cnt = 0;
		}
		
		// ����Ŀ�ͽ����Ʒ�����ı�ʱ������ֱ����parse_xml()������ͨ��JNI��UI����notify����������׵���Launcher������
		if(s_column_refresh>0){
			s_column_refresh ++;
			if(s_column_refresh>2){
				msg_send2_UI(STATUS_COLUMN_REFRESH, NULL, 0);
				s_column_refresh = 0;
			}
		}
		if(s_interface_refresh>0){
			s_interface_refresh ++;
			if(s_interface_refresh>2){
				msg_send2_UI(STATUS_COLUMN_REFRESH, NULL, 0);
				s_interface_refresh = 0;
			}
		}
		
		if(1==s_disk_manage_flag){
			DEBUG("will clean disk\n");
			disk_manage();
			s_disk_manage_flag = 0;
		}
	}
	DEBUG("exit from push monitor thread\n");
	
	return NULL;
}

void *push_xml_parse_thread()
{
	int i = 0;
	
	s_xmlparse_running = 1;
	while (1==s_xmlparse_running)
	{
		pthread_mutex_lock(&mtx_xml);
		pthread_cond_wait(&cond_xml,&mtx_xml); //wait
		DEBUG("awaked and parse xml\n");
		sleep(1);
		
		if(1==s_xmlparse_running){
			for(i=0; i<XML_NUM; i++){
				DEBUG("xml queue[%d]: %s\n", i, s_push_xml[i].uri);
				if(strlen(s_push_xml[i].uri)>0){
					DEBUG("will parse[%d] %s\n", i, s_push_xml[i].uri);
					parse_xml(s_push_xml[i].uri, s_push_xml[i].flag, s_push_xml[i].id);
					
					memset(s_push_xml[i].uri, 0, sizeof(s_push_xml[i].uri));
					s_push_xml[i].flag = PUSH_XML_FLAG_UNDEFINED;
					memset(s_push_xml[i].id, 0, sizeof(s_push_xml[i].id));
				}
			}
		}
		pthread_mutex_unlock(&mtx_xml);
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

int send_xml_to_parse(const char *path, int flag, char *id)
{
	int ret = 0;
	
	if(	PUSH_XML_FLAG_MINLINE<flag && flag<PUSH_XML_FLAG_MAXLINE){
		if(0==check_tail(path, ".xml", 0))
		{
			DEBUG("path: %s\n", path);
			pthread_mutex_lock(&mtx_xml);
			
			int i = 0;
			for(i=0; i<XML_NUM; i++){
				if(0==strlen(s_push_xml[i].uri)){
					DEBUG("add to index %d: [%s]path: %s\n", i,id,path);
					snprintf(s_push_xml[i].uri, sizeof(s_push_xml[i].uri),"%s", path);
					s_push_xml[i].flag = flag;
					if(id)
						snprintf(s_push_xml[i].id, sizeof(s_push_xml[i].id),"%s", id);
					break;
				}
			}
			if(XML_NUM<=i){
				DEBUG("xml name space is full\n");
				ret = -1;
			}
			else{
				pthread_cond_signal(&cond_xml); //send sianal
				ret = 0;
			}
				
			pthread_mutex_unlock(&mtx_xml);
			DEBUG("path in queue: %s ok\n", path);
		}
		else{
			DEBUG("this is not a xml\n");
			ret = -1;
		}
	}
	else{
		DEBUG("this file(%d) is ignore\n", flag);
		ret = -1;
	}
	
	return ret;
}

void callback(const char *path, long long size, int flag)
{
	DEBUG("\n\n\n===========================path:%s, size:%lld, flag:%d=============\n\n\n", path, size, flag);
	
	/* �����漰�����������ݿ���������ﲻֱ�ӵ���parseDoc�����ⵢ��push���������Ч�� */
	// settings/allpid/allpid.xml
	send_xml_to_parse(path, flag, NULL);
}


int push_decoder_buf_init()
{
	g_recvBuffer = (DataBuffer *)malloc(sizeof(DataBuffer)*MAX_PACK_BUF);
	if(NULL==g_recvBuffer){
		ERROROUT("can not malloc %d*%d\n", sizeof(DataBuffer), MAX_PACK_BUF);
		return -1;
	}
	else
		DEBUG("malloc for push decoder buffer %d*%d success\n", sizeof(DataBuffer), MAX_PACK_BUF);

#ifdef MEMSET_PUSHBUF_SAFE	
	memset(g_recvBuffer,0,sizeof(DataBuffer)*MAX_PACK_BUF);	
	DEBUG("g_recvBuffer=%p\n", g_recvBuffer);
#else	
	g_recvBuffer[0].m_len = 0;
	g_recvBuffer[1].m_len = 0;
#endif
	
	return 0;
}

static int push_decoder_buf_uninit()
{
	if(g_recvBuffer){
		DEBUG("free push decoder buf\n");
		DataBuffer *tmp_recvbuf = g_recvBuffer;
		g_recvBuffer = NULL;
		usleep(300);
		free(tmp_recvbuf);
		DEBUG("free push decoder buf: %p\n", tmp_recvbuf);
		tmp_recvbuf = NULL;
	}
	return 0;
}

int mid_push_init(char *push_conf)
{
	int i = 0;
	for(i=0;i<XML_NUM;i++){
		memset(s_push_xml[i].uri, 0, sizeof(s_push_xml[i].uri));
		s_push_xml[i].flag = PUSH_XML_FLAG_UNDEFINED;
		memset(s_push_xml[i].id, 0, sizeof(s_push_xml[i].id));
	}
	
	for(i=0; i<PROGS_NUM; i++){
		memset(&s_prgs[i], 0, sizeof(PROG_S));
	}
	
	/*
	* ��ʼ��PUSH��
	 */
	if (push_init(push_conf) != 0)
	{
		DEBUG("Init push lib failed with %s!\n", push_conf);
		return -1;
	}
	else
		DEBUG("Init push lib success with %s!\n", push_conf);
	
	
	/*
	��ʼ���ܾ����պͽ��ռ�أ�������push�����߳�֮ǰ��
	*/
	push_recv_manage_refresh(1,NULL);
	
	/*
	ȷ��������������һ��ɨ����ᣬ���׼ȷ�����ؽ��ȡ�
	*/
	s_push_has_data = 3;
	
	push_set_notice_callback(callback);
	
	//�������ݽ����߳�
	pthread_create(&tidDecodeData, NULL, push_decoder_thread, NULL);
	//pthread_detach(tidDecodeData);
	
	//���������߳�
	pthread_t tidMonitor;
	pthread_create(&tidMonitor, NULL, push_monitor_thread, NULL);
	pthread_detach(tidMonitor);
	
	//����xml�����߳�
	pthread_t tidxmlparse;
	pthread_create(&tidxmlparse, NULL, push_xml_parse_thread, NULL);
	pthread_detach(tidxmlparse);
	
	return 0;
}

int mid_push_uninit()
{
//	push_rely_condition_set(RELY_CONDITION_EXIT);
	
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
	push_decoder_buf_uninit();
	
	return 0;
}

/*
 ��push��ͣ��Ŀǰ������Ӳ��ǰ���á�
*/
int push_pause()
{
#if 0
	push_destroy();
#endif

	return 0;
}

/*
 ��push�ָ���Ŀǰ������Ӳ�̺���á�
*/
int push_resume()
{
#if 0
	if (push_init(PUSH_CONF) != 0)
	{
		DEBUG("Init push lib failed with %s!\n", push_conf);
		return -1;
	}
	else
		DEBUG("Init push lib success with %s!\n", push_conf);
#endif
	return 0;
}


int TC_loader_to_push_order(int ord)
{
	DEBUG("ord: %d\n", ord);
    if (ord)
    {
        s_decoder_running = 1;
    }
    else
    {
        s_decoder_running = 2;
       // g_wIndex = 0;
    }
    return 0;
}

int TC_loader_get_push_state(void)
{
    return push_idle;
}

int TC_loader_get_push_buf_size(void)
{
    return sizeof(DataBuffer)*MAX_PACK_BUF;
}

unsigned char * TC_loader_get_push_buf_pointer(void)
{
    return (unsigned char *)g_recvBuffer;
}

/*
ע���Ŀ
*/
static int mid_push_regist(PROG_S *prog)
{
	if(NULL==prog){
		DEBUG("arg is invalid\n");
		return -1;
	}
	if(0==prog_is_valid(prog)){
		DEBUG("invalid prog to regist monitor\n");
		return -1;
	}
	
	/*
	* Notice:��Ŀ·����һ�����·������Ҫ��'/'��ͷ��
	* ����Ŀ���и�����·����"/vedios/pushvod/1944"����ȥ���ʼ��'/'��
	* ��"vedios/pushvod/1944"����ע�ᡣ
	*
	* �˴�PRG����ṹ���ǳ���ʾ�����㶨��ģ���һ�����������ĳ�����
	*/
	int i = 0, ret = -1;
	
/*
 ���жϴ�uri�Ƿ��Ѿ���monitor�ڣ���ֹ�ظ�������ͬ��Ŀ¼��ص��¼�����鱬��
*/
	for(i=0; i<PROGS_NUM; i++)
	{
		if(1==prog_is_valid(&s_prgs[i]) && 0==strcmp(s_prgs[i].uri,prog->uri)){
			DEBUG("Warning: this prog[id=%s] is already regist, cover old record\n",s_prgs[i].id);
			
			snprintf(s_prgs[i].id, sizeof(s_prgs[i].id), "%s", prog->id);
			snprintf(s_prgs[i].uri, sizeof(s_prgs[i].uri), "%s", prog->uri);
			snprintf(s_prgs[i].descURI, sizeof(s_prgs[i].descURI), "%s", prog->descURI);
			snprintf(s_prgs[i].deadline, sizeof(s_prgs[i].deadline), "%s", prog->deadline);
			s_prgs[i].type = prog->type;
			s_prgs[i].cur = prog->cur;
			s_prgs[i].total = prog->total;
			s_prgs[i].parsed = prog->parsed;
			
			DEBUG("regist to push[%d]:%s %s %s %s %lld\n",
					i,
					s_prgs[i].id,
					s_prgs[i].caption,
					s_prgs[i].uri,
					s_prgs[i].descURI,
					s_prgs[i].total);
			
			return 0;
		}
	}
	
	for(i=0; i<PROGS_NUM; i++)
	{
		if(-1==prog_is_valid(&s_prgs[i])){
			snprintf(s_prgs[i].id, sizeof(s_prgs[i].id), "%s", prog->id);
			snprintf(s_prgs[i].uri, sizeof(s_prgs[i].uri), "%s", prog->uri);
			snprintf(s_prgs[i].descURI, sizeof(s_prgs[i].descURI), "%s", prog->descURI);
			snprintf(s_prgs[i].deadline, sizeof(s_prgs[i].deadline), "%s", prog->deadline);
			s_prgs[i].type = prog->type;
			s_prgs[i].cur = prog->cur;
			s_prgs[i].total = prog->total;
			s_prgs[i].parsed = prog->parsed;
			
			push_dir_register(s_prgs[i].uri, s_prgs[i].total, 0);
			s_push_monitor_active++;
			
			DEBUG("regist to push[%d]:%s %s %s %lld\n",
					i,
					s_prgs[i].id,
					s_prgs[i].uri,
					s_prgs[i].descURI,
					s_prgs[i].total);
			break;
		}
	}
	
	if(i>=PROGS_NUM){
		DEBUG("progs monitor array is overflow\n");
		ret = -1;
	}
	else
		ret = 0;
	
	return ret;
}


/*
 ��ע���ؽ�Ŀ��
*/
static int mid_push_unregist(PROG_S *prog)
{
	/*
	* Notice:��Ŀ·����һ�����·������Ҫ��'/'��ͷ��
	* ����Ŀ���и�����·����"/vedios/pushvod/1944"����ȥ���ʼ��'/'��
	* ��"vedios/pushvod/1944"����ע�ᡣ
	*
	* �˴�PRG����ṹ���ǳ���ʾ�����㶨��ģ���һ�����������ĳ�����
	*/
	int ret = -1;
	if(prog){
		DEBUG("unregist from push monitor: %s %lld\n", prog->uri, prog->total);
		
		push_dir_unregister(prog->uri);
		
		memset(prog, 0, sizeof(PROG_S));
		
		s_push_monitor_active--;
		ret = 0;
	}
	else{
		DEBUG("invalid arg\n");
		ret = -1;
	}
	
	return ret;
}

/*
����Ŀ������
*/
static int prog_name_fill()
{
	/*
	* Notice:��Ŀ·����һ�����·������Ҫ��'/'��ͷ��
	* ����Ŀ���и�����·����"/vedios/pushvod/1944"����ȥ���ʼ��'/'��
	* ��"vedios/pushvod/1944"����ע�ᡣ
	*
	* �˴�PRG����ṹ���ǳ���ʾ�����㶨��ģ���һ�����������ĳ�����
	*/
	int i;
	char sqlite_cmd[256];
	for(i=0; i<PROGS_NUM; i++)
	{
		if(1==prog_is_valid(&s_prgs[i]) && 0==strlen(s_prgs[i].caption)){
			memset(s_prgs[i].caption, 0, sizeof(s_prgs[i].caption));
			snprintf(sqlite_cmd,sizeof(sqlite_cmd),"SELECT StrValue FROM ResStr WHERE ServiceID='%s' AND ObjectName='ProductDesc' AND EntityID='%s' AND StrLang='%s' AND (StrName='ProductDescName' OR StrName='SProductName' OR StrName='ColumnName');", 
				serviceID_get(),s_prgs[i].id,language_get());
			
			if(0==str_sqlite_read(s_prgs[i].caption,sizeof(s_prgs[i].caption),sqlite_cmd)){
				DEBUG("read prog caption success: %s\n", s_prgs[i].caption);
			}
			else{
				DEBUG("read prog caption failed, filled with prog id: %s\n",s_prgs[i].id);
				snprintf(s_prgs[i].caption, sizeof(s_prgs[i].caption), "%s", s_prgs[i].id);
			}
		}
	}
	
	return 0;
}

int mid_push_reject(const char *prog_uri)
{
	if(NULL==prog_uri){
		DEBUG("invalid prog_uri\n");
		return -1;
	}
	
	int ret = 0;
	ret = push_dir_forbid(prog_uri);
	if(0==ret)
		DEBUG("push forbid: %s\n", prog_uri);
	else if(-1==ret)
		DEBUG("push forbid failed: %s, no such uri\n", prog_uri);
	else
		DEBUG("push forbid failed: %s, some other err(%d)\n", prog_uri, ret);
	
	ret = push_dir_remove(prog_uri);
	if(0==ret)
		DEBUG("push remove: %s\n", prog_uri);
	else if(-1==ret)
		DEBUG("push remove failed: %s, no such uri\n", prog_uri);
	else
		DEBUG("push remove failed: %s, some other err(%d)\n", prog_uri, ret);
	
	char reject_uri[128];
	snprintf(reject_uri,sizeof(reject_uri),"%s/%s",push_dir_get(),prog_uri);
	unlink(reject_uri);
	DEBUG("unlink(%s)\n", reject_uri);
	
	return ret;
}

/*
�ص�����ʱ��receiverЯ���Ƿ��н���ע��ı�ǣ�1��ʾ��ע�ᣬ0��ʾ��ע�ᡣ
*/
static int push_recv_manage_cb(char **result, int row, int column, void *receiver, unsigned int receiver_size)
{
	DEBUG("sqlite callback, row=%d, column=%d, receiver addr=%p, receive_size=%u\n", row, column, receiver,receiver_size);
	if(row<1){
		DEBUG("no record in table, return\n");
		return 0;
	}
//ProductDescID,ID,ReceiveType,URI,DescURI,TotalSize,PushStartTime,PushEndTime,ReceiveStatus,FreshFlag,Parsed
	int i = 0;
	int j = 0;
	int recv_flag = 1;
	int tmp_init_flag = *((int *)receiver);
	RECEIVESTATUS_E receive_status = RECEIVESTATUS_REJECT;
	long long totalsize = 0LL;
	
	DEBUG("*receiver(init flag)=%d\n", tmp_init_flag);
	
	*((int *)receiver) = 0;
	
	for(i=1;i<row+1;i++)
	{
		/*
		����ǿ���ʱע�᣻���߲��ǿ���ע�ᡢ��FreshFlagΪ1����ע��ܾ����ջ���ȼ��
		���򣬲��Ӵ���
		*/
		if(1==tmp_init_flag || (1!=tmp_init_flag && 1==atoi(result[i*column+9]))){
			/*
			���ڳ�Ʒ������û�ѡ�񲻽��գ���һ�������գ�����Ҫ������ϸ���ж�
			���򣬸���ҵ������������ж�
			*/
			if(RECEIVETYPE_PUBLICATION==atoi(result[i*column+2]) && 0==guidelist_select_status((const char *)(result[i*column+1]))){
				recv_flag = 0;
				DEBUG("this prog(%s) is reject by user in guidelist\n", result[i*column+3]);
			}
			else
			{
				receive_status = atoi(result[i*column+8]);
				if(RECEIVESTATUS_REJECT==receive_status){
					/*
					�ܾ�����ʱһ��ҪС�ģ���ͬ��Publication�п��ܼ����ڵ�ǰservice������������service�������ǣ�������Service����Ҫ���ա�
					*/
					recv_flag = 0;
					for(j=1;j<row+1;j++){
						if(0==strcmp(result[j*column+3],result[i*column+3]) && (RECEIVESTATUS_WAITING==atoi(result[j*column+8])|| RECEIVESTATUS_FINISH==atoi(result[j*column+8]))){
							DEBUG("this prog(%s) is need recv in other service, do not reject it\n",result[i*column+3]);
							recv_flag = 1;
							break;
						}
					}
				}
				else if (RECEIVESTATUS_WAITING==receive_status || RECEIVESTATUS_FINISH==receive_status){
					recv_flag = 1;
				}
				else{ // RECEIVESTATUS_FAILED==receive_status || RECEIVESTATUS_HISTORY==receive_status
					DEBUG("[%d:%s] %s is ignored by push monitor\n", i,result[i*column],result[i*column+3]);
					recv_flag = 0;
				}
			}
			
			if(0==recv_flag){
				mid_push_reject(result[i*column+3]);
			}
			else{
				sscanf(result[i*column+5],"%lld", &totalsize);
				
				PROG_S cur_prog;
				memset(&cur_prog,0,sizeof(cur_prog));
				snprintf(cur_prog.id,sizeof(cur_prog.id),"%s",result[i*column]);
				snprintf(cur_prog.uri,sizeof(cur_prog.uri),"%s",result[i*column+3]);
				snprintf(cur_prog.descURI,sizeof(cur_prog.descURI),"%s",result[i*column+4]);
				memset(cur_prog.caption,0,sizeof(cur_prog.caption));
				snprintf(cur_prog.deadline,sizeof(cur_prog.deadline),"%s",result[i*column+7]);
				cur_prog.type = atoi(result[i*column+2]);
				cur_prog.cur = 0LL;
				cur_prog.total = totalsize;
				cur_prog.parsed = atoi(result[i*column+10]);
				mid_push_regist(&cur_prog);
				
				/*
				�ص�����ʱ��receiverЯ���Ƿ��н���ע��ı�ǣ�1��ʾ��ע�ᣬ0��ʾ��ע�ᡣ
				*/
				*((int *)receiver) = 1;
			}
		}
	}
	
	return 0;
}

/*
 ���·��µ�ProductDesc.xml��Service.xmlʱˢ��push�ܾ�����ע��ͽ��ȼ��ע��
 init_flag����1����ʼ������ʾ��Ҫ����ProductDesc���еĽ�Ŀ
 init_flag����0���ǳ�ʼ������ʾ�Ƕ�̬�������յ��µ�Service.xml��ProductDesc.xml��ֻ����FreshFlagΪ1�Ľ�Ŀ
 init_flag����2����monitor�е��õ�ʵʱ��أ�Ŀ������������ڵĽ�Ŀ���ٽ��н��ȼ�أ������ն˼��첻�ػ������ۻ�
*/
int push_recv_manage_refresh(int init_flag, char *time_stamp_pointed)
{
	DEBUG("init_flag: %d, time_stamp_pointed: %s\n", init_flag, time_stamp_pointed);
	
	int ret = -1;
	char sqlite_cmd[256+128];
	int (*sqlite_callback)(char **, int, int, void *, unsigned int) = push_recv_manage_cb;
	
	char time_stamp[32];
	memset(time_stamp, 0, sizeof(time_stamp));
	if(NULL==time_stamp_pointed || 0==strlen(time_stamp_pointed)){
		snprintf(sqlite_cmd,sizeof(sqlite_cmd),"select datetime('now','localtime');");
		if(-1==str_sqlite_read(time_stamp,sizeof(time_stamp),sqlite_cmd)){
			DEBUG("can not process push regist\n");
			return -1;
		}
	}
	else
		snprintf(time_stamp,sizeof(time_stamp),"%s",time_stamp_pointed);
	
	pthread_mutex_lock(&mtx_push_monitor);
	
	if(1==init_flag){
/*
 ������ʼ��ʱ����ɾ�����й��ڵĽ�Ŀ
*/
		snprintf(sqlite_cmd,sizeof(sqlite_cmd),"DELETE FROM ProductDesc WHERE PushEndTime<'%s';", time_stamp);
		sqlite_execute(sqlite_cmd);
	}
	
	int flag_carrier = init_flag;
	
/*
 ����һ��Publication���ܴ����ڶ��service�У������Ҫȫ��ȡ�����ڻص��б�����Щ��Ҫ�ܾ���publication�Ƿ�ǡ��Ҳ����Ҫ����֮�С�
 ���Զ�FreshFlag���ж��ƶ����ص��н��У�������������FreshFlag֮��
*/
	snprintf(sqlite_cmd,sizeof(sqlite_cmd),"SELECT ProductDescID,ID,ReceiveType,URI,DescURI,TotalSize,PushStartTime,PushEndTime,ReceiveStatus,FreshFlag,Parsed FROM ProductDesc WHERE PushStartTime<='%s' AND PushEndTime>'%s';", time_stamp,time_stamp);
	
	ret = sqlite_read(sqlite_cmd, (void *)(&flag_carrier), sizeof(flag_carrier), sqlite_callback);
/*
�ص�����ʱ��flag_carrierЯ���Ƿ��н���ע��ı�ǣ�1��ʾ��ע�ᣬ0��ʾ��ע�ᡣ
*/	
	PRINTF("ret: %d, flag_carrier: %d\n", ret,flag_carrier);
	if(ret>0 && flag_carrier>0){
		prog_name_fill();
		
		snprintf(sqlite_cmd,sizeof(sqlite_cmd),"UPDATE ProductDesc SET FreshFlag=0 WHERE PushStartTime<='%s' AND PushEndTime>'%s' AND FreshFlag=1;", time_stamp,time_stamp);
		sqlite_execute(sqlite_cmd);

#if 0
		if(0==init_flag){
			pthread_cond_signal(&cond_push_monitor);
			DEBUG("refresh monitor arrary immediatly\n");
		}
#endif

	}
	
	pthread_mutex_unlock(&mtx_push_monitor);

	return ret;
}

