#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <signal.h>   //信号相关头文件 
#include <errno.h>    //errno
#include <unistd.h>

#include "sdr_func.h"
#include "sdr_macro.h"
#include "sdr_c_conf.h"
#include "dump1090.h"
#include "AIS-catcher.h"
#include "CbSDR.h"
//函数声明
static void sdr_start_worker_processes(int threadnums);
static int sdr_spawn_process(int threadnums,const char *pprocname);
static void sdr_worker_process_cycle(int inum,const char *pprocname);
static void sdr_upload_process_cycle(int inum, const char *pprocname);
static void sdr_worker_process_init(int &inum);

//变量声明  
static u_char  master_process[] = "master process";

//创建worker子进程
void sdr_master_process_cycle()
{    
    sigset_t set;        //信号集

    sigemptyset(&set);   //清空信号集

    //下列这些信号在执行本函数期间不希望收到（保护不希望由信号中断的代码临界区）
    //建议fork()子进程时学习这种写法，防止信号的干扰；
    sigaddset(&set, SIGCHLD);     //子进程状态改变
    sigaddset(&set, SIGALRM);     //定时器超时
    sigaddset(&set, SIGIO);       //异步I/O
    sigaddset(&set, SIGINT);      //终端中断符
    sigaddset(&set, SIGHUP);      //连接断开
    sigaddset(&set, SIGUSR1);     //用户定义信号
    sigaddset(&set, SIGUSR2);     //用户定义信号
    sigaddset(&set, SIGWINCH);    //终端窗口大小改变
    sigaddset(&set, SIGTERM);     //终止
    sigaddset(&set, SIGQUIT);     //终端退出符
    //.........可以根据开发的实际需要往其中添加其他要屏蔽的信号......
    
    //设置，此时无法接受的信号；阻塞期间，你发过来的上述信号，多个会被合并为一个，暂存着，等你放开信号屏蔽后才能收到这些信号。。。
    if (sigprocmask(SIG_BLOCK, &set, NULL) == -1) 
    {        
        sdr_log_error_core(SDR_LOG_ALERT,errno,"sdr_master_process_cycle()中sigprocmask()失败!");
    }
    //即便sigprocmask失败，程序流程 也继续往下走

    //设置主进程标题
    size_t size;
    int    i;
    size = sizeof(master_process);  
    size += g_argvneedmem;          //argv参数长度加进来    
    if(size < 1000) //长度小于这个
    {
        char title[1000] = {0};
        strcpy(title,(const char *)master_process); //"master process"
        strcat(title," "); 
        for (i = 0; i < g_os_argc; i++)         
        {
            strcat(title,g_os_argv[i]);
        }//end for
        sdr_setproctitle(title); //设置标题
        sdr_log_error_core(SDR_LOG_NOTICE,0,"%s %P 【master进程】启动并开始运行......!",title,sdr_pid); 
    }    
  
        
    //从配置文件中读取要创建的worker进程数量
    CConfig *p_config = CConfig::GetInstance(); //单例类
    int workprocess = p_config->GetIntDefault("WorkerProcesses",2); //从配置文件中得到要创建的worker进程数量
    sdr_start_worker_processes(workprocess);  //这里要创建worker子进程

    sigemptyset(&set); //信号屏蔽字为空，表示不屏蔽任何信号
    
    for ( ;; ) 
    {
        if(sdr_process != SDR_PROCESS_MASTER) break;

        sigsuspend(&set); //阻塞在这里，等待一个信号，
        
        sleep(1); //休息1秒    
        
        if(sdr_reap == 1)
        {
            CConfCmd md = {0};  
            sdr_write_pipe(&md, sdr_fd[0][1]);
        }else if(sdr_reap == 2)
        {
            break;
        }
    }// end for(;;)
    return;
}

//根据给定的参数创建指定数量的子进程
static void sdr_start_worker_processes(int threadnums)
{
    int i;
    for (i = 0;   (sdr_process == SDR_PROCESS_MASTER) && i < threadnums; i++)
    {
        if (pipe(sdr_fd[i]) == -1)
        {
            sdr_log_stderr(0, "管道创建失败，退出!");
            return;
        }

        if (i == 0)
            sdr_spawn_process(i, "upload process");
        else
            sdr_spawn_process(i, "worker process");
    } // end for
    return;
}

