#pragma once

#ifdef __cplusplus
extern "C" {
#endif

// Define CURL types
typedef void CURL;
typedef long CURLcode;
typedef long curl_off_t;

// Define CURL constants
#define CURLOPT_URL 1
#define CURLOPT_WRITEFUNCTION 2
#define CURLOPT_WRITEDATA 3
#define CURLOPT_XFERINFOFUNCTION 4
#define CURLOPT_XFERINFODATA 5
#define CURLE_OK 0
#define CURLINFO_RESPONSE_CODE 100
#define CURLINFO_CONTENT_TYPE 101

// Forward declarations of CURL functions
CURL* curl_easy_init();
void curl_easy_cleanup(CURL* handle);
CURLcode curl_easy_setopt(CURL* handle, int option, ...);
CURLcode curl_easy_perform(CURL* handle);
CURLcode curl_easy_getinfo(CURL* handle, int info, ...);
const char* curl_easy_strerror(CURLcode code);

// URL encoding/decoding functions
char* curl_easy_escape(void* handle, const char* string, int length);
void curl_free(void* ptr);
char* curl_easy_unescape(void* handle, const char* url, int length, int* outlength);

#ifdef __cplusplus
}
#endif 