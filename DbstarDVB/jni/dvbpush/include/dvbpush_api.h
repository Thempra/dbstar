﻿
#ifndef __DVBPUSH_API_H__
#define __DVBPUSH_API_H__

typedef enum {
	DBSTAR_COMMAND                  = 0x00000,
	DBSTAR_NOTIFY                   = 0xF0000,

	CMD_NETWORK_DISCONNECT	        = 0x00010,    // 网络断开
	CMD_NETWORK_CONNECT             = 0x00011,    // 网络恢复连接
	
	CMD_DISK_MOUNT                  = 0x00021,    // 硬盘插上可用
	CMD_DISK_UNMOUNT                = 0x00022,    // 硬盘拔掉
	CMD_DISK_FOREWARNING			= 0x00023,    // 硬盘到达预警空间
		
	CMD_DVBPUSH_GETINFO_START       = 0x00031,    // 开始获取push下载状态
	CMD_DVBPUSH_GETINFO				= 0x00032,    // 获取push下载状态
	CMD_DVBPUSH_GETINFO_STOP        = 0x00033,    // 停止获取push下载状态
	CMD_DVBPUSH_GETTS_STATUS		= 0x00034,    // 获取ts流状态
	
	CMD_UPGRADE_CANCEL              = 0x00041,    // 用户取消升级
	CMD_UPGRADE_CONFIRM             = 0x00042,    // 用户确认升级
	CMD_UPGRADE_TIMEOUT             = 0x00043,    // 用户操作对话框超时
	
	CMD_PUSH_SELECT                 = 0x00051,    // 用户从“选择接收”页面退出，选择完毕。

	CMD_DRM_SC_INSERT               = 0x00061,    // DRM smartcard Insert
	CMD_DRM_SC_REMOVE               = 0x00062,    // DRM smartcard Remove
	CMD_DRM_SC_USELESS              = 0x00063,    // DRM smartcard useless
	CMD_DRM_SC_SN_READ				= 0x00064,    // 智能卡号
	CMD_DRMLIB_VER_READ				= 0x00065,    // DRM库版本号
	CMD_DRM_SC_EIGENVALUE_READ		= 0x00066,    // 特征值
	CMD_DRM_ENTITLEINFO_READ		= 0x00067,    // 读取授权信息
	CMD_DRM_ENTITLEINFO_OUTPUT		= 0x00068,    // 授权信息导出
	CMD_DRM_ENTITLEINFO_INPUT		= 0x00069,    // 授权信息导入
	CMD_DRM_EMAILHEADS_READ			= 0x0006a,    // 读取所有邮件头
	CMD_DRM_EMAILCONTENT_READ		= 0x0006b,    // 读取指定邮件内容
	CMD_DRM_PVODPROGRAMINFO_READ	= 0x0006c,    // 读取加密文件信息

	CMD_MAX                         = 0x0FFFF,

	MSG_MARQUEE                     = 0x10000,    // 跑马灯
	MSG_UPGRADE                     = 0x20000,    // 升级成功
	MSG_STATUS						= 0x30000,    // 信息状态提示
	MSG_ERROR                       = 0x40000,    // 错误提示

	STATUS_DVBPUSH_INIT_FAILED      = 0x30010,    // dvbpush初始化失败
	STATUS_DVBPUSH_INIT_SUCCESS     = 0x30011,    // dvbpush初始化成功
	STATUS_DATA_SIGNAL_ON			= 0x30012,    // 信号正常，即有ts流
	STATUS_DATA_SIGNAL_OFF			= 0x30013,    // 无信号，即无ts流
	STATUS_COLUMN_REFRESH			= 0x30014,    // 动态栏目发生更新
	STATUS_PREVIEW_REFRESH			= 0x30015,    // 首页小片发生更新
	STATUS_INTERFACE_REFRESH		= 0x30016,    // 界面产品发生更新
	
	UPGRADE_NEW_VER                 = 0x20001,     // 有新版本到来，用户选择升级
	UPGRADE_NEW_VER_FORCE           = 0x20002,     // 有新版本到来，强制升级
	
	DRM_SC_INSERT_OK				= 0x20100,    // DRM smartcard Insert OK
	DRM_SC_INSERT_FAILED			= 0x20101,    // DRM smartcard Insert failed
	DRM_SC_REMOVE_OK				= 0x20102,    // DRM smartcard Remove OK
	DRM_SC_REMOVE_FAILED			= 0x20103,    // DRM smartcard Remove failed
	DRM_EMAIL_ICONHIDE				= 0x20104,    // DRM 隐藏邮件通知图标
	DRM_EMAIL_NEW					= 0x20105,    // DRM 新邮件到达
	DRM_EMAIL_SPACEEXHAUST			= 0x20106,    // DRM 邮箱已满
	DRM_OSD_SHOW					= 0x20107,    // DRM OSD需要显示
	DRM_OSD_HIDE					= 0x20108,    // DRM OSD需要隐藏
	
}DBSTAR_CMD_MSG_E;

typedef int (* dvbpush_notify_t)(int type, char *msg, int len);


/********************************************************
 功能：模块标准接口，初始化
 返回：0——成功；-1——失败
********************************************************/
int dvbpush_init();


/********************************************************
 功能：模块标准接口，反初始化
 返回：0——成功；-1——失败
********************************************************/
int dvbpush_uninit();



/********************************************************
 功能：UI向底层发送命令
 返回：0——成功；-1——失败
********************************************************/
int dvbpush_command(int cmd, char **buf, int *len);



/********************************************************
 功能：底层提供给UI使用，UI可以调用此函数设置回调（func），底层需要向上层发送消息时，可以通过调用回调（func）实现。
********************************************************/
int dvbpush_register_notify(void *func);

#endif
