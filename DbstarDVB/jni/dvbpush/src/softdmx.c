
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>
#include <sys/types.h>
#include "common.h"
#include "softdmx.h"
#include "prodrm20.h"
#include "dvbpush_api.h"
#include "bootloader.h"

Channel_t chanFilter[MAX_CHAN_FILTER+1];
int max_filter_num = 0;
int loader_dsc_fid;
LoaderInfo_t loaderInfo;
pthread_t loaderthread;
int tt=0;
#define upgradefile_all "/tmp/upgrade.zip"
#define upgradefile_img "/data/upgrade.zip"
#define COMMAND_FILE  "/cache/recovery/command"

static void* loader_thread(void *arg)
{
    unsigned char buf[1024];
    unsigned char sha0[64];
    //LoaderInfo_t *loader = (LoaderInfo_t *)arg;
    FILE *fp = fopen(upgradefile_img,"r");
  //  FILE *rfp = fopen(upgradefile_img,"w+");
    int ret;
    unsigned int len,wlen,rlen;   

/*fp=fopen("localfile","rb");// localfile�ļ���
fseek(fp,0,SEEK_SET);
fseek(fp,0,SEEK_END);
long longBytes=ftell(fp);// longBytes�����ļ��ĳ���
*/
//DEBUG("mtd_scan_partitions = [%d]\n",mtd_scan_partitions());
DEBUG("in loader thread...\n");
    wlen = 0;
    ret = fread(buf,1,48,fp);
    ret = fread(&len,1,1,fp);
DEBUG("in loader thread, read file len = [%u]\n",len);
    if (len > 1024) rlen = 1024;
    else rlen = len;
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
        else if (rlen < 0) 
            break;
    } while(wlen < len);
    if (wlen != len)
    {
DEBUG("received upgrade file is err, re download the file!!!!\n");
    } 
    
    wlen = 0;
    ret = fread(&len,4,1,fp);
    len = ((len&0xff)<<24)|((len&0xff00)<<8)|((len&0xff0000)>>8)|((len&0xff000000)>>24);
    fread(sha0,1,64,fp);
#if 0
    if (len > 1024) rlen = 1024;
    else rlen = len;
DEBUG("in loader thread, read file len = [%x]\n",len);
    do
    {
        ret = fread(buf,1,rlen,fp);
        if (ret > 0)
        {
            fwrite(buf,1,ret,rfp);
            wlen += ret;
        }
        else
        {
            break;
        }
        rlen = len - wlen;
        if (rlen > 1024)
            rlen = 1024;
        else if (rlen < 0) 
            break;
    } while(wlen < len);

    fclose(fp);
    fclose(rfp);
#endif
#if 1
    if (sha_verify(fp, sha0, loaderInfo.img_len) != 0)
    {
DEBUG("verify err \n");
while(1);
        Filter_param param;
        memset(&param,0,sizeof(param));
        param.filter[0] = 0xf0;
        param.mask[0] = 0xff;

        loader_dsc_fid=TC_alloc_filter(0x1ff0, &param, loader_des_section_handle, NULL, 1);
        fclose(fp);
        return NULL;
    }
#endif
    fclose(fp);
#if 1
	//1 checking img, if not correct,return
	FILE *cfp = fopen(COMMAND_FILE,"w");

        if (!cfp)
            return NULL;
	//2 set upgrade mark
	if (loaderInfo.download_type)
	{
	    //must upgrade,display upgrade info, wait 5 second, set uboot mark and then reboot 
            char msg[128];
        
            if (loaderInfo.file_type)
            {
                fprintf(cfp,"--update_package=%s\n",upgradefile_all);
  //              fprintf(cfp,"--wipe_data\n");
  //              fprintf(cfp,"--wipe_cache\n");
                fprintf(cfp,"--orifile=%s\n",upgradefile_img);
                snprintf(msg, sizeof(msg),"%.2x%.2x%.2x%.2x",loaderInfo.software_version[0],
                    loaderInfo.software_version[1],loaderInfo.software_version[2],loaderInfo.software_version[3]);
                fprintf(cfp,"%s\n",msg);
                snprintf(msg, sizeof(msg),"?????5??????? = %s",upgradefile_all);
            }
            else
            {
                snprintf(msg, sizeof(msg),"LOADER SOFTWARE = %s",upgradefile_all);
            }
            msg_send2_UI(UPGRADE_NEW_VER_FORCE, msg, strlen(msg));
	}
	else
	{
            //display info and ask to upgrade right now?
	    char msg[128];
	
            if (loaderInfo.file_type)
            {
                fprintf(cfp,"--update_package=%s\n",upgradefile_all);
//                fprintf(cfp,"--wipe_data\n");
//                fprintf(cfp,"--wipe_cache\n");
                fprintf(cfp,"--orifile=%s\n",upgradefile_img);
                snprintf(msg, sizeof(msg),"%.2x%.2x%.2x%.2x",loaderInfo.software_version[0],
                    loaderInfo.software_version[1],loaderInfo.software_version[2],loaderInfo.software_version[3]);
                fprintf(cfp,"%s\n",msg);

                snprintf(msg, sizeof(msg),"???????????? = %s",upgradefile_all);
            }
            else
            {
                snprintf(msg, sizeof(msg),"LOADER SOFTWARE = %s",upgradefile_img);
            }
	    msg_send2_UI(UPGRADE_NEW_VER, msg, strlen(msg));
        }
        fclose(cfp);
