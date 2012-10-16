#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <sys/time.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <sys/socket.h>
#include <net/if.h>
#include <arpa/inet.h>
#include <netpacket/packet.h>
#include <linux/if_ether.h>
#include <net/if_arp.h>
#include <semaphore.h>
#include <sys/types.h>
#include <dirent.h>

#include "common.h"

/* 
���ܣ���ָ���ַ�����ָ��λ�õ��ִ�������ָ���Ľ��ƽ���ת�����õ�long������
���룺	str				����ԭʼ�ַ��������Բ������ֿ�ͷ
		str_len			����ԭʼ�ַ�������
		start_position	����ָ��ת������ʼλ�ã�ԭʼ�ַ����Ŀ�ͷλ�ö���Ϊ0
		appoint_len		����ָ����Ҫת���ĳ���
		base			����ת���Ľ��ƣ�ȡֵ��strtolһ��
�����ʧ�ܷ���-1���������صõ���long int����
*/
int appoint_str2int(char *str, unsigned int str_len, unsigned int start_position, unsigned int appoint_len, int base)
{
	if(NULL==str || str_len<(start_position+appoint_len) || appoint_len>64 || (base<0 && 36<base)){
		DEBUG("some arguments are invalid\n");
		return -1;
	}

	char tmp_str[65];
	int ret_int = 0;
	
	memset(tmp_str, 0, sizeof(tmp_str));
	strncpy(tmp_str, str+start_position, appoint_len);
	ret_int = strtol(tmp_str, NULL, base);//atoi(tmp_str);
	DEBUG("tmp_str=%s, will return with 0x%x==%d, origine str=%s, start at %d, aspect len %d\n", tmp_str,ret_int,ret_int, str, start_position, appoint_len);
	return ret_int;
}

void ms_sleep(unsigned int ms)
{
	if(ms<=0)
		return;
	struct timeval timeout;
	timeout.tv_sec=ms/1000;
	timeout.tv_usec=(ms%1000)*1000;			///ms
	select(0,NULL,NULL,NULL,&timeout);
}

unsigned int randint()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	
	srand((unsigned int)(tv.tv_usec)%10000);
	return rand();
}


/*
Ŀ¼��ʼ��������ֱ�Ӵ����ļ�ʧ�ܡ�
�����Ҫȷ������Ŀ¼������Ҫ��·�������б�ܣ��������һ����Ϊ�ļ�����
�磺filenameΪ/home/test/aaa.txt����ȷ������Ŀ¼/home/test
�磺filenameΪ/home/mydir/����ȷ������Ŀ¼/home/mydir
*/
int dir_exist_ensure(char *filename)
{
	if(NULL==filename || strlen(filename)>128){
		DEBUG("file name is NULL or too long\n");
		return -1;
	}
		
	char tmp_dir[128];
	snprintf(tmp_dir, sizeof(tmp_dir), "%s", filename);
	char *last_slash = strrchr(tmp_dir, '/');
	if(NULL==last_slash)
		return 0;
	
	*last_slash = '\0';
	
	if(0!=access(tmp_dir, F_OK)){
		ERROROUT("dir %s is not exist\n", tmp_dir);
		if(0!=mkdir(tmp_dir, 0777)){
			ERROROUT("create dir %s failed\n", tmp_dir);
			return -1;
		}
		else{
			DEBUG("create dir %s success\n", tmp_dir);
			return 0;
		}
	}
	else{
		DEBUG("dir %s is exist\n", tmp_dir);
		return 0;
	}
}

void print_timestamp(int show_s_ms, int show_str)
{
	struct timeval tv_now;
	time_t t;
	struct tm area;
	tzset(); /* tzset()*/
	
	if(show_s_ms){
		if(-1==gettimeofday(&tv_now, NULL)){
			ERROROUT("gettimeofday failed\n");
		}
		else
			DEBUG("|s: %ld\t|ms: %ld\t|us:%ld\t", tv_now.tv_sec, (tv_now.tv_usec)/1000, (tv_now.tv_usec));
	}
	if(show_str){
		t = time(NULL);
		localtime_r(&t, &area);
		DEBUG("|%s", asctime(&area));
	}
	
	if(0==show_str)
		DEBUG("\n");
	
	return;
}
/*
�������������ڰ�����ִ��ʱ��ʾFloating point exception��ԭ��
�߰汾��gcc������ʱ�������µĹ�ϣ��������߶�̬���ӵ��ٶȣ����ڵͰ汾���ǲ�֧�ֵġ���˻ᷢ���������
���������
�����ӵ�ʱ�����ѡ��-Wl,--hash-style=sysv
���� gcc -Wl,--hash-type=sysv -o test test.c
http://fhqdddddd.blog.163.com/blog/static/18699154201002683914623/
--------------------------------------------------
�򵥷���:��̬����
��������� -static
g++ ....... -static

������������ʵ��ʹ��ʱ��Ч��ֻ���Լ���һ���ܼ򵥵ģ���֧������������ĺ�����

*/
int phony_div(unsigned int div_father, unsigned int div_son)
{
	if(0==div_son)
		return -1;
	
	if(0==div_father)
		return 0;
	
	int ret = 0;
	while(div_father>=div_son){
		ret ++;
		div_father -= div_son;
	}
	
	return ret;
}

