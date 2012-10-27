#include <pthread.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <errno.h>
#include <string.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <android/log.h>

#include "am/am_smc.h"
#include "linux/amsmc.h"
#include "prodrm20.h"

#define DRMVOD_LOG_TAG "DRMLIB"
#if 1
#define LOGI(...) __android_log_print(ANDROID_LOG_INFO, DRMVOD_LOG_TAG,__VA_ARGS__)
#define LOGD(...) __android_log_print(ANDROID_LOG_DEBUG, DRMVOD_LOG_TAG,__VA_ARGS__)
#define LOGE(...) __android_log_print(ANDROID_LOG_ERROR, DRMVOD_LOG_TAG,__VA_ARGS__)
#else
#define LOGI(...)
#define LOGD(...)
#define LOGE(...)
#endif


#if 0
typedef struct {
	char sn[CDCA_MAXLEN_SN + 1];
	FILE *fd;
} SCDCACardEntitleInfo;

typedef struct {
	CDCA_U8        byReqID;
	CDCA_U16      wPID;
	CDCA_U32      timeouttime;
} SCDCAFilterInfo;
#endif
//#define CDCA_MAX_CARD_NUM 2
#define SMC_DEVICE  "/dev/smc0"
#define BLOCK01_FILE "/data/dbstar/drm/entitle/block01"
#define ENTITLE_FILE_PATH "/data/dbstar/drm/entitle"
//#define DMX_DEV_NO 0

//CDCA_U8 checkTimeoutMark = 0;
int smc_fd = -1;


FILE *block01_fd = NULL;

SCDCACardEntitleInfo card_sn = {"", NULL}; //[CDCA_MAX_CARD_NUM];
//SCDCAFilterInfo dmx_filter[MAX_CHAN_FILTER];
//extern Channel_t chanFilter[];
//extern int max_filter_num;


static int mkdirp(char *path)
{
	int ret = 0;
	char cmd[128] = {};

	if (path == NULL) {
		LOGD("--- path NULL\n");
		return -1;
	}

	if (access(path, 0) == 0) {
		LOGD("--- path[%s] already exist.\n", path);
		ret = 0;
	} else {
		sprintf(cmd, "mkdir -p %s", path);
		ret = system(cmd);
	}

	return ret;
}

/*-------- �̹߳��� --------*/

void CDSTBCA_HDDec_CloseDecrypter(const void *pCtx)
{
}

/* ע������ */
#define MAX_DRM_THREADS 10

pthread_t tmpthread[MAX_DRM_THREADS];  //for max thread for drm module
CDCA_BOOL CDSTBCA_RegisterTask(const char* szName,
                               CDCA_U8     byPriority,
                               void*       pTaskFun,
                               void*       pParam,
                               CDCA_U16    wStackSize)
{
	static int i = 0;
	int ret = 0;
	LOGD("DRM create thread[%d], thread name [%s]\n", i, szName);
	if (pthread_create(&tmpthread[i], NULL, pTaskFun, pParam) != 0) {
		LOGD("DRM Create task [%s] ERROR\n", szName);
		return CDCA_FALSE;
	}
	i++;
	if (i >= MAX_DRM_THREADS) {
		i = MAX_DRM_THREADS - 1;
	}
	return CDCA_TRUE;

}

/* �̹߳��� */
void CDSTBCA_Sleep(CDCA_U16 wMilliSeconds)
{
	usleep(1000 * wMilliSeconds);
}


/*-------- �ź������� --------*/

/* ��ʼ���ź��� */
void CDSTBCA_SemaphoreInit(CDCA_Semaphore* pSemaphore, CDCA_BOOL bInitVal)
{
	if (bInitVal == CDCA_TRUE) {
		*pSemaphore = 1;
	} else {
		*pSemaphore = 0;
	}
	//LOGD("init a semaphore [%d]\n",*pSemaphore);
}

/* �ź��������ź� */
void CDSTBCA_SemaphoreSignal(CDCA_Semaphore* pSemaphore)
{
	//LOGD("release semaphore...\n");
	*pSemaphore = 1;
}

/* �ź�����ȡ�ź� */
void CDSTBCA_SemaphoreWait(CDCA_Semaphore* pSemaphore)
{
	//LOGD("wait semaphore 1\n");
	while (*pSemaphore == 0) {
		usleep(5000);
	}
	*pSemaphore = 0;
	//LOGD("got a semaphore 2\n");
}