#endif
	return NULL;
}

int TC_alloc_filter(unsigned short pid, Filter_param* param, dataCb hdle, void* userdata, char priority)
{
	int i,j,start_filter;
	unsigned char m;
	
	if (priority)
		start_filter = 0;
	else 
		start_filter = HIGH_PRIORITY_FILTER_NUM;
	
	for(i = start_filter; i < MAX_CHAN_FILTER; i++)
	{
		if (chanFilter[i].used == 0)
		{            
			chanFilter[i].neq = 0;    
			for(j=0; j<DMX_FILTER_SIZE; j++)
			{
				int pos = j?(j+2):j;
				unsigned char mask = param->mask[j];
				unsigned char mode = param->mode[j];
				
				chanFilter[i].value[pos] = param->filter[j];
				
				mode = ~mode;
				chanFilter[i].maskandmode[pos] = mask&mode;
				chanFilter[i].maskandnotmode[pos] = mask&~mode;
				
				if(chanFilter[i].maskandnotmode[pos])
					chanFilter[i].neq = 1;
			}
			
			chanFilter[i].maskandmode[1] = 0;
			chanFilter[i].maskandmode[2] = 0;
			chanFilter[i].maskandnotmode[1] = 0;
			chanFilter[i].maskandnotmode[2] = 0;
			
			if (i >= max_filter_num) 
				max_filter_num = i+1;
			
			m = 0;
			for(j = 0; j < max_filter_num; j++)
			{
				if (chanFilter[j].used)
				{
					if(chanFilter[j].pid == pid)
					m++;
				}
			}
			
			if (m)
			{
				for(j = 0; j < max_filter_num; j++)
				{
					if ((chanFilter[j].used)&&(chanFilter[j].pid == pid))
					{
						chanFilter[j].samepidnum = m;
					}
				}
			}
			chanFilter[i].fid = i;
			chanFilter[i].hdle = hdle;
			chanFilter[i].userdata = userdata;
			chanFilter[i].samepidnum = m;
			chanFilter[i].used = 1;
			chanFilter[i].pid = pid;
			//DEBUG("****************************allcoate a filter id[%d],num[%d],pid[0x%x]\n",i,m,pid);
			return i;
		}
	}
	return -1;
}

static void dump_bytes(int fid, const unsigned char *data, int len, void *user_data)
{
}

