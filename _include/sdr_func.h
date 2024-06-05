
#ifndef __SDR_FUNC_H__
#define __SDR_FUNC_H__

#include "sdr_global.h"

class CConfig;
//字符串相关函数
void   Rtrim(char *string);
void   Ltrim(char *string);

//设置可执行程序标题相关函数
void   sdr_init_setproctitle();
void   sdr_setproctitle(const char *title);

//和日志，打印输出有关
void   sdr_log_init();
void   sdr_log_stderr(int err, const char *fmt, ...);
void   sdr_log_error_core(int level,  int err, const char *fmt, ...);
u_char *sdr_log_errno(u_char *buf, u_char *last, int err);
u_char *sdr_snprintf(u_char *buf, size_t max, const char *fmt, ...);
u_char *sdr_slprintf(u_char *buf, u_char *last, const char *fmt, ...);
u_char *sdr_vslprintf(u_char *buf, u_char *last,const char *fmt,va_list args);

//和信号/主流程相关相关
int    sdr_init_signals();
void   sdr_master_process_cycle();
int    sdr_daemon();

bool sdr_file_exists(const char *path) ;

int  sdr_create_directory(const char *path);

int sdr_write_json(const char *path, const char *json);

void run_command(const RadioParams& params) ;

void modesFm(const RadioParams& params);

int to_shell_file(const RadioParams& params);

bool setFileExecutable(const std::string& filePath);

bool getRadioParams(RadioParams& rp);

void sdr_write_pipe(LPCConfCmd buf, const int fd);

size_t sdr_read_pipe(const int fd);

std::string sdr_get_extract_file_name(const std::string path);

std::string sdr_get_file_ext(const std::string& filename);

std::string sdr_replace_file_ext(const std::string& filename, const std::string& newExtension);

void sdr_traverse_upload(const std::string& dir_path, const sdr_upload_t  info);

void sdr_ftp_init();


#endif