/*-------- �ڴ���� --------*/

/* �����ڴ� */
void* CDSTBCA_Malloc(CDCA_U32 byBufSize)
{
	void *ptr = NULL;
	ptr = malloc(byBufSize);
	LOGD("&&& malloc(%d), ptr=0x%p\n", byBufSize, ptr);
	return ptr;
}

/* �ͷ��ڴ� */
void  CDSTBCA_Free(void* pBuf)
{
	LOGD("&&& free(0x%p)\n", pBuf);
	free(pBuf);
}

/* �ڴ渳ֵ */
void  CDSTBCA_Memset(void*    pDestBuf,
                     CDCA_U8  c,
                     CDCA_U32 wSize)
{
	memset(pDestBuf, c, wSize);
}

/* �ڴ渴�� */
void  CDSTBCA_Memcpy(void*       pDestBuf,
                     const void* pSrcBuf,
                     CDCA_U32    wSize)
{
	memcpy(pDestBuf, pSrcBuf, wSize);
}


/*--------- �洢�ռ䣨Flash������ ---------*/

/* ��ȡ�洢�ռ� */
#if 1
void CDSTBCA_ReadBuffer(CDCA_U8 byBlockID, CDCA_U8*  pbyData, CDCA_U32* pdwLen)
{
	int len;
	int ret = 0;

	LOGD("###############Read the flash 64k buffer [%d]\n", byBlockID);
	if (block01_fd == NULL) {
		//	CDCA_U8 tmp[128*1024];

		//	memset(tmp,0,128*1024);
		ret = mkdirp(ENTITLE_FILE_PATH);
		if (ret != 0) {
			LOGD("--- create the entitle path error. [%s]\n", strerror(errno));
			return;
		}
		block01_fd = fopen(BLOCK01_FILE, "r+");

		if (block01_fd == NULL) {
			LOGD("open the flash read file error!!!!\n");
			return;
		}
		//  fwrite(tmp,1,128*1024,block01_fd);
	}
	if (byBlockID == CDCA_FLASH_BLOCK_B) {
		fseek(block01_fd, 64 * 1024, SEEK_SET);
		len = *pdwLen;
		*pdwLen = fread(pbyData, 1, len, block01_fd);
		LOGD("read flash block 2[%d]\n", *pdwLen);
	} else if (byBlockID == CDCA_FLASH_BLOCK_A) {
		fseek(block01_fd, 0, SEEK_SET);
		len = *pdwLen;
		*pdwLen = fread(pbyData, 1, len, block01_fd);
		LOGD("read flash block 1[%d]\n", *pdwLen);
	}
}

/* д��洢�ռ� */
void CDSTBCA_WriteBuffer(CDCA_U8 byBlockID, const CDCA_U8* pbyData, CDCA_U32 dwLen)
{
	int ret = 0;

	LOGD("###############Write the flash 64k buffer [%d]\n", byBlockID);
	if (block01_fd == NULL) {
		//	CDCA_U8 tmp[128*1024];

		//	memset(tmp,0,128*1024);
		ret = mkdirp(ENTITLE_FILE_PATH);
		if (ret != 0) {
			LOGD("--- create the entitle path error. [%s]\n", strerror(errno));
			return;
		}
		block01_fd = fopen(BLOCK01_FILE, "r+");
		if (block01_fd == NULL) {
			LOGD("open the flash file error!!!!!\n");
			return;
		}
		//  fwrite(tmp,1,128*1024,block01_fd);
	}
	if (byBlockID == CDCA_FLASH_BLOCK_B) {
		fseek(block01_fd, 64 * 1024, SEEK_SET);
		fwrite(pbyData, 1, dwLen, block01_fd);
		LOGD("write flash block 2 [%d]\n", dwLen);
	} else if (byBlockID == CDCA_FLASH_BLOCK_A) {
		fseek(block01_fd, 0, SEEK_SET);
		fwrite(pbyData, 1, dwLen, block01_fd);
		LOGD("write flash block 1 [%d]\n", dwLen);
	}
	fflush(block01_fd);
}
#endif

/*-------- TS������ --------*/