static void loader_section_handle(int fid, const unsigned char *data, int len, void *user_data)
{
    unsigned char *datap;
    static int part, section, partw, filelen;
    static unsigned int start = 2, inited = 1, next = 0;
    unsigned int seq;
    static FILE *upgradefile=NULL;
    unsigned char *partbuf;//[1024*1024];
     

//DEBUG("Got upgrade img section len [%d][%x][%x][%x]\n",len,data[0],data[1],data[2]);
    if (len < 12)
    {
        DEBUG("loader data too small!!!!!!!!!!\n");
        return;
    }

    /*if (inited)
    {
        inited = 0;
        part = 0;
        section = 0;
        start = 1;
        partw = 0;
        filelen = 0;
        upgradefile = fopen("uptest","w+");
        partbuf = (unsigned char *)malloc(1024*1024);
    }*/
//DEBUG("[%x][%x][%x][%x][%x][%x][%x][%x]\n",data[0],data[1],data[2],data[3],data[4],data[5],data[6],data[7]);
    datap = (unsigned char *)data+4;
//DEBUG("start = [%x]\n",*datap++);

    seq = datap[0]*0x100 + datap[2];
//DEBUG("seq = [%u] part_num[%x] last_part_num[%x],sec_num[%x] last_sec_num[%x]\n",seq,datap[0],datap[1],datap[2],datap[3]);
    if (seq == next)
    {
//DEBUG("seq = next = [%u]\n",seq);
        if (seq==0)
        {
            if (!upgradefile)
            {
                upgradefile = fopen(upgradefile_img,"w+");
                DEBUG(">> %s\n", strerror(errno));
            }
            else
            {
                fseek(upgradefile,0,SEEK_SET);
            }
            filelen = 0;
        }
        if(upgradefile)
        	;//DEBUG("seq = [%u]\n",seq);//fwrite(datap+4,1,len-12,upgradefile);
        else
        	DEBUG("upgradefile is NULL, %s\n", upgradefile_img);
        filelen += len-12;
        next++;
        if (datap[2]==datap[3])  //last section num = current section num
        {
            if(datap[0]==datap[1])//last part num = current part num seq >= 3*0x100+0xad)
            {
                DEBUG("close the upgrade file\n");
                TC_free_filter(loaderInfo.fid);
                fclose(upgradefile);
                next = 0;
                filelen = 0;
DEBUG("creat the loader thread...\n");
                pthread_create(&loaderthread, NULL, loader_thread, &loaderInfo);
            }
            //fseek(upgradefile,0,SEEK_SET);
            // if ( filelen >= loaderInfo.img_len)
        }
        
    }
    else if (seq > next)
    {
        if (next == 1)  //filter the 0 packets alise as a start
            next = 0;
//DEBUG("lost packet num = [%u], next = [%u]\n",seq-next,next);
    }
/*    else if (seq >= 1000)
    {
DEBUG("close the upgrade file\n");
         TC_free_filter(loaderInfo.fid);
         fclose(upgradefile);
         // pthread_create(&loaderthread, NULL, loader_thread, &loaderInfo);
         filelen = 0;
         //fseek(upgradefile,0,SEEK_SET);
         // if ( filelen >= loaderInfo.img_len)
         next = 0;

    }*/ 
#if 0
    if (start == 0)
    {
//DEBUG("start==0 find the header\n");
        if (part == datap[0])
        {
newpart:
            memcpy(partbuf+partw,datap+4,len-12);
            partw += len-12;
            
            if (datap[2]==datap[3])
            {
                fwrite(partbuf,1,partw,upgradefile);
                filelen += partw;
                if (datap[0]==datap[1])
                {
                    if ( filelen >= loaderInfo.img_len)
                    {
                        start = 2;
                        inited = 1;
                        TC_free_filter(loaderInfo.fid);
                        free(partbuf);
                        fclose(upgradefile);
                       // pthread_create(&loaderthread, NULL, loader_thread, &loaderInfo);
                    }
                    else
                    {
                        start = 1;
                        part = 0;
                        section = 0;
                        partw = 0;
                        filelen = 0;
                        fseek(upgradefile,0,SEEK_SET);
                    }
                }
            }
        }
        else
        {
            part = datap[0];
            partw = 0;
            goto newpart;
        }
    }
    else if(start==1)
    {
//DEBUG("start==1,not the header\n");
DEBUG("datap0[%x][%x]=2[%x][%x],start=0\n",datap[0],data[1],datap[2],datap[3]);
        if (datap[0]||datap[2])
            return;
      /*  else
        {
DEBUG("datap0[%x]=2[%x],start=0\n",datap[0],datap[2]);
            start = 0;
            goto newpart;
        }*/
    }
#endif
}

static void read_loaderinfo(LoaderInfo_t * loader)
{
    loader->stb_id_h = 21234567;//
    loader->stb_id_l = 89012345;//2000317120000066;  //2xx0317120xxxxxx
    loader->oui = 0x3;
    loader->hardware_version[0] = 123;
    loader->hardware_version[1] = 24;
    loader->hardware_version[2] = 201;
    loader->hardware_version[3] = 69;

    loader->software_version[0] = 0x00;
    loader->software_version[1] = 0x01;
    loader->software_version[2] = 0x01;
    loader->software_version[3] = 0x01;
    loader->model_type = 0x1;//17;  //17 BCD
    loader->user_group_id = 1;
}

void ca_section_handle(int fid, const unsigned char *data, int len, void *user_data)
{
    static unsigned short emmpid=0xffff;
    unsigned short pid;
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
			DEBUG("---------set emm pid =[%x]\n",pid);
			if(0==drm_init())
            	CDCASTB_SetEmmPid(pid);
            else
            	DEBUG("drm init failed\n");
        }
    } 
}



