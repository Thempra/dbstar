
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <fcntl.h>
#include <time.h>
#include "common.h"
#include "softdmx.h"
#include "dmx.h"
#include "prodrm20.h"
#include "dvbpush_api.h"
#include "bootloader.h"
#include "softdmx_print.h"
#include "porting.h"

#include "libinclude/am_dmx.h"
#include "libinclude/am_fend.h"
#include "libinclude/am_util.h"

int loader_dsc_fid;
int tdt_dsc_fid = -1;;
LoaderInfo_t g_loaderInfo;
static pthread_t loaderthread = 0;
static int loaderAction = 0;
static int s_print_cnt = 0;
static unsigned char tc_tid = 0xff;
static unsigned short tc_pid = 0xffff;

static unsigned char software_version[4];

extern int TC_loader_get_push_state(void);
extern int TC_loader_get_push_buf_size(void);
extern unsigned char * TC_loader_get_push_buf_pointer(void);
static void mpe_section_handle(int dev_no, int fid, const unsigned char *data, int len, void *user_data);
extern int sha_verify(FILE *f,  uint8_t*sha0, size_t signed_len);
extern int TC_loader_to_push_order(int ord);
//int TC_loader_filter_handle(int aof);

#define UPGRADEFILE_ALL "/tmp/upgrade.zip"
#define UPGRADEFILE_IMG "/cache/recovery/upgrade.zip"
#define COMMAND_FILE  "/cache/recovery/command0"
#define LOADER_PACKAGE_SIZE		(4084)

#define FEND_DEV_NO 0
#define DMX_DEV_NO 0

// tuner api
static int tuner_inited = 0;
unsigned short chanFilter[MAX_CHAN_FILTER];

static void fend_cb(int dev_no, struct dvb_frontend_event *evt, void *user_data)
{
	fe_status_t status;
	int ber, snr, strength;
	struct dvb_frontend_info info;

	AM_FEND_GetInfo(dev_no, &info);
	printf("cb status: 0x%x\n", evt->status);
	
	AM_FEND_GetStatus(dev_no, &status);
	AM_FEND_GetBER(dev_no, &ber);
	AM_FEND_GetSNR(dev_no, &snr);
	AM_FEND_GetStrength(dev_no, &strength);
	
	printf("cb status: 0x%0x ber:%d snr:%d, strength:%d\n", status, ber, snr, strength);
}

int tuner_init(int freq, int symbolrate, int voltage)
{
	AM_FEND_OpenPara_t fpara;
	AM_DMX_OpenPara_t para;
	struct dvb_frontend_parameters p;
	fe_status_t status;
	
	if((freq)&&(tuner_inited==0))
	{
		memset(&para, 0, sizeof(para));
		AM_TRY(AM_DMX_Open(DMX_DEV_NO, &para));
	    AM_DMX_SetSource(DMX_DEV_NO, AM_DMX_SRC_TS2);
		
		memset(&fpara, 0, sizeof(fpara));
		fpara.mode = AM_FEND_DEMOD_DVBS;
		AM_TRY(AM_FEND_Open(FEND_DEV_NO, &fpara));
        AM_TRY(AM_FEND_SetCallback(FEND_DEV_NO, fend_cb, NULL));
        AM_FEND_SetVoltage(FEND_DEV_NO, voltage);   //v13-0/v18-1/v_off-2
        
		p.frequency = freq;   //khz
		p.inversion = INVERSION_AUTO;
		p.u.qpsk.symbol_rate = symbolrate;  //1hz
		p.u.qpsk.fec_inner = FEC_AUTO;
		
		AM_TRY(AM_FEND_Lock(FEND_DEV_NO, &p, &status));
		printf("lock status: 0x%x\n", status);
		
		tuner_inited = 1;
		return status;
	}
    else 
    {
    	tuner_inited = 0;
    	return -1;
    }
}

int tuner_uninit()
{
	if (tuner_inited)
    {
	    AM_DMX_Close(DMX_DEV_NO);
	    AM_FEND_Close(FEND_DEV_NO);
	    tuner_inited = 0;
	}
	return 0;
}

int TC_free_filter(int fid)
{
	AM_TRY(AM_DMX_StopFilter(DMX_DEV_NO, fid));
	AM_TRY(AM_DMX_FreeFilter(DMX_DEV_NO, fid));
	DEBUG("free fid: %d\n", fid);
        return 0;
}