#if 0
void filter_timeout_handler(int fid)
{
	if (fid >= max_filter_num) {
		return;
	}
	if (checkTimeoutMark) {
		checkTimeoutMark --;
	}
	TC_free_filter(fid);
	dmx_filter[fid].byReqID = 0xff;
	dmx_filter[fid].wPID = 0xffff;
	dmx_filter[fid].timeouttime = 0;
}


static void filter_dump_bytes(int fid, const uint8_t *data, int len, void *user_data)
{
	CDCA_U8        byReqID;
	CDCA_U16       wPid;
	SCDCAFilterInfo *filterinfo;

	LOGD("Got EMM data len [%d]\n", len);
	/*{
	 int i;

	                for(i=0;i<len;i++)
	                {
	                        LOGD("%02x ", data[i]);
	                        if(((i+1)%32)==0) LOGD("\n");
	                }

	                if((i%32)!=0) LOGD("\n");

	}*/
	if (!user_data) {
		return;
	}
	filterinfo = (SCDCAFilterInfo *)user_data;
	filterinfo += fid;
	byReqID = filterinfo->byReqID;
	wPid = filterinfo->wPID;
	CDCASTB_PrivateDataGot(byReqID, CDCA_FALSE, wPid, data, len);
	if ((byReqID & 0x80) == 0x80) {
		if (checkTimeoutMark) {
			checkTimeoutMark --;
		}
		dmx_filter[fid].timeouttime = 0;
		CDSTBCA_ReleasePrivateDataFilter(byReqID, wPid);
	}
}
#endif

/* ����˽�����ݹ����� */
CDCA_BOOL CDSTBCA_SetPrivateDataFilter(CDCA_U8  byReqID,
                                       const CDCA_U8* pbyFilter,
                                       const CDCA_U8* pbyMask,
                                       CDCA_U8        byLen,
                                       CDCA_U16       wPid,
                                       CDCA_U8        byWaitSeconds)
{
	LOGD("CDSTBCA_SetPrivateDataFilter() called\n");
#if 0
	Filter_param param;
	//Channel_t *filter;
	int fid, i;


	for (fid = 0; fid < MAX_CHAN_FILTER; fid++) {
		if ((dmx_filter[fid].byReqID == byReqID) && (chanFilter[fid].used)) {
			CDSTBCA_ReleasePrivateDataFilter(byReqID, dmx_filter[fid].wPID);
			break;
		}
	}

	//waitcplete
	memset(&param, 0, sizeof(param));
	for (i = 0; i < byLen; i++) {
		param.filter[i] = pbyFilter[i];
		param.mask[i] = pbyMask[i];
	}

	fid = TC_alloc_filter(wPid, &param, (dataCb)filter_dump_bytes, (void *)&dmx_filter[0], 0);
	if (fid >= MAX_CHAN_FILTER) {
		return  CDCA_FALSE;
	}
	dmx_filter[fid].wPID = wPid;
	dmx_filter[fid].byReqID = byReqID;
	dmx_filter[fid].fid = fid;
	if (byWaitSeconds) {
		int now;
		checkTimeoutMark ++;
		AM_TIME_GetClock(&now);
		dmx_filter[fid].timeouttime = now + byWaitSeconds * 1000;
	} else {
		dmx_filter[fid].timeouttime = 0;
	}

#endif
	return CDCA_TRUE;
}

/* �ͷ�˽�����ݹ����� */
void CDSTBCA_ReleasePrivateDataFilter(CDCA_U8  byReqID, CDCA_U16 wPid)
{
#if 0
	int fid;

	for (fid = 0; fid < max_filter_num; fid++) {
		if ((dmx_filter[fid].byReqID == byReqID) && (dmx_filter[fid].wPID == wPid) && (chanFilter[fid].used)) {
			break;
		}
	}
	LOGD("@@@@@@@@@@@release [%d] filter\n", fid);
	if (fid >= max_filter_num) {
		return;
	}
	TC_free_filter(fid);
	dmx_filter[fid].byReqID = 0xff;
	dmx_filter[fid].wPID = 0xffff;
	dmx_filter[fid].timeouttime = 0;
#endif
}

/* ����CW�������� */
void CDSTBCA_ScrSetCW(CDCA_U16       wEcmPID,
                      const CDCA_U8* pbyOddKey,
                      const CDCA_U8* pbyEvenKey,
                      CDCA_U8        byKeyLen,
                      CDCA_BOOL      bTapingEnabled)
{
	LOGD("####################CDSTBCA_ScrSetCW function not implementted\n");
}


