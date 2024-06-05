
#ifndef __SDR_MACRO_H__
#define __SDR__MACRO_H__


#define SDR_MAX_ERROR_STR   2048   //数组长度

//简单功能函数
#define sdr_cpymem(dst, src, n)   (((u_char *) memcpy(dst, src, n)) + (n))  
#define sdr_min(val1, val2)  ((val1 > val2) ? (val2) : (val1))              //比较大小，返回小值

//数字相关
#define SDR_MAX_UINT32_VALUE   (uint32_t) 0xffffffff              //最大的32位无符号数：十进制是‭4294967295‬
#define SDR_INT64_LEN          (sizeof("-9223372036854775808") - 1)     

//日志相关
#define SDR_LOG_STDERR            0    //控制台错误【stderr】
#define SDR_LOG_EMERG             1    //紧急 【emerg】
#define SDR_LOG_ALERT             2    //警戒 【alert】
#define SDR_LOG_CRIT              3    //严重 【crit】
#define SDR_LOG_ERR               4    //错误 【error】：属于常用级别
#define SDR_LOG_WARN              5    //警告 【warn】：属于常用级别
#define SDR_LOG_NOTICE            6    //注意 【notice】
#define SDR_LOG_INFO              7    //信息 【info】
#define SDR_LOG_DEBUG             8    //调试 【debug】：最低级别

#define SDR_ERROR_LOG_PATH       "logs/error.log"   

//进程相关
#define SDR_PROCESS_MASTER     0 
#define SDR_PROCESS_WORKER     1
#define SDR_PROCESS_UPLOAD     2  

#define SDR_FM_SCRIPT   "amfm"
#define SDR_SCRIPT_CMD   "./amfm"

#define SDR_STORAGE_PATH    "/home/topeet/sdr"

#endif