int TC_alloc_filter(unsigned short pid, Filter_param* sparam, AM_DMX_DataCb hdle, void* userdata, char priority)
{
	int fid,i;
	struct dmx_sct_filter_params param;
	
	AM_TRY(AM_DMX_AllocateFilter(DMX_DEV_NO, &fid));
	AM_TRY(AM_DMX_SetCallback(DMX_DEV_NO, fid, hdle, NULL));
	
	memset(&param, 0, sizeof(param));
	param.pid = pid;
	for(i=0; i<DMX_FILTER_SIZE;i++)
	{
		param.filter.filter[i] = sparam->filter[i];
	    param.filter.mask[i] = sparam->mask[i];
	    param.filter.mode[i] = sparam->mode[i];
	}
	param.flags = DMX_CHECK_CRC;
	AM_TRY(AM_DMX_SetSecFilter(DMX_DEV_NO, fid, &param));
	AM_TRY(AM_DMX_SetBufferSize(DMX_DEV_NO, fid, 32*1024));
	AM_TRY(AM_DMX_StartFilter(DMX_DEV_NO, fid));
        return 0;
}

int alloc_filter(unsigned short pid, char pro)
{
	Filter_param param;
	
	DEBUG("pid=%d|0x%x, pro=%d\n", pid, pid, pro);
	
	memset(&param,0,sizeof(param));
	param.filter[0] = 0x3e;
	param.mask[0] = 0xff;
	
	return TC_alloc_filter(pid, &param, mpe_section_handle, NULL, pro);
}

int free_filter(unsigned short pid)
{
	int i;
	for(i=0; i < MAX_CHAN_FILTER; i++)
	{
		if(pid==chanFilter[i])
		{
			TC_free_filter(i);
			chanFilter[i] = 0xffff;
			return 0;
		}
	}
	
	return -1;
}

//tuner api end 
//extern  int test_tc();
int upgradefile_clear()
{
//test_tc();
	PRINTF("unlink %s\n", UPGRADEFILE_IMG);
	return unlink(UPGRADEFILE_IMG);
}