/*--------- ���ܿ����� ---------*/

/* ���ܿ���λ */
CDCA_BOOL CDSTBCA_SCReset(CDCA_U8* pbyATR, CDCA_U8* pbyLen)
{
	struct am_smc_atr abuf;
	int ds, i;
	AM_SMC_CardStatus_t status;


	if (smc_fd == -1) {
		smc_fd = open(SMC_DEVICE, O_RDWR);
		if (smc_fd == -1) {
			LOGD("cannot open device smc0\n");
			return CDCA_FALSE;
		} else {
			LOGD("open the smc device succeful [%d]\n", smc_fd);
		}
	}

	//=========================
	LOGD("please insert a card\n");
	i = 0;
	do {
		//AM_TRY(AM_SMC_GetCardStatus(SMC_DEV_NO, &status));
		if (ioctl(smc_fd, AMSMC_IOC_GET_STATUS, &ds)) {
			LOGD("get card status failed\n");
			return -1;
		}

		status = ds ? AM_SMC_CARD_IN : AM_SMC_CARD_OUT;
		usleep(100000);
		i++;
		if (i > 50) {
			LOGD("########### there is no smard card in \n");
			return CDCA_FALSE;
		}
	} while (status == AM_SMC_CARD_OUT);

	LOGD("card in\n");


	//=============================

	LOGD("&&&&&&&&&&&&&&&&&&&&&&&&&& reset the card = [%d]\n", smc_fd);
	if (ioctl(smc_fd, AMSMC_IOC_RESET, &abuf)) {
		LOGD("reset the card failed");
		return  CDCA_FALSE;
	}

	memcpy(pbyATR, abuf.atr, abuf.atr_len);
	*pbyLen = abuf.atr_len;
	LOGD("reset the smc succeful!!!\n ART: length [%d][%d]\n", *pbyLen, abuf.atr_len);
	for (i = 0; i < *pbyLen; i++) {
		LOGD("0x%x,", abuf.atr[i]);
	}
	return CDCA_TRUE;
}

/* ���ܿ�ͨѶ */
extern AM_ErrorCode_t AM_SMC_readwrite(const uint8_t *send, int slen, uint8_t *recv, int *rlen);
CDCA_BOOL CDSTBCA_SCPBRun(const CDCA_U8* pbyCommand,
                          CDCA_U16       wCommandLen,
                          CDCA_U8*       pbyReply,
                          CDCA_U16*      pwReplyLen)
{
	int i;
	//LOGD("ooooooooooooori send len [%d][%d]--[%d][%d]\n",wCommandLen,*pwReplyLen,pbyCommand[0],pbyCommand[1]);
	for (i = 0; i < 3; i++) {
		if (AM_SMC_readwrite(pbyCommand, (int)wCommandLen,  pbyReply, (int *) pwReplyLen) == AM_SUCCESS) {
			/*{int j;
			LOGD("smart card command:\n");
			for (j=0;j<wCommandLen;j++)
			    LOGD("0x%x,",pbyCommand[j]);
			LOGD("\n");
			LOGD("smart card reply:\n");

			for (j=0;j<*pwReplyLen;j++)
			    LOGD("0x%x,",pbyReply[j]);
			LOGD("\n");

			}*/
			//LOGD("sssssssssssssssssssssssend read successful\n");
			return CDCA_TRUE;
		}
	}
	LOGD("sssssssssssssssssssssssssssend fail \n");
	return CDCA_FALSE;
}

/*-------- ��Ȩ��Ϣ���� -------*/

/* ֪ͨ��Ȩ�仯 */
void CDSTBCA_EntitleChanged(CDCA_U16 wTvsID)
{
	LOGD("###############CDSTBCA_EntitleChanged function not implemented\n");
}


/* ����Ȩȷ����֪ͨ */
void CDSTBCA_DetitleReceived(CDCA_U8 bstatus)
{
	LOGD("##############CDSTBCA_DetitleReceived function not implemented [%d]\n", bstatus);
}

/*-------- ��ȫ���� --------*/

/* ��ȡ������Ψһ��� */
void CDSTBCA_GetSTBID(CDCA_U16* pwPlatformID,
                      CDCA_U32* pdwUniqueID)
{
	*pwPlatformID = 0;//0x1122;
	*pdwUniqueID = 0x00000000;
	LOGD("######################get STBID pwPlatformID=0x%x, pdwUniqueIDpdwUniqueID=0x%x \n", *pwPlatformID, *pdwUniqueID);
}