void loader_des_section_handle(int fid, const unsigned char *data, int len, void *user_data)
{
    unsigned char *datap,mark;
    char tmp[10];
    static int loader_init = 0;
    static LoaderInfo_t loaderInfo;
    unsigned short tmp16;
    unsigned int tmp32;
    unsigned int stb_id_l,stb_id_h;

//DEBUG("Got loader des section len [%d]\n",len);
/*{
 int i;

                for(i=0;i<len;i++)
                {
                        DEBUG("%02x ", data[i]);
                        if(((i+1)%32)==0) DEBUG("\n");
                }

                if((i%32)!=0) DEBUG("\n");

}*/
    if (len < 55)
    {
        DEBUG("loader info too small!!!!!!!!!!\n");
//        return;
    }
    datap = (unsigned char *)data+4;
    //if ((datap[0] != datap[1])||(datap[2] != datap[3]))
    //    DEBUG("!!!!!!!!!!!!!!!!error section number,need modify code!\n");

    datap += 4;

    if (loader_init == 0)
    {
        get_loader_message(&mark,&loaderInfo);
        //read_loaderinfo(&loaderInfo);
        loader_init = 1;
    }

    //oui 
    tmp16 = *datap;
    datap++;
    tmp16 = (tmp16<<8)|(*datap);
//DEBUG("loader info oui = [%x]\n",tmp16);
//    if (tmp16 != loaderInfo.oui)
//        return;

    //model_type
    datap++;
    tmp16 = *datap;
    datap++;
    tmp16 = (tmp16<<8)|(*datap);
//DEBUG("loader info model type = [%x]\n",tmp16);
//    if (tmp16 != loaderInfo.model_type)
//        return;

    datap ++;  //usergroup id

    //hardware_version
    datap += 2;
    //tmp32 = ((datap[0]<<24)|(datap[1]<<16)|(datap[2]<<8)|(datap[3]));
//DEBUG("loader harder version [%u][%u][%u][%u]\n",datap[0],datap[1],datap[2],datap[3]);
    if ((datap[0] != loaderInfo.hardware_version[0])||(datap[1] != loaderInfo.hardware_version[1])
       ||(datap[2] != loaderInfo.hardware_version[2])||(datap[3] != loaderInfo.hardware_version[3]))
    {
   //    return;
    }
    //software_version
    datap += 4;
    //tmp32 = ((datap[0]<<24)|(datap[1]<<16)|(datap[2]<<8)|(datap[3]));
//DEBUG("loader info software version = [%x][%x]\n",tmp32,loaderInfo.software_version);
//DEBUG("loader info software version [%u[%u][%u][%u]\n",datap[0],datap[1],datap[2],datap[3]); 
    if ((datap[0] == loaderInfo.software_version[0])||(datap[1] == loaderInfo.software_version[1])
       ||(datap[2] == loaderInfo.software_version[2])||(datap[3] == loaderInfo.software_version[3]))
    {
    //    return;
    }
    loaderInfo.software_version[0] = datap[0];
    loaderInfo.software_version[1] = datap[1];
    loaderInfo.software_version[2] = datap[2];
    loaderInfo.software_version[3] = datap[3];
//DEBUG("get software version..\n");
    //stb_id
    datap += 4;
    sprintf(tmp,"%.2x%.2x%.2x%.2x",datap[0],datap[1],datap[2],datap[3]);
    stb_id_h = atol(tmp);
//DEBUG("start stb id h = [%u] me h[%u][%x][%x]\n",stb_id_h,loaderInfo.stb_id_h,datap[4],datap[5]);
    if (loaderInfo.stb_id_h < stb_id_h)
    {
        datap += 4;
        //DEBUG("stb id is not in this update sequence \n");
        //return;
    }
    else if (loaderInfo.stb_id_h == stb_id_h)
    {
        datap += 4;
        sprintf(tmp,"%.2x%.2x%.2x%.2x",datap[0],datap[1],datap[2],datap[3]);
        stb_id_l = atol(tmp);
//DEBUG("start id l=[%u], l=[%u]\n",stb_id_h, stb_id_l);
        if (loaderInfo.stb_id_l < stb_id_l)
        {
            //DEBUG("stb id is not in this update sequence \n");
            //return;
        }
    }
    else
        datap += 4;
    datap += 4;
    sprintf(tmp,"%.2x%.2x%.2x%.2x",datap[0],datap[1],datap[2],datap[3]);
    stb_id_h = atol(tmp);
//DEBUG("end stb id h [%u] me [%u]\n",stb_id_h,loaderInfo.stb_id_h); 
    if (loaderInfo.stb_id_h > stb_id_h)
    {
        datap += 4;
        //DEBUG("stb id is not in this update sequence \n");
        //return;
    }
    else if (loaderInfo.stb_id_h == stb_id_h)
    {
        datap += 4;
        sprintf(tmp,"%.2x%.2x%.2x%.2x",datap[0],datap[1],datap[2],datap[3]);
        stb_id_l = atol(tmp);
//DEBUG("end start id h=[%u], l=[%u]\n",stb_id_h, stb_id_l);
        if (loaderInfo.stb_id_l > stb_id_l)
        {
            DEBUG("stb id is not in this update sequence \n");
            //return;
        }
    }
    else
        datap += 4;

	DEBUG("loader_dsc_fid: %d=%x\n", loader_dsc_fid,loader_dsc_fid);
    TC_free_filter(loader_dsc_fid);
    datap += 4;
    {
    unsigned short pid;
    unsigned char tid;
    Filter_param param;

    pid = *datap;
    datap++;
    pid = ((pid<<8)|(*datap));//&0x1fff; 
    datap++;
    tid = *datap++;
//DEBUG(">>>> pid = [%x]  tid=[%x]\n",pid,tid);
    memset(&param,0,sizeof(param));
    param.filter[0] = tid;
    param.mask[0] = 0xff;
    loaderInfo.fid = TC_alloc_filter(pid, &param, loader_section_handle, NULL, 0);
    }

    loaderInfo.file_type = *datap++;
    loaderInfo.img_len = ((datap[0]<<24)|(datap[1]<<16)|(datap[2]<<8)|(datap[3]));
    loaderInfo.download_type = datap[4];
//DEBUG(">>>>>> filetype =[%d], img_len[%d], downloadtype=[%d]\n",loaderInfo.file_type,loaderInfo.img_len,loaderInfo.download_type);
}

