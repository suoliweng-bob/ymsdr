#include <iostream>
#include <stdlib.h>
#include <stdio.h>
#include <curl/curl.h>
#include <sys/stat.h>
#include <vector>
#include <fstream>
#include "sdr_func.h"

//
//  Make sure libcurl version > 7.32
//

// ---- common progress display ---- //
struct CustomProgress
{
    curl_off_t lastruntime; /* type depends on version, see above */
    CURL *curl;
};

// work for both download and upload
int progressCallback(void *p,
                     curl_off_t dltotal,
                     curl_off_t dlnow,
                     curl_off_t ultotal,
                     curl_off_t ulnow)
{
    struct CustomProgress *progress = (struct CustomProgress *)p;
    CURL *curl = progress->curl;
    curl_off_t curtime = 0;

    curl_easy_getinfo(curl, CURLINFO_TOTAL_TIME_T, &curtime);

    /* under certain circumstances it may be desirable for certain functionality
     to only run every N seconds, in order to do this the transaction time can
     be used */
    if ((curtime - progress->lastruntime) >= 3000000)
    {
        progress->lastruntime = curtime;
        printf("TOTAL TIME: %ld \n", curtime);
    }

    // do something to display the progress
    printf("UP: %ld bytes of %ld bytes, DOWN: %ld bytes of %ld bytes \n", ulnow, ultotal, dlnow, dltotal);
    if (ultotal)
        printf("UP progress: %0.2f\n", float(ulnow / ultotal));
    if (dltotal)
        printf("DOWN progress: %0.2f\n", float(dlnow / dltotal));

    return 0;
}

// ---- upload related ---- //
// parse headers for Content-Length
size_t getContentLengthFunc(void *ptr, size_t size, size_t nmemb, void *stream)
{
    int r;
    long len = 0;

    r = sscanf((const char*)ptr, "Content-Length: %ld\n", &len);
    if (r) /* Microsoft: we don't read the specs */
        *((long *) stream) = len;
    return size * nmemb;
}

// discard already downloaded data
size_t discardFunc(void *ptr, size_t size, size_t nmemb, void *stream)
{
    return size * nmemb;
}

// read data to upload
size_t readfunc(void *ptr, size_t size, size_t nmemb, void *stream)
{
    FILE *f = (FILE*)stream;
    size_t n;
    if (ferror(f))
        return CURL_READFUNC_ABORT;
    n = fread(ptr, size, nmemb, f) * size;
    return n;
}

