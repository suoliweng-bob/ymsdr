#pragma once

#include "curl/curl.h"
#include <string>
#include <vector>
int progressCallback(void *p,
					 curl_off_t dltotal,
					 curl_off_t dlnow,
					 curl_off_t ultotal,
					 curl_off_t ulnow);
size_t getContentLengthFunc(void *ptr, size_t size, size_t nmemb, void *stream);
size_t discardFunc(void *ptr, size_t size, size_t nmemb, void *stream);
size_t readfunc(void *ptr, size_t size, size_t nmemb, void *stream);
int FtpUpload(const char *remote_file_path,
			  const char *local_file_path,
			  const char *username,
			  const char *password,
			  long timeout, long tries = 3);
size_t writeFunc(void *ptr, size_t size, size_t nmemb, void *stream);
int FtpDownload(const char *remote_file_path,
				const char *local_file_path,
				const char *username,
				const char *password,
				long timeout = 3);
int GetFileList(
	const std::string &remote_file_dir,
	const std::string &local_file_dir,
	const std::string &username,
	const std::string &password,
	std::vector<std::string> &file_list,
	int timeout);


int upload_file(const std::string filepath, const sdr_upload_t  info);