static void* loader_thread()
{
	unsigned char buf[1024];
	unsigned char sha0[64];
	FILE *fp = fopen(UPGRADEFILE_IMG,"r");
	int ret;
	unsigned int len = 0,wlen = 0,rlen = 0;

reLoader:
	while (loaderAction == 0)
	{
		sleep(2);
		PRINTF("loaderAction == 0\n");
	}
	/*fp=fopen("localfile","rb");// localfile�ļ���
	fseek(fp,0,SEEK_SET);
	fseek(fp,0,SEEK_END);
	long longBytes=ftell(fp);// longBytes�����ļ��ĳ���
	*/
	//DEBUG("mtd_scan_partitions = [%d]\n",mtd_scan_partitions());
	DEBUG("in loader thread...\n");
	wlen = 0;
	ret = fread(buf,1,48,fp);
	len = 0;
	ret = fread(&len,1,1,fp);
	DEBUG("in loader thread, read file len = [%u]\n",len);
	if (len > 1024)
		rlen = 1024;
	else
		rlen = len;

	do
	{
		ret = fread(buf,1,rlen,fp);
		if (ret > 0)
			wlen += ret;
		else
		{
			break;
		}
		rlen = len - wlen;
		if (rlen > 1024)
			rlen = 1024;
		else
			DEBUG("rlen=%d\n", rlen);
		//        else if (rlen < 0)
		//            break;
	} while(wlen < len);
	if (wlen != len)
	{
		DEBUG("received upgrade file is err, re download the file!!!!\n");
	}

	wlen = 0;
	ret = fread(&len,4,1,fp);
	len = ((len&0xff)<<24)|((len&0xff00)<<8)|((len&0xff0000)>>8)|((len&0xff000000)>>24);
	fread(sha0,1,64,fp);

#if 1
	if (sha_verify(fp, sha0, g_loaderInfo.img_len-64) != 0)
	{
		DEBUG("sha verify err\n");
		
		loaderAction = 0;
		Filter_param param;
		memset(&param,0,sizeof(param));
		param.filter[0] = 0xf0;
		param.mask[0] = 0xff;

		loader_dsc_fid=TC_alloc_filter(0x1ff0, &param, loader_des_section_handle, NULL, 1);
		fclose(fp);
		
		goto reLoader;
		//return NULL;
	}
#endif
	fclose(fp);
#if 1
	//1 checking img, if not correct,return
	FILE *cfp = fopen(COMMAND_FILE,"w");

	if (!cfp)
		return NULL;
	//2 set upgrade mark
	DEBUG("g_loaderInfo.download_type: %d\n", g_loaderInfo.download_type);
	if (g_loaderInfo.download_type)
	{
		//must upgrade,display upgrade info, wait 5 second, set uboot mark and then reboot
		char msg[128];

		if (g_loaderInfo.file_type)
		{
			fprintf(cfp,"--update_package=%s\n",UPGRADEFILE_ALL);
			//              fprintf(cfp,"--wipe_data\n");
			//              fprintf(cfp,"--wipe_cache\n");
			fprintf(cfp,"--orifile=%s\n",UPGRADEFILE_IMG);
			snprintf(msg, sizeof(msg),"%.2x%.2x%.2x%.2x",software_version[0],software_version[1],software_version[2],software_version[3]);
			fprintf(cfp,"%s\n",msg);
			snprintf(msg, sizeof(msg),"%s",UPGRADEFILE_IMG);
		}
		else
		{
			snprintf(msg, sizeof(msg),"%s",UPGRADEFILE_IMG);
		}
		msg_send2_UI(UPGRADE_NEW_VER_FORCE, msg, strlen(msg));
	}
	else
	{
		//display info and ask to upgrade right now?
		char msg[128];

		if (g_loaderInfo.file_type)
		{
			fprintf(cfp,"--update_package=%s\n",UPGRADEFILE_ALL);
			//                fprintf(cfp,"--wipe_data\n");
			//                fprintf(cfp,"--wipe_cache\n");
			fprintf(cfp,"--orifile=%s\n",UPGRADEFILE_IMG);
			snprintf(msg, sizeof(msg),"%.2x%.2x%.2x%.2x",software_version[0],software_version[1],software_version[2],software_version[3]);
			fprintf(cfp,"%s\n",msg);

			snprintf(msg, sizeof(msg),"%s",UPGRADEFILE_IMG);
		}
		else
		{
			snprintf(msg, sizeof(msg),"%s",UPGRADEFILE_IMG);
		}
		msg_send2_UI(UPGRADE_NEW_VER, msg, strlen(msg));
	}
	fclose(cfp);
	
	upgrade_sign_set();
	
	//TC_loader_filter_handle(0);
#endif
	return NULL;
}


extern int send_mpe_sec_to_push_fifo(uint8_t *pkt, int pkt_len);
static void mpe_section_handle(int dev_no, int fid, const unsigned char *data, int len, void *user_data)
{
    send_mpe_sec_to_push_fifo((uint8_t *)data, len);
}

