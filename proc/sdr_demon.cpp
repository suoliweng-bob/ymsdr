
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>     //errno
#include <sys/stat.h>
#include <fcntl.h>


#include "sdr_func.h"
#include "sdr_macro.h"
#include "sdr_c_conf.h"

int sdr_daemon()
{
    switch (fork())  
    {
    case -1:
        //创建子进程失败
        sdr_log_error_core(SDR_LOG_EMERG,errno, "sdr_daemon()中fork()失败!");
        return -1;
    case 0:
        break;
    default:
        return 1;  
    } //end switch

    sdr_parent = sdr_pid;    
    sdr_pid = getpid();    
    
    if (setsid() == -1)  
    {
        sdr_log_error_core(SDR_LOG_EMERG, errno,"sdr_daemon()中setsid()失败!");
        return -1;
    }

    umask(0); 

    int fd = open("/dev/null", O_RDWR);
    if (fd == -1) 
    {
        sdr_log_error_core(SDR_LOG_EMERG,errno,"sdr_daemon()中open(\"/dev/null\")失败!");        
        return -1;
    }
    if (dup2(fd, STDIN_FILENO) == -1)
    {
        sdr_log_error_core(SDR_LOG_EMERG,errno,"sdr_daemon()中dup2(STDIN)失败!");        
        return -1;
    }
    if (dup2(fd, STDOUT_FILENO) == -1) 
    {
        sdr_log_error_core(SDR_LOG_EMERG,errno,"sdr_daemon()中dup2(STDOUT)失败!");
        return -1;
    }
    if (fd > STDERR_FILENO) 
     {
        if (close(fd) == -1)  
        {
            sdr_log_error_core(SDR_LOG_EMERG,errno, "sdr_daemon()中close(fd)失败!");
            return -1;
        }
    }
    return 0; 
}