//产生一个子进程
static int sdr_spawn_process(int inum,const char *pprocname)
{
    pid_t  pid;

    pid = fork(); //fork()系统调用产生子进程
    switch (pid)  //pid判断父子进程，分支处理
    {  
    case -1: //产生子进程失败
        sdr_log_error_core(SDR_LOG_ALERT,errno,"sdr_spawn_process()fork()产生子进程num=%d,procname=\"%s\"失败!",inum,pprocname);
        return -1;

    case 0:                   // 子进程分支
        sdr_parent = sdr_pid;
        sdr_pid = getpid();   
        close(sdr_fd[inum][1]);
        if (inum)
            sdr_worker_process_cycle(inum, pprocname);
        else
            sdr_upload_process_cycle(inum, pprocname);
        break;

    default: 
        close(sdr_fd[inum][0]);            
        break;
    }//end switch

    return pid;
}

static void sdr_upload_pipe(const int fd, std::atomic_bool & status)
{
    if(sdr_read_pipe(fd) != -1)
    {
        status.store(true);

    }
}

static void sdr_upload_process_cycle(int inum, const char *pprocname) 
{
    //设置一下变量
    sdr_process = SDR_PROCESS_UPLOAD;
    sdr_worker_process_init(inum);

    sdr_setproctitle(pprocname); //设置标题   
    sdr_log_error_core(SDR_LOG_NOTICE,0,"%s %P 【upload进程】启动并开始运行......!",pprocname,sdr_pid);

    try
    {
        std::atomic_bool bExit;
        bExit.store(false);

        std::thread td(sdr_upload_pipe, sdr_fd[0][0], std::ref(bExit));
        while (!bExit.load())
        {

            sleep(1);
            
            sdr_traverse_upload(std::string(sdr_upload.storagepath), sdr_upload);
        }
        td.join();
    }
    catch (const std::exception &e)
    {
        // 处理异常，例如记录日志或执行清理操作
        sdr_log_error_core(SDR_LOG_ERR,0,"%s %P 【upload进程】%s!",pprocname,sdr_pid,e.what());
    }
}

//worker子进程的功能函数
//inum：进程编号【0开始】
static void sdr_worker_process_cycle(int inum,const char *pprocname) 
{
    //设置一下变量
    sdr_process = SDR_PROCESS_WORKER;  //设置进程的类型，是worker进程

    //子进程设置进程名
    sdr_worker_process_init(inum);
    sdr_setproctitle(pprocname); //设置标题   

    switch (inum)
    {
    case COLLECTION_MODE::_ADS_B:
        sdr_log_error_core(SDR_LOG_NOTICE,0,"%s %P 【worker进程】ADS_B 启动并开始运行......!",pprocname,sdr_pid); 
        modesAds();
        break;
    case COLLECTION_MODE::_AIS:
        sdr_log_error_core(SDR_LOG_NOTICE,0,"%s %P 【worker进程】AIS 启动并开始运行......!",pprocname,sdr_pid); 
        modesAis();
        break;
    default:
    /* modesFm(rp); */
         sdr_log_error_core(SDR_LOG_NOTICE,0,"%s %P 【worker进程】启动并开始运行......!",pprocname,sdr_pid); 
         CbSDR::GetInstance()->OnInit();
        break;
    }

    return;
}

//描述：子进程创建时调用本函数进行一些初始化工作
static void sdr_worker_process_init(int &inum)
{
    sigset_t  set;      //信号集

    sigemptyset(&set);  //清空信号集
    if (sigprocmask(SIG_SETMASK, &set, NULL) == -1)  //不再屏蔽任何信号【接收任何信号】
    {
        sdr_log_error_core(SDR_LOG_ALERT,errno,"sdr_worker_process_init()中sigprocmask()失败!");
    }

    CConfig *p_config = CConfig::GetInstance();
    inum = p_config->GetIntDefault("CollectionMode",0); 
   
    return;
}