// do upload, will overwrite existing file
int FtpUpload(const char *remote_file_path,
           const char *local_file_path,
           const char *username,
           const char *password,
           long timeout, long tries = 3)
{
    // init curl handle
    curl_global_init(CURL_GLOBAL_ALL);
    CURL *curlhandle = curl_easy_init();

    // get user_key pair
    char user_key[1024] = {0};
    sprintf(user_key, "%s:%s", username, password);

    FILE *file;
    long uploaded_len = 0;
    CURLcode ret = CURLE_GOT_NOTHING;
    file = fopen(local_file_path, "rb");
    if (file == NULL) 
    {
        perror(NULL);
        return 0;
    }
    curl_easy_setopt(curlhandle, CURLOPT_UPLOAD, 1L);
    curl_easy_setopt(curlhandle, CURLOPT_URL, remote_file_path);
    curl_easy_setopt(curlhandle, CURLOPT_USERPWD, user_key);
    if (timeout)
        curl_easy_setopt(curlhandle, CURLOPT_FTP_RESPONSE_TIMEOUT, timeout);
    curl_easy_setopt(curlhandle, CURLOPT_HEADERFUNCTION, getContentLengthFunc);
    curl_easy_setopt(curlhandle, CURLOPT_HEADERDATA, &uploaded_len);
    curl_easy_setopt(curlhandle, CURLOPT_WRITEFUNCTION, discardFunc);
    curl_easy_setopt(curlhandle, CURLOPT_READFUNCTION, readfunc);
    curl_easy_setopt(curlhandle, CURLOPT_READDATA, file);
    curl_easy_setopt(curlhandle, CURLOPT_FTPPORT, "-"); /* disable passive mode */
    curl_easy_setopt(curlhandle, CURLOPT_FTP_CREATE_MISSING_DIRS, 1L);

    // set upload progress
    curl_easy_setopt(curlhandle, CURLOPT_XFERINFOFUNCTION, progressCallback);
    struct CustomProgress prog;
    curl_easy_setopt(curlhandle, CURLOPT_XFERINFODATA, &prog);
    curl_easy_setopt(curlhandle, CURLOPT_NOPROGRESS, 0);

//    curl_easy_setopt(curlhandle, CURLOPT_VERBOSE, 1L); // if set 1, debug mode will print some low level msg

    // upload: 断点续传
    for (int c = 0; (ret != CURLE_OK) && (c < tries); c++)
    {
        /* are we resuming? */
        if (c)
        { /* yes */
            /* determine the length of the file already written */
            /*
            * With NOBODY and NOHEADER, libcurl will issue a SIZE
            * command, but the only way to retrieve the result is
            * to parse the returned Content-Length header. Thus,
            * getContentLengthfunc(). We need discardfunc() above
            * because HEADER will dump the headers to stdout
            * without it.
            */
            curl_easy_setopt(curlhandle, CURLOPT_NOBODY, 1L);
            curl_easy_setopt(curlhandle, CURLOPT_HEADER, 1L);
            ret = curl_easy_perform(curlhandle);
            if (ret != CURLE_OK)
                continue;
            curl_easy_setopt(curlhandle, CURLOPT_NOBODY, 0L);
            curl_easy_setopt(curlhandle, CURLOPT_HEADER, 0L);
            fseek(file, uploaded_len, SEEK_SET);
            curl_easy_setopt(curlhandle, CURLOPT_APPEND, 1L);
        }
        else
            curl_easy_setopt(curlhandle, CURLOPT_APPEND, 0L);

        ret = curl_easy_perform(curlhandle);
    }
    fclose(file);

    int curl_state = 0;
    if (ret == CURLE_OK)
        curl_state = 1;
    else
    {
        fprintf(stderr, "%s\n", curl_easy_strerror(ret));
        curl_state = 0;
    }

    // exit curl handle
    curl_easy_cleanup(curlhandle);
    curl_global_cleanup();

    return curl_state;
}

// ---- download related ---- //
// write data to upload
size_t writeFunc(void *ptr, size_t size, size_t nmemb, void *stream)
{
    std::cout << "--- write func ---" << std::endl;
    return fwrite(ptr, size, nmemb, (FILE*)stream);
}