/* ��ȫоƬ�ӿ� */
CDCA_U16 CDSTBCA_SCFunction(CDCA_U8* pData)
{
	LOGD("#################CDSTBCA_SCFunction unsupport, return 0x9100\n");
	return 0x9100;//0x9400;//0x9100;
}

/*-------- IPPVӦ�� -------*/

/* IPPV��Ŀ֪ͨ */
void CDSTBCA_StartIppvBuyDlg(CDCA_U8                 byMessageType,
                             CDCA_U16                wEcmPid,
                             const SCDCAIppvBuyInfo* pIppvProgram)
{
	LOGD("##################### CDSTBCA_StartIppvBuyDlg not implemented\n");
}

/* ����IPPV�Ի��� */
void CDSTBCA_HideIPPVDlg(CDCA_U16 wEcmPid)
{
	LOGD("##################### CDSTBCA_HideIPPVDlg not implemented\n");
}

/*------- �ʼ�/OSD��ʾ���� -------*/

/* �ʼ�֪ͨ */
void CDSTBCA_EmailNotifyIcon(CDCA_U8 byShow, CDCA_U32 dwEmailID)
{
	LOGD("##################### CDSTBCA_EmailNotifyIcon not implemented\n");
}

/* ��ʾOSD��Ϣ */
void CDSTBCA_ShowOSDMessage(CDCA_U8     byStyle, const char* szMessage)
{
	LOGD("##################### CDSTBCA_ShowOSDMessage not implemented\n");
}

/* ����OSD��Ϣ*/
void CDSTBCA_HideOSDMessage(CDCA_U8 byStyle)
{
	LOGD("##################### CDSTBCA_HideOSDMessage not implemented\n");
}



/*-------- ��ĸ��Ӧ�� --------*/

/* ������ʾ��ȡι�����ݽ�� */
void  CDSTBCA_RequestFeeding(CDCA_BOOL bReadStatus)
{
	if (bReadStatus == CDCA_TRUE) {
		LOGD("Please insert child card!!!!!!!!!!!!!\n");
	} else {
		LOGD("Read mother card failure!!!!!!!!!!!\n");
	}
}

/*-------- ǿ���л�Ƶ�� --------*/

/* Ƶ������ */
void CDSTBCA_LockService(const SCDCALockService* pLockService)
{
	LOGD("##################### CDSTBCA_LockService not implemented\n");
}

/* ���Ƶ������ */
void CDSTBCA_UNLockService(void)
{
	LOGD("##################### CDSTBCA_UNLockService not implemented\n");
}

/*-------- ��ʾ������� --------*/

/* ���������տ���Ŀ����ʾ */
/*wEcmPID==0��ʾ��wEcmPID�޹ص���Ϣ���Ҳ��ܱ�������Ϣ����*/
void CDSTBCA_ShowBuyMessage(CDCA_U16 wEcmPID, CDCA_U8  byMessageType)
{
	LOGD("$$$$$$$$$$$$$$$$$ no right to see this program in 	CDSTBCA_ShowBuyMessage\n");
}

/* ָ����ʾ */
void CDSTBCA_ShowFingerMessage(CDCA_U16 wEcmPID, CDCA_U32 dwCardID)
{
	LOGD(" need display Ecm PID = %d  -------------Card ID =%d\n", (int)wEcmPID, (int)dwCardID);
}


/* ��ȫ������ʾ*/


/* ������ʾ */
void CDSTBCA_ShowProgressStrip(CDCA_U8 byProgress,  CDCA_U8 byMark)
{
	LOGD(" need display progress strip progress = %d  -------------byMark =%d\n", byProgress, byMark);
}

/*--------- ������֪ͨ --------*/

/* ������֪ͨ */
void  CDSTBCA_ActionRequest(CDCA_U16 wTVSID, CDCA_U8  byActionType)
{
	LOGD("######################### CDSTBCA_ActionRequest do not impletement\n");
}


/*--------- ˫��ģ��ӿ� --------*/

/* �ش�����֪ͨ*/
void CDSTBCA_NotifyCallBack(void)
{
	LOGD("######################### CDSTBCA_NotifyCallBack do not impletement\n");
}

