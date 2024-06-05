#include <stdio.h>
#include <stdlib.h>  
#include <unistd.h>
#include <time.h>  
#include <string.h>  
#include <sys/stat.h>  
#include <sys/types.h>
#include <iostream>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <cstring> 
#include <dirent.h> 

#include "sdr_func.h"
#include "sdr_c_conf.h"
#include "sdr_macro.h"
#include "sdr_ftp_session.h"

//截取字符串尾部空格
void Rtrim(char *string)   
{   
	size_t len = 0;   
	if(string == NULL)   
		return;   

	len = strlen(string);   
	while(len > 0 && string[len-1] == ' ')   //位置换一下   
		string[--len] = 0;   
	return;   
}

//截取字符串首部空格
void Ltrim(char *string)
{
	size_t len = 0;
	len = strlen(string);   
	char *p_tmp = string;
	if( (*p_tmp) != ' ') //不是以空格开头
		return;
	//找第一个不为空格的
	while((*p_tmp) != '\0')
	{
		if( (*p_tmp) == ' ')
			p_tmp++;
		else
			break;
	}
	if((*p_tmp) == '\0') //全是空格
	{
		*string = '\0';
		return;
	}
	char *p_tmp2 = string; 
	while((*p_tmp) != '\0')
	{
		(*p_tmp2) = (*p_tmp);
		p_tmp++;
		p_tmp2++;
	}
	(*p_tmp2) = '\0';
    return;
}

bool sdr_file_exists(const char *path) 
{
    struct stat file_stat;  
    return (stat(path, &file_stat) == 0);
}

