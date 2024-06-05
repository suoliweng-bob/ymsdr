
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdint.h>    //uintptr_t
#include <stdarg.h>    //va_start....
#include <unistd.h>    //STDERR_FILENO等
#include <sys/time.h>  //gettimeofday
#include <time.h>      //localtime_r
#include <fcntl.h>     //open
#include <errno.h>     //errno

#include "sdr_global.h"
#include "sdr_macro.h"
#include "sdr_func.h"
#include "sdr_c_conf.h"

//全局量---------------------
//错误等级，和sdr_macro.h里定义的日志等级宏是一一对应关系
static u_char err_levels[][20]  = 
{
    {"stderr"},    //0：控制台错误
    {"emerg"},     //1：紧急
    {"alert"},     //2：警戒
    {"crit"},      //3：严重
    {"error"},     //4：错误
    {"warn"},      //5：警告
    {"notice"},    //6：注意
    {"info"},      //7：信息
    {"debug"}      //8：调试
};
sdr_log_t   sdr_log;


void sdr_log_stderr(int err, const char *fmt, ...)
{    
    va_list args;                        //创建一个va_list类型变量
    u_char  errstr[SDR_MAX_ERROR_STR+1]; //2048 
    u_char  *p,*last;

    memset(errstr,0,sizeof(errstr));    

    last = errstr + SDR_MAX_ERROR_STR;  
                                                
    p = sdr_cpymem(errstr, "sdr: ", 5);    
    
    va_start(args, fmt); 
    p = sdr_vslprintf(p,last,fmt,args); 
    va_end(args);        

    if (err)  
    {
        //显示出来
        p = sdr_log_errno(p, last, err);
    }
    
    //若位置不够，那换行也要硬插入到末尾 
    if (p >= (last - 1))
    {
        p = (last - 1) - 1; 
    }
    *p++ = '\n';   
    
    //标准错误输出信息    
    write(STDERR_FILENO,errstr,p - errstr); 

    if(sdr_log.fd > STDERR_FILENO) 
    {
        err = 0;    
        p--;*p = 0; 
        sdr_log_error_core(SDR_LOG_STDERR,err,(const char *)errstr); 
    }    
    return;
}

u_char *sdr_log_errno(u_char *buf, u_char *last, int err)
{
    char *perrorinfo = strerror(err); 
    size_t len = strlen(perrorinfo);

    char leftstr[10] = {0}; 
    sprintf(leftstr," (%d: ",err);
    size_t leftlen = strlen(leftstr);

    char rightstr[] = ") "; 
    size_t rightlen = strlen(rightstr);
    
    size_t extralen = leftlen + rightlen; //左右的额外宽度
    if ((buf + len + extralen) < last)
    {
        buf = sdr_cpymem(buf, leftstr, leftlen);
        buf = sdr_cpymem(buf, perrorinfo, len);
        buf = sdr_cpymem(buf, rightstr, rightlen);
    }
    return buf;
}

void sdr_log_error_core(int level,  int err, const char *fmt, ...)
{
    u_char  *last;
    u_char  errstr[SDR_MAX_ERROR_STR+1];   

    memset(errstr,0,sizeof(errstr));  
    last = errstr + SDR_MAX_ERROR_STR;   
    
    struct timeval   tv;
    struct tm        tm;
    time_t           sec;   //秒
    u_char           *p;    //指向当前要拷贝数据到其中的内存位置
    va_list          args;

    memset(&tv,0,sizeof(struct timeval));    
    memset(&tm,0,sizeof(struct tm));

    gettimeofday(&tv, NULL);           

    sec = tv.tv_sec;             //秒
    localtime_r(&sec, &tm);      
    tm.tm_mon++;                 //月份
    tm.tm_year += 1900;          //年份
    
    u_char strcurrtime[40]={0};  
    sdr_slprintf(strcurrtime,  
                    (u_char *)-1,                       
                    "%4d/%02d/%02d %02d:%02d:%02d",     //格式是 年/月/日 时:分:秒
                    tm.tm_year, tm.tm_mon,
                    tm.tm_mday, tm.tm_hour,
                    tm.tm_min, tm.tm_sec);
    p = sdr_cpymem(errstr,strcurrtime,strlen((const char *)strcurrtime));  
    p = sdr_slprintf(p, last, " [%s] ", err_levels[level]);               
    p = sdr_slprintf(p, last, "%P: ",sdr_pid);                       

    va_start(args, fmt);                     
    p = sdr_vslprintf(p, last, fmt, args);   
    va_end(args);                        

    if (err)  //有错误发生
    {
        //错误代码和错误信息也要显示出来
        p = sdr_log_errno(p, last, err);
    }
    //若位置不够，那换行也要硬插入到末尾
    if (p >= (last - 1))
    {
        p = (last - 1) - 1;
    }
    *p++ = '\n'; //增加个换行符       

    ssize_t   n;
    while(1) 
    {        
        if (level > sdr_log.log_level) 
        {
            //这种日志就不打印了
            break;
        }

        //写日志文件        
        n = write(sdr_log.fd,errstr,p - errstr); 
        if (n == -1) 
        {
            //写失败有问题
            if(errno == ENOSPC) 
            {
                //没空间还写个毛线啊
                //先do nothing吧；
            }
            else
            {
                if(sdr_log.fd != STDERR_FILENO) 
                {
                    n = write(STDERR_FILENO,errstr,p - errstr);
                }
            }
        }
        break;
    } 
    return;
}

//日志初始化
void sdr_log_init()
{
    u_char *plogname = NULL;
    size_t nlen;

    //从配置文件中读取和日志相关的配置信息
    CConfig *p_config = CConfig::GetInstance();
    plogname = (u_char *)p_config->GetString("Log");
    if(plogname == NULL)
    {
        plogname = (u_char *) SDR_ERROR_LOG_PATH; 
    }
    sdr_log.log_level = p_config->GetIntDefault("LogLevel",SDR_LOG_NOTICE);

    sdr_log.fd = open((const char *)plogname,O_WRONLY|O_APPEND|O_CREAT,0644);  
    if (sdr_log.fd == -1) 
    {
        sdr_log_stderr(errno,"[alert] could not open error log file: open() \"%s\" failed", plogname);
        sdr_log.fd = STDERR_FILENO;      
    } 
    return;
}