/*-------- ���� --------*/

/* ��ȡ�ַ������� */
CDCA_U16 CDSTBCA_Strlen(const char* pString)
{
	LOGD("########## return string length = %d\n", strlen(pString));
	return (CDCA_U16)strlen(pString);
}

/* ������Ϣ��� */
void CDSTBCA_Printf(CDCA_U8 byLevel, const char* szMesssage)
{
	LOGD("[DRM](%d) %s\n", byLevel, szMesssage);
}

/*-------- PVODDRMģ��ӿ� -------*/

#if 0
void Card_Entitle_init()
{
	sprintf(card_sn[0].sn, "8000302100000333");
	card_sn[0].fd = NULL;
}

/* ����Ȩ�ļ� */
CDCA_BOOL CDSTBCA_DRM_OpenEntitleFile(char   CardSN[CDCA_MAXLEN_SN + 1],  void** pFileHandle)
{
	int i;

	LOGD("open the entitle file [%s]\n", CardSN);
	for (i = 0; i < CDCA_MAX_CARD_NUM; i++) {
		LOGD("candsn [%s][%s][%d][%d]\n", card_sn[i].sn, CardSN, i, strcmp(card_sn[i].sn, CardSN));
		if (!strcmp(card_sn[i].sn, CardSN)) {
			if (card_sn[i].fd) {
				*pFileHandle = card_sn[i].fd;
			} else {
				card_sn[i].fd = fopen(card_sn[i].sn, "r+"); //a+ �Ը��ӷ�ʽ�򿪿ɶ�д���ļ����������ڣ����������ڣ��ӵ��ļ�β��
				if (card_sn[i].fd) {
					LOGD("open the entitle [%d] successful\n", i);
				}
				*pFileHandle = card_sn[i].fd;
				//break;
			}
			break;
		}
	}

	if (i >= CDCA_MAX_CARD_NUM) {
		strcpy(card_sn[0].sn, CardSN);
		card_sn[0].fd = fopen(card_sn[0].sn, "w+"); //a+ �Ը��ӷ�ʽ�򿪿ɶ�д���ļ����������ڣ����������ڣ��ӵ��ļ�β��
		if (card_sn[0].fd) {
			LOGD("open the entitle 0 successful\n");
		}
		*pFileHandle = card_sn[0].fd;
	}

	if (*pFileHandle) {
		return CDCA_TRUE;
	}
	LOGD("open the entitle file failed!!!!!\n");
	return CDCA_FALSE;
}
#endif

CDCA_BOOL CDSTBCA_DRM_OpenEntitleFile(char   CardSN[CDCA_MAXLEN_SN + 1],  void** pFileHandle)
{
	int ret = 0;
	char fullentitle[CDCA_MAXLEN_SN_PATH];

	sprintf(fullentitle, "%s/%s", ENTITLE_FILE_PATH, CardSN);
	LOGD("open the entitle file [%s]\n", fullentitle);
	if (access(fullentitle, 0)) { //not exsit
		if (card_sn.fd) {
			fclose(card_sn.fd);
		}
		ret = mkdirp(ENTITLE_FILE_PATH);
		if (ret != 0) {
			LOGD("--- create the entitle path error. [%s]\n", strerror(errno));
			return CDCA_FALSE;
		}
		strncpy(card_sn.sn, fullentitle, CDCA_MAXLEN_SN_PATH);
		card_sn.fd = fopen(fullentitle, "w+"); //a+ �Ը��ӷ�ʽ�򿪿ɶ�д���ļ����������ڣ����������ڣ��ӵ��ļ�β��
		if (card_sn.fd) {
			LOGD("open the entitle 0 successful\n");
		}
		*pFileHandle = card_sn.fd;
	} else {
		if (card_sn.fd) {
			if (!strcmp(card_sn.sn, fullentitle)) {
				*pFileHandle = card_sn.fd;
			} else {
				fclose(card_sn.fd);
				strncpy(card_sn.sn, fullentitle, CDCA_MAXLEN_SN_PATH);
				card_sn.fd = fopen(card_sn.sn, "r+"); //a+ �Ը��ӷ�ʽ�򿪿ɶ�д���ļ����������ڣ����������ڣ��ӵ��ļ�β��
				if (card_sn.fd) {
					LOGD("open the entitle 0 successful\n");
				}
				*pFileHandle = card_sn.fd;
			}
		} else {
			strncpy(card_sn.sn, fullentitle, CDCA_MAXLEN_SN_PATH);
			card_sn.fd = fopen(fullentitle, "r+"); //a+ �Ը��ӷ�ʽ�򿪿ɶ�д���ļ����������ڣ����������ڣ��ӵ��ļ�β��
			if (card_sn.fd) {
				LOGD("open the entitle [0] successful\n");
			}
			*pFileHandle = card_sn.fd;
		}
	}

	if (*pFileHandle) {
		return CDCA_TRUE;
	}
	LOGD("open the entitle file failed!!!!!\n");
	return CDCA_FALSE;
}