int alloc_filter(unsigned short pid, char pro)
{
	int i;
	Filter_param param;
	
	memset(&param,0,sizeof(param));
	param.filter[0] = 0x3e;
	param.mask[0] = 0xff;
	
	return TC_alloc_filter(pid, &param, dump_bytes, NULL, pro);
}

int free_filter(unsigned short pid)
{
	if(-1==pid)
		return -1;
		
	int i = 0;
	for(i=0; i < MAX_CHAN_FILTER; i++)
	{
		if(pid==chanFilter[i].pid)
		{
			TC_free_filter(chanFilter[i].fid);
			return 0;
		}
	}
	
	return -1;
}

void TC_free_filter(int fid)
{
	if ((fid < MAX_CHAN_FILTER)&&(fid>=0))
	{
		DEBUG("free fid=%d, pid=%x\n", fid, chanFilter[fid].pid);
		chanFilter[fid].used = 0;
		chanFilter[fid].pid = -1;
		chanFilter[fid].bytes = 0;
		chanFilter[fid].fid = -1;
		chanFilter[fid].stage  = CHAN_STAGE_START;
		if(chanFilter[fid].samepidnum)
		{
			unsigned short pid,j;
			
			pid = chanFilter[fid].pid;
			for(j = 0; j < max_filter_num; j++)
			{
				if ((chanFilter[j].used)&&(chanFilter[j].pid == pid))
				{
					if(chanFilter[j].samepidnum) 
					chanFilter[j].samepidnum--;
				}
			}
		}
		chanFilter[fid].pid = -1;
	}
	else
		DEBUG("invalid fid: %d\n", fid);
}

static int get_filter(unsigned short pid)
{
	int i;
	
	for(i = 0; i < max_filter_num; i++)
	{
		if (chanFilter[i].pid == pid)
		{
			if(chanFilter[i].samepidnum)
			{
				chanFilter[MAX_CHAN_FILTER].pid = pid;
				return MAX_CHAN_FILTER;   
			}
			else
				return i;
		}
	}
	return -1;	
}