int sdr_create_directory(const char *path)
{
    if (!sdr_file_exists(path))
    {
        if (mkdir(path, S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == 0)
        {
            return 1;
        }
        else
        {
            return 0;
        }
    }
    return 1;
}

int sdr_write_json(const char *path, const char *json)
{
	time_t now;  
    struct tm *ltm;  
    char dir[256] = {0};  
  
    time(&now);  
    ltm = localtime(&now);  


	sprintf(dir,"%s%04d_%02d_%02d",path,ltm->tm_year + 1900,ltm->tm_mon + 1,ltm->tm_mday);

	if(!sdr_create_directory(dir))
	{
		return 0;
	}
    sprintf(dir,"%s/%04d%02d%02d-%02d-%02d-%02d.json",dir,ltm->tm_year + 1900,ltm->tm_mon + 1, ltm->tm_mday, ltm->tm_hour,ltm->tm_min,ltm->tm_sec);
	std::ofstream file(dir);
    file << json;
    file.close();
	return 1;
}

void run_command(const RadioParams& params) 
{   
    
     std::string cmd = "chmod +x ";
     cmd.append(params.ShellFile);
    std::system(cmd.c_str());  

    const char* command =  params.ShellcmdFile .c_str();      
    // 执行脚本文件  
    int result = std::system(command);  
    if (result == -1) {  
        std::cerr << "无法执行脚本文件。" << std::endl;  
    } else {  
        std::cout << "脚本文件执行完毕，返回码：" << result << std::endl;  
    }  
      
}  

void modesFm(const RadioParams& params) 
{  
    to_shell_file(params);  
    run_command(params);
}

int to_shell_file( const RadioParams& params) 
{  
    std::string shellfile = params.ShellFile;
    std::ofstream scriptFile(shellfile.c_str(), std::ios::out | std::ios::trunc);  
    if (!scriptFile.is_open()) {  
        std::cerr << "无法打开文件以写入脚本。" << std::endl;  
        return 1;  
    }  
  
    // RTL-SDR FM接收参数  
    std::ostringstream oss;  
    oss << "-f " << params.Frequency << " -s " << params.SampleRate;  
    if (!params.ExtraOptions.empty()) {  
        oss << " " << params.ExtraOptions;  
    }  
    std::string RTL_FM_ARGS = oss.str();  
  
    // FFmpeg MP3编码参数  
    const std::string FFMPEG_ARGS = "-f s16le -ac 1 -i pipe:0 -c:a libmp3lame -b:a 128k";  
  
    // 写入bash脚本的开头  
    scriptFile << "#!/bin/bash\n\n";  
  
    // 写入确保输出目录存在的命令  
    scriptFile << "mkdir -p \"" << params.OutputDir << "\"\n\n";  
  
    // 写入RTL-FM进程部分  
    scriptFile << " rtl_fm " << RTL_FM_ARGS << " | while true; do\n";  
    scriptFile << "    TIMESTAMP=$(date +\"%Y%m%d_%H%M%S\")\n";  
    scriptFile << "    MP3_FILE=\"" << params.OutputDir << "/${TIMESTAMP}.mp3\"\n";  
    scriptFile << "    ffmpeg " << FFMPEG_ARGS << " -t " << params.SegmentTime << " -f mp3 \"$MP3_FILE\" \n";  
    scriptFile << "    sleep " << params.Duration << "\n"; // 使用Duration
    scriptFile << "done &\n\n";  
  
    // 写入等待RTL-FM进程结束的命令  
    scriptFile << "wait $!\n";  
  
    // 关闭文件  
    scriptFile.close();  
  
    // 设置脚本文件为可执行  
    //setFileExecutable(shellfile);

    return 0;  
}

// 设置文件为可执行的函数  
bool setFileExecutable(const std::string& filePath)
 {  
    if (chmod(filePath.c_str(), S_IRUSR | S_IWUSR | X_OK) != 0) {  
        perror("chmod 失败");
        return false;  
    }  
    return true;  
}


bool getRadioParams(RadioParams& rp )
{
    CConfig *p_config = CConfig::GetInstance(); 
    rp.ExtraOptions =  p_config->GetString("ExtraOptions");
    rp.Frequency =  p_config->GetIntDefault("Frequency",230700000);
    rp.Gain =  p_config->GetIntDefault("Gain",0);
    rp.SampleRate =  p_config->GetIntDefault("SampleRate",170000);
    rp.Modulation =  p_config->GetString("Modulation");
    rp.Squelchlevel =  p_config->GetIntDefault("Squelchlevel",0);
    rp.PpmError =  p_config->GetIntDefault("PpmError",20);
    rp.ResampleRate =  p_config->GetIntDefault("ResampleRate",44100);
    rp.SegmentTime =  p_config->GetIntDefault("SegmentTime",120);
    rp.Duration =  p_config->GetIntDefault("Duration",120);
    rp.ShellFile =  SDR_FM_SCRIPT;
    rp.ShellcmdFile = SDR_SCRIPT_CMD;

    return true;
}

void sdr_write_pipe(LPCConfCmd buf, const int fd)
{
    write(fd, buf, sizeof(CConfCmd));
}

size_t sdr_read_pipe(const int fd)
{
    LPCConfCmd conf = new CConfCmd();
    size_t bytes_read;
    bytes_read = read(fd, conf, sizeof(CConfCmd));
/*     if (bytes_read == sizeof(CConfCmd))
    {
        std::cout << "Child received: " << conf->type << std::endl;
    }
 */
    if (conf)
        delete conf;
    return bytes_read;
}

std::string sdr_get_extract_file_name(const std::string path) {  
    size_t found = path.find_last_of("/\\");  
    if (found != std::string::npos) {  
        return path.substr(found + 1);  
    }

    return path;  
}


bool is_file_open(const char* filename) {  
    FILE* fp;  
    char path[1035];  
    snprintf(path, sizeof(path), "lsof %s", filename);  
    fp = popen(path, "r");  
    if (fp == NULL) {  
        return false;  
    }  
 
    char buffer[128];  
    while (fgets(buffer, sizeof(buffer)-1, fp) != NULL) {   
        if (strstr(buffer, filename) != NULL) {  
            pclose(fp);  
            return true;  
        }  
    }  
    pclose(fp);  
    return false;  
}  

void sdr_traverse_upload(const std::string& dir_path, const sdr_upload_t  info) {  
    DIR* dir;  
    struct dirent* ent;  
    std::string filepath;  
  
    if (info.upload && (dir = opendir(dir_path.c_str())) != NULL) {  
        /* 读取文件夹 */  
        while ((ent = readdir(dir)) != NULL) {  
            // 忽略目录项'.'和'..'  
            if (ent->d_name[0] == '.') {  
                continue;  
            }  
  
            filepath = dir_path + "/" + ent->d_name;  
  
            // 检查是否为文件  
            struct stat st;  
            if (stat(filepath.c_str(), &st) == 0 && S_ISREG(st.st_mode)) { 
                if(is_file_open(filepath.c_str())) 
                {
                    sleep(1);
                    continue;
                }
                 if (upload_file(filepath,  info)) {  
                     sdr_log_error_core(SDR_LOG_DEBUG,0,"%s【upload进程】已上传!",filepath); 
                     if(std::remove(filepath.c_str()))
                     {
                         sdr_log_error_core(SDR_LOG_DEBUG,0,"%s【upload进程】已删除!",filepath);  
                     }
                    
                } else {  
                    sdr_log_error_core(SDR_LOG_ERR,0,"%s【upload进程】上传失败!",filepath); 
                }
            }  
        }  
        closedir(dir);  
    }  
}  

std::string sdr_get_file_ext(const std::string& filename) {  
    size_t dotPosition = filename.rfind('.');  
    if (dotPosition != std::string::npos) {  
        return filename.substr(dotPosition + 1);  
    }  
    return ""; // 如果没有找到'.'，则返回空字符串  
} 

std::string sdr_replace_file_ext(const std::string& filename, const std::string& newExtension) {  
    size_t dotPosition = filename.rfind('.');  
    if (dotPosition == std::string::npos) {  
        // 如果没有找到'.'，则将新扩展名附加到文件名的末尾  
        return filename + "." + newExtension;  
    } else {  
        // 否则，替换从'.'到文件名末尾的所有字符  
        return filename.substr(0, dotPosition + 1) + newExtension;  
    }  
}

void sdr_ftp_init()
{
      u_char *pname = NULL;
    size_t nlen;

    //从配置文件读取相关的配置信息
    CConfig *p_config = CConfig::GetInstance();
    strcpy(sdr_upload.url, p_config->GetString("FtpServer"));
    strcpy(sdr_upload.user, p_config->GetString("User"));
    strcpy(sdr_upload.passwd, p_config->GetString("Passwd"));
    sdr_upload.upload = p_config->GetIntDefault("Upload", 0);

    pname = (u_char *)p_config->GetString("StoragePath");
    if(pname == NULL )
    {
        pname = (u_char *)SDR_STORAGE_PATH;
    }
     switch(p_config->GetIntDefault("CollectionMode", 0))
    {
    case COLLECTION_MODE::_ADS_B:
        sprintf(sdr_upload.storagepath,"%s/ads/", pname);
        break;
    case COLLECTION_MODE::_AIS:
        sprintf(sdr_upload.storagepath,"%s/ais/", pname);
        break;
    case COLLECTION_MODE::_FM:
        sprintf(sdr_upload.storagepath,"%s/fm/", pname);
        break;
    }
    return;
}