/* �ر���Ȩ�ļ� */
void CDSTBCA_DRM_CloseEntitleFile(void*  pFileHandle)
{
	LOGD("close the entitle file!!!!\n");
	fclose(pFileHandle);
	memset(&card_sn, 0, sizeof(SCDCACardEntitleInfo));
}

/* �ƶ��ļ�ָ��*/
CDCA_BOOL CDSTBCA_SeekPos(const void* pFileHandle,
                          CDCA_U8     byOrigin,
                          CDCA_U32    dwOffsetKByte,
                          CDCA_U32    dwOffsetByte)
{
	LOGD("seek the file ori=[%d] posk=[%d] pos=[%d] \n", byOrigin, dwOffsetKByte, dwOffsetByte);

	if (!pFileHandle) {
		return CDCA_FALSE;
	}
	if (byOrigin == CDCA_SEEK_SET) {
		if (fseek((FILE *)pFileHandle, 1024 * dwOffsetKByte + dwOffsetByte, SEEK_SET)) {
			LOGD("!!!!!!!!!!!!!!!!!!!!!!!!!!fseek error\n");
			return CDCA_FALSE;
		}
	} else if (byOrigin == CDCA_SEEK_CUR_BACKWARD) {
		if (fseek((FILE *)pFileHandle, 1024 * dwOffsetKByte + dwOffsetByte, SEEK_CUR)) {
			return CDCA_FALSE;
		}
	} else if (byOrigin == CDCA_SEEK_CUR_FORWARD) {
		if (fseek((FILE *)pFileHandle, -(1024 * dwOffsetKByte + dwOffsetByte), SEEK_CUR)) {
			LOGD("!!!!!!!!!!!!!!!!!!!!!!!!!!fseek error\n");
			return CDCA_FALSE;
		}
	} else if (byOrigin == CDCA_SEEK_END) {
		if (fseek((FILE *)pFileHandle, -(1024 * dwOffsetKByte + dwOffsetByte), SEEK_END)) {
			LOGD("!!!!!!!!!!!!!!!!!!!!!!!!!!fseek error\n");
			return CDCA_FALSE;
		}
	}
	LOGD("seek the file pos successful\n");
	return CDCA_TRUE;
}

/* ���ļ� */
CDCA_U32 CDSTBCA_ReadFile(const void* pFileHandle, CDCA_U8* pBuf, CDCA_U32 dwLen)
{
	int ret;
	//LOGD("read file len [%d]\n", dwLen);
	if (!pFileHandle) {
		return -1;
	}

	ret = fread(pBuf, 1, dwLen, (FILE *)pFileHandle);
	if (ret > 0) {
		//LOGD("read file successful[%d][%d]!!!!\n", ret, dwLen);
	} else {
		LOGD("read file failed!!!!!! \n");
	}
	return ret;
}

/* д�ļ� */
CDCA_U32 CDSTBCA_WriteFile(const void* pFileHandle, CDCA_U8* pBuf, CDCA_U32 dwLen)
{
	int ret;
	LOGD("write file len [%d]\n", dwLen);
	if (!pFileHandle) {
		return -1;
	}
	for (ret = 0 ; ret < dwLen; ret++) {
		LOGD("0x%x,", pBuf[ret]);
	}
	/*return*/ret =  fwrite(pBuf, 1, dwLen, (FILE *)pFileHandle);
	if (ret > 0) {
		LOGD("write file successful[%d][%d]!!!!\n", ret, dwLen);
	} else {
		LOGD("write file failed!!!!!!\n");
	}
	fflush((FILE *)pFileHandle);
	return ret;
}