static int parse_payload(int fid, int p, int dlen, int start, unsigned char *ptr)
{
	int part;
	unsigned char *optr;
	Channel_t *chan = &chanFilter[fid];
	static int total = 0;
	//DEBUG("startmak = [%d],chanbytes[%d]\n",start,chan->bytes);
	optr = ptr;
	if(start)
	{
		chan->bytes = 0;
		chan->stage = CHAN_STAGE_HEADER;
	}
	else if(chan->stage==CHAN_STAGE_START)
	{
		return 0;
	}
	
	if ((MULTI_BUF_SIZE - p) >= dlen)
	{
		memcpy(chan->buf+chan->bytes, optr+p, dlen);
		chan->bytes += dlen;
		p += dlen;
	}
	else
	{
		part = MULTI_BUF_SIZE - p;
		memcpy(chan->buf+chan->bytes, optr+p, part);
		chan->bytes += part;
		memcpy(chan->buf+chan->bytes, optr, dlen - part);
		chan->bytes += dlen - part;
		p = dlen - part;
	}
	//DEBUG("chan_bytes = [%d]\n",chan->bytes);
	if(chan->bytes<3)
		return 0;
	
	if(chan->stage==CHAN_STAGE_HEADER)
	{
		/*if((chan->buf[0]==0x00) && (chan->buf[1]==0x00) && (chan->buf[2]==0x01))
		{
			chan->type  = CHAN_TYPE_PES;
			chan->stage = CHAN_STAGE_PTS;
		}
		else*/
		if (chan->buf[0])
		{
			//chan->type  = CHAN_TYPE_SEC;
			chan->stage = CHAN_STAGE_PTR;
		}
		else if(chan->buf[1])
		{
			chan->stage = CHAN_STAGE_PTR;
		}
		else if(chan->buf[2]==0x01)
		{
			//chan->type  = CHAN_TYPE_PES;
			chan->stage = CHAN_STAGE_PTS;
		}
		else
		{
			//chan->type  = CHAN_TYPE_SEC;
			chan->stage = CHAN_STAGE_PTR;
		}
	}
	
	if(chan->stage==CHAN_STAGE_PTR)
	{
		int sec_len;
		int len = chan->buf[0]+1;
		int left = chan->bytes-len;
		//DEBUG("len = [%d] left[%d]\n",len,chan->bytes);		
		/*if(chan->bytes<len)
		return 0;
		
		if(left)
		memmove(chan->buf, chan->buf+len, left);
		chan->bytes = left;*/
		if (left < 3)
			return 0;
		if(chan->buf[len]==0xFF)
		{
			chan->stage = CHAN_STAGE_END;
			chan->bytes = 0;
			return 0;
		}
		else
		{
			sec_len = ((chan->buf[len+1]<<8)|chan->buf[len+2])&0xFFF;
			sec_len += 3;
			
			chan->offset = len;
			chan->sec_len = sec_len;
			chan->stage = CHAN_STAGE_DATA_SEC;
			if(left<sec_len)
				return 0;
		}
	}
	else if(chan->stage==CHAN_STAGE_PTS)
	{
		//uint64_t pts;
		
		if(chan->bytes<14)
			return 0;
		
		/*if(parse_pts(chan->buf, &pts)>=0)
		{
		int offset = ftell(parser->fp)-parser->bytes+parser->parsed;
		if(chan->pts)
		{
		parser->rate = ((uint64_t)(offset-chan->offset))*1000/((pts-chan->pts)/90);
		//AM_DEBUG(2, "ts bits rate %d", parser->rate*8);
		}
		
		if(!chan->pts)
		{
		chan->pts    = pts;
		chan->offset = offset;
		}
		}*/
		
		chan->stage = CHAN_STAGE_DATA_PES;
	}
	//if(!chan->bytes)
	//	return 0;
	
retry:
	if(chan->stage==CHAN_STAGE_DATA_SEC)
	{
		/*				if(chan->bytes<1)
		return 0;
		
		if(chan->buf[0]==0xFF)
		{
		//DEBUG("eeeeeeeeeeeeeeeeeeeend \n");
		chan->stage = CHAN_STAGE_END;
		}
		else
		{
		int sec_len, left;
		
		if(chan->bytes<3)
		return 0;
		
		sec_len = ((chan->buf[1]<<8)|chan->buf[2])&0xFFF;
		//DEBUG("section len = [%d]\n",sec_len);
		sec_len += 3;*/
		if((chan->bytes - chan->offset) < chan->sec_len)
			return 0;
		{
			unsigned char *chanbuf;
			int sec_len,left; 
			
			chanbuf = chan->buf + chan->offset;
			sec_len = chan->sec_len;
			if(*chanbuf == 0x3e)
			{
				send_mpe_sec_to_push_fifo(chanbuf, sec_len);
				// tt += sec_len;
				
				//DEBUG("payload [%d]\n",total);
			}
//			else if(*chanbuf == 0xf1){
//				//DEBUG("chan->pid: 0x%x\n", chan->pid);
//				loader_section_handle(0, chanbuf, sec_len, NULL);
//			}
			else
			{
				//DEBUG("===== chan->pid: 0x%x",chan->pid);
				int j;
				for(j = 0; j < max_filter_num; j++)
				{
					unsigned char match = 1;
					unsigned char neq = 0;
					Channel_t *f = &chanFilter[j];
					int i;
					
					if(!f->used||(f->pid != chan->pid))
						continue;
					
					for(i=0; i<DMX_FILTER_SIZE+2; i++)
					{
						unsigned char xor = chanbuf[i]^f->value[i];
						
						if(xor&f->maskandmode[i])
						{
							match = 0;
							break;
						}
						
						if(xor&f->maskandnotmode[i])
							neq = 1;
					}
					
					if(match && f->neq && !neq)
						match = 0;
					if(!match) 
						continue;
					else
					{
						if (f->hdle)
						{
							//DEBUG("call f->hdle\n");
							f->hdle(f->fid, chanbuf, sec_len, f->userdata);
						}
						break;
					}
				}
				left = chan->bytes - sec_len - chan->offset;
				if(left>0)
				{
					//DEBUG("llllllllllleft some data!!!!!!!!!!!!! [%d]\n",left);
					if(chanbuf[sec_len]==0xFF)
					{
						chan->stage = CHAN_STAGE_END;
						chan->bytes = 0;
						return 0;
					}
					memmove(chan->buf, chanbuf+sec_len, left);
				}
				else
					left = 0;
				chan->bytes = left;
				if(left)
					goto retry;
			}
		}
	}
	else if (chan->stage==CHAN_STAGE_DATA_PES)
	{
		/*for(f=parser->filters; f; f=f->next)
		{
		if(f->enable && (f->params.pid==chan->pid) && (f->type==chan->type))
		break;
		}
		if(f)*/
		/*{
		int left = f->buf_size-f->bytes;
		int len = AM_MIN(left, chan->bytes);
		int pos = (f->begin+f->bytes)%f->buf_size;
		int cnt = f->buf_size-pos;
		
		if(cnt>=len)
		{
		memcpy(f->buf+pos, chan->buf, len);
		}
		else
		{
		int cnt2 = len-cnt;
		memcpy(f->buf+pos, chan->buf, cnt);
		memcpy(f->buf, chan->buf+cnt, cnt2);
		}
		
		pthread_cond_broadcast(&parser->cond);
		}*/
		//             handle_pes_packet(fid, chan->buf, chan->bytes, userdata);
		//DEBUG("get a pes packet\n");
		chan->bytes = 0;
		return 0;
	}
	
	if(chan->stage==CHAN_STAGE_END)
		chan->bytes = 0;
	
	return 0;
}