// do download, will overwrite existing file
int FtpDownload(const char *remote_file_path,
                const char *local_file_path,
                const char *username,
                const char *password,
                long timeout = 3)
{
    // init curl handle
    curl_global_init(CURL_GLOBAL_ALL);
    CURL *curlhandle = curl_easy_init();

    // get user_key pair
    char user_key[1024] = {0};
    sprintf(user_key, "%s:%s", username, password);

    FILE *file;
    curl_off_t local_file_len = -1 ;
    long filesize =0 ;
    CURLcode ret = CURLE_GOT_NOTHING;
    struct stat file_info;
    int use_resume = 0; // resume flag

    // get local file info, if success ,set resume mode
    if(stat(local_file_path, &file_info) == 0)
    {
        local_file_len = file_info.st_size;
        use_resume = 1;
    }

    // read file in append mode: 断点续传
    file = fopen(local_file_path, "ab+");
    if (file == NULL)
    {
        perror(NULL);
        return 0;
    }
    curl_easy_setopt(curlhandle, CURLOPT_URL, remote_file_path);
    curl_easy_setopt(curlhandle, CURLOPT_USERPWD, user_key); // set user:password
    // set connection timeout
    curl_easy_setopt(curlhandle, CURLOPT_CONNECTTIMEOUT, timeout);
    // set header process, get content length callback
    curl_easy_setopt(curlhandle, CURLOPT_HEADERFUNCTION, getContentLengthFunc);
    curl_easy_setopt(curlhandle, CURLOPT_HEADERDATA, &filesize);

    // 断点续传 set download resume, if use resume, set current local file length
    curl_easy_setopt(curlhandle, CURLOPT_RESUME_FROM_LARGE, use_resume ? local_file_len : 0);
//    curl_easy_setopt(curlhandle, CURLOPT_WRITEFUNCTION, writeFunc);
    curl_easy_setopt(curlhandle, CURLOPT_WRITEDATA, file);

    // set download progress
    curl_easy_setopt(curlhandle, CURLOPT_XFERINFOFUNCTION, progressCallback);
    struct CustomProgress prog;
    curl_easy_setopt(curlhandle, CURLOPT_XFERINFODATA, &prog);
    curl_easy_setopt(curlhandle, CURLOPT_NOPROGRESS, 0);


//    curl_easy_setopt(curlhandle, CURLOPT_VERBOSE, 1); // if set 1, debug mode will print some low level msg

    ret = curl_easy_perform(curlhandle);
    fclose(file);

    int curl_state = 0;
    if (ret == CURLE_OK)
        curl_state = 1;
    else
    {
        fprintf(stderr, "%s\n", curl_easy_strerror(ret));
        curl_state = 0;
    }

    // exit curl handle
    curl_easy_cleanup(curlhandle);
    curl_global_cleanup();

    return curl_state;
}

int GetFileList(
	const std::string& remote_file_dir,
	const std::string& local_file_dir,
	const std::string& username,
	const std::string& password,
	std::vector<std::string>& file_list,
	int timeout
)
{
	/* local file name to store the file as */
	std::string file_list_path = local_file_dir + "file_list.txt";
	FILE* ftpfile = fopen(file_list_path.c_str(), "wb");

	CURLcode res = CURLE_GOT_NOTHING;
	CURL* curl = curl_easy_init();

	if (curl)
	{
		/* get a file listing from sunet */
		curl_easy_setopt(curl, CURLOPT_URL, remote_file_dir.c_str());

		/*user & pwd*/
		curl_easy_setopt(curl, CURLOPT_USERNAME, username.c_str());
		curl_easy_setopt(curl, CURLOPT_PASSWORD, password.c_str());

		/*ftp file info will be write in this file*/
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, ftpfile);

		if (timeout)
			curl_easy_setopt(curl, CURLOPT_FTP_RESPONSE_TIMEOUT, timeout);

		/*run the opt*/
		res = curl_easy_perform(curl);

		/* check for errors */
		if (res != CURLE_OK)
		{
			fprintf(stderr, "curl_easy_perform() failed: %s\n", curl_easy_strerror(res));
		}

		/* always cleanup */
		curl_easy_cleanup(curl);
	}

	/* close the local file */
	fclose(ftpfile);

	/*read all lines in file*/
	std::ifstream fin(file_list_path);
	while (!fin.eof())
	{
		char strline[260] = { 0 };
		fin.getline(strline, 260);
		file_list.push_back(strline);
	}
	fin.close();

	/*delete cache file*/
	remove(file_list_path.c_str());

	return res;
}

int upload_file(const std::string filepath, const sdr_upload_t  info)
{
	std::string url = "ftp://" + std::string(info.url) + "/data/fm/";
	std::string fileName = sdr_get_extract_file_name(filepath);
    std::string remoteUrl = url +fileName;
	int res = FtpUpload(remoteUrl.c_str(),filepath.c_str(), info.user, info.passwd, 3);
    return res;   
}