/* 
����ַ���str_dad��β���Ƿ���str_son������str_sonҪô������ָ����signchr�ַ����棬Ҫôstr_dad��ȫ����str_son��
������	���ȫ·��settings/allpid/allpid.xml���Ƿ����ļ���allpid.xml��ָ���ָ���Ϊ��/����
			
		strrstr_s("settings/allpid/allpid.xml", STR_SON, '/');
		
		STR_SON����
			1��������ȫ��ƥ�䣬llpid.xml�����ڣ�aallpid.xmlҲ�����ڣ�
			2�����str_dad��ȫ����str_son�����ж�Ϊ���ڣ�����settings/allpid/allpid.xml����
			3���Ӵ���������ڸ���ĩβ��allpid�����ڣ���Ϊ�����м䣻	
			4���Ӵ������зָ�����allpid/allpid.xml���ڣ�
		
����ֵ���ο�strrstr
 */
char *strrstr_s(const char *str_dad, char *str_son, char separater_sign)
{
	if(NULL==str_dad || NULL==str_son || strlen(str_dad)<strlen(str_son)){
		//DEBUG("can not compared between invalid string\n");
		return NULL;
	}
	
	if(0==strcmp(str_dad, str_son))
		return (char *)str_dad;
	
	
	int i = 0;
	char *p_dad = (char *)str_dad;
	char *p = NULL;
	for(i=0; i<256; i++){
		p = strchr(p_dad, separater_sign);
		//DEBUG("p_dad: %s, p: %s\n", p_dad, p);
		if(p && strlen(p)>=(strlen(str_son)+1)){
			p++;
			if(0==strcmp(p, str_son))
				return p;
			else
				p_dad = p;
		}
		else
			return NULL;
	}
	
	if(256==i)
		DEBUG("what a fucking string you check for, it has 256 separater sign at least.\n");
	
	return NULL;
}

/*
�������Ӻ���������Ψһ���롣��û��ͬ��ʱ��ʱ���п��ܵõ���ͬ��ֵ�������ں��뼶���ϸ���΢����΢��
*/
static char s_time_serial[32];
char *time_serial()
{
	struct timeval tv;
	gettimeofday(&tv, NULL);
	snprintf(s_time_serial, sizeof(s_time_serial),"%ld%ld", tv.tv_sec, tv.tv_usec);
	//DEBUG("tv.tv_sec=%ld, tv.tv_usec=%ld, s_time_serial=%s\n", tv.tv_sec, tv.tv_usec, s_time_serial);
	return s_time_serial;
}

/*
�������ĵ��ʮ����IPv4��ַ�Ƿ�Ϊ�Ϸ���IP��ַ��������Ƚ�����
-1���Ƿ���
0���Ϸ�
*/
int ipv4_simple_check(const char *ip_addr)
{
	if(NULL==ip_addr || 0==strlen(ip_addr))
		return -1;
	
	int ret = -1;
	int ip[4];
	if(4==sscanf(ip_addr, "%d.%d.%d.%d",&ip[0],&ip[1],&ip[2],&ip[3])){
		DEBUG("will check ip %d-%d-%d-%d\n", ip[0], ip[1], ip[2], ip[3]);
	}
	else{
		DEBUG("can NOT check %s, perhaps it has invalid format\n", ip_addr);
		return -1;
	}
	
	if((ip[0]>=0&&ip[0]<224)&&(ip[1]>=0&&ip[1]<256)&&(ip[2]>=0&&ip[2]<256)&&(ip[3]>=0&&ip[3]<256))
	{
		/*
		�ѵ�ַΪȫ���IP��ַ��ȥ
		*/
		if(ip[0]==0&&ip[1]==0&&ip[2]==0&&ip[3]==0)
			ret = -1;
		/*
		��127.0.0.0ȥ��
		*/
		else if(ip[0]==127)
			ret = -1;
		/*
		��������Ϊȫ1��ȥ��
		*/
		else if(ip[1]==255&&ip[2]==255&&ip[3]==255)
			ret = -1;
		/*
		��������Ϊȫ0��ȥ��
		*/
		else if(ip[1]==0&&ip[2]==0&&ip[3]==0)
			ret = -1;
		else
			ret = 0;
	}
	else
		ret = -1;
	
	return ret;
}

