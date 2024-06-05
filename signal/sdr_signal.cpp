
#include <string.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>    //信号相关头文件 
#include <errno.h>     //errno
#include <sys/wait.h>  //waitpid

#include "sdr_global.h"
#include "sdr_macro.h"
#include "sdr_func.h" 

//一个信号有关的结构 sdr_signal_t
typedef struct 
{
    int           signo;       //信号对应的数字编号 
    const  char   *signame;  

    //信号处理函数
    void  (*handler)(int signo, siginfo_t *siginfo, void *ucontext); 
} sdr_signal_t;

//声明一个信号处理函数
static void sdr_signal_handler(int signo, siginfo_t *siginfo, void *ucontext); 
static void sdr_process_get_status(void);                                   

//数组 ，定义本系统处理的各种信号
sdr_signal_t  signals[] = {
    // signo      signame             handler
    { SIGHUP,    "SIGHUP",           sdr_signal_handler },        //终端断开信号，对于守护进程常用于reload重载配置文件通知--标识1
    { SIGINT,    "SIGINT",           sdr_signal_handler },        //标识2   
	{ SIGTERM,   "SIGTERM",          sdr_signal_handler },        //标识15
    { SIGCHLD,   "SIGCHLD",          sdr_signal_handler },        //子进程退出时，父进程会收到这个信号--标识17
    { SIGQUIT,   "SIGQUIT",          sdr_signal_handler },        //标识3
    { SIGIO,     "SIGIO",            sdr_signal_handler },        //指示一个异步I/O事件【通用异步I/O信号】
    { SIGSYS,    "SIGSYS, SIG_IGN",  NULL               },        //忽略这个信号，SIGSYS表示收到了一个无效系统调用，如果我们不忽略，进程会被操作系统杀死，--标识31
    { 0,         NULL,               NULL               }         //信号对应的数字至少是1，所以可以用0作为一个特殊标记
};

//初始化信号的函数，用于注册信号处理程序
//返回值：0成功  ，-1失败
int sdr_init_signals()
{
    sdr_signal_t      *sig;  
    struct sigaction   sa;   

    for (sig = signals; sig->signo != 0; sig++)  
    {        
        memset(&sa,0,sizeof(struct sigaction));

        if (sig->handler)  
        {
            sa.sa_sigaction = sig->handler;  
            sa.sa_flags = SA_SIGINFO;       
        }
        else
        {
            sa.sa_handler = SIG_IGN;                                
        } //end if

        sigemptyset(&sa.sa_mask);                   
        
        //设置信号处理动作(信号处理函数)
        if (sigaction(sig->signo, &sa, NULL) == -1) 
        {   
            sdr_log_error_core(SDR_LOG_EMERG,errno,"sigaction(%s) failed",sig->signame); 
            return -1; //有失败就直接返回
        }	
        else
        {            
            //sdr_log_error_core(SDR_LOG_EMERG,errno,"sigaction(%s) succed!",sig->signame);     //成功不用写日志 
            //sdr_log_stderr(0,"sigaction(%s) succed!",sig->signame); //直接往屏幕上打印看看 ，不需要时可以去掉
        }
    } //end for
    return 0; //成功    
}

//信号处理函数
static void sdr_signal_handler(int signo, siginfo_t *siginfo, void *ucontext)
{    
    //printf("来信号了\n");    
    sdr_signal_t    *sig;    //自定义结构
    char            *action; 
    
    for (sig = signals; sig->signo != 0; sig++) 
    {         
        //找到对应信号，即可处理
        if (sig->signo == signo) 
        { 
            break;
        }
    } //end for

    action = (char *)"";  

    if(sdr_process == SDR_PROCESS_MASTER)   
    {
        //master进程的往这里走
        switch (signo)
        {
        case SIGCHLD: 
            sdr_reap++;
            break;
            
        default:
            break;
        } //end switch
    }
    else if(sdr_process == SDR_PROCESS_WORKER) 
    {
        //......以后再增加
        //....
    }
    else
    {
        //do nothing
    } //end if(sdr_process == SDR_PROCESS_MASTER)

    //这里记录一些日志信息
    //siginfo这个
    if(siginfo && siginfo->si_pid)  
    {
        sdr_log_error_core(SDR_LOG_NOTICE,0,"signal %d (%s) received from %P%s", signo, sig->signame, siginfo->si_pid, action); 
    }
    else
    {
        sdr_log_error_core(SDR_LOG_NOTICE,0,"signal %d (%s) received %s",signo, sig->signame, action);
    }

    //子进程状态有变化
    if (signo == SIGCHLD) 
    {
        sdr_process_get_status(); 
    } //end if

    return;
}

//获取子进程的结束状态，
static void sdr_process_get_status(void)
{
    pid_t            pid;
    int              status;
    int              err;
    int              one=0; 

    //当你杀死一个子进程时，父进程会收到这个SIGCHLD信号。
    for ( ;; ) 
    {
        pid = waitpid(-1, &status, WNOHANG);       

        if(pid == 0) 
        {
            return;
        } //end if(pid == 0)
        //-------------------------------
        if(pid == -1)//这表示这个waitpid调用有错误
        {
            err = errno;
            if(err == EINTR)          
            {
                continue;
            }

            if(err == ECHILD  && one)  
            {
                return;
            }

            if (err == ECHILD)        
            {
                sdr_log_error_core(SDR_LOG_INFO,err,"waitpid() failed!");
                return;
            }
            sdr_log_error_core(SDR_LOG_ALERT,err,"waitpid() failed!");
            return;
        }  //end if(pid == -1)
        //-------------------------------

        one = 1;  
        if(WTERMSIG(status))  
        {
            sdr_log_error_core(SDR_LOG_ALERT,0,"pid = %P exited on signal %d!",pid,WTERMSIG(status)); 
        }
        else
        {
            sdr_log_error_core(SDR_LOG_NOTICE,0,"pid = %P exited with code %d!",pid,WEXITSTATUS(status)); 
        }
    } //end for
    return;
}
