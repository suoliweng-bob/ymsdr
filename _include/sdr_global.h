
#ifndef __SDR_GBLDEF_H__
#define __SDR_GBLDEF_H__

#include <signal.h>
#include <string>     

typedef struct _CConfItem
{
	char itemname[50];
	char itemcontent[500];
}CConfItem, *LPCConfItem;

typedef struct
{
	int log_level;
	int fd;
}sdr_log_t;

typedef struct 
{
    char url[128];
    char user[32];
    char passwd[32];
    char storagepath[256];
    int upload;
}sdr_upload_t;

typedef struct  _CConfCmd
{
	int type;
	char data[100];
}CConfCmd, *LPCConfCmd;

enum COLLECTION_MODE
{
	_ADS_B = 0,
	_AIS,
	_FM
};

typedef  struct  _RadioParams
{
     std::string ExtraOptions;  
    unsigned long Frequency;  
    int Gain;  
    unsigned long SampleRate;  
    std::string Modulation;  
    int Squelchlevel;  
    int PpmError;  
    unsigned long ResampleRate;  
    int SegmentTime;  
    int Duration; // 假设我们添加了一个Duration字段来表示录制时长  
    std::string OutputDir; // 假设我们添加了一个OutputDir字段来指定输出目录 
    std::string  ShellFile;
	std::string  ShellcmdFile;
}RadioParams, *LPRadioParams;


//外部全局量声明
extern size_t        g_argvneedmem;
extern size_t        g_envneedmem; 
extern int           g_os_argc; 
extern char          **g_os_argv;
extern char          *gp_envmem; 
extern int           g_daemonized;
extern  char        *g_app_name;


extern pid_t         sdr_pid;
extern pid_t         sdr_parent;
extern sdr_log_t     sdr_log;
extern int           sdr_process;   
extern sig_atomic_t  sdr_reap;   
extern int           g_stopEvent;
extern int sdr_fd[2][2]; 
extern RadioParams g_radioParams;
extern sdr_upload_t sdr_upload;

#endif
