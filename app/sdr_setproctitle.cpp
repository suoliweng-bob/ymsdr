
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>  //env
#include <string.h>

#include "sdr_global.h"

//设置可执行程序标题
void sdr_init_setproctitle()
{   
    gp_envmem = new char[g_envneedmem]; 
    memset(gp_envmem,0,g_envneedmem); 

    char *ptmp = gp_envmem;
    //把原来的内存内容搬到新地方来
    for (int i = 0; environ[i]; i++) 
    {
        size_t size = strlen(environ[i])+1 ; 
        strcpy(ptmp,environ[i]);      
        environ[i] = ptmp;       
        ptmp += size;
    }
    return;
}

//设置可执行程序标题
void sdr_setproctitle(const char *title)
{
    size_t ititlelen = strlen(title); 
   
    size_t esy = g_argvneedmem + g_envneedmem; 
    if( esy <= ititlelen)
    {
        return;
    }

    g_os_argv[1] = NULL;  

    char *ptmp = g_os_argv[0]; 
    strcpy(ptmp,title);
    ptmp += ititlelen;

    size_t cha = esy - ititlelen;  
    memset(ptmp,0,cha);
    return;
}