static void loader_section_handle(int dev_no, int fid, const unsigned char *data, int len, void *user_data)
{
	static unsigned char startWrite = 0, getMaxSeq = 0, total_subone = 0, total_cycle = 0;
	static unsigned char *recv_buf=NULL, *recv_mark=NULL;
	unsigned char *datap = NULL;
	static int total_loader=0, lastSeq=-1, totalLen=0;
	static int maxSeq = -1;
	int seq=0;
	FILE *upgradefile=NULL;
	
	static int s_first_package_flag = -1;
	int tmp_i = 0;
	
	//DEBUG("call loader_section_handle\n");
	if ((len < 12)||(len > 4096))
	{
		DEBUG("loader data too small!!!!!!!!!!\n");
		return;
	}
	len -= 12;
	
	datap = (unsigned char *)data+4;
	if (getMaxSeq==0)
	{
        if (datap[2]==datap[3])  //last section num = current section num
		{
			if(datap[0]==datap[1])//last part num = current part num seq >= 3*0x100+0xad)
			{
				maxSeq = datap[1]*0x100+datap[3]+1;
				getMaxSeq = 1;
				DEBUG("datap[1]=%d, datap[3]=%d, maxSeq=%d\n", datap[1], datap[3], maxSeq);
			}
		}
	}
	
	seq = datap[0]*0x100 + datap[2];
	
	if(s_first_package_flag>0 && (s_first_package_flag==seq)){
		DEBUG("has recv %d/%d for one loop, lost such packages:\n", total_loader, maxSeq);
		int count_need = 0;
		for(tmp_i=0;tmp_i<maxSeq;tmp_i++){
			if(0==recv_mark[tmp_i]){
				count_need ++;
				PRINTF("lost %d: %d\n", count_need, tmp_i);
			}
		}
		s_first_package_flag = -2;
	}
	
	if (startWrite)
	{
		if(seq < maxSeq)
		{
			if(recv_mark[seq]==0)
			{
    	    	recv_mark[seq]=1;
				total_loader++;
				
				if(-2==s_first_package_flag){
					PRINTF("supply seq: %d, len=%d, total_loader=%d\n", seq, len, total_loader);
				}
//				if(LOADER_PACKAGE_SIZE!=len){
//					DEBUG("monitor this package: seq=%u, len=%u, maxSeq=%u, total_loader=%d, totalLen=%d\n", seq, len, maxSeq, total_loader, totalLen);
//				}
				
				if(((lastSeq+1) != seq && lastSeq!=0) || g_loaderInfo.fid>32 || g_loaderInfo.fid<0)
					PRINTF("total_loader=%d/%d,lastSeq=%d,seq=[%u],fid=%d\n", total_loader,maxSeq,lastSeq,seq,g_loaderInfo.fid);
				
				totalLen += len;
patch0:
				memcpy(recv_buf+seq*LOADER_PACKAGE_SIZE,datap+4,len);
				if (seq == 1)
				{
					if (lastSeq == 0)
					{        
                                                if (recv_mark[0] == 1)
                                                {
                                                    if (total_subone == 1)
                                                    {
                                                        total_loader++;
                                                        total_subone = 0;
                                                    }
						    recv_mark[0] = 2;
                                                }
					}
				}
                                else if (seq == 0)
                                {
                                        if ((lastSeq+1) == maxSeq)
                                        {
                                                recv_mark[0] = 2;
                                        }
                                }
				lastSeq = seq;
				
				//DEBUG("total_loader: %d, maxSeq: %d\n", total_loader, maxSeq);
				if (total_loader >= maxSeq)
				{
					if (recv_mark[0] < 2)
					{
						total_loader -= 1;
                                                total_subone = 1;
						DEBUG("---subone and total_loader=%u, maxseq=%u\n", total_loader,maxSeq);
						return;
					}
					
					PRINTF("total_loader: %u, maxSeq: %u, g_loaderInfo.fid: %d\n", total_loader, maxSeq,g_loaderInfo.fid);
					TC_free_filter(g_loaderInfo.fid);
					TC_loader_to_push_order(1);                                        
					upgradefile = fopen(UPGRADEFILE_IMG,"w");
					if (!upgradefile)
					{
						PRINTF("open %s failed\n", UPGRADEFILE_IMG);
						startWrite = 0;
						getMaxSeq = 0;
                                                total_subone = 0;
                                                lastSeq = -1;
                                                maxSeq = -1;
                                                total_cycle = 0;
						if (recv_buf!= TC_loader_get_push_buf_pointer())
							free(recv_buf);
						free(recv_mark);
						return;
					}
					else
						PRINTF("open %s OK\n", UPGRADEFILE_IMG);
					
					fwrite(recv_buf,1,totalLen,upgradefile);
					fclose(upgradefile);
					if (recv_buf!= TC_loader_get_push_buf_pointer())
						free(recv_buf);
					free(recv_mark);
					startWrite = 0;
					getMaxSeq = 0;
                                        total_subone = 0;
                                        lastSeq = -1;
                                        maxSeq = -1;
                                        total_cycle = 0;
					if (loaderthread == 0)
					{
						pthread_create(&loaderthread, NULL, loader_thread, NULL);
						loaderAction = 1;
					}
					return;
				}
			}
			else
			{
				if (seq == 0)
				{
					if(recv_mark[0] == 1)
					{
						if ((lastSeq+1) == maxSeq)
						{
							recv_mark[0] = 2;
                                                        if (total_subone)
                                                        {
                                                            total_subone = 0;
                                                            total_loader++;
                                                        }
DEBUG("monitor this package: seq=%u, len=%u, maxSeq=%u, total_loader=%d, totalLen=%d\n", seq, len, maxSeq, total_loader, totalLen);
						}
						goto patch0;
					}
DEBUG("monit0 this package:seq=%u, len=%u, maxSeq=%u, total_loader=%d, totalLen=%d, recvmark0=%d\n", seq, len, maxSeq, total_loader, totalLen,recv_mark[0]);
				}
				else
				{
					lastSeq = seq;
				}
                                if (seq == 1)
                                {
                                        total_cycle++;
                                        if(total_cycle > 5)
                                        {
                                                startWrite = 0;
                                                getMaxSeq = 0;
                                                total_subone = 0;
                                                lastSeq = -1;
                                                maxSeq = -1;
                                                if (recv_buf!= TC_loader_get_push_buf_pointer())
                                                        free(recv_buf);
                                                free(recv_mark);
                                                total_cycle = 0;
                                                TC_free_filter(g_loaderInfo.fid);
                                                TC_loader_to_push_order(1);  //send leaving idle command 
                                                {
                                                Filter_param param;
                                                memset(&param,0,sizeof(param));
                                                param.filter[0] = 0xf0;
                                                param.mask[0] = 0xff;

                                                loader_dsc_fid=TC_alloc_filter(0x1ff0, &param, loader_des_section_handle, NULL, 1);
                                                }
                                        } 
                                }
			}
		}
		else
			PRINTF("seq=[%u] part_num[%x] last_part_num[%x],sec_num[%x] last_sec_num[%x]\n",seq,datap[0],datap[1],datap[2],datap[3]);
	}
	else
	{
		if ((datap[0]==datap[1])&&(datap[2]==datap[3]))  //donot delete, for this section if not the full section,section size is not correct
			return;
		if (maxSeq == -1)
			maxSeq = (datap[1]+1)*0x100+1;
		
		while(TC_loader_get_push_state()==0)
		{
			usleep(100000);
		}
		
		if (TC_loader_get_push_buf_size() >= maxSeq*LOADER_PACKAGE_SIZE)
		{
			recv_buf = TC_loader_get_push_buf_pointer();
			DEBUG("use push buf for upgrade: %p\n", recv_buf);
		}
		else 
		{
			DEBUG("push buffer is too small for upgrade file!!!!\n");
			recv_buf = (unsigned char *)malloc(maxSeq*LOADER_PACKAGE_SIZE);
			DEBUG("malloc %d x %d = %d for upgrade file\n", maxSeq,LOADER_PACKAGE_SIZE,maxSeq*LOADER_PACKAGE_SIZE);
		}
		
		if (recv_buf == NULL)
		{
			DEBUG("can not allcate mem for upgrade file!!!!\n");
			return;
		}
		
		recv_mark = (unsigned char *)malloc(maxSeq);
		if (recv_mark == NULL)
		{
			DEBUG("can not allcocate mem for recv_mark!!!\n");
			return;
		}
		memset(recv_mark,0,maxSeq);
		
		if (seq < maxSeq)
		{
			recv_mark[seq] = 1;
			lastSeq = seq;
			memcpy(recv_buf+seq*len,datap+4,len);
			startWrite = 1;
			totalLen = len;
			total_loader = 1;
			s_first_package_flag = seq;
		}
		DEBUG("maxSeq=%d, lastSeq=%d, seq=%d, startWrite=%d, totalLen=%d, total_loader=%d\n", maxSeq, lastSeq, seq, startWrite, totalLen, total_loader);
	}
}