int parse_ts_packet(unsigned char *ptr, int write_ptr, int *read)
{
	static int p = 0;
	int left,p1,tmp,size,chan;
	unsigned char *optr;
	unsigned short pid;
	unsigned char  tei, cc, af_avail, p_avail, ts_start, sc;
	
	//DEBUG("aaaaaaaa\n");
	optr = ptr;
	/*Scan the sync byte*/
	if (optr[p]!=0x47)
	{
resync:
		while(optr[p]!=0x47)
		{
			//DEBUG("eeeeeeeeeeeerror\n");
			p++;
			if( p == write_ptr)
			{
				*read = p;
				return 0;
			}
			else if(p>=MULTI_BUF_SIZE)
				p = 0;
		}
		
		if ((p+188) < MULTI_BUF_SIZE)
		{
			if (optr[p+188] != 0x47)
			{
				p++;
				goto resync;
			}
		}
		else
		{
			if (optr[p+188 - MULTI_BUF_SIZE] != 0x47)
			{
				p++;
				if (p >= MULTI_BUF_SIZE)
					p = 0;
				goto resync;
			}
		}
	}
	
	if (write_ptr > p) 
		size = write_ptr - p;
	else
		size = (MULTI_BUF_SIZE - p) + write_ptr;
	
	if (size < 188)
	{
		*read = p;
		return 0;
	}
	left = 188;
	p1 = p;
	p += 188;
	if ( p >= MULTI_BUF_SIZE)
		p = p - MULTI_BUF_SIZE;
	*read = p;
	//ptr = &optr[p];//p;
	//t
	//return 0;
	
	p1++;
	if (p1 >= MULTI_BUF_SIZE)
		p1 = 0;
	tmp =  optr[p1];  
	tei  = tmp&0x80;
	//tei = ptr[1]&0x80;
	ts_start = tmp&0x40;
	//ts_start = ptr[1]&0x40;
	tmp = optr[p1];
	p1++;
	if (p1 >= MULTI_BUF_SIZE)
		p1 = 0;
	pid = ((tmp<<8)|optr[p1])&0x1FFF;
	
	p1++;
	if (p1 >= MULTI_BUF_SIZE)
		p1 = 0;
	tmp = optr[p1];
	sc = tmp&0xC0;
	//sc  = ptr[3]&0xC0;
	cc  = tmp&0x0F;
	//cc  = ptr[3]&0x0F;
	af_avail = tmp&0x20;
	//af_avail = ptr[3]&0x20;
	p_avail  = tmp&0x10;
	//p_avail  = ptr[3]&0x10;
	
	if(pid==0x1FFF)
		goto end;
	
	if(tei)
	{
		//AM_DEBUG(3, "ts error");
		goto end;
	}
	
	//chan = get_filter(pid);
	/*if(chan == -1)
	goto end;*/
	/*chan = parser_get_chan(parser, pid);
	if(!chan)
	goto end;
	*/
	/*if(chan->cc!=0xFF)
	{
	if(chan->cc!=cc)
	AM_DEBUG(3, "discontinuous");
	}*/
	
	/*if(p_avail)
	cc = (cc+1)&0x0f;
	chan->cc = cc;*/
	
	p1++;
	if (p1 >= MULTI_BUF_SIZE)
		p1 = 0;
	//ptr += 4;
	left-= 4;
	
	if(af_avail)
	{
		/*left-= ptr[0]+1;
		ptr += ptr[0]+1;*/
		left -= optr[p1]+1;
		p1 += optr[p1]+1;
		if(p1 >= MULTI_BUF_SIZE)
		p1 = p1 - MULTI_BUF_SIZE;
	}
	
	if(p_avail && (left>0) && !sc)
	{
		chan = get_filter(pid);
		//DEBUG("get channel [%d][%d]\n",chan,pid);
		if(chan != -1)
			parse_payload(chan, p1, left, ts_start, ptr);
	}
	
	end:
	return 1;
}