/*
 ��·��path�л�ȡ��һ���ļ���ʽΪfilefmt���ļ�������ƥ��preferential_file��
 ����õ��ļ�uri�����file��
*/
int distill_file(char *path, char *file, unsigned int file_size, char *filefmt, char *preferential_file)
{
	DIR * pdir;
	struct dirent *ptr;
	char newpath[512];
	struct stat filestat;
	int file_count = 0;
	int file_count_max = 64;
	int ret = -1;
	
	if(NULL==path || NULL==file || 0==file_size){
		DEBUG("some arguments are invalid\n");
		return -1;
	}
	
	if(stat(path, &filestat) != 0){
		DEBUG("The file or path(%s) can not be get stat!\n", path);
		return -1;
	}
	if((filestat.st_mode & S_IFDIR) != S_IFDIR){
		DEBUG("(%s) is not be a path!\n", path);
		return -1;
	}
	pdir =opendir(path);
	while((ptr = readdir(pdir))!=NULL)
	{
		if((file_count+1) > file_count_max){
			DEBUG("The count of the files is too much(%d > %d)!\n", file_count + 1, file_count_max);
			break;
		}
		
		if(0==strcmp(ptr->d_name, ".") || 0==strcmp(ptr->d_name, ".."))
			continue;
		
		snprintf(newpath,sizeof(newpath),"%s/%s", path,ptr->d_name);
		if(stat(newpath, &filestat) != 0){
			DEBUG("The file or path(%s) can not be get stat!\n", newpath);
			continue;
		}
		/* Check if it is file. */
		if((filestat.st_mode & S_IFREG) == S_IFREG){
			snprintf(file,file_size,"%s/%s", path,ptr->d_name);
			if(NULL!=preferential_file && 0!=strlen(preferential_file)){
				if(0==strcmp(ptr->d_name, preferential_file)){
					DEBUG("match %s in %s\n", preferential_file, path);
					ret = 0;
					break;
				}
			}
			else if(NULL!=filefmt && 0!=strlen(filefmt)){
				char *p = strrchr(ptr->d_name,'.');
				if(p){
					p++;
					if(p && strlen(filefmt)==strlen(p) && 0==strncasecmp(p, filefmt, strlen(filefmt))){
						DEBUG("match file format %s file %s\n", filefmt, file);
						ret = 0;
						break;
					}
				}
			}
			else{
				ret = 0;
				DEBUG("get %s in %s\n", ptr->d_name, path);
				break;
			}
			
//			if(filefmt[0] != '\0'){
//				char* p;
//				if((p = strrchr(ptr->d_name,'.')) == 0) continue;
//				
//				char fileformat[64];
//				char* token;
//				strcpy(fileformat, filefmt);        
//				if((token = strtok( fileformat,";")) == NULL){
//					strcpy(file[file_count], newpath);
//					file_count++;
//					continue;
//				}else{
//					if(strcasecmp(token,p) == 0){
//						strcpy(file[file_count], newpath);
//						file_count++;
//						continue;
//					}
//				}
//				while((token = strtok( NULL,";")) != NULL){
//					if(strcasecmp(token,p) == 0){
//						strcpy(file[file_count], newpath);
//						file_count++;
//						continue;
//					}
//				}
//			}
//			else{
//				strcpy(file[file_count], newpath);
//				file_count++;
//			}
		}
//		else if((filestat.st_mode & S_IFDIR) == S_IFDIR){
//			if(ReadPath(newpath, file, filefmt) != 0){
//				DEBUG("Path(%s) reading is fail!\n", newpath);
//				continue;
//			}
//		} 
	}
	closedir(pdir);
	return ret;   
}