static char s_time_sync_2_ui[128];
void tdt_section_handle(int dev_no, int fid, const unsigned char *data, int len, void *user_data)
{
    int year,month,day;
    int mjd;
    char tdt[64];

    if (len < 8)
    {
        return;
    }
    
    if ((data[0]!= 0x70)&&(data[0]!= 0x73))
        return;
    mjd = (int)((data[3]<<8)|data[4]);
    year = (int)((mjd -15078.2)/365.25);
    month = (int)((mjd - 14956.1 - (int)(year*365.25))/30.6001);
    day   = mjd - 14956 - (int)(year*365.25) - (int)(month*30.6001);
    if ((month == 14)||(month == 15))
    {
        year += 1;
        month -= 13; 
    }
    else
    {
        month -= 1;
    }
    
    snprintf(tdt,sizeof(tdt),"%4d-%2d-%2d %2x:%2x:%2x",year+1900,month,day,data[5],data[6],data[7]);
    TC_free_filter(tdt_dsc_fid);
    DEBUG("catch tdt time(%s) and free tdt_dsc_fid(%d)\n",tdt,tdt_dsc_fid);
    
    struct tm tm_tdt;
    memset(&tm_tdt,0,sizeof(tm_tdt));
	sscanf(tdt,"%d-%d-%d %d:%d:%d",&(tm_tdt.tm_year),&(tm_tdt.tm_mon),&(tm_tdt.tm_mday),&(tm_tdt.tm_hour),&(tm_tdt.tm_min),&(tm_tdt.tm_sec));
    
	tm_tdt.tm_year-=1900;	/*���ֵ��ȥ1900���õ�tm�ṹ�б�����������*/
	tm_tdt.tm_mon-=1;		/*�·�ֵ��ȥ1���õ�tm�ṹ�б�����·�����*/
	tm_tdt.tm_isdst = 0;
	
	time_t local_time_s = mktime(&tm_tdt);
	
// ��ǰ��tdt��1.3��2.0���ã����õ��Ǳ�׼ʱ�䣬�ʱ������8СʱתΪ����ʱ�䡣

#if 0  //TOT table parse
    if (data[0]==0x73)
    {
        if(data[10] == 0x58)
        {
            unsigned char *dsc_data = data+12;
            unsigned char add_sub = 0;
            time_t dif_time=0;
            //int len = 0, i=0;

            if (data[11] >= 13)
            {
                //do
                {
                    add_sub = dsc_data[3]&0x1;
                    dif_time = ((dsc_data[4]>>4)*10 + dsc_data[4]&0xf)*3600 + ((dsc_data[5]>>4)*10+dsc_data[5]&0xf)*60;
                  //  len += 13;
                }  // while (len < data[11]);
            }
        }
    }

#endif
	local_time_s += (8*60*60);
	DEBUG("rectify time for 8 hours\n");
	
    snprintf(s_time_sync_2_ui,sizeof(s_time_sync_2_ui),"%ld",local_time_s);
	
#if 0
// only for test
	char sqlite_cmd[512];
	snprintf(sqlite_cmd,sizeof(sqlite_cmd),"replace into RejectRecv(ID,URI) values('%s','%s');",s_time_sync_2_ui,tdt);
	sqlite_execute(sqlite_cmd);
#endif
	
	msg_send2_UI(TDT_TIME_SYNC, s_time_sync_2_ui, strlen(s_time_sync_2_ui));
    
    //DEBUG("distill %4d-%2d-%2d %2d:%2d:%2d, send to UI %s\n",(tm_tdt.tm_year),(tm_tdt.tm_mon),(tm_tdt.tm_mday),(tm_tdt.tm_hour),(tm_tdt.tm_min),(tm_tdt.tm_sec),s_time_sync_2_ui);
	tdt_dsc_fid = -1;
	
	// following code only for test, check the seconds if correctly.
	//time_t timep = strtol(s_time_sync_2_ui,NULL,0);
	//struct tm *check_tdt = localtime(&timep);
	//DEBUG("check tdt time which is send to UI, seconds(str type): %s, seconds(long type): %ld, localtime: %04d-%02d-%02d %02d:%02d:%02d\n", s_time_sync_2_ui,timep,
	//	check_tdt->tm_year+1900,check_tdt->tm_mon+1,check_tdt->tm_mday,check_tdt->tm_hour,check_tdt->tm_min,check_tdt->tm_sec);
}