void chanFilterInit(void)
{
	int i;
	
	for(i = 0; i < MAX_CHAN_FILTER; i++)
	{
		chanFilter[i].pid = -1;
		chanFilter[i].used = 0;
		chanFilter[i].bytes = 0;
		chanFilter[i].stage  = CHAN_STAGE_START;
		chanFilter[i].samepidnum = 0;
		chanFilter[i].fid = -1;
	}
}

#if 0
int main(void)
{
	FILE *fd1,*fde;
	//int writer_p = 0;
	unsigned char buf[MULTI_BUF_SIZE];
	int ret,left,len,read,filter1,total;
	unsigned int dwnext = 0;
	unsigned int ennum=10;
	SCDCAPVODEntitleInfo einfo[10];
	
	char pbyBuffer[4*1024];
	int pdwBufferLen=4*1024;
	
	chanFilterInit();
	//Card_Entitle_init();
#if 1
	if (CDCASTB_Init(0))
		DEBUG("DRM Init successful!!!!!!\n");
	else
		DEBUG("DRM Init failure!!!!!!!!!!\n");
	sleep(5);
	//CDCASTB_FormatBuffer();
	DEBUG("2222222222222222222\n");
	if (CDCASTB_SCInsert())
		DEBUG("CARD inserted!!!!!!!!!\n");
	else
		DEBUG("CARD out!!!!!!!!!!!!\n");
	CDCASTB_SetEmmPid(0x64);
	sleep(2);
	
	
#if 1 	
	//fde = fopen("expentitle.txt","w+");
	//ret = CDCASTB_DRM_ExportEntitleFile("8000302100000333",fde);
	//fclose(fde);
	ret = CDCASTB_DRM_GetEntitleInfo(&dwnext,einfo,&ennum);
	
	DEBUG("ggggggget entitle info ret[%d],ennum[%d]\n",ret,ennum);
	
	while(1);
#endif
	
#endif
	//	filter1 = alloc_filter(123);
	//DEBUG("alloc _filter [%d]\n",filter1);
	//      fd1 = fopen("hytd.ts", "r");
#if 1
	FILE *fp1,*fp2, *fp3;
	if ((fp1 = fopen("test.ts","r")) == NULL)
	//if ((fp1 = fopen("content1.txt","r")) == NULL) 
		DEBUG("open content1.txt error\n");
	
	if ((fp2 = fopen("1.drm","r")) == NULL)
	//if ((fp2 = fopen("product1.drm","r")) == NULL)
		DEBUG("open product1.drm error\n");
	DEBUG("opening file.....\n");
	ret = CDCASTB_DRM_OpenFile((const void*)fp1,(const void*)fp2);
	
	DEBUG("!!!!!!!!!!!!!!!open the two file [%d]\n",ret);
	ret = CDCASTB_DRM_SyncEntitleToCard();
	DEBUG("sybc result [%d]\n",ret);
	while(1);
	if ((fp3 = fopen("result.txt","w+")) == NULL)
		DEBUG("open result.txt error\n");
	
	do 
	{
		ret = CDCASTB_DRM_ReadFile((const void*)fp1,pbyBuffer,&pdwBufferLen);
		DEBUG("read file [%d][%d]\n",ret,pdwBufferLen);
		//if ((fp3 = fopen("result.txt","wt")) == NULL)
		//  DEBUG("open result.txt error\n");
		
		fwrite(pbyBuffer,1,pdwBufferLen,fp3);
	}while(ret == 0);
	
	DEBUG("ret = [%d]\n",ret);
	fclose(fp3);
	fclose(fp2);
	fclose(fp1);
	
	while(1)
	{
		usleep(100000);
	}
#endif
	fd1 = fopen("3_DRM.ts", "r");
	if(!fd1) DEBUG("open file error\n");
		left = 0;
	read = 0;
	total = 0;
	if (!fd1)
	{
		DEBUG("open hytd.ts error\n");
		return -1;
	}
	
	while(1)
	{
		len = 1024;
		if ((left + len) < MULTI_BUF_SIZE)
		{
			ret = fread(buf+left,1,len,fd1);
			if (ret < 0) break;
		}
		else
		{
			len = MULTI_BUF_SIZE - left;
			if (len > 1024)
			len = 1024;
			ret = fread(buf+left,1,len,fd1);
			if (ret < 0) break;
		}
		if (ret > 0) {
			left += ret;
			total += ret;
			if (left >= MULTI_BUF_SIZE) left = 0;
		}
		else
		break;
		
		while(1)
		{
			if (left >= read)
				len = left - read;
			else
				len = MULTI_BUF_SIZE - read + left;
			if (len >188)	
				parse_ts_packet(buf,left,&read);
			else
				break;
		}
		usleep(1);
		//DEBUG("total = [%d]\n",total);
	}
	DEBUG("total = [%d][%d]\n",total,tt);
	fclose(fd1);
	CDCASTB_Close();
	
	return 0;
}
#endif
