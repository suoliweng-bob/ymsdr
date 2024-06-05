
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h> 
#include <errno.h>
#include <arpa/inet.h>
#include <sys/time.h>      

#include "sdr_macro.h"        
#include "sdr_func.h"         
#include "sdr_c_conf.h"       
 #include "CbSDR.h"
#include "dump1090.h"
#include "AIS-catcher.h" 

//本文件用的函数声明
static void freeresource();

 size_t  g_argvneedmem=0;        //保存下这些argv参数所需要的内存大小
 size_t  g_envneedmem=0;         //环境变量所占内存大小
 int     g_os_argc;              //参数个数 
 char    **g_os_argv;            //原始命令行参数数组,在main中会被赋值
 char    *gp_envmem=NULL;        //指向自己分配的env环境变量的内存
 int     g_daemonized=0;         //0：未启用，1：启用了
 char *g_app_name=NULL;

 pid_t   sdr_pid;                //当前进程的pid
 pid_t   sdr_parent;             //父进程的pid
 int     sdr_process;            //进程类型
 int     g_stopEvent;            //标志程序退出,0不退出1，退出

sdr_upload_t sdr_upload;

int sdr_fd[2][2]; 

RadioParams g_radioParams;

sig_atomic_t  sdr_reap;        

//程序主入口函数----------------------------------
int main(int argc, char *const *argv)
{     
    int exitcode = 0;           //退出代码
    int i;                    
    
    g_stopEvent = 0;            //标记程序是否退出，0不退出  

    sdr_pid    = getpid();      //取得进程pid
    sdr_parent = getppid();     //取得父进程的id 

    g_argvneedmem = 0;
    for(i = 0; i < argc; i++) 
    {
        g_argvneedmem += strlen(argv[i]) + 1; 
    } 
    //统计环境变量所占的内存。注意判断方法是environ[i]是否为空作为环境变量结束标记
    for(i = 0; environ[i]; i++) 
    {
        g_envneedmem += strlen(environ[i]) + 1; 
    } 

    g_os_argc = argc;          
    g_os_argv = (char **) argv; 

    sdr_process = SDR_PROCESS_MASTER; 
    sdr_reap = 0;                    
   
    CConfig *p_config = CConfig::GetInstance(); 
    if(p_config->Load("sdr.conf") == false)         
    {   
        sdr_log_init();    //初始化日志
        sdr_log_stderr(0,"配置文件[%s]载入失败，退出!","sdr.conf");

        exitcode = 2; 
        goto lblexit;
    }
    sdr_log_init();           
       
    if(sdr_init_signals() != 0) 
    {
        exitcode = 1;
        goto lblexit;
    }        
   
    sdr_init_setproctitle();    

    g_app_name = const_cast<char *>(p_config->GetString("AppName"));
    if(g_app_name == NULL) g_app_name = const_cast<char *>("sdr");

    sdr_ftp_init();

     //创建守护进程
    if(p_config->GetIntDefault("Daemon",0) == 1) 
    {
        int cdaemonresult = sdr_daemon();
        if(cdaemonresult == -1) 
        {
            exitcode = 1;   
            goto lblexit;
        }
        if(cdaemonresult == 1)
        {
            //这是原始的父进程
            freeresource();   
            exitcode = 0;
            return exitcode; 
        }
        g_daemonized = 1;   
    }

    sdr_master_process_cycle(); 

lblexit:
    //资源要释放掉
    if (sdr_process == SDR_PROCESS_MASTER){
        sdr_log_stderr(0, "程序退出，再见了!");
    }
    else if(sdr_process == SDR_PROCESS_UPLOAD)
    {
        sdr_log_error_core(SDR_LOG_NOTICE,0,"%P 【upload进程】程序退出!",sdr_pid); 
    }else
    {
        sdr_log_error_core(SDR_LOG_NOTICE,0,"%P 【worker进程】程序退出!",sdr_pid); 
    }
     freeresource();   
    return exitcode;
}

//释放资源的函数
void freeresource()
{
    if(gp_envmem)
    {
        delete []gp_envmem;
        gp_envmem = NULL;
    }

    if(sdr_log.fd != STDERR_FILENO && sdr_log.fd != -1)  
    {        
        close(sdr_log.fd); 
        sdr_log.fd = -1; 
    }
    
}