void ca_section_handle(int dev_no, int fid, const unsigned char *data, int len, void *user_data)
{
	static unsigned short emmpid=0xffff;
	unsigned short pid = 0;
	static unsigned char version=0xff;
	unsigned char tmp;
	
	if (len < 18)
	{
		DEBUG("ca section too small!!!! %d\n",len);
		return;
	}
	
	tmp = data[5]&0x3e;
	if (version != tmp)
	{
		version = tmp;
		pid = ((data[12]&0x1f)<<8)|data[13];
		if (pid != emmpid)
		{
			DEBUG("set emm pid =[%x] start...\n",pid);
			if(0==drm_init()){
				CDCASTB_SetEmmPid(pid);
				DEBUG("set emm pid =[%x] finished\n",pid);
			}
			else
				DEBUG("drm init failed\n");
		}
	}
}

void loader_des_section_handle(int dev_no, int fid, const unsigned char *data, int len, void *user_data)
{
	unsigned char *datap=NULL;
//	unsigned char mark = 0;
	char tmp[10];
	unsigned short tmp16=0;
	unsigned int stb_id_l=0,stb_id_h=0;
	
	if(s_print_cnt>2048)
		s_print_cnt = 0;
	else
		s_print_cnt ++;
	
	if (len < 55)
	{
		INTERMITTENT_PRINT("loader info too small!!!!!!!!!![%d]\n",len);
		//        return;
	}

	datap = (unsigned char *)data+4;
	//if ((datap[0] != datap[1])||(datap[2] != datap[3]))
	//    DEBUG("!!!!!!!!!!!!!!!!error section number,need modify code!\n");
	
	datap += 4;
#if 0	
	memset(&g_loaderInfo, 0, sizeof(g_loaderInfo));
	get_loader_message(&mark,&g_loaderInfo);
#endif
	
	//oui
	tmp16 = *datap;
	datap++;
	tmp16 = (tmp16<<8)|(*datap);
//	INTERMITTENT_PRINT("loader info oui = [%x]\n",tmp16);
	if (tmp16 != g_loaderInfo.oui){
		INTERMITTENT_PRINT("loader oui check failed [0x%x], compare with my oui [0x%x]\n",tmp16,g_loaderInfo.oui);
		return;
	}
	
	//model_type
	datap++;
	tmp16 = *datap;
	datap++;
	tmp16 = (tmp16<<8)|(*datap);
	INTERMITTENT_PRINT("loader info model type = [%x]\n",tmp16);
	if (tmp16 != g_loaderInfo.model_type){
		INTERMITTENT_PRINT("model type check failed [%x]\n",tmp16);
		return;
	}
	
	datap ++;  //usergroup id
	
	//hardware_version
	datap += 2;
	//tmp32 = ((datap[0]<<24)|(datap[1]<<16)|(datap[2]<<8)|(datap[3]));
	INTERMITTENT_PRINT("loader harder version [%u][%u][%u][%u]\n",datap[0],datap[1],datap[2],datap[3]);
	if ((datap[0] != g_loaderInfo.hardware_version[0])||(datap[1] != g_loaderInfo.hardware_version[1])
	||(datap[2] != g_loaderInfo.hardware_version[2])||(datap[3] != g_loaderInfo.hardware_version[3]))
	{
		INTERMITTENT_PRINT("hardware version check failed!!!!!\n");
		return;
	}
	//software_version
	datap += 4;
	//tmp32 = ((datap[0]<<24)|(datap[1]<<16)|(datap[2]<<8)|(datap[3]));
	//DEBUG("loader info software version = [%x][%x]\n",tmp32,g_loaderInfo.software_version);
	INTERMITTENT_PRINT("new software ver: [%u][%u][%u][%u]\n",datap[0],datap[1],datap[2],datap[3]);
	INTERMITTENT_PRINT("cur software ver: [%u][%u][%u][%u]\n",g_loaderInfo.software_version[0],g_loaderInfo.software_version[1],g_loaderInfo.software_version[2],g_loaderInfo.software_version[3]);
	if(255==datap[0] && 255==datap[1] && 255==datap[2] && 255==datap[3]){
		INTERMITTENT_PRINT("software version is 255.255.255.255, do upgrade directly\n");
	}
	else{
		if ((datap[0] == g_loaderInfo.software_version[0])&&(datap[1] == g_loaderInfo.software_version[1])
		&&(datap[2] == g_loaderInfo.software_version[2])&&(datap[3] == g_loaderInfo.software_version[3]))
		{
//			if(-1==software_check()){
//				INTERMITTENT_PRINT("software version is equal, but ignore it and continue to do upgrade\n");
//			}
//			else{
				INTERMITTENT_PRINT("software version is equal, do not upgrade\n");
				return;
//			}
		}
		else if(	(datap[0] < g_loaderInfo.software_version[0])
					||(((datap[0] == g_loaderInfo.software_version[0]))&&(datap[1] < g_loaderInfo.software_version[1]))
					||(((datap[0] == g_loaderInfo.software_version[0]))&&(datap[1] == g_loaderInfo.software_version[1])&&(datap[2] < g_loaderInfo.software_version[2]))
					||(((datap[0] == g_loaderInfo.software_version[0]))&&(datap[1] == g_loaderInfo.software_version[1])&&(datap[2] == g_loaderInfo.software_version[2])&&(datap[3] < g_loaderInfo.software_version[3]))
		)
		{
			INTERMITTENT_PRINT("software version of new upgrade package is less than mini, do not upgrade\n");
			return;
		}
		else
	    {
			INTERMITTENT_PRINT("software version is not equal, do upgrade\n");
		}
	}
	
	software_version[0] = datap[0];
	software_version[1] = datap[1];
	software_version[2] = datap[2];
	software_version[3] = datap[3];
	//DEBUG("get software version..\n");
	//stb_id
	datap += 4;
	snprintf(tmp,sizeof(tmp),"%.2x%.2x%.2x%.2x",datap[0],datap[1],datap[2],datap[3]);
	stb_id_h = atol(tmp);
	INTERMITTENT_PRINT("start stb id h = [%u] me h[%u]\n",stb_id_h,g_loaderInfo.stb_id_h);
	if (g_loaderInfo.stb_id_h < stb_id_h)
	{
		datap += 4;
		INTERMITTENT_PRINT("stb id is not in this update sequence \n");
		return;
	}
	else if (g_loaderInfo.stb_id_h == stb_id_h)
	{
		datap += 4;
		snprintf(tmp,sizeof(tmp),"%.2x%.2x%.2x%.2x",datap[0],datap[1],datap[2],datap[3]);
		stb_id_l = atol(tmp);
		INTERMITTENT_PRINT("start id l=[%u], l=[%u]\n",stb_id_h, stb_id_l);
		if (g_loaderInfo.stb_id_l < stb_id_l)
		{
			INTERMITTENT_PRINT("stb id is not in this update sequence \n");
			return;
		}
	}
	else
		datap += 4;
	datap += 4;
	snprintf(tmp,sizeof(tmp),"%.2x%.2x%.2x%.2x",datap[0],datap[1],datap[2],datap[3]);
	stb_id_h = atol(tmp);
	INTERMITTENT_PRINT("end stb id h [%u] me [%u]\n",stb_id_h,g_loaderInfo.stb_id_h);
	if (g_loaderInfo.stb_id_h > stb_id_h)
	{
		datap += 4;
		INTERMITTENT_PRINT("stb id is not in this update sequence \n");
		return;
	}
	else if (g_loaderInfo.stb_id_h == stb_id_h)
	{
		datap += 4;
		snprintf(tmp,sizeof(tmp),"%.2x%.2x%.2x%.2x",datap[0],datap[1],datap[2],datap[3]);
		stb_id_l = atol(tmp);
		INTERMITTENT_PRINT("end start id h=[%u], l=[%u]\n",stb_id_h, stb_id_l);
		if (g_loaderInfo.stb_id_l > stb_id_l)
		{
			INTERMITTENT_PRINT("stb id is not in this update sequence \n");
			return;
		}
	}
	else
		datap += 4;
	
	INTERMITTENT_PRINT("loader_dsc_fid: %d=%x\n", loader_dsc_fid,loader_dsc_fid);
	s_print_cnt = 0;
	TC_free_filter(loader_dsc_fid);
	datap += 4;
	{
		//unsigned short pid;
		//unsigned char tid;
		Filter_param param;
		
		tc_pid = *datap;
		datap++;
		tc_pid = ((tc_pid<<8)|(*datap));//&0x1fff;
		datap++;
		tc_tid = *datap++;
		DEBUG(">>>> pid = [%x]  tid=[%x] loader_section_handle=%p\n",tc_pid,tc_tid,loader_section_handle);
		//TC_loader_filter_handle(1);
		TC_loader_to_push_order(0);  //let push idle;
		memset(&param,0,sizeof(param));
		param.filter[0] = tc_tid;
		param.mask[0] = 0xff;
		g_loaderInfo.fid = TC_alloc_filter(tc_pid, &param, loader_section_handle, NULL, 1);
		//DEBUG("pid: %d|0x%x, fid: %d\n", tc_pid,tc_pid, g_loaderInfo.fid);
	}
	
	g_loaderInfo.file_type = *datap++;
	g_loaderInfo.img_len = ((datap[0]<<24)|(datap[1]<<16)|(datap[2]<<8)|(datap[3]));
	g_loaderInfo.download_type = datap[4];
	DEBUG("g_loaderInfo.file_type=%d, g_loaderInfo.img_len=%d, g_loaderInfo.fid: %d\n", g_loaderInfo.file_type, g_loaderInfo.img_len, g_loaderInfo.fid);
	//DEBUG(">>>>>> filetype =[%d], img_len[%d], downloadtype=[%d]\n",g_loaderInfo.file_type,g_loaderInfo.img_len,g_loaderInfo.download_type);
}

extern void dmx_filter_init(void);
void chanFilterInit(void)
{
    int i;
    
    for (i=0;i<	MAX_CHAN_FILTER;i++)
        chanFilter[i]=0xffff;
    dmx_filter_init();
}